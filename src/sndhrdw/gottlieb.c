/* macroscopic sound emulation from my understanding of the 6502 rom (FF) */

#include "driver.h"

static int current_fx,interrupted_fx;

void gottlieb_sh_w(int offset,int data)
{
   int fx= 255-data;


   if (Machine->samples == 0) return;

   if (fx && fx<48 && Machine->samples->sample[fx])
	switch (fx) {
		case 44: /* reset */
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		default:
		     osd_play_sample(0,Machine->samples->sample[fx]->data,
					Machine->samples->sample[fx]->length,
					 Machine->samples->sample[fx]->smpfreq,
					  Machine->samples->sample[fx]->volume,0);
		     break;
	}
}

void gottlieb_sh_update(void)
{
	if (interrupted_fx && osd_get_sample_status(1)) {
		current_fx=interrupted_fx;
		interrupted_fx=0;
		osd_restart_sample(0);
	}
	if (current_fx && osd_get_sample_status(0))
		current_fx=0;
}

void qbert_sh_w(int offset,int data)
{
   static int fx_priority[]={
	0,
	3,3,3,3,3,3,3,3,3,3,3,3,5,3,4,3,
	3,3,3,3,3,5,5,3,3,3,3,3,3,3,3,3,
	15,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
   int fx= 255-data;
   int channel;

   if (Machine->samples == 0) return;

   if (fx==44) { /* reset */
	interrupted_fx=0;
	current_fx=0;
	osd_stop_sample(0);
	osd_stop_sample(1);
   }
   if (fx && fx<44 && fx_priority[fx] >= fx_priority[current_fx]) { 
	if (current_fx==25 || current_fx==26 || current_fx==27)
		interrupted_fx=current_fx;
	if (fx==25 || fx==26 || fx==27 || fx==35) 
		interrupted_fx=0;
	if (interrupted_fx) 
		channel=1;
	else 
		channel=0;
	osd_stop_sample(0);
	osd_stop_sample(1);
	if (Machine->samples->sample[fx]) 
		osd_play_sample(channel,Machine->samples->sample[fx]->data,
				   Machine->samples->sample[fx]->length,
				    Machine->samples->sample[fx]->smpfreq,
				      Machine->samples->sample[fx]->volume,0);
	current_fx=fx;
   }
}



