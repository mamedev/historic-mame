#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int pooyan_portB_r(int offset)
{
	int clock;

#define TIMER_RATE 38

	clock = cpu_gettotalcycles() / TIMER_RATE;

#if 0	/* temporarily removed */
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	if (cpu_getreturnpc() == 0x00cf)
	{
		/* wait until clock & 0x80 == 0 */
		if ((clock & 0x80) != 0)
		{
			clock = clock + 0x80;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
	else if (cpu_getreturnpc() == 0x00d6)
	{
		/* wait until clock & 0x80 != 0 */
		if ((clock & 0x80) == 0)
		{
			clock = (clock + 0x80) & ~0x7f;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
#endif

	return clock;
}



void pooyan_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 1 && data == 0)
	{
		/* setting bit 0 high then low triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHZ ?? */
	{ 0x30ff, 0x30ff },
	{ soundlatch_r },
	{ pooyan_portB_r },
	{ 0 },
	{ 0 }
};



int pooyan_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
