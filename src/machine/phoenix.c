/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



int phoenix_interrupt (void)
{
	return Z80_IGNORE_INT;
}
