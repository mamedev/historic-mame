#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/2203intf.h"
#include "Z80.h"



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



void bublbobl_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,Z80_NMI_INT);
}



int bublbobl_sh_start(void)
{
	return YM2203_sh_start(&interface);
}
