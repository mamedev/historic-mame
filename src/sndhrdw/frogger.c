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



static int rates[8] = { 256, 2048, 0, 128, 1024, 0, 0, 0 };


static int frogger_portB_r(int offset)
{
	int clockticks,clock,i;


	clockticks = (Z80_IPeriod - cpu_geticount());

	clock = 0;
	for (i = 0;i < 8;i++)
	{
		clock <<= 1;
		if (rates[i]) clock |= ((clockticks / rates[i]) & 1);
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
	{ sound_command_r },
	{ frogger_portB_r },
	{ },
	{ }
};



int frogger_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
