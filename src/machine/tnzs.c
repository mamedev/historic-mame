/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Commands for the MCU (i8742), written to $c001 on the 2nd CPU:

  0x01: Read player 1 joystick
  0x02: Read player 2 joystick
  0x1a: Read status of coin slots
  0x21: Read service & tilt switches
  0x41: Reset credit counter
  0xa1: Read number of credits, then player 1 buttons

  On startup, the MCU returns the sequence 0x5a/0xa5/0x55

  The MCU also takes care of handling the coin inputs and the tilt switch.
  To simulate this, we read the status in the interrupt handler for the main
  CPU and update the counters appropriately. We also must take care of
  handling the coin/credit settings ourselves.

***************************************************************************/

#include "driver.h"

extern unsigned char *tnzs_workram;

static int tnzs_credits;
static int tnzs_coin_a, tnzs_dip_a, tnzs_dip_b;
static int tnzs_oldcoin_a, tnzs_oldcoin_b, tnzs_oldservice;
static int tnzs_tilt;

static int tnzs_inputport;
static int tnzs_mcu_1;

static void tnzs_update_mcu (void);

static int current_inputport;	/* reads of c000 (sound cpu) expect a sequence of values */
static int number_of_credits;


void arkanoi2_init_machine (void)
{
    current_inputport = -3;
    number_of_credits = 0;
}


/* number of input ports to be cycled through (coins, buttons etc.) */
#define ip_num 2

int arkanoi2_inputport_r(int offset)
{
	int ret;

	if (offset == 0)
	{
        switch(current_inputport)
		{
			case -3: ret = 0x55; break;
			case -2: ret = 0xaa; break;
			case -1: ret = 0x5a; break;
		case ip_num: current_inputport = 0; /* fall through */
			case 0:  ret = number_of_credits; break;
			default: ret = readinputport(current_inputport+2); break;
		}
	  current_inputport++;
	  return ret;
	}
	else
		return (0x01);	/* 0xE1 for tilt, 31 service ok */
}


void arkanoi2_inputport_w(int offset, int data)
{
	if (offset == 0)
		number_of_credits -= ((~data)&0xff)+1;	/* sub data from credits */
	else
/* TBD: value written (or its low nibble) subtracted from sequence index? */
		if (current_inputport>=0)	/* if the initial sequence is done */
			current_inputport=0;	/* reset input port number */
}


int arkanoi2_sh_f000_r(int offs)
{
	int val;

	val = readinputport(2);
	if (offs == 0)
	{
		if (errorlog) fprintf (errorlog, "f000_r: %02x\n", val & 0xff);
		return val & 0xff;
	}
	else
	{
		if (errorlog) fprintf (errorlog, "f001_r: %02x\n", (val >> 8) & 0xff);
		return ((val >> 8) & 0xff);
	}
}

int arkanoi2_interrupt(void)
{
	static int insertcoin;
	int t;

    /* Update credit counter. The game somehow tells the hardware when
       to decrease the counter. (maybe by writing to c001 on the 2nd cpu?) */
	t = (readinputport(2) & 0x7000) ^ 0x5000;
	if (t && !insertcoin)
		number_of_credits++;
	insertcoin = t;

	return 0;
}




int tnzs_interrupt (void)
{
	static int insertcoin;
	int t;

    /* Update credit counter. The game somehow tells the hardware when
       to decrease the counter. (maybe by writing to c001 on the 2nd cpu?) */
	t = osd_key_pressed(OSD_KEY_3);
	if (t && !insertcoin)
		number_of_credits++;
	insertcoin = t;

	tnzs_update_mcu ();
	return 0;
}

void tnzs_bankswitch_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (errorlog && (data < 0x10 || data > 0x17))
	{
		fprintf(errorlog, "WARNING: writing %02x to bankswitch\n", data);
		return;
	}

//	if (errorlog) fprintf(errorlog, "writing %02x to bankswitch\n", data);
	cpu_setbank (1, &RAM[0x10000 + 0x4000 * (data & 0x07)]);
}

void tnzs_bankswitch1_w (int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

//	if (errorlog) fprintf (errorlog, "writing %02x to bankswitch 1\n", data);
	cpu_setbank (2, &RAM[0x10000 + 0x2000 * (data & 3)]);
}

