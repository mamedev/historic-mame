#include "driver.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1832727040,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int arabian_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
