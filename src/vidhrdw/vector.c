/*
 * vector.c: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996 Eric Smith
 *
 * Modified for the MAME project 1997 by
 * Brad Oliver, Bernd Wiebelt & Aaron Giles
 */

#include "driver.h"
#include "vector.h"

#undef VG_DEBUG
int dvg = 0;

int x_res;	/* X-resolution of the display device */
int y_res;	/* Y-resolution of the display device */

/* This struct holds the actual X/Y coordinates the vector game uses.
 * Gets initialized in vg_init().
 */
static struct { int width; int height;
	 int x_cent; int y_cent;
	 int x_min; int x_max;
	 int y_min; int y_max; } vg_video;

int flip_word;	/* determines the endian-ness of the words read from vectorram */

/* ASG 080497 */
#define DVG_SHIFT 10		/* NOTE: must be even */
#define AVG_SHIFT 16		/* NOTE: must be even */
#define RES_SHIFT 16		/* NOTE: must be even */

#define DVG_X(x,xs)	( ( ((x) >> DVG_SHIFT/2) * ((xs) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + DVG_SHIFT/2) )
#define DVG_Y(y,ys)	( ( ((y) >> DVG_SHIFT/2) * ((ys) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + DVG_SHIFT/2) )

#define AVG_X(x,xs)	( ( ((x) >> AVG_SHIFT/2) * ((xs) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + AVG_SHIFT/2) )
#define AVG_Y(y,ys)	( ( ((y) >> AVG_SHIFT/2) * ((ys) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + AVG_SHIFT/2) )

int vg_step = 0; /* single step the vector generator */
int last_vgo_cyc=0;
int vgo_count=0;

int vg_busy = 0;
int vg_done_cyc = 0; /* cycle after which VG will be done */

unsigned char *vectorram; /* LBO 062797 */

#define MAXSTACK 8 	/* Tempest needs more than 4     BW 210797 */

#define VCTR 0
#define HALT 1
#define SVEC 2
#define STAT 3
#define CNTR 4
#define JSRL 5
#define RTSL 6
#define JMPL 7
#define SCAL 8

#define DVCTR 0x01
#define DLABS 0x0a
#define DHALT 0x0b
#define DJSRL 0x0c
#define DRTSL 0x0d
#define DJMPL 0x0e
#define DSVEC 0x0f

#define twos_comp_val(num,bits) ((num&(1<<(bits-1)))?(num|~((1<<bits)-1)):(num&((1<<bits)-1)))

char *avg_mnem[] = { "vctr", "halt", "svec", "stat", "cntr", "jsrl", "rtsl",
		 "jmpl", "scal" };

char *dvg_mnem[] = { "????", "vct1", "vct2", "vct3",
		     "vct4", "vct5", "vct6", "vct7",
		     "vct8", "vct9", "labs", "halt",
		     "jsrl", "rtsl", "jmpl", "svec" };

#define map_addr(n) (((n)<<1))
#define memrdwd(offset,PC,cyc) ((vectorram[offset]) | (vectorram[offset+1]<<8))
#define memrdwd_flip(offset,PC,cyc) ((vectorram[offset+1]) | (vectorram[offset]<<8))

#define max(x,y) (((x)>(y))?(x):(y))

static void vector_timer (int deltax, int deltay)
{
	deltax = abs (deltax);
	deltay = abs (deltay);
/*	vg_done_cyc += max (deltax, deltay) >> 17;*/
	vg_done_cyc += max (deltax, deltay) >> (AVG_SHIFT+1);
}


static void dvg_vector_timer (int scale)
{
/*	vg_done_cyc += 4 << scale;*/
	vg_done_cyc += (DVG_SHIFT-8) << scale;
}


