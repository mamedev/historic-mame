/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static int bankaddress;



void gng_bankswitch_w(int offset,int data)
{
	bankaddress = 0x10000 + data * 0x2000;
}



int gng_bankedrom_r(int offset)
{
	return RAM[bankaddress + offset];
}
