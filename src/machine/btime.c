/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"


#define IN1_VBLANK (1<<7)


static int vblank;


int btime_DSW1_r(int offset)
{
	int res;


	res = readinputport(3);

	if (vblank == 0) res |= IN1_VBLANK;

	return res;
}



/***************************************************************************

  Burger Time doesn't have VBlank interrupts; the software polls port IN0 to
  know when a vblank is happening. Therefore we set a flag here, to let
  btime_DSW1_r() know when it has to report a vblank.

***************************************************************************/
int btime_interrupt(void)
{
	/* let btime_DSW1_r() know when it is time to report a vblank */
	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 1 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	vblank = (vblank + 1) % 12;

	/* IRQs are used to check coin insertion and diagnostic commands */
	return INT_IRQ;
}
