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
#define IN0_FIRE (1<<4)
#define IN0_UP (1<<3)
#define IN0_RIGHT (1<<2)
#define IN0_DOWN (1<<1)
#define IN0_LEFT (1<<0)
#define IN1_COIN1 (1<<6)
#define DSW1_RACK_TEST (1<<2)



int mrdo_IN0_r(int offset)
{
	byte res = 0xff;


	if (osd_key_pressed(OSD_KEY_2)) res &= ~IN0_START2;
	if (osd_key_pressed(OSD_KEY_1)) res &= ~IN0_START1;

	osd_poll_joystick();
	if (osd_key_pressed(OSD_KEY_CONTROL) || osd_joy_b1 || osd_joy_b2) res &= ~IN0_FIRE;
	if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down) res &= ~IN0_DOWN;
	if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right) res &= ~IN0_RIGHT;
	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left) res &= ~IN0_LEFT;
	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_up) res &= ~IN0_UP;

	return res;
}



int mrdo_IN1_r(int offset)
{
	byte res = 0xff;


	if (osd_key_pressed(OSD_KEY_3)) res &= ~IN1_COIN1;
	return res;
}



int mrdo_DSW1_r(int offset)
{
	int res = Machine->dsw[0];


	if (osd_key_pressed(OSD_KEY_F1)) res &= ~DSW1_RACK_TEST;
	return res;
}



int mrdo_DSW2_r(int offset)
{
	return Machine->dsw[1];
}



/* this looks like some kind of protection. The game doesn't clear the screen */
/* if a read from this address doesn't return the value it expects. */
int mrdo_SECRE_r(int offset)
{
	Z80_Regs regs;


	Z80_GetRegs(&regs);
	return RAM[regs.HL.D];
}
