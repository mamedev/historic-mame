/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define IN0_VBLANK (1<<6)



static int vblank;



int centiped_IN0_r(int offset)
{
	int res;


	res = readinputport(0);

	if (vblank) res |= IN0_VBLANK;
	else vblank = 0;

	return res;
}



int centiped_init_machine(const char *gamename)
{
	/* patch the roms to pass the startup test */
	RAM[0x38ae] = 0xea;
	RAM[0x38af] = 0xea;

	return 0;
}



int centiped_rand_r(int offset)
{
	return rand();
}



int centiped_interrupt(void)
{
	vblank = 1;

	return interrupt();
}
