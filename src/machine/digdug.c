/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"
#include "vidhrdw/generic.h"


unsigned char *digdug_sharedram;
static unsigned char interrupt_enable_1,interrupt_enable_2,interrupt_enable_3;
unsigned char digdug_hiscoreloaded;

static int credits;

void digdig_init_machine(void)
{
	/* halt the slave CPUs until they're reset */
	cpu_halt(1,0);
	cpu_halt(2,0);

	credits = 0;
}


int digdug_reset_r(int offset)
{
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
	if (offset == 0x1b3d && data == 0 && cpu_getpc () == 0x1df1 && Z80_ICount > 50)
		Z80_ICount = 50;

	/* location 9b3c is set to zero just before CPU 3 spins */
	if (offset == 0x1b3c && data == 0 && cpu_getpc () == 0x0316 && Z80_ICount > 50)
		Z80_ICount = 50;

	digdug_sharedram[offset] = data;
}


/***************************************************************************

 Emulate the custom IO chip.

 In the real digdug machine, the chip would cause an NMI on CPU #1 to ask
 for data to be transferred. We don't bother causing the NMI, we just look
 into the CPU register to see where the data has to be read/written to, and
 emulate the behaviour of the NMI interrupt.

***************************************************************************/
void digdug_customio_w(int offset,int data)
{
	static int mode;
	Z80_Regs regs;

	switch (data)
	{
		case 0x10:	/* nop */
			return;

		case 0x71:
			{
				int p2 = readinputport (2);
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

				if (mode)	/* switch mode */
					cpu_writemem16(0x7000,p3 & 0x80);
				else	/* credits mode: return number of credits in BCD format */
					cpu_writemem16(0x7000,(credits / 10) * 16 + credits % 10);

				cpu_writemem16(0x7000 + 1,p2);
				cpu_writemem16(0x7000 + 2,0xff);
			}
			break;

		case 0xb1:	/* status? */
			credits = 0;	/* this is a good time to reset the credits counter */
			cpu_writemem16(0x7000,0);
			cpu_writemem16(0x7000 + 1,0);
			cpu_writemem16(0x7000 + 2,0);
			break;

		case 0xa1:	/* go into switch mode */
			mode = 1;
			break;

		case 0xc1:
		case 0xe1:	/* go into credit mode */
			mode = 0;
			break;

		case 0xd2:	/* checking the dipswitches */
			cpu_writemem16(0x7000,readinputport(0));
			cpu_writemem16(0x7001,readinputport(1));
			break;

		default:
			if (errorlog) fprintf(errorlog,"%04x: warning: unknown custom IO command %02x\n",cpu_getpc(),data);
			break;
	}

	/* copy all but the last byte of the data into the destination, just like the NMI */
	Z80_GetRegs(&regs);
	while (regs.BC2.D > 1)
	{
		cpu_writemem16(regs.DE2.D,cpu_readmem16(regs.HL2.D));	/* ASG 971005 */
		++regs.DE2.W.l;
		++regs.HL2.W.l;
		--regs.BC2.W.l;
	}

	Z80_SetRegs(&regs);

	/* actually generate an NMI for the final byte, to handle special processing */
	cpu_cause_interrupt(0,Z80_NMI_INT);
}



int digdug_customio_r(int offset)
{
	return 0x10;	/* everything is handled by customio_w() */
}



void digdug_halt_w(int offset,int data)
{
	cpu_halt(1,data);
	cpu_halt(2,data);
}



void digdug_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = data;
}



int digdug_interrupt_1(void)
{
	if (interrupt_enable_1) return 0xff;
	else return Z80_IGNORE_INT;
}



void digdug_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = data;
}



int digdug_interrupt_2(void)
{
	if (interrupt_enable_2) return 0xff;
	else return Z80_IGNORE_INT;
}



void digdug_interrupt_enable_3_w(int offset,int data)
{
	interrupt_enable_3 = data;
}



int digdug_interrupt_3(void)
{
	if (interrupt_enable_3) return Z80_IGNORE_INT;
	else return Z80_NMI_INT;
}
