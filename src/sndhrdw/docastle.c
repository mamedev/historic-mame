#include "sn76496.h"



static struct SN76496interface interface =
{
	4,	/* 4 chips */
	4000000,	/* 4 Mhz? */
	{ 255, 255, 255, 255 }
};



int docastle_sh_start(void)
{
	return SN76496_sh_start(&interface);
}
