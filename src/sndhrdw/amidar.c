#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int rates[8] = { 3000, 0, 0, 0, 0, 0, 0, 0 };


static int amidar_portB_r(int offset)
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



int amidar_sh_interrupt(void)
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
	{ amidar_portB_r },
	{ },
	{ }
};



int amidar_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
