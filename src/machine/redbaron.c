/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/* starwars uses A-to-D converter to get joystick-thrust as 8-bit data */
/* trying the same, redbaron doesn't seem to like the full range 0-255? */
/* dead response occurs at limits if min and max range is increased */
#define RBADCMAX 160                           /* +5v */
#define RBADCMIN 96                             /* gnd */
#define RBADCCTR 128
#define RESPONSE 1                             /* just a guess */


static int roll=RBADCCTR;	/* start centered */
static int pitch=RBADCCTR;
static int input_select; 	/* 0 is roll_data, 1 is pitch_data */

void redbaron_joyselect (int offset, int data)
{
	input_select=(data & 0x01);
}

int redbaron_joy_r (int offset)
{
	int res;
	int trak;
	
	res=readinputport(3);
	trak=readtrakport(0);
	if (trak != NO_TRAK) 
		roll+=trak;
	if (res & 0x01)
		roll-=RESPONSE;
	if (res & 0x02)
		roll+=RESPONSE;
	if (roll<RBADCMIN)
		roll=RBADCMIN;
	if (roll>RBADCMAX)
		roll=RBADCMAX;

	trak=readtrakport(1);
	if (trak != NO_TRAK) 
		pitch+=trak;
	if (res & 0x04)
		pitch-=RESPONSE;
	if (res & 0x08)
		pitch+=RESPONSE;
	if (pitch<RBADCMIN)
		pitch=RBADCMIN;
	if (pitch>RBADCMAX)
		pitch=RBADCMAX;
	
	if (input_select)
		return (roll);
	else
		return (pitch);
}

