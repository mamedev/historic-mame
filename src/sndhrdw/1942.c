#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int c1942_sh_interrupt(void)
{
        return 0xff;
	if (pending_commands) return 0xff;
	else return Z80_IGNORE_INT;
}



int c1942_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
