#include "driver.h"
#include "generic.h"
#include "sn76496.h"



int tp84_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 256

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

int tp84_sh_interrupt(void)
{
	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}



static struct SN76496interface interface =
{
	3,	/* 3 chips */
//	4000000,	/* 4 Mhz?? */
	1789750,	/* 1.78975 Mhz ? */
	{ 255, 255, 255 }
};



int tp84_sh_start(void)
{
	pending_commands = 0;

	return SN76496_sh_start(&interface);
}
