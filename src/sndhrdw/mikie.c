#include "driver.h"
#include "generic.h"
#include "sn76496.h"



int mikie_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 512

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}



void mikie_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct SN76496interface interface =
{
	2,	/* 2 chips */
//	4000000,	/* 4 Mhz?? */
	1789750,	/* 1.78975 Mhz ? */
	{ 255, 255 }
};



int mikie_sh_start(void)
{
	pending_commands = 0;

	return SN76496_sh_start(&interface);
}
