#include "driver.h"
#include "pokey.h"



void dkong_sh1_w(int offset,int data)
{
	static state[8];


	if (Machine->samples == 0) return;

	if (state[offset] != data)
	{
		if ((Machine->samples->sample[offset] != 0) && (data))
			osd_play_sample(offset,Machine->samples->sample[offset]->data,
					Machine->samples->sample[offset]->length,
                                        Machine->samples->sample[offset]->smpfreq,
                                        Machine->samples->sample[offset]->volume,0);

		state[offset] = data;
	}
}


void dkong_sh3_w(int offset,int data)
{
	static last_dead;


	if (Machine->samples == 0) return;

	if (last_dead != data)
	{
		last_dead = data;
		if (data)
		{
			if (Machine->samples->sample[24])
				osd_play_sample(0,Machine->samples->sample[24]->data,
						Machine->samples->sample[24]->length,
                                                Machine->samples->sample[24]->smpfreq,
                                                Machine->samples->sample[24]->volume,0);
			osd_stop_sample(1);		  /* kill other samples */
			osd_stop_sample(2);
			osd_stop_sample(3);
			osd_stop_sample(4);
			osd_stop_sample(5);
		}
	}
}


void dkong_sh2_w(int offset,int data)
{
	static last;
	static hit = 0;

	if (Machine->samples == 0) return;

    if (data != 6)
	{
	   hit = 0;
    }

	if (last != data)
	{
		last = data;

		switch (data)
		{
		   case 6:	  /* mix in hammer hit on different channel */
		   last = 4;
		   if (!hit)
		   {
		       if (Machine->samples->sample[data+8] != 0)
			   {
			   	  osd_play_sample(3,Machine->samples->sample[data+8]->data,
				     Machine->samples->sample[data+8]->length,
                                     Machine->samples->sample[data+8]->smpfreq,
                                     Machine->samples->sample[data+8]->volume,0);
			   }
			   hit = 1;
			}
			break;

            case 13:
			last = 11;
		    if (Machine->samples->sample[data+8] != 0)
			{
			   osd_play_sample(3,Machine->samples->sample[data+8]->data,
				  Machine->samples->sample[data+8]->length,
                                  Machine->samples->sample[data+8]->smpfreq,
                                  Machine->samples->sample[data+8]->volume,0);
			}
			break;

			case 8:		  /* these samples repeat */
			case 9:
			case 10:
			case 11:
			case 4:
			case 3:
			if (Machine->samples->sample[data+8] != 0)
			{
				osd_play_sample(4,Machine->samples->sample[data+8]->data,
						Machine->samples->sample[data+8]->length,
                                                Machine->samples->sample[data+8]->smpfreq,
                                                Machine->samples->sample[data+8]->volume,1);
			}
			break;

			default:		  /* the rest do not */
			if (data != 0)
			{
				if (Machine->samples->sample[data+8] != 0)
				{
					osd_play_sample(4,Machine->samples->sample[data+8]->data,
							Machine->samples->sample[data+8]->length,
                                                        Machine->samples->sample[data+8]->smpfreq,
                                                        Machine->samples->sample[data+8]->volume,0);

				}
			}
			break;
		}
	}
}



void dkong_sh_update(void)
{
}
