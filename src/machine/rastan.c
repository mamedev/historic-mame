/*************************************************************************

  Rastan machine hardware

*************************************************************************/

#include "driver.h"


/*************************************
 *
 *		Globals we own
 *
 *************************************/

int rastan_ram_size;


/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *ram;


/*************************************
 *
 *		Actually called by the video system to initialize memory regions.
 *
 *************************************/

int rastan_system_start (void)
{
	/* allocate the RAM bank */
	if (!ram)
		ram = calloc (rastan_ram_size, 1);
	if (!ram)
		return 1;

	/* point to the generic RAM bank */
	cpu_setbank (1, ram);

	return 0;
}


/*************************************
 *
 *		Actually called by the video system to free memory regions.
 *
 *************************************/

int rastan_system_stop (void)
{
	/* free the bank we allocated */
	if (ram)
		free (ram);
	ram = 0;

	return 0;
}


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
		case 1:
			return input_port_0_r (offset);
		case 3:
			return input_port_1_r (offset);
		case 7:
			return input_port_2_r (offset);
		case 9:
			return input_port_3_r (offset);
		case 11:
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
	int result = speed_check_ram[offset];

	if (offset == 1)
	{
		int time = cpu_getfcount ();
		int delta = last_speed_check - time;
		
		last_speed_check = time;
		if (delta >= 0 && delta <= 150 && result == 0 && speed_check_ram[0] == 0)
			cpu_seticount (0);
	}

	return result;
}


void rastan_speedup_w (int offset, int data)
{
	if (!(offset & 2))
		last_speed_check = -10000;
	speed_check_ram[offset] = data;
}
