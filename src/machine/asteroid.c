/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

#define IN0_HYPER	(1 << 3)
#define IN0_FIRE	(1 << 4)
#define IN0_SLAM	(1 << 6)
#define IN0_TEST	(1 << 7)

#define IN1_COIN	(1 << 0)
#define IN1_COIN2	(1 << 1)
#define IN1_COIN3	(1 << 2)
#define IN1_P1		(1 << 3)
#define IN1_P2		(1 << 4)
#define IN1_THRUST	(1 << 5)
#define IN1_RIGHT	(1 << 6)
#define IN1_LEFT	(1 << 7)

static int bank = 0;

int asteroid_IN0_r (int offset) {

	int res;
	int res1;

	res1 = readinputport(0);
	res = 0x00;

	switch (offset & 0x07) {
		case 0: /* nothing */
			break;
		case 1:  /* clock toggles at 3 KHz*/
			if (cpu_gettotalcycles() & 0x100)
				res = 0x80;
		case 2: /* vector generator halt */
			break;
		case 3: /* hyperspace/shield */
			if (res1 & IN0_HYPER) res |= 0x80;
			break;
		case 4: /* fire */
			if (res1 & IN0_FIRE) res |= 0x80;
			break;
		case 5: /* diagnostics */
			break;
		case 6: /* slam */
			if (res1 & IN0_SLAM) res |= 0x80;
			break;
		case 7: /* self test */
			if (res1 & IN0_TEST) res |= 0x80;
			break;
		}
	return res;
	}

/*

These 7 memory locations are used to read the player's controls.
Typically, only the high bit is used. Note that the coin inputs are active-low.

*/

int asteroid_IN1_r (int offset) {

	int res;
	int res1;

	res1 = readinputport(1);
	res = 0x00;

	switch (offset & 0x07) {
		case 0: /* left coin slot */
			res = 0x80;
			if (res1 & IN1_COIN) res &= ~0x80;
			break;
		case 1:  /* center coin slot */
			res = 0x80;
/*			if (res1 & IN1_COIN2) res &= ~0x80; */
			break;
		case 2: /* right coin slot */
			res = 0x80;
/*			if (res1 & IN1_COIN3) res &= ~0x80; */
			break;
		case 3: /* 1P start */
			if (res1 & IN1_P1) res |= 0x80;
			break;
		case 4: /* 2P start */
			if (res1 & IN1_P2) res |= 0x80;
			break;
		case 5: /* thrust */
			if (res1 & IN1_THRUST) res |= 0x80;
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

int asteroid_DSW1_r (int offset) {

	int res;
	int res1;

	res1 = readinputport(2);

	res = 0xfc | ((res1 >> (2 * (3 - (offset & 0x3)))) & 0x3);
	return res;
	}

void asteroid_bank_switch_w (int offset,int data) {

	int newbank;

	newbank = (data >> 2) & 1;
	if (bank != newbank) {
		/* Perform bankswitching on page 1 and page 2 */
		int temp;
		int i;

		bank = newbank;
		for (i = 0; i < 0x100; i++) {
			temp = RAM[0x100 + i];
			RAM[0x100 + i] = RAM[0x200 + i];
			RAM[0x200 + i] = temp;
			}
		}
	}

void astdelux_bank_switch_w (int offset,int data) {

	int newbank;

	switch (offset & 0x07) {
		case 0: /* Player 1 start LED */
			break;
		case 1:  /* Player 2 start LED */
			break;
		case 2:
			break;
		case 3: /* Thrust sound */
			break;
		case 4: /* bank switch */
			newbank = (data >> 2) & 1;
			if (bank != newbank) {
				/* Perform bankswitching on page 1 and page 2 */
				int temp;
				int i;

				bank = newbank;
				for (i = 0; i < 0x100; i++) {
					temp = RAM[0x100 + i];
					RAM[0x100 + i] = RAM[0x200 + i];
				RAM[0x200 + i] = temp;
					}
				}
			break;
		case 5: /* left coin counter */
			break;
		case 6: /* center coin counter */
			break;
		case 7: /* right coin counter */
			break;
		}
	}

void asteroid_init_machine(void)
{
	bank = 0;
}

