/***************************************************************************

  This sound driver is used by 1942, Commando, Ghosts 'n Goblins and
  Exed Exes.

***************************************************************************/

#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int c1942_sh_interrupt(void)
{
	AY8910_update();

	/* the sound CPU needs 4 interrupts per frame, the handler is called 12 times */
	if (cpu_getiloops() % 3 == 0) return interrupt();
	else return ignore_interrupt();
}



static struct AY8910interface interface =
{
	2,	/* 2 chips */
	12,	/* 12 updates per video frame (good quality) */
	1500000000,	/* 1.5 MHZ ? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int c1942_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
