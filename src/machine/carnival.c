/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


#define IN1_VBLANK (1<<3)



static int vblank;



int carnival_IN1_r(int offset)
{
	int res;


	res = readinputport(1);

	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 0 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	if (vblank && Z80_ICount >
			Z80_IPeriod - Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second / 12)
		res &= ~IN1_VBLANK;
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
	static int coin;


	/* let carnival_IN1_r() know that it is time to report a vblank */
	vblank = 1;

	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0)
		{
			coin = 1;
			Z80_Reset();	/* cause a reset, IN3 will be read and the coin acknowledged */
		}
	}
	else coin = 0;

	return Z80_IGNORE_INT;
}
