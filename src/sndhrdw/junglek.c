#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int junglek_sh_interrupt(void)
{
	static int count;


	count = (count + 1) % 2;

	if (count) return 0xff;
	else
	{
		if (pending_commands) return Z80_NMI_INT;
		else return Z80_IGNORE_INT;
	}
}



static struct AY8910interface interface =
{
	3,	/* 3 chips */
	1789750000,	/* 1.78975 MHZ ???? */
	{ 255, 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int junglek_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
