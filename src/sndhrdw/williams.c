#include <stdio.h>

#include "driver.h"

void williams_play_sound(int n)
{
	if (errorlog)
		fprintf(errorlog, "Playing sound %x\n", n);
		
	if (Machine->samples == 0 || Machine->samples->sample[n] == 0) return;

	if (n < 0 || n >= 0x3f) {
		osd_stop_sample(0);		  /* kill sample */
	} else {
		osd_play_sample(0,
				Machine->samples->sample[n]->data,
				Machine->samples->sample[n]->length,
                Machine->samples->sample[n]->smpfreq,
                Machine->samples->sample[n]->volume,
                0);
	}
}

void williams_sh_update(void)
{
}
