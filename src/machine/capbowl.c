/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



void capbowl_rom_select_w(int offset,int data)
{
	int bankaddress;


	switch (data & 0x1f)
	{
		case 0:
		default:
			bankaddress = 0x10000;
			break;
		case 1:
			bankaddress = 0x14000;
			break;
		case 4:
			bankaddress = 0x18000;
			break;
		case 5:
			bankaddress = 0x1c000;
			break;
		case 8:
			bankaddress = 0x20000;
			break;
		case 9:
			bankaddress = 0x24000;
			break;
	}

	cpu_setbank(1,&RAM[bankaddress]);
}
