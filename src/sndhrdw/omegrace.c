#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "vidhrdw/avgdvg.h"

int omegrace_sh_interrupt(void)
{
	AY8910_update();

	if (cpu_getiloops() % 2 == 0) return nmi_interrupt();
	else
	{
		if (pending_commands) return interrupt();
		else return ignore_interrupt();
	}
}


static struct AY8910interface interface =
{
	2,	/* 2 chips */
	12,	/* 12 updates per video frame (good quality) */
	1500000000,	/* 1.5 MHZ */
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
