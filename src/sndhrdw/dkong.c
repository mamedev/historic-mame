#include "driver.h"
#include "pokey.h"



#define UPDATES_PER_SECOND 60
#define emulation_rate 11025



void dkong_sh1_w(int offset,int data)
{
	static state[8];


	if (Machine->samples == 0) return;

	if (state[offset] != data)
	{
		if ((Machine->samples->sample[offset] != 0) && (data))
			osd_play_sample(offset,Machine->samples->sample[offset]->data,
					Machine->samples->sample[offset]->length,emulation_rate, 255,0);

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
						Machine->samples->sample[24]->length,emulation_rate,255,0);
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


	if (Machine->samples == 0) return;

	if (last != data)
	{
		switch (data)
		{
			case 8:		  /* these samples repeat */
			case 9:
			case 10:
			case 11:
			case 4:
			if (Machine->samples->sample[data+8] != 0)
			{
				osd_play_sample(4,Machine->samples->sample[data+8]->data,
						Machine->samples->sample[data+8]->length,emulation_rate,255,1);
			}
			break;

			default:		  /* the rest do not */
			if (data != 0)
			{
				if (Machine->samples->sample[data+8] != 0)
				{
					osd_play_sample(4,Machine->samples->sample[data+8]->data,
							Machine->samples->sample[data+8]->length,emulation_rate,255,0);
				}
			}
			break;
		}
		last = data;
	}
}



void dkong_sh_update(void)
{
}
