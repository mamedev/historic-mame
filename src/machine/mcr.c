/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Tapper machine started by Chris Kirmse

***************************************************************************/

#include <stdio.h>

#include "driver.h"
#include "Z80.h"
#include "machine/Z80fmly.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/dac.h"
#include "sndhrdw/5220intf.h"
#include "M6808/m6808.h"
#include "M6809/m6809.h"
#include "machine/6821pia.h"


int mcr_loadnvram;
int spyhunt_lamp[8];

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


/* used for the sound boards */
static int dacval;
static int buffered_data;



/* Chip Squeak Deluxe (CSD) interface */
static void csd_porta_w (int offset, int data);
static void csd_portb_w (int offset, int data);
static void csd_irq (void);

static pia6821_interface csd_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_NOP, PIA_DDRB, PIA_NOP, PIA_CTLA, PIA_NOP, PIA_CTLB, PIA_NOP }, /* offsets */
	{ 0 },            /* input port A */
	{ 0 },            /* input port B */
	{ csd_porta_w },  /* output port A */
	{ csd_portb_w },  /* output port B */
	{ 0 },            /* output CA2 */
	{ 0 },            /* output CB2 */
	{ csd_irq },      /* IRQ A */
	{ csd_irq },      /* IRQ B */
};


/* Sounds Good (SG) PIA interface */
static int sg_portb_r (int offset);
static void sg_porta_w (int offset, int data);
static void sg_portb_w (int offset, int data);
static void sg_irq (void);

static pia6821_interface sg_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_NOP, PIA_DDRB, PIA_NOP, PIA_CTLA, PIA_NOP, PIA_CTLB, PIA_NOP }, /* offsets */
	{ 0 },            /* input port A */
	{ sg_portb_r },	/* input port B */
	{ sg_porta_w },   /* output port A */
	{ sg_portb_w },   /* output port B */
	{ 0 },            /* output CA2 */
	{ 0 },            /* output CB2 */
	{ sg_irq },       /* IRQ A */
	{ sg_irq },       /* IRQ B */
};


/* Turbo Chip Squeak (TCS) PIA interface */
static int tcs_portb_r (int offset);
static void tcs_irq (void);

static pia6821_interface tcs_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_DDRB, PIA_CTLA, PIA_CTLB }, /* offsets */
	{ 0 },            /* input port A */
	{ tcs_portb_r },	/* input port B */
	{ DAC_data_w },   /* output port A */
	{ 0 },            /* output port B */
	{ 0 },            /* output CA2 */
	{ 0 },            /* output CB2 */
	{ tcs_irq },      /* IRQ A */
	{ tcs_irq },      /* IRQ B */
};


/* Squawk & Talk (SNT) PIA interface */
static int snt_porta1_r (int offset);
static void snt_porta1_w (int offset, int data);
static void snt_porta2_w (int offset, int data);
static void snt_portb2_w (int offset, int data);
static void snt_irq (void);

static pia6821_interface snt_pia_intf =
{
	2,                /* 2 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, /* offsets */
	{ snt_porta1_r, 0 },               /* input port A */
	{ 0, 0 },			      /* input port B */
	{ snt_porta1_w, snt_porta2_w },               /* output port A */
	{ 0, snt_portb2_w },               /* output port B */
	{ 0, 0 },               /* output CA2 */
	{ 0, 0 },            	/* output CB2 */
	{ snt_irq, snt_irq },   /* IRQ A */
	{ snt_irq, snt_irq },   /* IRQ B */
};


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

void spyhunt_init_machine (void)
{
   mcr_init_machine ();

   /* reset the PIAs */
   pia_startup (&csd_pia_intf);
}


void rampage_init_machine (void)
{
	mcr_init_machine ();
	watchdogon = 0;
	mcr_loadnvram = 1;

   /* reset the PIAs */
   buffered_data = -1;
   pia_startup (&sg_pia_intf);
}


void sarge_init_machine (void)
{
	mcr_init_machine ();
	watchdogon = 0;
	mcr_loadnvram = 1;

   /* reset the PIAs */
   buffered_data = -1;
   pia_startup (&tcs_pia_intf);
}


