/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static int bank;



int c1942_init_machine(const char *gamename)
{
	bank = -1;

	return 0;
}



void c1942_bankswitch_w(int offset,int data)
{
	if ((data & 0x03) != bank)
	{
		memcpy(&RAM[0x8000],&RAM[0x10000 + (data & 0x03) * 0x4000],0x4000);

		bank = data & 0x03;
	}
}



int c1942_interrupt(void)
{
	static int count;

	count = (count + 1) % 2;

	if (count) return 0x00cf;	/* RST 08h */
	else return 0x00d7;	/* RST 10h */
}
