#include "driver.h"
#include "generic.h"
#include "sn76496.h"
#include "dac.h"

unsigned char *sbasketb_dac;

static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

int sbasketb_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 1024

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}



static struct SN76496interface interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 Mhz ? */
	{ 255 }
};



void sbasketb_dac_w(int offset,int data)
{
	*sbasketb_dac = data;
	DAC_data_w(0,data<<1 );
}



void sbasketb_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(1,0xff);
}



int sbasketb_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	if (SN76496_sh_start(&interface) != 0)
	{
		DAC_sh_stop();
		return 1;
	}

	return 0;
}

void sbasketb_sh_stop(void)
{
	SN76496_sh_stop();
	DAC_sh_stop();
}

void sbasketb_sh_update(void)
{
	SN76496_sh_update();
	DAC_sh_update();
}
