/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int vblank;

#define DSW_VBLANK (1<<7)



int phoenix_DSW_r(int offset)
{
	int res;


	res = readinputport(1);

	if (vblank)
	{
		vblank = 0;
		res |= DSW_VBLANK;
	}

	return res;
}



/***************************************************************************

  Phoenix doesn't have VBlank interrupts; the software polls port DSW to
  know when a vblank is happening. Therefore we set a flag here, to let
  phoenix_DSW_r() know when it has to report a vblank.

***************************************************************************/
int phoenix_interrupt (void)
{
	/* let phoenix_DSW_r() know that it is time to report a vblank */
	vblank = 1;

	return Z80_IGNORE_INT;
}
