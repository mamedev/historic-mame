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



#define IN0_RIGHT (1<<7)
#define IN0_LEFT (1<<6)
#define IN0_DOWN (1<<5)
#define IN0_UP (1<<4)
#define IN0_FIRE (1<<3)
#define IN2_START2 (1<<3)
#define IN2_START1 (1<<2)
#define IN2_CREDIT (1<<1)



int ckong_IN0_r(int offset)
{
	int res = 0;


	if (osd_key_pressed(OSD_KEY_CONTROL) || osd_joy_b1 || osd_joy_b2) res |= IN0_FIRE;
	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_up) res |= IN0_UP;
	if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down) res |= IN0_DOWN;
	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left) res |= IN0_LEFT;
	if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right) res |= IN0_RIGHT;

	return res;
}



int ckong_IN2_r(int offset)
{
	int res = 0xff;


	if (osd_key_pressed(OSD_KEY_2)) res &= ~IN2_START2;
	if (osd_key_pressed(OSD_KEY_1)) res &= ~IN2_START1;
	if (osd_key_pressed(OSD_KEY_3)) res &= ~IN2_CREDIT;
	return res;
}



int ckong_DSW1_r(int offset)
{
	return Machine->dsw[0];
}

