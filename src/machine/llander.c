/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

#define IN0_VBLANK	(1 << 6)
#define IN0_SLAM	(1 << 2)
#define IN0_TEST	(1 << 1)
#define IN0_VG		(1 << 0)

#define IN1_P1		(1 << 0)
#define IN1_COIN	(1 << 1)
#define IN1_COIN2	(1 << 2)
#define IN1_COIN3	(1 << 3)
#define IN1_P2		(1 << 4)
#define IN1_ABORT	(1 << 5)
#define IN1_RIGHT	(1 << 6)
#define IN1_LEFT	(1 << 7)

#define IN3_THRUST		(1 << 0)
#define IN3_MAXTHRUST	(1 << 1)

static int vblank;

int llander_IN0_r (int offset) {

	int res;

	res = readinputport(0);

	if (vblank) res &= ~IN0_VBLANK;
	else vblank = 0;
	
//	res &= (vg_done (cpu_geticount()));

	return res;
	}


/*

These 7 memory locations are used to read the player's controls.
Typically, only the high bit is used. Note that the coin inputs are active-low.

*/

int llander_IN1_r (int offset) {

	int res;
	int res1;

	res1 = readinputport(1);
	res = 0x00;

	switch (offset & 0x07) {
		case 0: /* start */
			if (res1 & IN1_P1) res |= 0x80;
			break;
		case 1:  /* left coin slot */
			res = 0x80;
			if (res1 & IN1_COIN) res &= ~0x80;
			break;
		case 2: /* center coin slot */
			res = 0x80;
/*			if (res1 & IN1_COIN2) res &= ~0x80; */
			break;
		case 3: /* right coin slot */
			res = 0x80;
/*			if (res1 & IN1_COIN3) res &= ~0x80; */
			break;
		case 4: /* 2P start/select */
			res = 0x80;
			if (res1 & IN1_P2) res &= ~0x80;
			break;
		case 5: /* abort */
			if (res1 & IN1_ABORT) res |= 0x80;
			break;
		case 6: /* rotate right */
			if (res1 & IN1_RIGHT) res |= 0x80;
			break;
		case 7: /* rotate left */
			if (res1 & IN1_LEFT) res |= 0x80;
			break;
		}
	return res;
	}

int llander_IN3_r (int offset) {

	int res;

	res = readinputport(3);

	if (res & IN3_THRUST)
		return 0x80;
	if (res & IN3_MAXTHRUST)
		return 0xff;
	return 0x00;
	}

int llander_DSW1_r (int offset) {

	int res;
	int res1;

	res1 = readinputport(2);

	res = 0xfc | ((res1 >> (2 * (3 - (offset & 0x3)))) & 0x3);
	return res;
	}

int llander_interrupt (void) {

	vblank = 1;

	return nmi_interrupt();
	}


