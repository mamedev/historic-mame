#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int frogger_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE 170

	clockticks = cpu_getfcount();
	clock = clockticks / TIMER_RATE;

#if 0	/* temporarily removed */
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	if (cpu_getreturnpc() == 0x0140)
	{
		/* wait until clock & 0x08 == 0 */
		if ((clock & 0x08) != 0)
		{
			clock = clock + 0x08;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
	else if (cpu_getreturnpc() == 0x0149)
	{
		/* wait until clock & 0x08 != 0 */
		if ((clock & 0x08) == 0)
		{
			clock = (clock + 0x08) & ~0x07;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
#endif

	return clock;
}



int frogger_sh_interrupt(void)
{
	AY8910_update();

	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	10,	/* 1 updates per video frame (good quality) */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255 },
	{ sound_command_latch_r },
	{ frogger_portB_r },
	{ 0 },
	{ 0 }
};



int frogger_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
