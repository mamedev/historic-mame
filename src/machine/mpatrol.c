/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


/* this looks like some kind of protection. The game does strange things */
/* if a read from this address doesn't return the value it expects. */
int mpatrol_protection_r(int offset)
{
	Z80_Regs regs;


	if (errorlog) fprintf(errorlog,"%04x: read protection\n",Z80_GetPC());
	Z80_GetRegs(&regs);
	return regs.DE.B.l;
}
