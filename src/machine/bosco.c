/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"


unsigned char *bosco_sharedram;
static unsigned char interrupt_enable_1,interrupt_enable_2,interrupt_enable_3;
unsigned char bosco_hiscoreloaded;
int		HiScore;
int		Score,Score1,Score2;

static int credits;

void bosco_sample_play(int, int);
void bosco_vh_interrupt(void);

static void *nmi_timer_1, *nmi_timer_2;

void bosco_halt_w(int offset,int data);

void bosco_init_machine(void)
{
	credits = 0;
	nmi_timer_1 = nmi_timer_2 = 0;
	bosco_halt_w (0, 0);

	Machine->memory_region[0][0x8c00] = 1;
	Machine->memory_region[0][0x8c01] = 1;
}


int bosco_reset_r(int offset)
{
	extern unsigned char *RAM;


	bosco_hiscoreloaded = 0;

	return RAM[offset];
}


int bosco_sharedram_r(int offset)
{
	return bosco_sharedram[offset];
}



void bosco_sharedram_w(int offset,int data)
{
	bosco_sharedram[offset] = data;
}



int bosco_dsw_r(int offset)
{
	int bit0,bit1;


	bit0 = (input_port_0_r(0) >> offset) & 1;
	bit1 = (input_port_1_r(0) >> offset) & 1;

	return bit0 | (bit1 << 1);
}



/***************************************************************************

 Emulate the custom IO chip.

***************************************************************************/
static int customio_command_1;
static unsigned char customio_1[16];
static int mode;


void bosco_customio_data_w_1 (int offset,int data)
{
	customio_1[offset] = data;

if (errorlog) fprintf(errorlog,"%04x: custom IO offset %02x data %02x\n",cpu_getpc(),offset,data);
}


int bosco_customio_data_r_1 (int offset)
{
	switch (customio_command_1)
	{
		case 0x71:
			if (offset == 0)
			{
				int p4 = readinputport (4);

				/* check if the user inserted a coin */
				if ((p4 & 0x10) == 0 && credits < 99)
					credits++;

				/* check if the user inserted a coin */
				if ((p4 & 0x20) == 0 && credits < 99)
					credits++;

				/* check if the user inserted a coin */
				if ((p4 & 0x40) == 0 && credits < 99)
					credits++;

				/* check for 1 player start button */
				if ((p4 & 0x04) == 0 && credits >= 1)
					credits--;

				/* check for 2 players start button */
				if ((p4 & 0x08) == 0 && credits >= 2)
					credits -= 2;

				if (mode)	/* switch mode */
					return (p4 & 0x80);
				else	/* credits mode: return number of credits in BCD format */
					return (credits / 10) * 16 + credits % 10;
			}
			else if (offset == 1)
			{
				int in = readinputport(2), dir;

			/*
				  Direction is returned as shown below:
								0
							7		1
						6				2
							5		3
								4
				  For the previous direction return 8.
			 */
				dir = 8;
				if ((in & 0x01) == 0)		/* up */
				{
					if ((in & 0x02) == 0)	/* right */
						dir = 1;
					else if ((in & 0x08) == 0) /* left */
						dir = 7;
					else
						dir = 0;
				}
				else if ((in & 0x04) == 0)	/* down */
				{
					if ((in & 0x02) == 0)	/* right */
						dir = 3;
					else if ((in & 0x08) == 0) /* left */
						dir = 5;
					else
						dir = 4;
				}
				else if ((in & 0x02) == 0)	/* right */
					dir = 2;
				else if ((in & 0x08) == 0) /* left */
					dir = 6;

				/* check fire */
				dir |= (in & 0x10);

				return dir;
			}
			break;

		case 0x94:
			if (offset == 0)
			{
				int hi = (Score / 10000000) % 10;
				int lo = (Score / 1000000) % 10;
				if (Score >= HiScore)
				{
					HiScore = Score;
					return ((hi * 16) + lo) | 0x80;
				}
				else
					return (hi * 16) + lo;
			}
			else if (offset == 1)
			{
				int hi = (Score / 100000) % 10;
				int lo = (Score / 10000) % 10;
				return (hi * 16) + lo;
			}
			else if (offset == 2)
			{
				int hi = (Score / 1000) % 10;
				int lo = (Score / 100) % 10;
				return (hi * 16) + lo;
			}

			else if (offset == 3)
			{
				int hi = (Score / 10) % 10;
				int lo = Score % 10;
				return (hi * 16) + lo;
			}
			break;

		case 0x91:
			if (offset <= 2)
				return 0;
			break;
	}

	return -1;
}


int bosco_customio_r_1 (int offset)
{
	return customio_command_1;
}

void bosco_nmi_generate_1 (int param)
{
	cpu_cause_interrupt (0, Z80_NMI_INT );
}

