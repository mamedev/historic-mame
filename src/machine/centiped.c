/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int centiped_init_machine(const char *gamename)
{
	/* patch the roms to pass the startup test */
	RAM[0x38a8] = 0xea;
	RAM[0x38a9] = 0xea;
	RAM[0x38ae] = 0xea;
	RAM[0x38af] = 0xea;

	return 0;
}



int centiped_rand_r(int offset)
{
	return rand();
}
