/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

#define IN0_VBLANK (1<<6)

static int vblank = 0;

/* JB 23-JUL-97 modified again */
int milliped_IN_r(int offset)
{
    int res;

    /* ASG -- rewrote to incorporate fire controls and to
       toggle the IN0_VBLANK bit every other read after a VBLANK */
    res = readinputport(offset) & 0xbf;

    if (offset==0)
    if (vblank)
    {
        static int val = IN0_VBLANK;
        val ^= IN0_VBLANK;
        res |= val;
        vblank = 0;
    }

    if ( cpu_geticount() > 4000 )
        return (res & 0x70) | (readtrakport(offset) & 0x8f);
    else
        return res;
}



/* ASG added to set the vblank flag once per interrupt (a read to the in port above happens next */
int milliped_interrupt(void)
{
	vblank = 1;

	return interrupt();
}
