/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"


#define IN1_VBLANK (1<<7)


static int vblank;


int mystston_DSW2_r(int offset)
{
	int res;
		int cycles;


		cycles = Machine->drv->cpu[0].cpu_clock /
				(Machine->drv->frames_per_second * Machine->drv->cpu[0].interrupts_per_frame);



	res = readinputport(3);

	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 1 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	if (vblank && cpu_geticount() >
			cycles - Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second / 12)
	{
		res |= IN1_VBLANK;
	}
	else
	{
		vblank = 0;
	}

	return res;
}



/***************************************************************************

  Mysterious Stones doesn't have VBlank interrupts; the software polls port
  DSW2 to know when a vblank is happening. Therefore we set a flag here, to
  let mystston_DSW2_r() know when it has to report a vblank.

***************************************************************************/
int mystston_interrupt(void)
{
	static int coin;


	/* let mystston_DSW2_r() know when it is time to report a vblank */
	vblank = 1;

	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0)
		{
			coin = 1;
			return INT_NMI;
		}
	}
	else coin = 0;

	return INT_IRQ;
}
