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



int scramble_IN2_r(int offset)
{
	int res;


	res = readinputport(2);

	/* avoid protection */
	if (Z80_GetPC() == 0x00e4) res &= 0x7f;

	return res;
}



extern int scramble_protection_r(int offset)
{
	return 0x6f;
}
