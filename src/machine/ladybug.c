/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



int ladybug_IN0_r(int offset)
{
	int res;
	int newdirection;
	static int lastdirection;


	res = readinputport(0);

	/* Lady Bug doesn't support two directions at the same time: if two */
	/* directions are pressed at the same time, the player will stop, even */
	/* if it could go in one of the directions. To avoid this problem, I */
	/* always return only one direction, giving precedence to the one which */
	/* is pressed first. This makes going around corners much easier. */

	newdirection = ~res & 0x0f;
	switch (newdirection)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x08:
			break;
		default:
			if ((newdirection & lastdirection) != 0)
				newdirection = lastdirection;
			else
			{
				if (newdirection & 0x01) newdirection = 0x01;
				else if (newdirection & 0x02) newdirection = 0x02;
				else if (newdirection & 0x04) newdirection = 0x04;
				else if (newdirection & 0x08) newdirection = 0x08;
			}
			break;
	}

	lastdirection = newdirection;

	return (res & 0xf0) | (~newdirection & 0x0f);
}



/***************************************************************************

  Lady Bug doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots. Left chute generates an interrupt, Right chute a NMI.

***************************************************************************/
int ladybug_interrupt(void)
{
	static int coin;


	/* user asks to insert coin: generate an interrupt. */
	if (osd_key_pressed(OSD_KEY_3))
	{
		if (coin == 0)
		{
			coin = 1;
			return 0xff;
		}
	}
	else coin = 0;

	return Z80_IGNORE_INT;
}
