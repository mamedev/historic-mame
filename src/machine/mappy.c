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
static int coin, cointrig, credits, fire, start;

static int crednum[] = { 1, 2, 3, 6, 1, 3, 1, 2 };
static int credden[] = { 1, 1, 1, 1, 2, 2, 3, 3 };


int mappy_init_machine(const char *gamename)
{
	/* Reset all flags */
	coin = cointrig = credits = fire = start = 0;

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
		m6809_ICount = 0;
	return mappy_sharedram[offset];
}


int mappy_cpu1ram_r(int offset)
{
	/* to speed up emulation, we check for the loop the main CPU sits in much of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0x1382 && RAM[offset] == 0)
		m6809_ICount = 0;
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
					if (val & 1)
					{
						if (coin)
						{
							if (!cointrig) val &= ~1;
							else cointrig -= 1;
						}
						else coin = cointrig = 1, credits += 1;
					}
					else coin = 0;
					break;

				case 1:
					temp = readinputport (1) & 7;
					val = readinputport (3) >> 4;
					if (val & 1)
					{
						if (start || credits < credden[temp]) val &= ~1;
						else credits -= credden[temp], start = 1;
					}
					else start = 0;
					if (val & 2)
					{
						if (start || credits < 2 * credden[temp]) val &= ~2;
						else credits -= 2 * credden[temp], start = 1;
					}
					else start = 0;
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
					if (val & 1)
					{
						if (fire) val &= ~1;
						else fire = 1;
					}
					else fire = 0;
					break;
					
				case 6:
					val = 0xf;
					break;
					
				case 7:
					val = 0xf;
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
					val = 0;
					break;

				case 6:
					val = 0;
					/* bit 3 = configuration mode */
					/* val |= 0x08; */
					break;

				case 7:
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
