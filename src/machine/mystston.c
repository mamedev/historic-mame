/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"



int mystston_interrupt(void)
{
	static int coin;


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
