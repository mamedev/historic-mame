/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"

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
        0x00,0x0A,0x05,0x00,0x06,0x02,0x01,0x00,
        0x09,0x08,0x04,0x00,0x00,0x00,0x00,0x00,
        0x10,0x1A,0x15,0x10,0x16,0x12,0x11,0x10,
        0x19,0x18,0x14,0x10,0x10,0x10,0x10,0x10 };

int bzone_IN3_r(int offset)
{
    static int directors_cut=0x80; /* JB 970901 */
	int res,res1;

	res1=readinputport(4);

	/* Decide between red/green or red only */
	if ((res1 & 0x80) != directors_cut) {
		directors_cut=res1&0x80;
		if (directors_cut) {
			atari_vg_colorram_w(3,3);
			atari_vg_colorram_w(11,11);
		} else {
			atari_vg_colorram_w(3,0);
			atari_vg_colorram_w(11,8);
		}
	}

	res=readinputport(3);
	res|=one_joy_trans[res1&0x1f];

	return (res);
}
