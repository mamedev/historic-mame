/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
/* JB 970527 */
#ifdef __MWERKS__
#	include "m6809.h"
#else
#	include "m6809/m6809.h"
#endif



int superpac_init_machine(const char *gamename)
{
	
	/* APPLY SUPERPAC.PCH PATCHES */
	RAM[0xe11b] = 0x7e;
	RAM[0xe11c] = 0xe1;
	RAM[0xe11d] = 0x5c;
	RAM[0xe041] = 0x12;
	RAM[0xe042] = 0x12;
	RAM[0xe3c5] = 0x12;
	RAM[0xe3c6] = 0x12;
	
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_OP | M6809_FAST_S | M6809_FAST_U;

	return 0;
}

