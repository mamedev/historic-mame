/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


int gberet_interrupt(void)
{
	static int nmi;


	nmi = (nmi + 1) % Machine->drv->cpu[0].interrupts_per_frame;

	if (nmi == 0) return 0xff;	/* vblank */
	else
	{
		if ((RAM[0xe044] & 1) == 0) return Z80_IGNORE_INT;

		return Z80_NMI_INT;
	}
}
