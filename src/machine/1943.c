/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



/* this is a protection check. The game crashes (thru a jump to 0x8000) */
/* if a read from this address doesn't return the value it expects. */
int c1943_protection_r(int offset)
{
	Z80_Regs regs;


	Z80_GetRegs(&regs);
	if (errorlog) fprintf(errorlog,"protection read, PC: %04x Result:%02x\n",cpu_getpc(),regs.BC.B.h);
	return regs.BC.B.h;
}
