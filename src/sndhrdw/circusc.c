#include "driver.h"
#include "dac.h"
#include "sn76496.h"



int circusc_sh_timer_r(int offset)
{
	int clock;
#define CIRCUSCHALIE_TIMER_RATE (14318180/6144)

	clock = (cpu_gettotalcycles()*4) / CIRCUSCHALIE_TIMER_RATE;

	return clock & 0xF;
}



void circusc_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(1,0xff);
}



void circusc_dac_w(int offset,int data)
{
	DAC_data_w(0,data);
}



static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

static struct SN76496interface interface =
{
	2,	/* 2 chips */
	14318180/8,	/*  1.7897725 Mhz */
	{ 255*2, 255*2 }
};



int circusc_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	if (SN76496_sh_start(&interface) != 0)
	{
		DAC_sh_stop();
		return 1;
	}
	return 0;
}

void circusc_sh_stop(void)
{
	SN76496_sh_stop();
	DAC_sh_stop();
}

void circusc_sh_update(void)
{
	SN76496_sh_update();
	DAC_sh_update();
}
