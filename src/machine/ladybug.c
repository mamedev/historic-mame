/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


#define IN1_VBLANK (1<<7)
#define IN1_NOT_VBLANK (1<<6)



static int vblank;


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



int ladybug_IN1_r(int offset)
{
	int res,pc;


	res = 0x3f;	/* no player input here, just the vblank */

	/* to speed up the emulation, detect when the program is looping waiting */
	/* for a vblank, and force an interrupt in that case */
	pc = Z80_GetPC();
	if (!vblank && (pc == 0x1fd4 || pc == 0x0530))
		Z80_ICount = 0;

	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 1 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	if (vblank && Z80_ICount >
			Z80_IPeriod - Machine->drv->cpu_clock / Machine->drv->frames_per_second / 12)
	{
		res |= IN1_VBLANK;
	}
	else
	{
		vblank = 0;
		res |= IN1_NOT_VBLANK;
	}

	return res;
}



/***************************************************************************

  Lady Bug doesn't have VBlank interrupts; the software polls port IN0 to
  know when a vblank is happening. Therefore we set a flag here, to let
  ladybug_IN1_r() know when it has to report a vblank.
  Interrupts are still used by the game: but they are related to coin
  slots. Left chute generates an interrupt, Right chute a NMI.

***************************************************************************/
int ladybug_interrupt(void)
{
	static int coin;


	/* let ladybug_IN1_r() know that it is time to report a vblank */
	vblank = 1;

	/* wait for the user to release the key before allowing another coin to */
	/* be inserted. */
	if (coin == 1 && osd_key_pressed(OSD_KEY_3) == 0) coin = 0;

	/* user asks to insert coin: generate an interrupt. */
	if (coin == 0 && osd_key_pressed(OSD_KEY_3))
	{
		coin = 1;
		return 0xff;
	}

	return Z80_IGNORE_INT;
}
