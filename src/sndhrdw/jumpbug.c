#include "driver.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	1536000000,	/* 1.536000000 MHZ????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int jumpbug_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
