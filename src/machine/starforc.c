#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "Z80.h"


static int starforc_portB_r(int offset)
{
	int clock;

#define TIMER_RATE 25

	clock = cpu_gettotalcycles() / TIMER_RATE;

	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */

	return clock;
}

static struct AY8910interface interface =
{
	3,	/* 3 chips */
	10,	/* 10 updates per video frame (good quality) */
	1832727040,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int starforc_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}


int starforc_sh_interrupt(void)
{
	AY8910_update();

	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}
