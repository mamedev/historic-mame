/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int docastle_init_machine(const char *gamename)
{
	/* patch the roms to pass the communication test */
/*	RAM[0x01fe] = 0x18;*/
/*	RAM[0x01e2] = 0xc3;*/

	return 0;
}
