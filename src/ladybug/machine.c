/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"



#define IN0_START2 (1<<6)
#define IN0_START1 (1<<5)
#define IN0_UP (1<<3)
#define IN0_RIGHT (1<<2)
#define IN0_DOWN (1<<1)
#define IN0_LEFT (1<<0)
#define IN1_VBLANK (1<<7)
#define IN1_NOT_VBLANK (1<<6)
#define DSW1_RACK_TEST (1<<3)



static int vblank;


int ladybug_IN0_r(int address,int offset)
{
	int res = 0xff;
	int newdirection;
	static int lastdirection;


	if (osd_key_pressed(OSD_KEY_2)) res &= ~IN0_START2;
	if (osd_key_pressed(OSD_KEY_1)) res &= ~IN0_START1;

/* Lady Bug doesn't support two directions at the same time: if two */
/* directions are pressed at the same time, the player will stop, even */
/* if it could go in one of the directions. To avoid this problem, I */
/* always return only one direction, giving precedence to the one which */
/* is pressed first. This makes going around corners much easier. */
	newdirection = 0;
	if (lastdirection == IN0_DOWN && (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down))
		newdirection = IN0_DOWN;
	else if (lastdirection == IN0_RIGHT && (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right))
		newdirection = IN0_RIGHT;
	if (lastdirection == IN0_LEFT && (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left))
		newdirection = IN0_LEFT;
	if (lastdirection == IN0_UP && (osd_key_pressed(OSD_KEY_UP) || osd_joy_up))
		newdirection = IN0_UP;

	if (newdirection == 0)
	{
		if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down) newdirection = IN0_DOWN;
		else if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right) newdirection = IN0_RIGHT;
		else if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left) newdirection = IN0_LEFT;
		else if (osd_key_pressed(OSD_KEY_UP) || osd_joy_up) newdirection = IN0_UP;
	}

	lastdirection = newdirection;
	res &= ~newdirection;

	return res;
}



int ladybug_IN1_r(int address,int offset)
{
	byte res = 0x3f;


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



int ladybug_DSW1_r(int address,int offset)
{
	int res = Machine->dsw[0];


	if (osd_key_pressed(OSD_KEY_F1)) res &= ~DSW1_RACK_TEST;
	return res;
}



int ladybug_DSW2_r(int address,int offset)
{
	return Machine->dsw[1];
}



/***************************************************************************

  Lady Bug doesn't have VBlank interrupts; the software polls port IN0 to
  know when a vblank is happening. Therefore we set a flag here, to let
  ladybug_IN0_r() know when it has to report a vblank.
  Interrupts are still used by the game: but they are related to coin
  slots. Left chute generates an interrupt, Right chute a NMI.

***************************************************************************/
int ladybug_interrupt(void)
{
	static int coin;


	/* let ladybug_IN0_r() know that it is time to report a vblank */
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
