#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/2203intf.h"



int ironhors_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 128

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}



void ironhors_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(1,0xff);
}



static struct YM2203interface OPNinterface =
{
	1,			/* 1 chip */
	3000000,	/* 3.0 MHZ ? */
	{ YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


int ironhors_sh_start(void)
{
	return YM2203_sh_start(&OPNinterface);
}