static void dvg_draw_vector_list (void)
{
	int pc;
	int sp;
	int stack [MAXSTACK];

	int scale;
	int statz;

	int xscale, yscale;
	int currentx, currenty;

	int done = 0;

	int firstwd;
	int secondwd = 0; /* Initialize to tease the compiler */
	int opcode;

	int x, y;
	int z, temp;
	int a;

	int deltax, deltay;

	pc = 0;
	sp = 0;
	scale = 0;
	statz = 0;

#ifdef VG_DEBUG
	open_page (&x_res,&y_res,vg_step);
#else
	open_page (&x_res,&y_res,0);
#endif

	xscale = (x_res << RES_SHIFT)/vg_video.width;		/* ASG 080497 */
	yscale = (y_res << RES_SHIFT)/vg_video.height;		/* ASG 080497 */

	currentx = 0;
	currenty = 0;

  	while (!done)
	{
		vg_done_cyc += 8;
#ifdef VG_DEBUG
		if (vg_step)
		{
	  		if (errorlog)
				fprintf (errorlog,"Current beam position: (%d, %d)\n",
			currentx, currenty);
	  		getchar();
		}
#endif

		if (flip_word)
			firstwd = memrdwd_flip (map_addr (pc), 0, 0);
		else
			firstwd = memrdwd (map_addr (pc), 0, 0);
		opcode = firstwd >> 12;
#ifdef VG_DEBUG
		if (errorlog)
			fprintf (errorlog,"%4x: %4x ", map_addr (pc), firstwd);
#endif
		pc++;
		if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
		{
			if (flip_word)
				secondwd = memrdwd_flip (map_addr (pc), 0, 0);
			else
				secondwd = memrdwd (map_addr (pc), 0, 0);
			pc++;
#ifdef VG_DEBUG
			if (errorlog) {
				fprintf (errorlog,"%s ", dvg_mnem [opcode]);
				fprintf (errorlog,"%4x  ", secondwd);
			}
#endif
		}
#ifdef VG_DEBUG
		else if (errorlog)
			fprintf (errorlog,"Illegal opcode ");
#endif

		switch (opcode)
		{
			case 0:
#ifdef VG_DEBUG
	 			if (errorlog) fprintf (errorlog,"Error: DVG opcode 0!  Addr %4x Instr %4x %4x\n", map_addr (pc-2), firstwd, secondwd);
				done = 1;
				break;
#endif
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
	  			y = firstwd & 0x03ff;
				if (firstwd & 0x400)
					y=-y;
				x = secondwd & 0x3ff;
				if (secondwd & 0x400)
					x=-x;
				z = secondwd >> 12;
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"(%d,%d) z: %d scal: %d", x, y, z, opcode);
#endif
	  			temp = ((scale + opcode) & 0x0f);
	  			if (temp > 9)
					temp = -1;
	  			deltax = (x << DVG_SHIFT) >> (9-temp);		/* ASG 080497 */
				deltay = (y << DVG_SHIFT) >> (9-temp);		/* ASG 080497 */
	  			currentx += deltax;
				currenty -= deltay;
				dvg_vector_timer (temp);
				/* ASG 080497 */
				draw_to (DVG_X (currentx, xscale), DVG_Y (currenty, yscale), z ? 7+(z<<4) : -1);
				break;

			case DLABS:
				x = twos_comp_val (secondwd, 12);
				y = twos_comp_val (firstwd, 12);
	  			scale = (secondwd >> 12);
				currentx = ((x-vg_video.x_min) << DVG_SHIFT);		/* ASG 080497 */
				currenty = ((vg_video.y_max-y) << DVG_SHIFT);		/* ASG 080497 */
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"(%d,%d) scal: %d", x, y, secondwd >> 12);
#endif
				break;

			case DHALT:
#ifdef VG_DEBUG
				if (errorlog && ((firstwd & 0x0fff) != 0))
	      				fprintf (errorlog,"(%d?)", firstwd & 0x0fff);
#endif
				done = 1;
				break;

			case DJSRL:
				a = firstwd & 0x0fff;
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"%4x", map_addr(a));
#endif
				stack [sp] = pc;
				if (sp == (MAXSTACK - 1))
	    			{
					if (errorlog) fprintf (errorlog,"\n*** Vector generator stack overflow! ***\n");
					done = 1;
					sp = 0;
				}
				else
					sp++;
				pc = a;
				break;

			case DRTSL:
