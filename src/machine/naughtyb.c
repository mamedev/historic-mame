/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


int naughtyb_interrupt (void)
{
	int res;
	static int coin;


	res = Z80_IGNORE_INT;
	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0) res = Z80_NMI_INT;
		coin = 1;
	}
	else coin = 0;

	return res;
}