void bosco_customio_w_1 (int offset,int data)
{
	Z80_Regs regs;

	customio_command_1 = data;

	switch (data)
	{
		case 0x10:
			if (nmi_timer_1) timer_remove (nmi_timer_1);
			nmi_timer_1 = 0;
			return;

		case 0x48:
			Z80_GetRegs(&regs);
			switch(regs.HL2.D)
			{
				case 0x16F0:	 //		Mid Bang
					sample_start (0, 0, 0);
					break;
				case 0x16F2:	 //		Big Bang
					sample_start (1, 1, 0);
					break;
				case 0x16F4:	 //		Shot
					sample_start (2, 2, 0);
					break;
			}
			break;

		case 0x64:
			Z80_GetRegs(&regs);
			switch(cpu_readmem16(regs.HL2.D))	/* ASG 971005 */
			{
				case 0x01:	/*	??	*/
					break;
				case 0x10:	/*	??	*/
					break;
				case 0x40:	/*	??	*/
					break;
				case 0x60:	/* 1P Score */
					Score2 = Score;
					Score = Score1;
					break;
				case 0x68:	/* 2P Score */
					Score1 = Score;
					Score = Score2;
					break;
				case 0x80:	/*	??	*/
					break;
				case 0x81:
					Score += 10;
					break;
				case 0x83:
					Score += 20;
					break;
				case 0x87:
					Score += 50;
					break;
				case 0x88:
					Score += 60;
					break;
				case 0x8D:
					Score += 200;
					break;
				case 0x93:
					Score += 200;
					break;
				case 0x95:
					Score += 300;
					break;
				case 0x96:
					Score += 400;
					break;
				case 0xA0:
					Score += 500;
					break;
				case 0xA1:
					Score += 1000;
					break;
				case 0xA2:
					Score += 1500;
					break;
				case 0xB7:
					Score += 500;
					break;
				case 0xC3:	/*	??	*/
					break;
#if 0
				case 0x89:
					Score += ???;
					break;
				case 0xB8:
					Score += ???;
					break;
#endif
				default:
					if (errorlog)
						fprintf(errorlog,"unknown score: %02x\n",
								cpu_readmem16(regs.HL2.D));	/* ASG 971005 */
					break;
			}
			break;

		case 0x61:
			mode = 1;
			break;

		case 0xC1:
			Score = 0;
			Score1 = 0;
			Score2 = 0;
			break;

		case 0xC8:
			break;

		case 0x84:
			break;

		case 0x91:
			mode = 0;
			break;

		case 0xa1:
			mode = 1;
			break;
	}

	nmi_timer_1 = timer_pulse (TIME_IN_USEC (50), 0, bosco_nmi_generate_1);
}



/***************************************************************************

 Emulate the second (!) custom IO chip.

***************************************************************************/
static int customio_command_2;
static unsigned char customio_2[16];

void bosco_customio_data_w_2 (int offset,int data)
{
	customio_2[offset] = data;

if (errorlog) fprintf(errorlog,"%04x: custom IO offset %02x data %02x\n",cpu_getpc(),offset,data);
}


int bosco_customio_data_r_2 (int offset)
{
	switch (customio_command_2)
	{
		case 0x91:
			if (offset == 2)
				return cpu_readmem16(0x89cc);
			else if (offset <= 3)
				return 0;
			break;
	}

	return -1;
}


int bosco_customio_r_2 (int offset)
{
	return customio_command_2;
}

void bosco_nmi_generate_2 (int param)
{
	cpu_cause_interrupt (1, Z80_NMI_INT);
}

void bosco_customio_w_2 (int offset,int data)
{
	Z80_Regs regs;

	customio_command_2 = data;

	switch (data)
	{
		case 0x10:
			if (nmi_timer_2) timer_remove (nmi_timer_2);
			nmi_timer_2 = 0;
			return;

		case 0x82:
			Z80_GetRegs(&regs);
			switch (regs.HL2.D)
			{
				case 0x1BEE:	// Blast Off
					bosco_sample_play(0x0020 * 2, 0x08D7 * 2);
					break;
				case 0x1BF1:	// Alert, Alert
					bosco_sample_play(0x8F7 * 2, 0x0906 * 2);
					break;
				case 0x1BF4:	// Battle Station
					bosco_sample_play(0x11FD * 2, 0x07DD * 2);
					break;
				case 0x1BF7:	// Spy Ship Sighted
					bosco_sample_play(0x19DA * 2, 0x07DE * 2);
					break;
				case 0x1BFA:	// Condition Red
					bosco_sample_play(0x21B8 * 2, 0x079F * 2);
					break;
			}
			break;
	}

	nmi_timer_2 = timer_pulse (TIME_IN_USEC (50), 0, bosco_nmi_generate_2);
}






void bosco_halt_w(int offset,int data)
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



void bosco_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = (data&1);
}



int bosco_interrupt_1(void)
{
	bosco_vh_interrupt();	/* update the background stars position */

	if (interrupt_enable_1) return interrupt();
	else return ignore_interrupt();
}



void bosco_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = data & 1;
}



int bosco_interrupt_2(void)
{
	if (interrupt_enable_2) return interrupt();
	else return ignore_interrupt();
}



void bosco_interrupt_enable_3_w(int offset,int data)
{
	interrupt_enable_3 = !(data & 1);
}



int bosco_interrupt_3(void)
{
	if (interrupt_enable_3) return nmi_interrupt();
	else return ignore_interrupt();
}
