/*
 * vector.c: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996 Eric Smith
 *
 * Modified for the MAME project 1997 by
 * Brad Oliver, Bernd Wiebelt & Aaron Giles
 *
 * 971008 Disabled the vector timing routines. The vector engine is now
 *        busy directly afer avgdvg_go(). This can be changed by the game
 *        doing a avgdvg_reset() or a custom interrupt routine calling
 *        avgdvg_clr_busy() on every frame. No vector updates will happen
 *        until busy is cleared (avgdvg_go() will return without
 *        modifying MAME's hidden bitmap)!
 *        Good frame rate choices seem to be
 *        DVG games   - 60fps
 *	  AVG games   - 45fps
 *        Tempest     - 30fps
 *        Major Havoc - 30fps???
 *        Quantum     - later...
 *        ...Battlezone runs fine with 45fps,
 *        thanks to Neil Bradley for the info
 *
 *        The old code is still there for reference. BW
 */

#include "driver.h"
#include "avgdvg.h"

#define BZONE_TOP	0x00500000		/* The Y coordinates above which the screen should be red */
#define BZONE_RED	12
#define BZONE_GREEN	10

static int vectorEngine = USE_DVG;

int x_res;	/* X-resolution of the display device */
int y_res;	/* Y-resolution of the display device */

/*
 * The next few variables hold the actual X/Y coordinates the vector game uses.
 * They are initialized in vg_init().
 */

int width; int height;
int xcenter; int ycenter;
int xmin; int xmax;
int ymin; int ymax;

/* ASG 080497 */
#define DVG_SHIFT 10		/* NOTE: must be even */
#define AVG_SHIFT 16		/* NOTE: must be even */
#define RES_SHIFT 16		/* NOTE: must be even */

#define DVG_X(x,xs)	( ( ((x) >> DVG_SHIFT/2) * ((xs) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + DVG_SHIFT/2) )
#define DVG_Y(y,ys)	( ( ((y) >> DVG_SHIFT/2) * ((ys) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + DVG_SHIFT/2) )

#define AVG_X(x,xs)	( ( ((x) >> AVG_SHIFT/2) * ((xs) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + AVG_SHIFT/2) )
#define AVG_Y(y,ys)	( ( ((y) >> AVG_SHIFT/2) * ((ys) >> RES_SHIFT/2) ) >> (RES_SHIFT/2 + AVG_SHIFT/2) )

int vg_step = 0; /* single step the vector generator */

/*  We use these to fake vector cycle counting BW 071097 */

int busy = 0;

#ifdef CYCLE_COUNTING
int last_go_cyc;
int cyc;
#endif

unsigned char *vectorram; /* LBO 062797 */
int vectorram_size;

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
/* The AVG used by Star Wars reads the bytes in the opposite order */
#define memrdwd_flip(offset,PC,cyc) ((vectorram[offset+1]) | (vectorram[offset]<<8))

#define max(x,y) (((x)>(y))?(x):(y))

#ifdef CYCLE_COUNTING
static void vector_timer (int deltax, int deltay)
{
        deltax = abs (deltax);
        deltay = abs (deltay);
	/* The original vecsim code does this */
	/* cyc += max (deltax, deltay) >> 17; */
        cyc += max (deltax, deltay) >> 15;
}

static void dvg_vector_timer (int scale)
{
	/* The original vecsim code does this */
	/* cyc += 4 << scale; */
	cyc += 4 << scale;
}
#endif /* CYCLE_COUTING */


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
	if (osd_update_vectors(&x_res,&y_res,vg_step))
		return;
#else
	if (osd_update_vectors(&x_res,&y_res,vg_step))
		return;
