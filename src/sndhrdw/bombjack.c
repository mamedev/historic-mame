#include "driver.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"



static struct AY8910interface interface =
{
	3,	/* 3 chips */
	1832727040,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int bombjack_sh_intflag_r(int offset)
{
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for an interrupt, and force it in that case */
	if (cpu_getpc() == 0x0099)
		cpu_seticount(0);

	return 1;
}



int bombjack_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
