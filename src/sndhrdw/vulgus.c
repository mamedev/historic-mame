#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHZ ? */
	{ 0x20ff, 0x20ff },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int vulgus_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
