/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int elevator_init_machine(const char *gamename)
{
	/* patch the roms to remove protection check */
	RAM[0x33c6] = 0;
	RAM[0x33c7] = 0;
	RAM[0x33c8] = 0;

	/* remove ROM checkusm check as well */
	RAM[0x34be] = 0xc9;

	return 0;
}
