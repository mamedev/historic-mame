#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int dipsw;

static void dipsw_w(int offset,int data)
{
	dipsw = data;
}

int blueprnt_sh_dipsw_r(int offset)
{
	return dipsw;
}



void blueprnt_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	1000000,	/* 1MHz ????? */
	{ 0x20ff, 0x20ff },
	{            0, input_port_2_r },
	{ soundlatch_r, input_port_3_r },
	{ dipsw_w },
	{ 0 }
};



int blueprnt_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
