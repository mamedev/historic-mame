#include "driver.h"
#include "M6502.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1536000000,	/* 1 MHZ ? */
	{ 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



static int interrupt_enable;


void btime_sh_interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;
}



int btime_sh_interrupt(void)
{
	if (pending_commands) return INT_IRQ;
	else if (interrupt_enable != 0) return INT_NMI;
	else return INT_NONE;
}



int btime_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
