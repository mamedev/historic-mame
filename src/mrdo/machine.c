/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"



/* this looks like some kind of protection. The game doesn't clear the screen */
/* if a read from this address doesn't return the value it expects. */
int mrdo_SECRE_r(int offset)
{
	Z80_Regs regs;


	Z80_GetRegs(&regs);
	return RAM[regs.HL.D];
}
