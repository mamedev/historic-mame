/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  This file is used by the Vanguard and Nibbler drivers.

***************************************************************************/

#include "driver.h"


int vanguard_interrupt(void)
{
	if (cpu_getiloops() != 0)
	{
		static int coin;


		/* user asks to insert coin: generate a NMI interrupt. */
		if (osd_key_pressed(OSD_KEY_3))
		{
			if (coin == 0)
			{
				coin = 1;
				return nmi_interrupt();
			}
		}
		else coin = 0;

		return ignore_interrupt();
	}
	return interrupt();
}