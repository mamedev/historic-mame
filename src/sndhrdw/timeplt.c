#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int timeplt_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE 32

	clockticks = cpu_getfcount();
	clock = clockticks / TIMER_RATE;

#if 0	/* temporarily removed */
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	if (cpu_getreturnpc() == 0x00c3)
	{
		/* wait until clock & 0xf0 == 0 */
		if ((clock & 0xf0) != 0)
		{
			clock = (clock + 0x100) & ~0xff;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
#endif

	return clock;
}



int timeplt_sh_interrupt(void)
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
	{ timeplt_portB_r },
	{ 0 },
	{ 0 }
};



int timeplt_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