#ifdef VG_DEBUG
				if (errorlog && ((firstwd & 0x0fff) != 0))
					 fprintf (errorlog,"(%d?)", firstwd & 0x0fff);
#endif
				if (sp == 0)
	    			{
					if (errorlog) fprintf (errorlog,"\n*** Vector generator stack underflow! ***\n");
					done = 1;
					sp = MAXSTACK - 1;
				}
				else
					sp--;
				pc = stack [sp];
				break;

			case DJMPL:
				a = firstwd & 0x0fff;
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"%4x", map_addr(a));
#endif
				pc = a;
				break;

			case DSVEC:
				y = firstwd & 0x0300;
				if (firstwd & 0x0400)
					y = -y;
				x = (firstwd & 0x03) << 8;
				if (firstwd & 0x04)
					x = -x;
				z = (firstwd >> 4) & 0x0f;
				temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >>11) & 0x01);
	  			temp = ((scale + temp) & 0x0f);
				if (temp > 9)
					temp = -1;
#ifdef VG_DEBUG
				if (errorlog)
	      				fprintf (errorlog,"(%d,%d) z: %d scal: %d", x, y, z, temp);
#endif

				deltax = (x << DVG_SHIFT) >> (9-temp);	/* ASG 080497 */
				deltay = (y << DVG_SHIFT) >> (9-temp);	/* ASG 080497 */
	  			currentx += deltax;
				currenty -= deltay;
				dvg_vector_timer (temp);
				/* ASG 080497 */
				draw_to (DVG_X (currentx, xscale), DVG_Y (currenty, yscale), z ? 7+(z<<4) : -1);
				break;

			default:
				if (errorlog)
					fprintf (errorlog,"internal error\n");
				done = 1;
		}
#ifdef VG_DEBUG
      		if (errorlog)
			fprintf (errorlog,"\n");
#endif
	}
	close_page ();
}


static void avg_draw_vector_list (void)
{

	int pc;
	int sp;
	int stack [MAXSTACK];

	int xscale, yscale;

	int scale;
	int statz;

	int z_inline; // SJB 17/8/97

	int color;

	int currentx, currenty;
	int done = 0;

	int firstwd, secondwd;
	int opcode;

	int x, y, z, b, l, d, a;

	int deltax, deltay;

	pc = 0;
	sp = 0;
	statz = 0;
	color = 0;

	if (flip_word) {
		firstwd = memrdwd_flip (map_addr (pc), 0, 0);
		secondwd = memrdwd_flip (map_addr (pc+1), 0, 0);
	} else {
		firstwd = memrdwd (map_addr (pc), 0, 0);
		secondwd = memrdwd (map_addr (pc+1), 0, 0);
	}
	if ((firstwd == 0) && (secondwd == 0)) {
		if (errorlog) fprintf (errorlog,"VGO with zeroed vector memory\n");
		return;
	}

#ifdef VG_DEBUG
	open_page (&x_res,&y_res,vg_step);
#else
	open_page (&x_res,&y_res,0);
#endif

	xscale = (x_res << RES_SHIFT)/vg_video.width;		/* ASG 080497 */
	yscale = (y_res << RES_SHIFT)/vg_video.height;		/* ASG 080497 */

	scale = 0;		/* ASG 080497 */
	currentx = vg_video.x_cent << AVG_SHIFT;		/* ASG 080497 */
	currenty = vg_video.y_cent << AVG_SHIFT;		/* ASG 080497 */

	while (!done) {
#ifdef VG_DEBUG
		vg_done_cyc += 8;
		if (vg_step)
			getchar();
#endif

		if (flip_word)
			firstwd = memrdwd_flip (map_addr (pc), 0, 0);
		else
			firstwd = memrdwd (map_addr (pc), 0, 0);
		opcode = firstwd >> 13;
#ifdef VG_DEBUG
		if (errorlog)
			fprintf (errorlog,"%4x: %4x ", map_addr (pc), firstwd);
#endif
		pc++;
		if (opcode == VCTR) {
			if (flip_word)
				secondwd = memrdwd_flip (map_addr (pc), 0, 0);
			else
				secondwd = memrdwd (map_addr (pc), 0, 0);
			pc++;
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"%4x  ", secondwd);
#endif
		}
#ifdef VG_DEBUG
		else if (errorlog)
			fprintf (errorlog,"      ");
#endif

		if ((opcode == STAT) && ((firstwd & 0x1000) != 0))
			opcode = SCAL;

#ifdef VG_DEBUG
		if (errorlog)
			fprintf (errorlog,"%s ", avg_mnem [opcode]);
#endif

		switch (opcode) {
		case VCTR:
			x = twos_comp_val (secondwd,13);
			y = twos_comp_val (firstwd,13);
			z_inline = (secondwd >>13); // SJB 17/8/97
			z = z_inline+z_inline;  // SJB 17/8/97
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"%d,%d,", x, y);
#endif
			if (z == 0) {
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"blank");
#endif
			}
			else if ((z == 2 && flip_word == 0) || flip_word == 1) // SJB 17/8/97
			{
				z = statz;
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"stat (%d)", z);
#endif
			}
