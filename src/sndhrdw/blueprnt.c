#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int blueprnt_sh_interrupt(void)
{
	if (cpu_getiloops() % 2 == 0) return interrupt();
	else
	{
		AY8910_update();

		if (pending_commands) return nmi_interrupt();
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
	{ sound_command_r },
	{ 0 },
	{ 0 }
};



int blueprnt_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
