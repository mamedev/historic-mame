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



#define IN0_PUSH (1<<7)
#define IN0_CREDIT (1<<6)
#define IN0_RIGHT (1<<3)
#define IN0_LEFT (1<<2)
#define IN0_DOWN (1<<1)
#define IN0_UP (1<<0)
#define IN1_START2 (1<<6)
#define IN1_START1 (1<<5)
#define IN1_TEST (1<<4)
#define DSW1_RACK_TEST (1<<5)


int pengo_IN0_r(int address,int offset)
{
	int res = 0xff;


	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	if (osd_key_pressed(OSD_KEY_CONTROL) || osd_joy_b1 || osd_joy_b2) res &= ~IN0_PUSH;
	if (osd_key_pressed(OSD_KEY_3)) res &= ~IN0_CREDIT;
	if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down) res &= ~IN0_DOWN;
	if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right) res &= ~IN0_RIGHT;
	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left) res &= ~IN0_LEFT;
	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_up) res &= ~IN0_UP;
	return res;
}



int pengo_IN1_r(int address,int offset)
{
	int res = 0xff;


	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	if (osd_key_pressed(OSD_KEY_2)) res &= ~IN1_START2;
	if (osd_key_pressed(OSD_KEY_1)) res &= ~IN1_START1;
	if (osd_key_pressed(OSD_KEY_F2)) res &= ~IN1_TEST;
	return res;
}



int pengo_DSW1_r(int address,int offset)
{
	int res = Machine->dsw[0];


	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	if (osd_key_pressed(OSD_KEY_F1)) res &= ~DSW1_RACK_TEST;
	return res;
}
