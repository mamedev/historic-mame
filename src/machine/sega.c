/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"

/* dummy bits for IN4 to control the spinner */
#define IN4_LEFT	0x40
#define IN4_RIGHT	0x80

void sega_vg_draw (void);

unsigned char mult1;
unsigned short result;
unsigned char ioSwitch; /* determines whether we're reading the spinner or the buttons */
unsigned char dir = 0;

int sega_interrupt (void) {

	sega_vg_draw ();
	return interrupt ();
	}

void sega_mult1_w (int offset, int data) {

	mult1 = data;
	}

void sega_mult2_w (int offset, int data) {

	result = mult1 * data;
	}

void sega_switch_w (int offset, int data) {

	ioSwitch = data;
/*	if (errorlog) fprintf (errorlog,"ioSwitch: %02x\n",ioSwitch); */
	}

int sega_mult_r (int offset) {

	int c;

	c = result & 0xff;
	result >>= 8;
	return (c);
	}

int zektor_IN4_r (int offset) {

	int res;
	int trak_in;
	static unsigned char spinner = 0;

	res = readinputport (4);

	if (ioSwitch & 1) /* ioSwitch = 0x01 or 0xff */
		return (res & 0x3f); /* mask out the left/right dummy bits */
	else { /* ioSwitch = 0xfe */
		if (res & IN4_LEFT) {
			spinner += 2;
			dir = 1;
			}
		if (res & IN4_RIGHT) {
			spinner += 2;
			dir = 0;
			}
		trak_in = readtrakport(0);

			if (trak_in < 0) {
				dir = 1;
				trak_in = abs(trak_in);
			} else if (trak_in > 0)
				dir = 0;
			spinner += trak_in;

		return ((spinner << 1) | dir);
	}
}

int startrek_IN4_r (int offset) {

	int res;
	int trak_in;
	static unsigned char spinner = 0;

	res = readinputport (4);

	if (ioSwitch & 1) /* ioSwitch = 0x01 or 0xff */
		return (res & 0x3f); /* mask out the left/right dummy bits */
	/* ioSwitch = 0xfe */
	/* The spinner controls in Star Trek are inverted from the rest of */
	/* the Sega vector games that use the spinner */
	if (res & IN4_LEFT) {
		spinner -= 10;
		dir = 0;
	}
	if (res & IN4_RIGHT) {
		spinner -= 10;
		dir = 1;
	}
	trak_in = readtrakport(0);
	if (trak_in != NO_TRAK) {
		if (trak_in < 0) {
			dir = 0;
			trak_in = abs(trak_in);
		} else if (trak_in > 1)
			dir = 1;
		spinner -= trak_in;
	}
	return ((spinner << 1) | dir);
}

int sega_spinner(int data) {

#define MAX_SPINNER	64

	if (data > MAX_SPINNER)
		data = MAX_SPINNER;
	if (data < -MAX_SPINNER)
		data = -MAX_SPINNER;
	return (data);
	}