#endif

	xscale = (x_res << RES_SHIFT)/width;		/* ASG 080497 */
	yscale = (y_res << RES_SHIFT)/height;		/* ASG 080497 */

	currentx = 0;
	currenty = 0;

  	while (!done)
	{
#ifdef CYCLE_COUNTING
		cyc+=8;
#endif
#ifdef VG_DEBUG
		if (vg_step)
		{
	  		if (errorlog)
				fprintf (errorlog,"Current beam position: (%d, %d)\n",
			currentx, currenty);
	  		getchar();
		}
#endif

		firstwd = memrdwd (map_addr (pc), 0, 0);
		opcode = firstwd >> 12;
#ifdef VG_DEBUG
		if (errorlog)
			fprintf (errorlog,"%4x: %4x ", map_addr (pc), firstwd);
#endif
		pc++;
		if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
		{
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
#ifdef CYCLE_COUNTING
				dvg_vector_timer(temp);
#endif
				/* ASG 080497 */
				osd_draw_to (DVG_X (currentx, xscale), DVG_Y (currenty, yscale), z ? 7+(z<<4) : -1);
				break;

			case DLABS:
				x = twos_comp_val (secondwd, 12);
				y = twos_comp_val (firstwd, 12);
	  			scale = (secondwd >> 12);
				currentx = ((x-xmin) << DVG_SHIFT);		/* ASG 080497 */
				currenty = ((ymax-y) << DVG_SHIFT);		/* ASG 080497 */
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
#ifdef CYCLE_COUNTING
				dvg_vector_timer(temp);
#endif
				/* ASG 080497 */
				osd_draw_to (DVG_X (currentx, xscale), DVG_Y (currenty, yscale), z ? 7+(z<<4) : -1);
				break;

			default:
				if (errorlog)
					fprintf (errorlog,"Unknown DVG opcode found\n");
				done = 1;
		}
#ifdef VG_DEBUG
      		if (errorlog)
			fprintf (errorlog,"\n");
#endif
	}
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

	if (vectorEngine == USE_AVG_SWARS) {
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
	if (osd_update_vectors(&x_res,&y_res,vg_step))
		return;
#else
	if (osd_update_vectors(&x_res,&y_res,vg_step))
		return;
#endif

	xscale = (x_res << RES_SHIFT)/width;		/* ASG 080497 */
	yscale = (y_res << RES_SHIFT)/height;		/* ASG 080497 */

	scale = 0;		/* ASG 080497 */
	currentx = xcenter << AVG_SHIFT;		/* ASG 080497 */
	currenty = ycenter << AVG_SHIFT;		/* ASG 080497 */

	while (!done) {
#ifdef CYCLE_COUNTING
		cyc+=8;
#endif
#ifdef VG_DEBUG
		if (vg_step)
			getchar();
#endif

		if (vectorEngine == USE_AVG_SWARS)
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
			if (vectorEngine == USE_AVG_SWARS)
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
			int vecColor;

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
			else if ((z == 2 && vectorEngine != USE_AVG_SWARS) || vectorEngine == USE_AVG_SWARS) // SJB 17/8/97
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
#ifdef CYCLE_COUNTING
			vector_timer(deltax, deltay);
#endif

			/* Battlezone uses a red overlay for the top of the screen. */
			if (vectorEngine == USE_AVG_BZONE) {
				if (currenty < BZONE_TOP)
					color = BZONE_RED;
				else
					color = BZONE_GREEN;
				}

			/* Calculate the vector color */
			if (vectorEngine == USE_AVG_SWARS)
				/* Star Wars does it differently however... */
				vecColor = z ? color+( ((z_inline*z)>>3) & 0xf8 ) : -1;  // SJB 17/8/97
			else
				vecColor = z ? color+(z<<4) : -1;

			osd_draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), vecColor);
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
			else if ((z == 2 && vectorEngine != USE_AVG_SWARS) || vectorEngine == USE_AVG_SWARS) // SJB 17/8/97
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
#ifdef CYCLE_COUNTING
			vector_timer(deltax, deltay);
#endif


			/* Battlezone uses a red overlay for the top of the screen. */
			if (vectorEngine == USE_AVG_BZONE) {
				if (currenty < BZONE_TOP)
					color = BZONE_RED;
				else
					color = BZONE_GREEN;
				}

			/* Calculate the vector color */
			if (vectorEngine == USE_AVG_SWARS)
				/* Star Wars does it differently however... */
				vecColor = z ? color+( ((z_inline*z)>>3) & 0xf8 ) : -1;  // SJB 17/8/97
			else
				vecColor = z ? color+(z<<4) : -1;

			osd_draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), vecColor);
			break;

		case STAT:
			if (vectorEngine == USE_AVG_SWARS) {
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
			currentx = xcenter << AVG_SHIFT;	/* ASG 080497 */
			currenty = ycenter << AVG_SHIFT;	/* ASG 080497 */

			/* ASG 080497 */
			osd_draw_to (AVG_X (currentx, xscale), AVG_Y (currenty, yscale), -1);
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

}

int avgdvg_done (void)
{
#ifdef CYCLE_COUNTING
	if (cpu_gettotalcycles()-last_go_cyc<cyc)
		busy=1;
	else
		busy=0;
#endif

	if (errorlog)
		fprintf (errorlog,"avgdvg_done() @%04x, cyc %d, busy=%d\n",
			 cpu_getpc(), cpu_gettotalcycles(), busy);
	if (busy)
		return 0;
	else
		return 1;
}

void avgdvg_go (int offset, int data)
{
	if (errorlog)
		fprintf (errorlog,"avgdvg_go() @%04x, cyc %d, busy=%d\n",
			 cpu_getpc(), cpu_gettotalcycles(), busy);
#ifdef CYCLE_COUNTING
	if (errorlog && busy)
		fprintf (errorlog,"IMPORTANT! avgdvg_go() with busy set!\n");
	last_go_cyc=cpu_gettotalcycles();
	cyc=8;
#endif
	if (busy)
		return;

	busy=1;
	if (vectorEngine == USE_DVG)
		dvg_draw_vector_list ();
	else
		avg_draw_vector_list ();
}

void avgdvg_reset (int offset, int data)
{
	if (errorlog)
		fprintf (errorlog,"avgdvg_reset() @%04x, cyc %d,busy=%d\n",
			 cpu_getpc(), cpu_gettotalcycles(), busy);
	busy=0;
}

