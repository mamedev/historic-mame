/*************************************************************************

  Food Fight machine hardware

*************************************************************************/

#include <stdlib.h>
#include "driver.h"


/*
 *		Globals we own
 */

unsigned char *foodf_nvram;


/*
 *		Statics
 */

static int whichport = 0;


/*
 *		Interrupt handlers.
 */

void foodf_delayed_interrupt (int param)
{
	cpu_cause_interrupt (0, 2);
}

int foodf_interrupt (void)
{
	/* INT 2 once per frame in addition to... */
	if (cpu_getiloops () == 0)
		timer_set (TIME_IN_USEC (100), 0, foodf_delayed_interrupt);

	/* INT 1 on the 32V signal */
	return 1;
}


/*
 *		NVRAM read/write.
 */

int foodf_nvram_r (int offset)
{
	return READ_WORD (&foodf_nvram[offset]) & 0x0f;
}


void foodf_nvram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&foodf_nvram[offset], data);
}


/*
 *		Analog controller read dispatch.
 */

int foodf_analog_r (int offset)
{
	switch (offset)
	{
		case 0:
		case 2:
		case 4:
		case 6:
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
		case 0:
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