#ifdef VG_DEBUG
			else if (errorlog)
				fprintf (errorlog,"%d", z);
#endif
			deltax = x * scale;
			deltay = y * scale;
			currentx += deltax;
			currenty -= deltay;
			vector_timer (deltax, deltay);
			/* ASG 080497 */
			if(flip_word)
				draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), z ? color+( ((z_inline*z)>>3) & 0xf8 ) : -1); // SJB 17/8/97
			else
				draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), z ? color+(z<<4) : -1);
			break;

		case SVEC:
			x = twos_comp_val (firstwd, 5) << 1;
			y = twos_comp_val (firstwd >> 8, 5) << 1;
			z_inline = ((firstwd >> 5) & 7);
			z = z_inline+z_inline; // SJBNEW
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"%d,%d,", x, y);
#endif
			if (z == 0) {
#ifdef VG_DEBUG
				if (errorlog)
					fprintf (errorlog,"blank");
#endif
			}
			else if ((z == 2 && flip_word == 0) || flip_word == 1) // SJB 17/8/97
			{
				z = statz;
#ifdef VG_DEBUG
				if (errorlog) fprintf (errorlog,"stat");
#endif
			}
#ifdef VG_DEBUG
			else if (errorlog)
				fprintf (errorlog,"%d", z);
#endif
			deltax = x * scale;
			deltay = y * scale;
			currentx += deltax;
			currenty -= deltay;
			vector_timer (deltax,deltay);
			/* ASG 080497 */
			if(flip_word)
				draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), z ? color+( ((z_inline*z)>>3) & 0xf8 ) : -1); // SJB 17/8/97
			else
				draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), z ? color+(z<<4) : -1);
			break;

		case STAT:
			if (flip_word) {
				color=(char)((firstwd & 0x0700)>>8); /* Colour code 0-7 stored in top 3 bits of `colour'*/
				statz = (firstwd &0xff); // SJB 17/8/97
			} else {
				color = firstwd & 0x0f;
				statz = (firstwd >> 4) & 0x0f;
        		}
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"z: %d color: %d", statz, color);
#endif
			/* should do e, h, i flags here! */
			break;

		case SCAL:
			b = ((firstwd >> 8) & 0x07)+8;
			l = (~firstwd) & 0xff;
			scale = (l << AVG_SHIFT) >> b;		/* ASG 080497 */

#ifdef VG_DEBUG
			if (errorlog) {
				fprintf (errorlog,"bin: %d, lin: ", b);
				if (l > 0x80)
					fprintf (errorlog,"(%d?)", l);
				else
					fprintf (errorlog,"%d", l);
				fprintf (errorlog," scale: %f", (scale/(float)(1<<AVG_SHIFT)));
			}
#endif
			break;

		case CNTR:
			d = firstwd & 0xff;
