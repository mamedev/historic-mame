#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int scramble_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE (512*2)

	clockticks = (Z80_IPeriod - cpu_geticount());

	clock = clockticks / TIMER_RATE;

	clock = ((clock & 0x01) << 4) | ((clock & 0x02) << 6) |
			((clock & 0x08) << 2) | ((clock & 0x10) << 2);

	return clock;
}



int scramble_sh_interrupt(void)
{
	if (pending_commands) return 0xff;
	else return Z80_IGNORE_INT;
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255, 255 },
	{ sound_command_r },
	{ scramble_portB_r },
	{ },
	{ }
};



int scramble_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
