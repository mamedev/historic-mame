/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


int bagman_rand_r(int offset)
{
	return 1 + (rand() & 254);
}
