/*
 * vector.c: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996 Eric Smith
 */

#include "driver.h"
#include "vector.h"

int dvg = 0;
int portrait = 0;

#ifdef VG_DEBUG
int trace_vgo = 0;
int vg_step = 0; /* single step the vector generator */
int vg_print = 1;
unsigned long last_vgo_cyc=0;
unsigned long vgo_count=0;
#endif /* VG_DEBUG */

int vg_busy = 0;
unsigned long vg_done_cyc; /* cycle after which VG will be done */

unsigned char *vectorram; /* LBO 062797 */

#define MAXSTACK 4

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

#define map_addr(n) (((n)<<1)) /* LBO 062797 */
#define memrdwd(offset,PC,cyc) ((vectorram[offset]) | (vectorram[offset+1]<<8)) /* LBO 062797 */

#define max(x,y) (((x)>(y))?(x):(y))

static void vector_timer (long deltax, long deltay)
{
  deltax = labs (deltax);
  deltay = labs (deltay);
  vg_done_cyc -= max (deltax, deltay) >> 17; /* LBO 062797 */
}


static void dvg_vector_timer (int scale)
{
  vg_done_cyc -= 4 << scale; /* LBO 062797 */
}


static void dvg_draw_vector_list (void)
{
  static int pc;
  static int sp;
  static int stack [MAXSTACK];

  static long scale;
  static int statz;

  static long currentx;
  static long currenty;

  int done = 0;

  int firstwd, secondwd;
  int opcode;

  long x, y;
  int z, temp;
  int a;

  long oldx, oldy;
  long deltax, deltay;

#if 0
  if (!cont)
#endif
    {
      pc = 0;
      sp = 0;
      scale = 0;
      statz = 0;
      if (portrait)
	{
	  currentx = 1023 * 8192;
	  currenty = 511 * 8192;
	}
      else
	{
	  currentx = 511 * 8192;
	  currenty = 1023 * 8192;
	}
    }

#ifdef VG_DEBUG
  portrait = open_page (vg_step);
#else
  portrait = open_page (0);
#endif

  while (!done)
    {
      vg_done_cyc -= 8; /* LBO 062797 */
#ifdef VG_DEBUG
      if (vg_step)
	{
	  if (errorlog) fprintf (errorlog,"Current beam position: (%d, %d)\n",
		  currentx, currenty);
	  getchar();
	}
#endif
      firstwd = memrdwd (map_addr (pc), 0, 0);
      opcode = firstwd >> 12;
#ifdef VG_DEBUG
      if (vg_print)
	if (errorlog) fprintf (errorlog,"%4x: %4x ", map_addr (pc), firstwd);
#endif
      pc++;
      if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
	{
	  secondwd = memrdwd (map_addr (pc), 0, 0);
	  pc++;
#ifdef VG_DEBUG
	  if (vg_print)
	    if (errorlog) fprintf (errorlog,"%4x  ", secondwd);
#endif
	}
#ifdef VG_DEBUG
      else
	if (vg_print)
	  if (errorlog) fprintf (errorlog,"      ");
#endif

#ifdef VG_DEBUG
      if (vg_print)
	if (errorlog) fprintf (errorlog,"%s ", dvg_mnem [opcode]);
#endif

      switch (opcode)
	{
	case 0:
#ifdef DVG_OP_0_ERR
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
	  if (firstwd & 0x0400)
	    y = -y;
	  x = secondwd & 0x03ff;
	  if (secondwd & 0x400)
	    x = -x;
	  z = secondwd >> 12;
#ifdef VG_DEBUG
	  if (vg_print)
	    {
	      if (errorlog) fprintf (errorlog,"(%d,%d) z: %d scal: %d", x, y, z, opcode);
	    }
#endif
#if 0
	  if (vg_step)
	    {
	      if (errorlog) fprintf (errorlog,"\nx: %x  x<<21: %x  (x<<21)>>%d: %x\n", x, x<<21, 30-(scale+opcode), (x<<21)>>(30-(scale+opcode)));
	      if (errorlog) fprintf (errorlog,"y: %x  y<<21: %x  (y<<21)>>%d: %x\n", y, y<<21, 30-(scale+opcode), (y<<21)>>(30-(scale+opcode)));
	    }
#endif
	  oldx = currentx; oldy = currenty;
	  temp = (scale + opcode) & 0x0f;
	  if (temp > 9)
	    temp = -1;
	  deltax = (x << 21) >> (30 - temp);
	  deltay = (y << 21) >> (30 - temp);
#if 0
	  if (vg_step)
	    {
	      if (errorlog) fprintf (errorlog,"deltax: %x  deltay: %x\n", deltax, deltay);
	    }
#endif
	  currentx += deltax;
	  currenty -= deltay;
	  dvg_vector_timer (temp);
	  draw_line (oldx>>1, oldy>>1, currentx>>1, currenty>>1, 7, z);
	  break;

	case DLABS:
	  x = twos_comp_val (secondwd, 12);
	  y = twos_comp_val (firstwd, 12);
/*
	  x = secondwd & 0x07ff;
	  if (secondwd & 0x0800)
	    x = x - 0x1000;
	  y = firstwd & 0x07ff;
	  if (firstwd & 0x0800)
	    y = y - 0x1000;
*/
	  scale = secondwd >> 12;
	  currentx = x;
	  currenty = (896 - y);
#ifdef VG_DEBUG
	  if (vg_print)
	    {
	      if (errorlog) fprintf (errorlog,"(%d,%d) scal: %d", x, y, secondwd >> 12);
	    }
#endif
	  break;

	case DHALT:
#ifdef VG_DEBUG
	  if (vg_print)
	    if ((firstwd & 0x0fff) != 0)
	      if (errorlog) fprintf (errorlog,"(%d?)", firstwd & 0x0fff);
#endif
	  done = 1;
	  break;

	case DJSRL:
	  a = firstwd & 0x0fff;
#ifdef VG_DEBUG
	  if (vg_print)
	    if (errorlog) fprintf (errorlog,"%4x", map_addr(a));
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
	  if (vg_print)
	    if ((firstwd & 0x0fff) != 0)
	      if (errorlog) fprintf (errorlog,"(%d?)", firstwd & 0x0fff);
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
	  if (vg_print)
	    if (errorlog) fprintf (errorlog,"%4x", map_addr(a));
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
	  temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >> 11) & 0x01);
#ifdef VG_DEBUG
	  if (vg_print)
	    {
	      if (errorlog) fprintf (errorlog,"(%d,%d) z: %d scal: %d", x, y, z, temp);
	    }
#endif
#if 0
	  if (vg_step)
	    {
	      if (errorlog) fprintf (errorlog,"\nx: %x  x<<21: %x  (x<<21)>>%d: %x\n", x, x<<21, 30-(scale+temp), (x<<21)>>(30-(scale+temp)));
	      if (errorlog) fprintf (errorlog,"y: %x  y<<21: %x  (y<<21)>>%d: %x\n", y, y<<21, 30-(scale+temp), (y<<21)>>(30-(scale+temp)));
	    }
#endif
	  oldx = currentx; oldy = currenty;
	  temp = (scale + temp) & 0x0f;
	  if (temp > 9)
	    temp = -1;
	  deltax = (x << 21) >> (30 - temp);
	  deltay = (y << 21) >> (30 - temp);
#if 0
	  if (vg_step)
	    {
	      if (errorlog) fprintf (errorlog,"deltax: %x  deltay: %x\n", deltax, deltay);
	    }
#endif
	  currentx += deltax;
	  currenty -= deltay;
	  dvg_vector_timer (temp);
	  draw_line (oldx>>1, oldy>>1, currentx>>1, currenty>>1, 7, z);
	  break;

	default:
	  if (errorlog) fprintf (errorlog,"internal error\n");
	  done = 1;
	}
