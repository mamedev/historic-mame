#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static int junglek_portB_r(int offset)
{
	int clockticks,clock;

#define TIMER_RATE (32)

	clockticks = (Z80_IPeriod - cpu_geticount());

	clock = clockticks / TIMER_RATE;

	return clock;
}



int junglek_sh_interrupt(void)
{
	return Z80_NMI_INT;

/*  if (pending_commands) return 0xff; */
/*  else return Z80_IGNORE_INT;        */
}



static struct AY8910interface interface =
{
	3,	/* 3 chips */
	1789750000,	/* 1.78975 MHZ ?? */
	{ 255, 255, 255 },
    { }, /*sound_command_r },*/
    { }, /*junglek_portB_r },*/
	{ },
	{ }
};



int junglek_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
