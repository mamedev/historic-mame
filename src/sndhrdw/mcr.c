#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


int mcr_sh_interrupt(void)
{
	/* unlike most other AY8910 games, the MCR games run at a very high interrupt rate */
	/* so instead of generating extra interrupts to make the emulation sound better, */
	/* we actually hold off in order to prevent killing the emulation speed */
	if (cpu_getiloops() % 2 == 0) AY8910_update ();

	return interrupt();
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	13,	/* 13 updates per video frame (good quality) */
	2000000000,	/* 2 MHZ ?? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int mcr_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}