#ifdef VG_DEBUG
      if (vg_print)
	if (errorlog) fprintf (errorlog,"\n");
#endif
    }

  close_page ();
}


static void avg_draw_vector_list (void) {

	static int pc;
	static int sp;
	static int stack [MAXSTACK];

	static long scale;
	static int statz;
	static int color;

	static long currentx;
	static long currenty;

	int done = 0;

	int firstwd, secondwd;
	int opcode;

	int x, y, z, b, l, d, a;

	long oldx, oldy;
	long deltax, deltay;

#if 0
	if (!cont)
#endif
	{
		pc = 0;
		sp = 0;
		scale = 16384;
		statz = 0;
		color = 0;
		if (portrait) {
			currentx = 550 * 8192;
			currenty = 440 * 8192;
			}
		else {
			currentx = 511 * 8192;
			currenty = 511 * 8192;
			}

		firstwd = memrdwd (map_addr (pc), 0, 0);
		secondwd = memrdwd (map_addr (pc+1), 0, 0);
		if ((firstwd == 0) && (secondwd == 0)) {
			if (errorlog) fprintf (errorlog,"VGO with zeroed vector memory\n");
			return;
			}
	}

#ifdef VG_DEBUG
	portrait = open_page (vg_step);
#else
	portrait = open_page (0);
#endif

	while (!done) {
		vg_done_cyc -= 8; /* LBO 062797 */
#ifdef VG_DEBUG
		if (vg_step)
			getchar();
#endif
		firstwd = memrdwd (map_addr (pc), 0, 0);
		opcode = firstwd >> 13;
#ifdef VG_DEBUG
		if (vg_print)
			if (errorlog) fprintf (errorlog,"%4x: %4x ", map_addr (pc), firstwd);
#endif
		pc++;
		if (opcode == VCTR) {
			secondwd = memrdwd (map_addr (pc), 0, 0);
			pc++;
#ifdef VG_DEBUG
			if (vg_print)
				if (errorlog) fprintf (errorlog,"%4x  ", secondwd);
#endif
			}
#ifdef VG_DEBUG
		else
			if (vg_print)
				if (errorlog) fprintf (errorlog,"      ");
#endif

		if ((opcode == STAT) && ((firstwd & 0x1000) != 0))
			opcode = SCAL;

#ifdef VG_DEBUG
		if (vg_print)
			if (errorlog) fprintf (errorlog,"%s ", avg_mnem [opcode]);
#endif

		switch (opcode) {
			case VCTR:
				x = twos_comp_val (secondwd,13);
				y = twos_comp_val (firstwd,13);
				z = 2 * (secondwd >> 13);
#ifdef VG_DEBUG
				if (vg_print)
					if (errorlog) fprintf (errorlog,"%d,%d,", x, y);
#endif
				if (z == 0) {
#ifdef VG_DEBUG
					if (vg_print)
						if (errorlog) fprintf (errorlog,"blank");
#endif
					}
				else if (z == 2) {
					z = statz;
#ifdef VG_DEBUG
					if (vg_print)
						if (errorlog) fprintf (errorlog,"stat (%d)", z);
#endif
					}
#ifdef VG_DEBUG
				else if (vg_print)
					if (errorlog) fprintf (errorlog,"%d", z);
#endif
				oldx = currentx; oldy = currenty;
				deltax = x * scale; deltay = y * scale;
				currentx += deltax;
				currenty -= deltay;
				vector_timer (deltax, deltay);
				if (portrait)
					draw_line (oldx>>15, oldy>>14, currentx>>15, currenty>>14, color, z);
				else
					draw_line (oldx>>14, oldy>>14, currentx>>14, currenty>>14, color, z);
				break;
	
			case SVEC:
				x = twos_comp_val (firstwd, 5) << 1;
				y = twos_comp_val (firstwd >> 8, 5) << 1;
				z = 2 * ((firstwd >> 5) & 7);
#ifdef VG_DEBUG
				if (vg_print)
					if (errorlog) fprintf (errorlog,"%d,%d,", x, y);
#endif
				if (z == 0) {
#ifdef VG_DEBUG
					if (vg_print)
						if (errorlog) fprintf (errorlog,"blank");
#endif
					}
				else if (z == 2) {
					z = statz;
#ifdef VG_DEBUG
					if (vg_print)
						if (errorlog) fprintf (errorlog,"stat");
#endif
					}
#ifdef VG_DEBUG
				else if (vg_print)
					if (errorlog) fprintf (errorlog,"%d", z);
#endif
				oldx = currentx; oldy = currenty;
				deltax = x * scale; deltay = y * scale;
				currentx += deltax;
				currenty -= deltay;
				vector_timer (labs (deltax), labs (deltay));
				if (portrait)
					draw_line (oldx>>15, oldy>>14, currentx>>15, currenty>>14, color, z);
				else
					draw_line (oldx>>14, oldy>>14, currentx>>14, currenty>>14, color, z);
				break;
	
			case STAT:
				color = firstwd & 0x0f;
				statz = (firstwd >> 4) & 0x0f;
#ifdef VG_DEBUG
				if (vg_print)
					if (errorlog) fprintf (errorlog,"z: %d color: %d", statz, color);
#endif
				/* should do e, h, i flags here! */
				break;
      
			case SCAL:
				b = (firstwd >> 8) & 0x07;
				l = firstwd & 0xff;
				scale = (16384 - (l << 6)) >> b;
#ifdef 0
				scale = (1.0-(l/256.0)) * (2.0 / (1 << b));
#endif

#ifdef VG_DEBUG
				if (vg_print) {
					if (errorlog) fprintf (errorlog,"bin: %d, lin: ", b);
					if (l > 0x80)
						if (errorlog) fprintf (errorlog,"(%d?)", l);
					else
						if (errorlog) fprintf (errorlog,"%d", l);
					if (errorlog) fprintf (errorlog," scale: %f", (scale/8192.0));
					}
#endif
				break;
	
			case CNTR:
				d = firstwd & 0xff;
#ifdef VG_DEBUG
				if (vg_print) {
					if (d != 0x40)
						if (errorlog) fprintf (errorlog,"%d", d);
					}
#endif
				if (portrait) {
					currentx = 550 * 8192;
					currenty = 440 * 8192;
					}
				else {
					currentx = 511 * 8192;
					currenty = 511 * 8192;
					}
				break;
	
			case RTSL:
#ifdef VG_DEBUG
				if (vg_print)
					if ((firstwd & 0x1fff) != 0)
						if (errorlog) fprintf (errorlog,"(%d?)", firstwd & 0x1fff);
#endif
				if (sp == 0) {
					if (errorlog) fprintf (errorlog,"\n*** Vector generator stack underflow! ***\n");
					done = 1;
					sp = MAXSTACK - 1;
					}
				else
					sp--;
				pc = stack [sp];
				break;

			case HALT:
#ifdef VG_DEBUG
				if (vg_print)
					if ((firstwd & 0x1fff) != 0)
						if (errorlog) fprintf (errorlog,"(%d?)", firstwd & 0x1fff);
#endif
				done = 1;
				break;

			case JMPL:
				a = firstwd & 0x1fff;
#ifdef VG_DEBUG
				if (vg_print)
					if (errorlog) fprintf (errorlog,"%4x", map_addr(a));
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
				if (vg_print)
					if (errorlog) fprintf (errorlog,"%4x", map_addr(a));
#endif
				/* if a = 0x0000, treat as HALT */
				if (a == 0x0000)
					done = 1;
				else {
					stack [sp] = pc;
					if (sp == (MAXSTACK - 1)) {
						if (errorlog) fprintf (errorlog,"\n*** Vector generator stack overflow! ***\n");
						done = 1;
						sp = 0;
						}
					else
						sp++;
					pc = a;
					}
				break;
	
			default:
				if (errorlog) fprintf (errorlog,"internal error\n");
			}
#ifdef VG_DEBUG
		if (vg_print)
			if (errorlog) fprintf (errorlog,"\n");
#endif
		}

	close_page ();
	}

