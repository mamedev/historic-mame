/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"



int bagman_rand_r(int offset)
{
	return 1 + (rand() & 254);
}
