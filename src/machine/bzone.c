/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

#define IN0_VBLANK (1<<7)
#define IN0_VG (1<<6)


static int vblank;



int bzone_IN0_r(int offset)
{
	int res;


	res = readinputport(0);

	if (vblank) res |= IN0_VBLANK;
	else vblank = 0;
	
//	res |= (vg_done (cpu_geticount())<<6);

	return res;
}


int bzone_rand_r(int offset)
{
	return rand();
}



int bzone_interrupt(void)
{
	vblank = 1;

	return nmi_interrupt();
}
