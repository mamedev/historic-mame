/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/avgdvg.h"
#include "sndhrdw/pokyintf.h"

/*
 * Catch the following busy loop:
 * C7A7	LDA $53
 * C7A9 CMP #$09
 * C7AB BCC C7A7
 *
 * Unfortunately, we'll have to put a memory hook on $53, which
 * affects performace, since zero page access can no longer take
 * the "shortcut" in MAME's memory handler. It's probably not worth
 * it, however let's try :-)
 */

int tempest_catch_busyloop (int offset)
{
	int data;

	data = RAM[0x0053];
	if (cpu_getpreviouspc()==0xc7a7)
	{
		cpu_seticount(0);
	}
	return RAM[0x0053];
}

int tempest_IN0_r(int offset)
{
	int res;

	res = readinputport(0);

	if (avgdvg_done())
		res|=0x40;

	/* Emulate the 3Khz source on bit 7 (divide 1.5Mhz by 512) */
	if (cpu_gettotalcycles() & 0x100)
		res |=0x80;

	return res;
}


int tempest_interrupt(void)
{
	update_analog_ports();
	/* We cheat and update the pokey sound here since it needs frequent attention */
	pokey_update ();
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	return interrupt();
}
