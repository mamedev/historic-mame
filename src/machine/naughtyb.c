/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


static int vblank;

#define DSW_VBLANK (1<<7)



int naughtyb_DSW_r(int offset)
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



int naughtyb_interrupt (void)
{
	int res;
	static int coin;


	/* let naughtyb_DSW_r() know that it is time to report a vblank */
	vblank = 1;

	res = Z80_IGNORE_INT;
	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0) res = Z80_NMI_INT;
		coin = 1;
	}
	else coin = 0;

	return res;
}
