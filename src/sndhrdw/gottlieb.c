#include "driver.h"


void gottlieb_sh_w(int offset,int data)
{
   int fx= 255-data;


	if (Machine->samples == 0) return;

#if 0

/*   ok, emulating the priority of fx's does not seem to work great */

   static int current_fx=0;
   static int fx_priority[]={
	0,
	3,3,3,3,3,3,3,3,3,3,3,3,5,3,4,3,
	3,3,3,3,3,5,5,3,3,3,3,3,3,3,3,3,
	15,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

   if (fx && fx<48 && buf_size[fx] && samp_buf[fx]) {
	switch (fx) {
		case 44: /* reset */
/* it seems I can't do this, otherwise the fx are cut too early */
/*
			osd_stop_sample(0);
*/
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		default:
/* does this test work? it is only correct if the previous fx has not
   finished.... or if a 0 priority (fx #0) is sent between others */
			if (fx_priority[fx] >= fx_priority[current_fx])
			{
			  /*      osd_stop_sample(0);    */
				osd_play_sample(0,samp_buf[fx],buf_size[fx],44100,255,0);

			}
			break;
	}
   }
   if (fx<48) current_fx=fx;


#else

/* so, why not improve over the original machine and play several sounds */
/* simultaneously ?                                                      */
   if (fx && fx<48 && Machine->samples->sample[fx])
	switch (fx) {
		case 44: /* reset */
			break;
		case 45:
		case 46:
		case 47:
			/* read expansion socket */
			break;
		case 11: /* noser hop */
		case 23: /* noser fall */
		case 25: /* noser on disk */
		     osd_play_sample(1,Machine->samples->sample[fx]->data,
					Machine->samples->sample[fx]->length,44100,255,0);
		     break;
		default:
		     osd_play_sample(0,Machine->samples->sample[fx]->data,
					Machine->samples->sample[fx]->length,44100,255,0);
		     break;
	}
#endif
}

void gottlieb_sh_update(void)
{
}
