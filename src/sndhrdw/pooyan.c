#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int pooyan_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE 36

	clockticks = (Z80_IPeriod - cpu_geticount());
	clock = clockticks / TIMER_RATE;

	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	if (cpu_getreturnpc() == 0x00cf)
	{
		/* wait until clock & 0x80 == 0 */
		if ((clock & 0x80) != 0)
		{
			clock = clock + 0x80;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
	else if (cpu_getreturnpc() == 0x00d6)
	{
		/* wait until clock & 0x80 != 0 */
		if ((clock & 0x80) == 0)
		{
			clock = (clock + 0x80) & ~0x7f;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}

	return clock;
}



int pooyan_sh_interrupt(void)
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
	{ pooyan_portB_r },
	{ },
	{ }
};



int pooyan_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
