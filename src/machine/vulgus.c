/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static int bankaddress;



void vulgus_bankswitch_w(int offset,int data)
{
	bankaddress = 0x10000 + (data & 0x03) * 0x4000;
}



int vulgus_bankedrom_r(int offset)
{
	return RAM[bankaddress + offset];
}



int vulgus_interrupt(void)
{
	static int count;

	count = (count + 1) % 2;

	if (count) return 0x00cf;	/* RST 08h */
	else return 0x00d7;	/* RST 10h */
}