void dotron_init_machine (void)
{
   mcr_init_machine ();

   /* reset the PIAs */
   buffered_data = -1;
   pia_startup (&snt_pia_intf);
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

/* Translation table for one-joystick emulation */
static int one_joy_trans[32]={
        0x00,0x05,0x0A,0x00,0x06,0x04,0x08,0x00,
        0x09,0x01,0x02,0x00,0x00,0x00,0x00,0x00 };

int sarge_IN1_r(int offset)
{
	int res,res1;

	res=readinputport(1);
	res1=readinputport(6);

	res&=~one_joy_trans[res1&0xf];

	return (res);
}

int sarge_IN2_r(int offset)
{
	int res,res1;

	res=readinputport(2);
	res1=readinputport(6)>>4;

	res&=~one_joy_trans[res1&0xf];

	return (res);
}

void spyhunt_writeport(int port,int value)
{
	static int lastport4;

	switch (port)
	{
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			/* mux select is in bit 7 */
		   spyhunt_mux = value & 0x80;

	   	/* lamp driver command triggered by bit 5, data is in low four bits */
		   if (((lastport4 ^ value) & 0x20) && !(value & 0x20))
		   {
		   	if (value & 8)
		   		spyhunt_lamp[value & 7] = 1;
		   	else
		   		spyhunt_lamp[value & 7] = 0;
		   }

	   	/* CSD command triggered by bit 4, data is in low four bits */
	   	pia_1_portb_w (0, value & 0x0f);
	   	pia_1_ca1_w (0, value & 0x10);

		   /* remember the last value */
		   lastport4 = value;
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


void rampage_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
		case 0x05:
			break;

		case 0x06:
	   	pia_1_portb_w (0, (value >> 1) & 0x0f);
	   	pia_1_ca1_w (0, ~value & 0x01);

	   	/* need to give the sound CPU time to process the first nibble */
	   	cpu_seticount (0);
	   	break;

		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


void sarge_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
		case 0x05:
			break;

		case 0x06:
	   	pia_1_portb_w (0, (value >> 1) & 0x0f);
	   	pia_1_ca1_w (0, ~value & 0x01);

	   	/* need to give the sound CPU time to process the first nibble */
	   	cpu_seticount (0);
			break;

		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


void dotron_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
	   	pia_1_porta_w (0, ~value & 0x0f);
	   	pia_1_cb1_w (0, ~value & 0x10);

	   	/* need to give the sound CPU time to process the first nibble */
	   	if (value & 0x10)
		   	cpu_seticount (0);
   		cpu_halt(2, 1);
			break;

		case 0x05:
		case 0x06:
		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


/***************************************************************************

  Game-specific input handlers

***************************************************************************/

/* Spy Hunter -- normal port plus CSD status bits */
int spyhunt_port_1_r(int offset)
{
	return input_port_1_r(offset) | ((pia_1_portb_r (0) & 0x30) << 1);
}


/* Spy Hunter -- multiplexed steering wheel/gas pedal */
int spyhunt_port_2_r(int offset)
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


/***************************************************************************

  Sound board-specific PIA handlers

***************************************************************************/

/*
 *		Chip Squeak Deluxe (Spy Hunter) board
 *
 *		MC68000, 1 PIA, 10-bit DAC
 */
static void csd_porta_w (int offset, int data)
{
	int temp;
	dacval = (dacval & ~0x3fc) | (data << 2);
	temp = dacval/2;
	if (temp > 0xff) temp = 0xff;
	DAC_data_w (0, temp);
}

static void csd_portb_w (int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void csd_irq (void)
{
  	cpu_cause_interrupt (2, 4);
}


/*
 *		Sounds Good (Rampage) board
 *
 *		MC68000, 1 PIA, 10-bit DAC
 *		Extra handshaking needed because data is written in 2 nibbles
 */
static int sg_portb_r (int offset)
{
	if (buffered_data != -1)
	{
		int temp = buffered_data;
		buffered_data = -1;
		cpu_seticount (0);
		return temp;
	}
	return pia_1_portb_r (offset);
}

static void sg_porta_w (int offset, int data)
{
	int temp;
	dacval = (dacval & ~0x3fc) | (data << 2);
	temp = dacval/2;
	if (temp > 0xff) temp = 0xff;
	DAC_data_w (0, temp);
}

static void sg_portb_w (int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void sg_irq (void)
{
  	cpu_cause_interrupt (1, 4);
   buffered_data = pia_1_portb_r (0);
}


/*
 *		Turbo Chip Squeak (Sarge) board
 *
 *		MC6809, 1 PIA, 8-bit DAC
 *		Extra handshaking needed because data is written in 2 nibbles
 */
static int tcs_portb_r (int offset)
{
	if (buffered_data != -1)
	{
		int temp = buffered_data;
		buffered_data = -1;
		cpu_seticount (0);
		return temp;
	}
	return pia_1_portb_r (offset);
}

static void tcs_irq (void)
{
	cpu_cause_interrupt (1, M6809_INT_IRQ);
   buffered_data = pia_1_portb_r (0);
}


/*
 *		Squawk & Talk (Discs of Tron) board
 *
 *		MC6802, 2 PIAs, TMS5220, AY8912 (not used), 8-bit DAC (not used)
 *		Extra handshaking needed because data is written in 2 nibbles
 */
static int tms_command;
static int tms_strobes;
static int halt_on_read;

static int snt_porta1_r (int offset)
{
  	cpu_seticount (0);
  	if (halt_on_read)
		cpu_halt(2, 0);
	halt_on_read = 0;
	return pia_1_porta_r (offset);
}

static void snt_porta1_w (int offset, int data)
{
	/*printf ("Write to AY-8912\n");*/
}

static void snt_porta2_w (int offset, int data)
{
	tms_command = data;
}

static void snt_portb2_w (int offset, int data)
{
	/* bits 0-1 select read/write strobes on the TMS5220 */
	data &= 0x03;
	if (((data ^ tms_strobes) & 0x02) && !(data & 0x02))
	{
		tms5220_data_w (offset, tms_command);

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_2_ca2_w (0,1);
		pia_2_ca2_w (0,0);
	}
	else if (((data ^ tms_strobes) & 0x01) && !(data & 0x01))
	{
		pia_2_porta_w (0,tms5220_status_r(offset));

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_2_ca2_w (0,1);
		pia_2_ca2_w (0,0);
	}
	tms_strobes = data;
}

static void snt_irq (void)
{
	cpu_cause_interrupt (2, M6808_INT_IRQ);
	halt_on_read = 1;
}
