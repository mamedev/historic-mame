/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



unsigned char *gberet_interrupt_enable;



int gberet_interrupt(void)
{
	static int nmi;


	nmi = (nmi + 1) % 32;

	if (nmi == 0) return 0xff;
	else if (nmi % 2)
	{
		if (*gberet_interrupt_enable & 1) return Z80_NMI_INT;
	}

	return Z80_IGNORE_INT;
}
