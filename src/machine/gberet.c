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


	nmi = (nmi + 1) % 10;

	if (nmi == 0) return 0xff;
	else
	{
		if (RAM[0xe044] & 1) return Z80_NMI_INT;
		else return Z80_IGNORE_INT;
	}
}
