#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int frogger_sh_init(const char *gamename)
{
	int j;
	unsigned char *RAM;


	/* ROM 1 has data lines D0 and D1 swapped. Decode it. */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	for (j = 0;j < 0x0800;j++)
		RAM[j] = (RAM[j] & 0xfc) | ((RAM[j] & 1) << 1) | ((RAM[j] & 2) >> 1);

	return 0;
}



static int frogger_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE 170

	clockticks = (Z80_IPeriod - cpu_geticount());
	clock = clockticks / TIMER_RATE;

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

	return clock;
}



int frogger_sh_interrupt(void)
{
	if (pending_commands) return 0xff;
	else return Z80_IGNORE_INT;
}



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255 },
	{ sound_command_latch_r },
	{ frogger_portB_r },
	{ },
	{ }
};



int frogger_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
