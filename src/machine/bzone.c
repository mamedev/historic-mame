/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

#define IN0_3KHZ (1<<7)
#define IN0_VG_HALT (1<<6)

int bzone_IN0_r(int offset)
{
	int res;

	res = readinputport(0);

	if (cpu_geticount() & 0x100)
		res|=IN0_3KHZ;
	else
		res&=~IN0_3KHZ;

	if (vg_done(cpu_gettotalcycles()))
		res|=IN0_VG_HALT;
	else
		res&=~IN0_VG_HALT;

	return res;
}

/* Translation table for one-joystick emulation */

static int one_joy_trans[32]={
	0x00,0x0A,0x05,0x00,0x06,0x00,0x00,0x00,
	0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x10,0x1A,0x15,0x10,0x16,0x10,0x10,0x10,
	0x19,0x10,0x10,0x10,0x10,0x10,0x10,0x10 };

int bzone_IN3_r(int offset)
{
	int res,res1;
	
	res=readinputport(3);
	res1=readinputport(4);
	res|=one_joy_trans[res1&0x1f];

	return (res);
}
