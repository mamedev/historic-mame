/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define IN1_VBLANK (1<<3)



static int vblank;



int carnival_IN1_r(int offset)
{
	int res;


	res = 0x00;

	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 1 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	if (vblank && Z80_ICount >
			Z80_IPeriod - Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second / 12)
		res |= IN1_VBLANK;
	else vblank = 0;

	return res;
}



/***************************************************************************

  Carnival doesn't have VBlank interrupts; the software polls port IN0 to
  know when a vblank is happening. Therefore we set a flag here, to let
  carnival_IN1_r() know when it has to report a vblank.

***************************************************************************/
int carnival_interrupt(void)
{
	/* let carnival_IN1_r() know that it is time to report a vblank */
	vblank = 1;

	return Z80_IGNORE_INT;
}
