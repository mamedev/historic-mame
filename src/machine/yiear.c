/***************************************************************************

  machine\yiear.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"


int yiear_init_machine(const char *gamename)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S;
	return 0;
}

