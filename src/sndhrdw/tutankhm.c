#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int tutankhm_sh_interrupt(void)
{
	AY8910_update();

	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}


static int tutankhm_portB_r(int offset)
{
	int clock;

#define TIMER_RATE 25

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}


static struct AY8910interface interface =
{
	3,	/* 3 chips */
	10,	/* 10 updates per video frame (good quality) */
	1832727040,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ sound_command_r},
	{ tutankhm_portB_r },
	{ 0 },
	{ 0 }
};



int tutankhm_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