#ifdef VG_DEBUG
			if (errorlog && (d != 0x40))
				fprintf (errorlog,"%d", d);
#endif
			currentx = vg_video.x_cent << AVG_SHIFT;		/* ASG 080497 */
			currenty = vg_video.y_cent << AVG_SHIFT;		/* ASG 080497 */

			/* ASG 080497 */
			draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), -1);
			break;

		case RTSL:
#ifdef VG_DEBUG
			if (errorlog && ((firstwd & 0x1fff) != 0))
				fprintf (errorlog,"(%d?)", firstwd & 0x1fff);
#endif
			if (sp == 0) {
				if (errorlog)
					fprintf (errorlog,"\n*** Vector generator stack underflow! ***\n");
				done = 1;
				sp = MAXSTACK - 1;
			} else
				sp--;
			pc = stack [sp];
			break;

		case HALT:
#ifdef VG_DEBUG
			if (errorlog && ((firstwd & 0x1fff) != 0))
				fprintf (errorlog,"(%d?)", firstwd & 0x1fff);
#endif
			done = 1;
			break;

		case JMPL:
			a = firstwd & 0x1fff;
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"%4x", map_addr(a));
#endif
			/* if a = 0x0000, treat as HALT */
			if (a == 0x0000)
				done = 1;
			else
				pc = a;
			break;

		case JSRL:
			a = firstwd & 0x1fff;
#ifdef VG_DEBUG
			if (errorlog)
				fprintf (errorlog,"%4x", map_addr(a));
#endif
			/* if a = 0x0000, treat as HALT */
			if (a == 0x0000)
				done = 1;
			else {
				stack [sp] = pc;
				if (sp == (MAXSTACK - 1)) {
					if (errorlog)
						fprintf (errorlog,"\n*** Vector generator stack overflow! ***\n");
					done = 1;
					sp = 0;
				}
				else
					sp++;
				pc = a;
			}
			break;

		default:
			if (errorlog)
				fprintf (errorlog,"internal error\n");
		}
#ifdef VG_DEBUG
	if (errorlog) fprintf (errorlog,"\n");
#endif
	}

	close_page ();
}

int vg_done (int cyc)
{
	if (cyc-last_vgo_cyc>vg_done_cyc)
		vg_busy = 0;
	return (!vg_busy);
}

void vg_go (int cyc)
{
	if (errorlog) {
		vgo_count++;
		fprintf (errorlog,"VGO #%d cpu-cycle %d vector cycles %d\n",
			vgo_count, cpu_gettotalcycles(), vg_done_cyc);
	}
	last_vgo_cyc=cyc;
	vg_busy=1;
	vg_done_cyc = 8;
	if (dvg)
		dvg_draw_vector_list ();
	else
		avg_draw_vector_list ();
}

void vg_reset (int offset, int data)
{
	vg_busy = 0;
	if (errorlog)
		fprintf (errorlog,"vector generator reset @%04x\n",cpu_getpc());
}

int vg_init (int len, int usingDvg, int flip)
{
	if (usingDvg)
		dvg = 1;
	else
		dvg = 0;
	flip_word = flip;

	vg_step = 0;
	last_vgo_cyc = 0;
	vgo_count = 0;
	vg_busy = 0;
	vg_done_cyc = 0;

	vg_video.width =Machine->drv->visible_area.max_x-Machine->drv->visible_area.min_x;
	vg_video.height=Machine->drv->visible_area.max_y-Machine->drv->visible_area.min_y;
	vg_video.x_cent=(Machine->drv->visible_area.max_x+Machine->drv->visible_area.min_x)/2;
	vg_video.y_cent=(Machine->drv->visible_area.max_y+Machine->drv->visible_area.min_y)/2;
	vg_video.x_min=Machine->drv->visible_area.min_x;
	vg_video.y_min=Machine->drv->visible_area.min_y;
	vg_video.x_max=Machine->drv->visible_area.max_x;
	vg_video.y_max=Machine->drv->visible_area.max_y;

	return 0;
}

void vg_stop (void)
{
}

