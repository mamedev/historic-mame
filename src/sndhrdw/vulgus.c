#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int vulgus_sh_interrupt(void)
{
	AY8910_update();

	return interrupt();
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	8,	/* 8 updates per video frame (good quality) */
	1500000000,	/* 1.5 MHZ ? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int vulgus_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
