/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"


unsigned char *mappy_sharedram;
unsigned char *mappy_customio_1,*mappy_customio_2;

static unsigned char interrupt_enable_1,interrupt_enable_2;
static int coin, credits, fire1, fire2, start1, start2;

static int crednum[] = { 1, 2, 3, 6, 1, 3, 1, 2 };
static int credden[] = { 1, 1, 1, 1, 2, 2, 3, 3 };


int mappy_init_machine(const char *gamename)
{
	/* Reset all flags */
	credits = coin = fire1 = fire2 = start1 = start2 = 0;

	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S | M6809_FAST_U;

	return 0;
}


int mappy_sharedram_r(int offset)
{
	return mappy_sharedram[offset];
}


int mappy_sharedram_r2(int offset)
{
	/* to speed up emulation, we check for the loop the sound CPU sits in most of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0x010a - 0x40 && mappy_sharedram[offset] == 0)
		cpu_seticount (0);
	return mappy_sharedram[offset];
}


int digdug2_sharedram_r2(int offset)
{
	/* to speed up emulation, we check for the loop the sound CPU sits in most of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0x0a1 - 0x40 && mappy_sharedram[offset] == 0 && cpu_getpc () == 0xe383)
		cpu_seticount (0);
	return mappy_sharedram[offset];
}


int mappy_cpu1ram_r(int offset)
{
	/* to speed up emulation, we check for the loop the main CPU sits in much of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0x1382 && RAM[offset] == 0)
		cpu_seticount (0);
	return RAM[offset];
}


int digdug2_cpu1ram_r(int offset)
{
	/* to speed up emulation, we check for the loop the main CPU sits in much of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0x1000 && RAM[offset] == 0 && cpu_getpc () == 0x80c4)
		cpu_seticount (0);
	return RAM[offset];
}


void mappy_sharedram_w(int offset,int data)
{
	mappy_sharedram[offset] = data;
}


void mappy_customio_w_1(int offset,int data)
{
	mappy_customio_1[offset] = data;
}


void mappy_customio_w_2(int offset,int data)
{
	mappy_customio_2[offset] = data;
}


int mappy_customio_r_1(int offset)
{
	static int testvals[] = { 8, 4, 6, 14, 13, 9, 13 };
	int val, temp;

	switch (mappy_customio_1[8])
	{
		/* mode 3 is the standard, and returns actual important values */
		case 1:
		case 3:
			switch (offset)
			{
				case 0:
					val = readinputport (3) & 0x0f;
					
					/* bit 0 is a trigger for the coin slot */
					if (val & 1)
					{
						if (!coin) ++credits, ++coin;
						if (coin != 1) val &= ~1;
					}
					else coin = 0;
					break;

				case 1:
					temp = readinputport (1) & 7;
					val = readinputport (3) >> 4;

					/* bit 0 is a trigger for the 1 player start */
					if (val & 1)
					{
						if (!start1 && credits >= credden[temp]) credits -= credden[temp], ++start1;
						if (start1 != 1) val &= ~1;
					}
					else start1 = 0;

					/* bit 1 is a trigger for the 2 player start */
					if (val & 2)
					{
						if (!start2 && credits >= 2 * credden[temp]) credits -= 2 * credden[temp], ++start2;
						if (start2 != 1) val &= ~2;
					}
					else start2 = 0;
					break;
					
				case 2:
					temp = readinputport (1) & 7;
					val = (credits * crednum[temp] / credden[temp]) / 10;
					break;
					
				case 3:
					temp = readinputport (1) & 7;
					val = (credits * crednum[temp] / credden[temp]) % 10;
					break;
					
				case 4:
					val = readinputport (2) & 0x0f;
					break;

				case 5:
					val = readinputport (2) >> 4;

					/* bit 0 is a trigger for the fire 1 key */
					if (val & 2)
					{
						if (!fire1) ++fire1;
						if (fire1 == 1) val |= 1;
					}
					else fire1 = 0;
					break;
					
				case 6:
				case 7:
					val = 0;
					break;
					
				default:
					val = mappy_customio_1[offset];
					break;
			}
			return val;
			
		case 5:
			credits = 0;
			if (offset >= 1 && offset <= 7) 
				return testvals[offset - 1];
			break;
	}
	return mappy_customio_1[offset];
}


int mappy_customio_r_2(int offset)
{
	static int testvals[] = { 8, 4, 6, 14, 13, 9, 13 };
	int val;

	switch (mappy_customio_2[8])
	{
		/* mode 4 is the standard, and returns actual important values */
		case 4:
			switch (offset)
			{
				case 0:
					val = readinputport (1) & 0x0f;
					break;

				case 1:
					val = readinputport (1) >> 4;
					break;

				case 2:
					val = readinputport (0) & 0x0f;
					break;

				case 3:
					val = 0;
					break;
					
				case 4:
					val = readinputport (0) >> 4;
					break;

				case 5:
					val = readinputport (4) >> 4;

					/* bit 0 is a trigger for the fire 2 key (Dig Dug 2 only) */
					if (val & 2)
					{
						if (!fire2) ++fire2;
						if (fire2 == 1) val |= 1;
					}
					else fire2 = 0;
					break;

				case 6:
					/* bit 3 = configuration mode (Mappy) */
					/* val |= 0x08; */
					val = readinputport (4) & 0x0f;
					break;

				case 7:
					/* bit 3 = configuration mode (Dig Dug 2) */
					/* val |= 0x08; */
					val = 0;
					break;

				default:
					val = mappy_customio_2[offset];
					break;
			}
			return val;
			
		case 5:
			credits = 0;
			if (offset >= 1 && offset <= 7) 
				return testvals[offset - 1];
	}
	return mappy_customio_2[offset];
}


void mappy_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = offset;
}



int mappy_interrupt_1(void)
{
	/* clear all the triggered inputs */
	if (start1 == 1) ++start1;
	if (start2 == 1) ++start2;
	if (fire1 == 1) ++fire1;
	if (fire2 == 1) ++fire2;
	if (coin == 1) ++coin;

	if (interrupt_enable_1) return INT_IRQ;
	else return INT_NONE;
}



void mappy_interrupt_enable_2_w(int offset,int data)
{
	interrupt_enable_2 = offset;
}



int mappy_interrupt_2(void)
{
	if (interrupt_enable_2) return INT_IRQ;
	else return INT_NONE;
}


void mappy_cpu_enable_w(int offset,int data)
{
	cpu_halt(1, offset);
}
