#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1500000000,	/* 1.5 MHZ ? */
	{ 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int vulgus_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
