#include "sn76496.h"



static struct SN76496interface interface =
{
	3,	/* 3 chips */
	3867120,	/* ?? the main oscillator is 15.46848 Mhz */
	{ 255, 255, 255 }
};



int bankp_sh_start(void)
{
	return SN76496_sh_start(&interface);
}
