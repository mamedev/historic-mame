#include "driver.h"
#include "m6809/m6809.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/dac.h"
#include "sndhrdw/2203intf.h"
#include "sndhrdw/fm.h"



void capbowl_sndcmd_w(int offset,int data)
{
	soundlatch_w(offset, data);

	cpu_cause_interrupt(1,M6809_INT_IRQ);
}



static int firq_count;

int capbowl_sound_interrupt(void)
{
return M6809_INT_FIRQ;

/* the following doesn't seem to work too well */
#if 0
	if (firq_count)
	{
		firq_count--;
		return M6809_INT_FIRQ;
	}
	else return ignore_interrupt();
#endif
}

/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	firq_count++;
}



void capbowl_dac_w(int offset,int data)
{
	DAC_data_w(0,data);
}



static struct DACinterface DAinterface =
{
	1,
	441000,
	{ 64, 64 },
	{  1,  1 }
};

static struct YM2203interface interface =
{
	1,			/* 1 chip */
	3000000,	/* 3.0 MHZ ??? */
	{ YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

int capbowl_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	if (YM2203_sh_start(&interface) != 0)
	{
		DAC_sh_stop();
		return 1;
	}

	OPNSetIrqHandler(0,irqhandler);

	return 0;
}

void capbowl_sh_stop(void)
{
	YM2203_sh_stop();
	DAC_sh_stop();
}

void capbowl_sh_update(void)
{
	YM2203_sh_update();
	DAC_sh_update();
}
