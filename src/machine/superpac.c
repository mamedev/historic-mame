/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6809/m6809.h"


unsigned char *superpac_sharedram;
unsigned char *superpac_customio_1,*superpac_customio_2;

static unsigned char interrupt_enable_1;
static int coin, credits, fire, start;

static int crednum[] = { 1, 2, 3, 6, 7, 1, 3, 1 };
static int credden[] = { 1, 1, 1, 1, 1, 2, 2, 3 };


int superpac_init_machine(const char *gamename)
{
	/* Reset all flags */
	coin = credits = fire = start = 0;

	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S | M6809_FAST_U;

	return 0;
}


int superpac_sharedram_r(int offset)
{
	return superpac_sharedram[offset];
}


int superpac_sharedram_r2(int offset)
{
	/* to speed up emulation, we check for the loop the sound CPU sits in most of the time
	   and end the current iteration (things will start going again with the next IRQ) */
	if (offset == 0xfb - 0x40 && superpac_sharedram[offset] == 0)
		m6809_ICount = 0;
	return superpac_sharedram[offset];
}


void superpac_sharedram_w(int offset,int data)
{
	superpac_sharedram[offset] = data;
}


void superpac_customio_w_1(int offset,int data)
{
	superpac_customio_1[offset] = data;
}


void superpac_customio_w_2(int offset,int data)
{
	superpac_customio_2[offset] = data;
}


void superpac_update_credits (void)
{
	int val = readinputport (3) & 0x0f, temp;
	if (val & 1)
	{
		if (coin) val &= ~1;
		else coin = 1, credits += 1;
	}
	else coin = 0;
	
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
}


int superpac_customio_r_1(int offset)
{
/*	static int testvals[] = { 8, 4, 6, 14, 13, 9, 13 };*/
	int val, temp;

	switch (superpac_customio_1[8])
	{
		/* mode 4 is the standard, and returns actual important values */
		case 4:
			switch (offset)
			{
				case 0:
					superpac_update_credits ();
					temp = readinputport (1) & 7;
					val = (credits * crednum[temp] / credden[temp]) / 10;
					break;
					
				case 1:
					superpac_update_credits ();
					temp = readinputport (1) & 7;
					val = (credits * crednum[temp] / credden[temp]) % 10;
					break;
					
				case 4:
					val = readinputport (2) & 0x0f;
					break;

				case 5:
					val = readinputport (2) >> 4;
					if (val & 2)
					{
						if (!fire) val |= 1, fire = 1;
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
					val = superpac_customio_1[offset];
					break;
			}
			return val;
		
		/* mode 8 is the test mode: always return 0 for these locations */
		case 8:
			credits = 0;
			if (offset >= 9 && offset <= 15) 
				return 0;
			break;
	}
	return superpac_customio_1[offset];
}


int superpac_customio_r_2(int offset)
{
/*	static int testvals[] = { 8, 4, 6, 14, 13, 9, 13 }; */
	int val;

	switch (superpac_customio_2[8])
	{
		/* mode 9 is the standard, and returns actual important values */
		case 9:
			switch (offset)
			{
				case 0:
					val = readinputport (1) & 0x0f;
					break;

				case 1:
					val = readinputport (1) >> 4;
					break;

				case 2:
					val = 0;
					break;

				case 3:
					val = readinputport (0) & 0x0f;
					break;
					
				case 4:
					val = readinputport (0) >> 4;
					break;

				case 5:
					val = 0;
					break;

				case 6:
					val = 0;
					/* bit 3 = configuration mode? */
					/* val |= 0x08; */
					break;

				case 7:
					val = 0;
					break;

				default:
					val = superpac_customio_2[offset];
					break;
			}
			return val;
			
		/* mode 8 is the test mode: always return 0 for these locations */
		case 8:
			credits = 0;
			if (offset >= 9 && offset <= 15) 
				return 0;
			break;
	}
	return superpac_customio_2[offset];
}


void superpac_interrupt_enable_1_w(int offset,int data)
{
	interrupt_enable_1 = offset;
}



int superpac_interrupt_1(void)
{
	if (interrupt_enable_1) return INT_IRQ;
	else return INT_NONE;
}


int superpac_interrupt_2(void)
{
	/* second CPU doesn't use IRQ */
	return INT_NONE;
}


void superpac_cpu_enable_w(int offset,int data)
{
	cpu_halt(1, offset);
}
