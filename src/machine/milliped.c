/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6502.h"

int milliped_clock_reset_w(int offset)
{
        return INT_IRQ;
}

int milliped_rand_r(int offset)
{
	return rand();
}
