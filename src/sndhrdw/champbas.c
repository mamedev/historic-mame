#include "driver.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	1536000000,	/* 1.536000000 MHZ ? */
	{ 255 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};



int champbas_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
