/*************************************************************************

  Rastan machine hardware

*************************************************************************/

#include "driver.h"


/*************************************
 *
 *		Globals we own
 *
 *************************************/


/*************************************
 *
 *		Statics
 *
 *************************************/


/*************************************
 *
 *		Interrupt handler
 *
 *************************************/

int rastan_interrupt(void)
{
	return 5;  /*Interrupt vector 5*/
}


/*************************************
 *
 *		Input handler
 *
 *************************************/

int rastan_input_r (int offset)
{
	switch (offset)
	{
		case 0:
			return input_port_0_r (offset);
		case 2:
			return input_port_1_r (offset);
		case 6:
			return input_port_2_r (offset);
		case 8:
			return input_port_3_r (offset);
		case 10:
			return input_port_4_r (offset);
		default:
			return 0;
	}
}


/*************************************
 *
 *		Speed cheats
 *
 *************************************/

static int last_speed_check;
static unsigned char speed_check_ram[4];

int rastan_speedup_r (int offset)
{
	int result = READ_WORD (&speed_check_ram[offset]);

	if (offset == 0)
	{
		int time = cpu_getfcount ();
		int delta = last_speed_check - time;

		last_speed_check = time;
		if (delta >= 0 && delta <= 150 && result == 0)
			cpu_seticount (0);
	}

	return result;
}


void rastan_speedup_w (int offset, int data)
{
	if (!(offset & 2))
		last_speed_check = -10000;
	COMBINE_WORD_MEM (&speed_check_ram[offset], data);
}
