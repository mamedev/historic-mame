/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


int scramble_IN2_r(int offset)
{
	int res;


	res = readinputport(2);

/*if (errorlog) fprintf(errorlog,"%04x: read IN2\n",Z80_GetPC());*/

	/* avoid protection */
	if (Z80_GetPC() == 0x00e4) res &= 0x7f;

	return res;
}



int scramble_protection_r(int offset)
{
if (errorlog) fprintf(errorlog,"%04x: read protection\n",Z80_GetPC());

	return 0x6f;

	/* codes for the Konami version (not working yet) */
	if (Z80_GetPC() == 0x00a8) return 0xf0;
	if (Z80_GetPC() == 0x00be) return 0xb0;
	if (Z80_GetPC() == 0x0c1d) return 0xf0;
	if (Z80_GetPC() == 0x0c6a) return 0xb0;
	if (Z80_GetPC() == 0x0ceb) return 0x40;
	if (Z80_GetPC() == 0x0d37) return 0x60;
}
