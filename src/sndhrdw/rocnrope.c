#include "driver.h"
#include "generic.h"
#include "8910intf.h"



static int rocnrope_portB_r(int offset)
{
	int clock;

#define ROCNROPE_TIMER_RATE (14318180/6144)

	clock = (cpu_gettotalcycles()*4) / ROCNROPE_TIMER_RATE;

	return (clock << 4) & 0xF0;
}



void rocnrope_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHZ ?? */
	{ 0x38ff, 0x38ff },
	{ soundlatch_r },
	{ rocnrope_portB_r },
	{ 0 },
	{ 0 }
};



int rocnrope_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
