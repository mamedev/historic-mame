/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *cloak_sharedram;
unsigned char *cloak_nvRAM;
unsigned char *enable_nvRAM;

READ_HANDLER( cloak_sharedram_r )
{
	return cloak_sharedram[offset];
}

WRITE_HANDLER( cloak_sharedram_w )
{
	cloak_sharedram[offset] = data;
}
