#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int scramble_portB_r(int offset)
{
	int clock;

#define TIMER_RATE (512*2)

	clock = cpu_gettotalcycles() / TIMER_RATE;

	clock = ((clock & 0x01) << 4) | ((clock & 0x02) << 6) |
			((clock & 0x08) << 2) | ((clock & 0x10) << 2);

	return clock;
}



int scramble_sh_interrupt(void)
{
	AY8910_update();

	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	10,	/* 10 updates per video frame (good quality) */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255, 255 },
	{ sound_command_r },
	{ scramble_portB_r },
	{ 0 },
	{ 0 }
};



int scramble_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
