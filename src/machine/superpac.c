/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



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

	return 0;
}

