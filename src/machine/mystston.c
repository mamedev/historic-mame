/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



int mystston_interrupt(void)
{
	static int coin;


	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0)
		{
			coin = 1;
			return nmi_interrupt();
		}
	}
	else coin = 0;

	return interrupt();
}
