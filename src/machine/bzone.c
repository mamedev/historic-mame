/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/avgdvg.h"
#include "machine/atari_vg.h"

#define IN0_3KHZ (1<<7)
#define IN0_VG_HALT (1<<6)

int bzone_IN0_r(int offset)
{
	int res;

	res = readinputport(0);

	if (cpu_gettotalcycles() & 0x100)
		res |= IN0_3KHZ;
	else
		res &= ~IN0_3KHZ;

	if (avgdvg_done())
		res |= IN0_VG_HALT;
	else
		res &= ~IN0_VG_HALT;

	return res;
}

/* Translation table for one-joystick emulation */
static int one_joy_trans[32]={
        0x00,0x0A,0x05,0x00,0x06,0x02,0x01,0x00,
        0x09,0x08,0x04,0x00,0x00,0x00,0x00,0x00 };

int bzone_IN3_r(int offset)
{
	int res,res1;

	res=readinputport(3);
	res1=readinputport(4);

	res|=one_joy_trans[res1&0xf];

	return (res);
}

int bzone_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	if (readinputport(0) & 0x10)
		return nmi_interrupt();
	else
		return ignore_interrupt();
}

void bzone_init_machine(void)
{
	avgdvg_clr_busy();
}

void redbaron_init_machine(void)
{
	avgdvg_clr_busy();
	avg_fake_colorram_w(0,3);
}

int rb_input_select;

int redbaron_joy_r (int offset)
{
	if (rb_input_select)
		return readinputport (5);
	else
		return readinputport (6);
}
