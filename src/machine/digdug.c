/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


unsigned char *digdug_sharedram;
static unsigned char interrupt_enable_1,interrupt_enable_2,interrupt_enable_3;
unsigned char digdug_hiscoreloaded;

static int credits;

static void *nmi_timer;

void digdug_halt_w(int offset,int data);


void digdig_init_machine(void)
{
	credits = 0;
	nmi_timer = 0;
	digdug_halt_w (0, 0);
}


int digdug_reset_r(int offset)
{
	extern unsigned char *RAM;


	digdug_hiscoreloaded = 0;

	return RAM[offset];
}


int digdug_sharedram_r(int offset)
{
	return digdug_sharedram[offset];
}


void digdug_sharedram_w(int offset,int data)
{
	/* a video ram write */
	if (offset < 0x400)
		dirtybuffer[offset] = 1;

	/* location 9b3d is set to zero just before CPU 2 spins */
	if (offset == 0x1b3d && data == 0 && cpu_getpc () == 0x1df1 && cpu_getactivecpu () == 1)
		cpu_spinuntil_int ();

	digdug_sharedram[offset] = data;
}


/***************************************************************************

 Emulate the custom IO chip.

***************************************************************************/
static int customio_command;
static unsigned char customio[16];
static int mode;

void digdug_customio_data_w(int offset,int data)
{
	customio[offset] = data;

if (errorlog) fprintf(errorlog,"%04x: custom IO offset %02x data %02x\n",cpu_getpc(),offset,data);
}


int digdug_customio_data_r(int offset)
{
	switch (customio_command)
	{
		case 0x71:
			if (offset == 0)
			{
				int p3 = readinputport (3);

				/* check if the user inserted a coin */
				if ((p3 & 0x01) == 0 && credits < 99)
					credits++;

				/* check if the user inserted a coin */
				if ((p3 & 0x02) == 0 && credits < 99)
					credits++;

				/* check for 1 player start button */
				if ((p3 & 0x10) == 0 && credits >= 1)
					credits--;

				/* check for 2 players start button */
				if ((p3 & 0x20) == 0 && credits >= 2)
					credits -= 2;

				if (mode)	/* switch mode */
					return (p3 & 0x80);
				else	/* credits mode: return number of credits in BCD format */
					return (credits / 10) * 16 + credits % 10;
			}
			else if (offset == 1)
			{
				int p2 = readinputport (2);

				if (mode == 0)
				{
					/* check directions, according to the following 8-position rule */
					/*         0          */
					/*        7 1         */
					/*       6 8 2        */
					/*        5 3         */
					/*         4          */
					if ((p2 & 0x01) == 0)		/* up */
						p2 = (p2 & ~0x0f) | 0x00;
					else if ((p2 & 0x02) == 0)	/* right */
						p2 = (p2 & ~0x0f) | 0x02;
					else if ((p2 & 0x04) == 0)	/* down */
						p2 = (p2 & ~0x0f) | 0x04;
					else if ((p2 & 0x08) == 0) /* left */
						p2 = (p2 & ~0x0f) | 0x06;
					else
						p2 = (p2 & ~0x0f) | 0x08;
				}

				return p2;
			}
			break;

		case 0xb1:	/* status? */
			if (offset <= 2)
				return 0;
			break;

		case 0xd2:	/* checking the dipswitches */
			if (offset == 0)
				return readinputport (0);
			else if (offset == 1)
				return readinputport (1);
			break;
	}

	return -1;
}


int digdug_customio_r(int offset)
{
	return customio_command;
}

void digdug_nmi_generate (int param)
{
	cpu_cause_interrupt (0, Z80_NMI_INT);
}


void digdug_customio_w(int offset,int data)
{
if (errorlog && data != 0x10 && data != 0x71) fprintf(errorlog,"%04x: custom IO command %02x\n",cpu_getpc(),data);

	customio_command = data;

	switch (data)
	{
		case 0x10:
			if (nmi_timer) timer_remove (nmi_timer);
			nmi_timer = 0;
			return;

		case 0xa1:	/* go into switch mode */
			mode = 1;
			break;

		case 0xc1:
		case 0xe1:	/* go into credit mode */
			mode = 0;
			break;

		case 0xb1:	/* status? */
			credits = 0;	/* this is a good time to reset the credits counter */
			break;
	}

	nmi_timer = timer_pulse (TIME_IN_USEC (50), 0, digdug_nmi_generate);
}



void digdug_halt_w(int offset,int data)
{
	static int reset23;

	data &= 1;
	if (data && !reset23)
	{
		cpu_reset (1);
		cpu_reset (2);
		cpu_halt (1,1);
		cpu_halt (2,1);
	}
	else if (!data)
	{
		cpu_halt (1,0);
		cpu_halt (2,0);
	}

	reset23 = data;
}



void digdug_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = (data&1);
}



int digdug_interrupt_1(void)
{
	if (interrupt_enable_1) return interrupt();
	else return ignore_interrupt();
}



void digdug_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = data & 1;
}



int digdug_interrupt_2(void)
{
	if (interrupt_enable_2) return interrupt();
	else return ignore_interrupt();
}



void digdug_interrupt_enable_3_w(int offset,int data)
{
	interrupt_enable_3 = !(data & 1);
}



int digdug_interrupt_3(void)
{
	if (interrupt_enable_3) return nmi_interrupt();
	else return ignore_interrupt();
}
