/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"

#define IN0_3KHZ (1<<7)
#define IN0_VG_HALT (1<<6)

int tempest_IN0_r(int offset)
{
	int res;

	res = readinputport(0);

	/* Emulate the 3Khz source on bit 7 (divide 1.5Mhz by 512) */
	if (cpu_geticount() & 0x100) {
		res &=~IN0_3KHZ;
	} else {
		res|=IN0_3KHZ;
	}

	if (vg_done(cpu_gettotalcycles()))
		res |=IN0_VG_HALT;
	else
		res &=~IN0_VG_HALT;
	return (res);
}
	
int tempest_IN3_r(int offset)
{
	static int spinner = 0;
	int res, trak_in;

	res = readinputport(3);
	if (res & 1) 
		spinner--;
	if (res & 2)
		spinner++;
	trak_in = readtrakport(0);
	if (trak_in != NO_TRAK)
		spinner += trak_in;
	spinner &= 0x0f;	
	return ((res & 0xf0) | spinner);
}

int tempest_spinner(int data)
{
	if (data>7)
		data=7;
	if (data<-7)
		data=-7;
	return (data);
}	


