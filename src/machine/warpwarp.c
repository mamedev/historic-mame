/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


int warpwarp_input_c000_7_r(int offset)
{
	int res;
	res = readinputport(1);
	return(res & (1<<offset) ? 1 : 0);
/*
	switch (offset&7) {
		case 0x0 : if (osd_key_pressed(OSD_KEY_3)) return(0); else return(1);
		case 0x1 : return(1);
		case 0x2 : if (osd_key_pressed(OSD_KEY_1)) return(0); else return(1);
		case 0x3 : if (osd_key_pressed(OSD_KEY_2)) return(0); else return(1);
		case 0x4 : if (osd_key_pressed(OSD_KEY_ALT)) return(0); else return(1);
		case 0x5 : if (osd_key_pressed(OSD_KEY_F1)) return(0); else return(1);
		case 0x6 : return(1);
		case 0x7 : return(1);
		default:
			break;
	}
	return(0);
*/
}

/* Read the Dipswitches */
int warpwarp_input_c020_27_r(int offset)
{
	int res;

	res = readinputport(0);
	return(res & (1<<(offset & 7)) ? 1 : 0);
}

int warpwarp_input_controller_r(int offset)
{
	int res;

	res = readinputport(2);
	if (res & 1) return(111);
	if (res & 2) return(167);
	if (res & 4) return(63);
	if (res & 8) return(23);
	return(255);
/*
	unsigned char value=255;
	if (osd_key_pressed(OSD_KEY_LEFT)) {
		value=111;
	} else if (osd_key_pressed(OSD_KEY_RIGHT)) {
		value=167;
	} else if (osd_key_pressed(OSD_KEY_UP)) {
		value=63;
	} else if (osd_key_pressed(OSD_KEY_DOWN)) {
		value=23;
	}
	return(value);
*/
}

