/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



int carnival_interrupt(void)
{
	static int coin;


	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0)
		{
			coin = 1;
			cpu_reset(0);	/* cause a CPU reset, IN3 will be read and the coin acknowledged */
		}
	}
	else coin = 0;

	return ignore_interrupt();
}
