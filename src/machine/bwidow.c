/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/avgdvg.h"

#define IN_LEFT	(1 << 0)
#define IN_RIGHT (1 << 1)
#define IN_FIRE (1 << 2)
#define IN_SHIELD (1 << 3)
#define IN_THRUST (1 << 4)
#define IN_P1 (1 << 5)
#define IN_P2 (1 << 6)

/*

These 7 memory locations are used to read the 2 players' controls as well
as sharing some dipswitch info in the lower 4 bits pertaining to coins/credits
Typically, only the high 2 bits are read.

*/

int spacduel_IN3_r (int offset) {

	int res;
	int res1;
	int res2;

	res1 = readinputport(3);
	res2 = readinputport(4);
	res = 0x00;

	switch (offset & 0x07) {
		case 0:
			if (res1 & IN_SHIELD) res |= 0x80;
			if (res1 & IN_FIRE) res |= 0x40;
			break;
		case 1: /* Player 2 */
			if (res2 & IN_SHIELD) res |= 0x80;
			if (res2 & IN_FIRE) res |= 0x40;
			break;
		case 2:
			if (res1 & IN_LEFT) res |= 0x80;
			if (res1 & IN_RIGHT) res |= 0x40;
			break;
		case 3: /* Player 2 */
			if (res2 & IN_LEFT) res |= 0x80;
			if (res2 & IN_RIGHT) res |= 0x40;
			break;
		case 4:
			if (res1 & IN_THRUST) res |= 0x80;
			if (res1 & IN_P1) res |= 0x40;
			break;
		case 5:  /* Player 2 */
			if (res2 & IN_THRUST) res |= 0x80;
			break;
		case 6:
			if (res1 & IN_P2) res |= 0x80;
			break;
		case 7:
			res = (0x00 /* upright */ | (0 & 0x40));
			break;
		}
	return res;
	}

int bwidow_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	return interrupt();
}

int gravitar_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	return interrupt();
}

int spacduel_interrupt(void)
{
	if (cpu_getiloops() == 5)
		avgdvg_clr_busy();
	return interrupt();
}


