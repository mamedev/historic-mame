/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"

static int bankaddress;

void gunsmoke_bankswitch_w(int offset,int data)
{
if (bankaddress != 0x10000 + ((data & 0x0c) * 0x1000))
{
	bankaddress = 0x10000 + ((data & 0x0c) * 0x1000);
	memcpy(&RAM[0x8000],&RAM[bankaddress],0x4000);
}
}

int gunsmoke_bankedrom_r(int offset)
{
	return RAM[bankaddress + offset];
}
