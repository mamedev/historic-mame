/*************************************************************************

  Food Fight machine hardware

*************************************************************************/

#include <stdlib.h>
#include "driver.h"


/*
 *		Globals we own
 */

int foodf_nvram_size;


/*
 *		Statics
 */

static unsigned char *foodf_nvram;
static int whichport = 0;


/*
 *		Actually called by the video system to free memory regions.
 */

int foodf_system_stop (void)
{
	/* free the banks we allocated */
	if (foodf_nvram) free (foodf_nvram); foodf_nvram = 0;

	return 0;
}


/*
 *		Actually called by the video system to initialize memory regions.
 */

int foodf_system_start (void)
{
	/* allocate the NVRAM bank */
	if (!foodf_nvram) foodf_nvram = calloc (foodf_nvram_size, 1);
	if (!foodf_nvram)
	{
		foodf_system_stop ();
		return 1;
	}

	/* point to the generic RAM banks */
	cpu_setbank (1, foodf_nvram);

	return 0;
}


/*
 *		Returns the base of the NVRAM memory for hiscore save/load.
 */

unsigned char *foodf_nvram_base (void)
{
	return foodf_nvram;
}


/*
 *		Interrupt handlers.
 */

int foodf_interrupt (void)
{
	/* INT 2 once per frame in addition to... */
	if (cpu_getiloops () == 0)
		cpu_cause_interrupt (0, 2);

	/* INT 1 on the 32V signal */
	return 1;
}


/*
 *		NVRAM read/write.
 */

int foodf_nvram_r (int offset)
{
	if (!(offset & 1))
		return 0;
	return foodf_nvram[offset] & 0x0f;
}


void foodf_nvram_w (int offset, int data)
{
	if (!(offset & 1))
		return;
	foodf_nvram[offset] = data & 0x0f;
}


/*
 *		Analog controller read dispatch.
 */

int foodf_analog_r (int offset)
{
	switch (offset)
	{
		case 1:
		case 3:
		case 5:
		case 7:
			return readinputport (whichport);
	}
	return 0;
}


/*
 *		Digital controller read dispatch.
 */

int foodf_digital_r (int offset)
{
	switch (offset)
	{
		case 1:
			return input_port_4_r (offset);
	}
	return 0;
}


/*
 *		Analog write dispatch.
 */

void foodf_analog_w (int offset, int data)
{
	whichport = 3 - ((offset/2) & 3);
}


/*
 *		Digital write dispatch.
 */

void foodf_digital_w (int offset, int data)
{
}