void avgdvg_clr_busy (void)
{
#ifndef CYCLE_COUNTING
	if (errorlog)
		fprintf (errorlog,"busy cleared by interrupt\n");
	busy=0;
#else
	if (errorlog)
		fprintf (errorlog,"Cycle counting, avgdvg_clr_busy() ignored!\n");
#endif
}

int avgdvg_init (int vgType)
{
	if (vectorram_size == 0) {
		if (errorlog) fprintf(errorlog,"Error: vectorram_size not initialized\n");
		return 1;
	}

	vectorEngine = vgType;
	if ((vectorEngine<AVGDVG_MIN) || (vectorEngine>AVGDVG_MAX)) {
		if (errorlog) fprintf(errorlog,"Error: unknown Atari Vector Game Type\n");
		return 1;
	}

	vg_step = 0;
	busy = 0;

#ifdef CYCLE_COUNTING
	cyc=0;
	last_go_cyc=0;
#endif

	xmin=Machine->drv->visible_area.min_x;
	ymin=Machine->drv->visible_area.min_y;
	xmax=Machine->drv->visible_area.max_x;
	ymax=Machine->drv->visible_area.max_y;
	width=xmax-xmin;
	height=ymax-ymin;
	xcenter=(xmax+xmin)/2;
	ycenter=(ymax+ymin)/2;

	return 0;
}

/*
 * This function initialises the colors for all atari games
 * We must have the basic palette (8 colors) in color_prom[],
 * and the rest is set up to the correct intensity
 * This code is derived from Brad Oliver
 */

#if 0
void avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int c,i,e;   	/* (c)olor, (i)ntensity, palette (e)ntry */
	int r,g,b;
	for (c=0; c<16; c++)
	{
		for (i=0; i<16; i++)
		{
			e=c+i*16;
			colortable[e]=e;
			r=color_prom[3*c  ]*i*(8+i);
			g=color_prom[3*c+1]*i*(8+i);
			b=color_prom[3*c+2]*i*(8+i);
			palette[3*e  ]=(r < 256) ? r : 0xff;
			palette[3*e+1]=(g < 256) ? g : 0xff;
			palette[3*e+2]=(b < 256) ? b : 0xff;
		}
	}
}
#endif

void avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{

#define IMAX 230
#define ISTEP (IMAX/16)
#define ISTEP_LOW_INTENSITY (IMAX/40)

	int c,e;   	/* (c)olor, palette (e)ntry */
	int r,g,b,i;	/* (r)ed, (g)reen, (b)lue, (i)ntensity */
	int j;
	for (c=0; c<16; c++)
	{
		i = (c & 0x08) ? 1 : 0;
		r = (c & 0x04) ? 1 : 0;
		g = (c & 0x02) ? 1 : 0;
		b = (c & 0x01) ? 1 : 0;

		for (j=0; j<16; j++)
		{
			/* Note: the meaning of i is inverted! */
			/*       i=0 --> high intensity        */
			if (i)
				i=j*ISTEP;
			else
				i=j*ISTEP_LOW_INTENSITY;
			e=c+j*16;
			colortable[e]=e;
			palette[3*e  ]=r*i;
			palette[3*e+1]=g*i;
			palette[3*e+2]=b*i;
		}
	}
}


/* If you want to use the next two functions, please make sure that you have
 * a fake GfxLayout, otherwise you'll crash */
void avg_fake_colorram_w (int offset, int data)
{
	int i;

	data&=0x0f;
	for (i=0; i<16; i++)
		Machine->gfx[0]->colortable[offset+i*16]=Machine->pens[data+i*16];
}

/*
 * The (inverted) meaning of the bits is:
 * 3-green 2-blue 1-red 0-intensity.
 * Since this differs from the rgb-order above, we need a translation table.
 */
void tempest_colorram_w (int offset, int data)
{
	int trans[]= { 7, 15, 3, 11, 6, 14, 2, 10,
                       5, 13, 1, 9, 4, 12, 0, 8 };

	avg_fake_colorram_w (offset, trans[data & 0x0f]);
}


void sw_avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int c,i,e;   	/* (c)olor, (i)ntensity, palette (e)ntry */
	int r,g,b;
	for (c=0; c<8; c++)
	{
		for (i=0; i<32; i++)
		{
			e=c+i*8;
			colortable[e]=e;
			r=color_prom[3*c  ]*i*8.25;
			g=color_prom[3*c+1]*i*8.25;
			b=color_prom[3*c+2]*i*8.25;
			palette[3*e  ]=(r < 256) ? r : 0xff;
			palette[3*e+1]=(g < 256) ? g : 0xff;
			palette[3*e+2]=(b < 256) ? b : 0xff;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void avg_screenrefresh(struct osd_bitmap *bitmap)
{
}

void dvg_screenrefresh(struct osd_bitmap *bitmap)
{
}

int dvg_start(void)
{
	return avgdvg_init (USE_DVG);
}

int avg_start(void)
{
	return avgdvg_init (USE_AVG);
}

int avg_start_starwars(void)
{
	return avgdvg_init (USE_AVG_SWARS);
}

int avg_start_bzone(void)
{
	return avgdvg_init (USE_AVG_BZONE);
}


void avg_stop(void)
{
}

void dvg_stop(void)
{
}

