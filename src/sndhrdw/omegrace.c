#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"

int omegrace_sh_interrupt(void)
{
	AY8910_update();

	if (cpu_getiloops() % 4 == 0) return nmi_interrupt();
	else
	{
		if (pending_commands) return interrupt();
		else return ignore_interrupt();
	}
}


static struct AY8910interface interface =
{
	2,	/* 2 chips */
	8,	/* 8 updates per video frame (good quality) */
	1789750000,	/* 1.78975 MHZ ????? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

int omegrace_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