int vg_done (unsigned long cyc)
{
  if (vg_busy && (cyc < vg_done_cyc)) /* LBO 062797 */
    vg_busy = 0;

  return (! vg_busy);
}

void vg_go (int offset, int data)
{
 unsigned long	cyc;

 cyc = cpu_geticount ();
  vg_busy = 1;
  vg_done_cyc = cyc - 8; /* LBO 062797 */
#ifdef VG_DEBUG
  vgo_count++;
  if (trace_vgo)
    if (errorlog) fprintf (errorlog,"VGO #%d at cycle %d, delta %d\n", vgo_count, cyc, last_vgo_cyc-cyc);
  last_vgo_cyc = cyc;
#endif
  if (dvg)
    dvg_draw_vector_list ();
  else
    avg_draw_vector_list ();
}

void vg_reset (int offset, int data)
{
 unsigned long	cyc;

 cyc = cpu_geticount ();
  vg_busy = 0;
#ifdef VG_DEBUG
  if (trace_vgo)
    if (errorlog) fprintf (errorlog,"vector generator reset @%04x\n", cpu_getpc());
#endif
}

int vg_init (int len, int usingDvg)
{
	if (usingDvg) dvg = 1;
	else dvg = 0;
	return 0;
}

void vg_stop (void)
{
}

