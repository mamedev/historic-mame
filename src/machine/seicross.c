/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int seicross_init_machine(const char *gamename)
{
	/* patch the roms to pass startup test */
	RAM[0x2225] = 0x2f;

	return 0;
}
