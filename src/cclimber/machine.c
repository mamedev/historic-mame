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



#define IN0_RIGHT_RIGHT (1<<7)
#define IN0_RIGHT_LEFT (1<<6)
#define IN0_RIGHT_DOWN (1<<5)
#define IN0_RIGHT_UP (1<<4)
#define IN0_LEFT_RIGHT (1<<3)
#define IN0_LEFT_LEFT (1<<2)
#define IN0_LEFT_DOWN (1<<1)
#define IN0_LEFT_UP (1<<0)
#define IN2_STANDUP (1<<4)
#define IN2_START2 (1<<3)
#define IN2_START1 (1<<2)
#define IN2_CREDIT (1<<1)
#define DSW1_RACK_TEST (1<<3)



int cclimber_IN0_r(int offset)
{
	int res = 0;


	if (osd_key_pressed(OSD_KEY_E) || osd_joy_up) res |= IN0_LEFT_UP;
	if (osd_key_pressed(OSD_KEY_D) || osd_joy_down) res |= IN0_LEFT_DOWN;
	if (osd_key_pressed(OSD_KEY_S) || osd_joy_left) res |= IN0_LEFT_LEFT;
	if (osd_key_pressed(OSD_KEY_F) || osd_joy_right) res |= IN0_LEFT_RIGHT;

	if (osd_key_pressed(OSD_KEY_I) || osd_joy_b2) res |= IN0_RIGHT_UP;
	if (osd_key_pressed(OSD_KEY_K) || osd_joy_b3) res |= IN0_RIGHT_DOWN;
	if (osd_key_pressed(OSD_KEY_J) || osd_joy_b1) res |= IN0_RIGHT_LEFT;
	if (osd_key_pressed(OSD_KEY_L) || osd_joy_b4) res |= IN0_RIGHT_RIGHT;

	return res;
}



int cclimber_IN2_r(int offset)
{
	int res = IN2_STANDUP;


	if (osd_key_pressed(OSD_KEY_2)) res |= IN2_START2;
	if (osd_key_pressed(OSD_KEY_1)) res |= IN2_START1;
	if (osd_key_pressed(OSD_KEY_3)) res |= IN2_CREDIT;
	return res;
}



int cclimber_DSW1_r(int offset)
{
	int res = Machine->dsw[0];


	if (osd_key_pressed(OSD_KEY_F1)) res |= DSW1_RACK_TEST;
	return res;
}