void tnzs_init_machine (void)
{
    tnzs_credits = 0;
    tnzs_coin_a = 0;
    tnzs_oldcoin_a = readinputport (3) & 0x01;
    tnzs_oldcoin_b = readinputport (3) & 0x02;
    tnzs_oldservice = readinputport (4) & 0x01;
    tnzs_tilt = 0;

    tnzs_dip_a = readinputport (0) & 0x30;
    tnzs_dip_b = readinputport (0) & 0xc0;

    tnzs_inputport = -4;
    tnzs_mcu_1 = 0;

    /* Set up the banks */
    tnzs_bankswitch_w  (0, 0);
    tnzs_bankswitch1_w (0, 0);
}

int tnzs_workram_r (int offset)
{
	return tnzs_workram[offset];
}

void tnzs_workram_w (int offset, int data)
{
	tnzs_workram[offset] = data;
}

/* i8742 MCU */
static void tnzs_update_mcu (void)
{
	int val;

    /* read coin switch A */
    val = readinputport (3) & 0x01;
    if (val && !tnzs_oldcoin_a)
    {
    	tnzs_coin_a ++;

    	/* coin was inserted, adjust based on the dip settings */
    	switch (tnzs_dip_a)
    	{
    		case 0x30: tnzs_credits ++; tnzs_coin_a = 0; break;
    		case 0x20: if (tnzs_coin_a == 2) { tnzs_credits ++; tnzs_coin_a = 0; } break;
    		case 0x10: if (tnzs_coin_a == 3) { tnzs_credits ++; tnzs_coin_a = 0; } break;
    		case 0x00: if (tnzs_coin_a == 4) { tnzs_credits ++; tnzs_coin_a = 0; } break;
    	}
    }
    tnzs_oldcoin_a = val;

	/* read coin switch B */
    val = readinputport (3) & 0x02;
    if (val && !tnzs_oldcoin_b)
    {
    	/* coin was inserted, adjust based on the dip settings */
    	switch (tnzs_dip_b)
    	{
    		case 0xc0: tnzs_credits += 2; break;
    		case 0x80: tnzs_credits += 3; break;
    		case 0x40: tnzs_credits += 4; break;
    		case 0x00: tnzs_credits += 6; break;
    	}
    }
    tnzs_oldcoin_b = val;

	/* read the service coin switch */
    val = readinputport (4) & 0x01;
    if (val && !tnzs_oldservice) tnzs_credits ++;
    tnzs_oldservice = val;

	/* clamp the maximum number of credits to 9 */
	if (tnzs_credits > 9) tnzs_credits = 9;

	/* read the tilt switch */
    if ((readinputport (4) & 0x02) == 0x00) tnzs_tilt = 1;
}

int tnzs_mcu_r (int offset)
{
	if (offset == 0)
	{
		tnzs_inputport ++;

		/* check for the startup sequence */
        switch (tnzs_inputport)
		{
			case -3: return 0x5a;
			case -2: return 0xa5;
			case -1: return 0x55;
		}

		/* otherwise handle the inputs */
		switch (tnzs_mcu_1)
		{
			case 0x1a: return (readinputport (3));
			case 0x21: return (readinputport (4));
			case 0x01: return (readinputport (5));
			case 0x02: return (readinputport (6));

			/* Reset the coin counter */
			case 0x41: return 0xff;

			/* Read the coin counter or the inputs */
			case 0xa1:
				if (tnzs_inputport == 1)
					return tnzs_credits;
				else
					return readinputport (2);
				break;

			/* Should not be reached */
			default:
#ifdef macintosh
				SysBeep (0);
#endif
				if (errorlog) fprintf (errorlog, "*** %02x, %02x\n", tnzs_mcu_1, tnzs_inputport);
				return 0xff;
				break;
		}
	}
	else
	{
		if (tnzs_tilt) return 0xe8; /* TODO: verify this */
		else           return 0x01;
	}
}

void tnzs_mcu_w (int offset, int data)
{
	if (offset == 0)
	{
        tnzs_inputport = (tnzs_inputport + 5) % 6;
		if (errorlog) fprintf(errorlog, "* $c000: %02x, count set back to %d\n", data, tnzs_inputport);
	}
	else
	{
		tnzs_mcu_1 = data;
		if (data == 0x41)
		{
			tnzs_credits --;
			if (tnzs_credits < 0) tnzs_credits = 0;
		}
		if (data == 0xa1) tnzs_inputport = 0;
//		if (errorlog) fprintf (errorlog, "** $c001: %02x\n", data);
	}
}
