/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int warpwarp_input_c000_7_r(int offset)
{
	int res;
	res = readinputport(1);
	return(res & (1<<offset) ? 1 : 0);
}

/* Read the Dipswitches */
int warpwarp_input_c020_27_r(int offset)
{
	int res;

	res = readinputport(0);
	return(res & (1<<(offset & 7)) ? 1 : 0);
}

int warpwarp_input_controller_r(int offset)
{
	int res;

	res = readinputport(2);
	if (res & 1) return(111);
	if (res & 2) return(167);
	if (res & 4) return(63);
	if (res & 8) return(23);
	return(255);
}

