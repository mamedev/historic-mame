#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


static struct AY8910interface espial_interface =
{
	1,	/* 1 chip */
	1832727,	/* 1.832727040 MHZ?????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


int espial_sh_start(void)
{
	return AY8910_sh_start(&espial_interface);
}



/* Send sound data to the sound cpu and cause an irq */
void espial_sound_command_w (int offset, int data)
{
	/* The sound cpu runs in interrupt mode 1 */
	cpu_cause_interrupt (1, 1);
	soundlatch_w(0,data);
}
