/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Tapper machine started by Chris Kirmse

***************************************************************************/

#include "driver.h"
#include "Z80.h"
#include "machine/Z80fmly.h"
#include "sndhrdw/generic.h"

int mcr_loadnvram;

extern int spyhunt_scrollx,spyhunt_scrolly;
static int spyhunt_mux;

static unsigned char soundlatch[4];
static unsigned char soundstatus;

static int vblholdcycles;

static int watchdogcount = 0;
static int watchdogmax;
static int watchdogon;

/* z80 ctc */
static Z80CTC ctc;


/***************************************************************************

  Generic MCR handlers

***************************************************************************/

void mcr_init_machine(void)
{
   int i;

	/* reset the sound */
   for (i = 0; i < 4; i++)
      soundlatch[i] = 0;
   soundstatus = 0;

   /* compute the duration of the VBL for the CTC -- assume it's is about 1/12th of the frame time */
   vblholdcycles = Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second / 12;
   z80ctc_reset (&ctc, Machine->drv->cpu[0].cpu_clock);

   /* can't load NVRAM right away */
   mcr_loadnvram = 0;

   /* set up the watchdog */
   watchdogon = 1;
   watchdogcount = 0;
   watchdogmax = Machine->drv->frames_per_second * Machine->drv->cpu[0].interrupts_per_frame;
}


void mcr_init_machine_no_watchdog(void)
{
	mcr_init_machine ();
	watchdogon = 0;
	mcr_loadnvram = 1;
}


int mcr_interrupt(void)
{
   int irq;

	/* watchdog time? */
	if (watchdogon && ++watchdogcount > watchdogmax)
	{
		machine_reset ();
		watchdogcount = 0;
		return ignore_interrupt ();
	}

   /* clock the external clock at 30Hz, but update more frequently */
   z80ctc_update (&ctc, 0, 0, 0);
   z80ctc_update (&ctc, 1, 0, 0);
   z80ctc_update (&ctc, 2, 0, 0);
   z80ctc_update (&ctc, 3, cpu_getiloops () == 0, vblholdcycles);

   /* handle any pending interrupts */
   irq = z80ctc_irq_r (&ctc);
   if (irq != Z80_IGNORE_INT)
      if (errorlog)
			fprintf (errorlog, "  (Interrupt from ch. %d)\n", (irq - ctc.vector) / 2);

   return irq;
}


void mcr_writeport(int port,int value)
{
	switch (port)
	{
		case 0:	/* OP0  Write latch OP0 (coin meters, 2 led's and cocktail 'flip') */
		   if (errorlog)
		      fprintf (errorlog, "mcr write to OP0 = %02i\n", value);
			return;

		case 4:	/* Write latch OP4 */
		   if (errorlog)
		      fprintf (errorlog, "mcr write to OP4 = %02i\n", value);
			return;

		case 0x1c:	/* WIRAM0 - write audio latch 0 */
		case 0x1d:	/* WIRAM0 - write audio latch 1 */
		case 0x1e:	/* WIRAM0 - write audio latch 2 */
		case 0x1f:	/* WIRAM0 - write audio latch 3 */
			soundlatch[port - 0x1c] = value;
			return;

		case 0xe0:	/* clear watchdog timer */
			watchdogcount = 0;
			return;

		case 0xe8:
			/* A sequence of values gets written here at startup; we don't know why;
			   However, it does give us the opportunity to tweak the IX register before
			   it's checked in Tapper and Timber, thus eliminating the need for patches
			   The value 5 is written last; we key on that, and only modify IX if it is
			   currently 0; hopefully this is 99.9999% safe :-) */
			if (value == 5)
			{
				Z80_Regs temp;
				Z80_GetRegs (&temp);
				if (temp.IX.D == 0)
					temp.IX.D += 1;
				Z80_SetRegs (&temp);
			}
			return;

		case 0xf0:	/* These are the ports of a Z80-CTC; it generates interrupts in mode 2 */
		case 0xf1:
		case 0xf2:
		case 0xf3:
	      z80ctc_w (&ctc, port - 0xf0, value);
	      return;
	}

	/* log all writes that end up here */
   if (errorlog)
      fprintf (errorlog, "mcr unknown write port %02x %02i\n", port, value);
}


int mcr_readport(int port)
{
	/* ports 0-4 are read directly via the input ports structure */
	port += 5;

   /* only a few ports here */
   switch (port)
   {
		case 0x07:	/* Read audio status register */

			/* once the system starts checking the sound, memory tests are done; load the NVRAM */
		   mcr_loadnvram = 1;
			return soundstatus;

		case 0x10:	/* Tron reads this as an alias to port 0 -- does this apply to all ports 10-14? */
			return cpu_readport (port & 0x0f);
   }

	/* log all reads that end up here */
   if (errorlog)
      fprintf (errorlog, "reading port %i (PC=%04X)\n", port, cpu_getpc ());
	return 0;
}


void mcr_soundstatus_w (int offset,int data)
{
   soundstatus = data;
}


int mcr_soundlatch_r (int offset)
{
   return soundlatch[offset];
}


/***************************************************************************

  Game-specific port handlers

***************************************************************************/

void spyhunt_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
		   spyhunt_mux = value;
		   break;

		case 0x84:
			spyhunt_scrollx = (spyhunt_scrollx & ~0xff) | value;
			break;

		case 0x85:
			spyhunt_scrollx = (spyhunt_scrollx & 0xff) | ((value & 0x07) << 8);
			spyhunt_scrolly = (spyhunt_scrolly & 0xff) | ((value & 0x80) << 1);
			break;

		case 0x86:
			spyhunt_scrolly = (spyhunt_scrolly & ~0xff) | value;
			break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


/***************************************************************************

  Game-specific input handlers

***************************************************************************/

/* Spy Hunter -- multiplexed steering wheel/gas pedal */
int spyhunt_port_r(int offset)
{
	int port = readinputport (6);

	/* mux high bit on means return steering wheel */
	if (spyhunt_mux & 0x80)
	{
		if (port & 8)
			return 0x94;
		else if (port & 4)
			return 0x54;
		else
			return 0x74;
	}

	/* mux high bit off means return gas pedal */
	else
	{
		static int val = 0x30;
		if (port & 1)
		{
			val += 4;
			if (val > 0xff) val = 0xff;
		}
		else if (port & 2)
		{
			val -= 4;
			if (val < 0x30) val = 0x30;
		}
		return val;
	}
}


/* Destruction Derby -- 6 bits of steering plus 2 bits of normal inputs */
int destderb_port_r(int offset)
{
	return readinputport (1 + offset);
}


/* Kozmik Krooz'r -- dial reader */
int kroozr_dial_r(int offset)
{
	int dial = readinputport (7);
	int val = readinputport (1);

	val |= (dial & 0x80) >> 1;
	val |= (dial & 0x70) >> 4;

	return val;
}


/* Kozmik Krooz'r -- joystick readers */
int kroozr_trakball_x_r(int data)
{
	int val = readinputport (6);

	if (val & 0x02)		/* left */
		return 0x64 - 0x34;
	if (val & 0x01)		/* right */
		return 0x64 + 0x34;
	return 0x64;
}

int kroozr_trakball_y_r(int data)
{
	int val = readinputport (6);

	if (val & 0x08)		/* up */
		return 0x64 - 0x34;
	if (val & 0x04)		/* down */
		return 0x64 + 0x34;
	return 0x64;
}
