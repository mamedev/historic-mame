#include "driver.h"
#include "pokey.h"

static int test0, test1, port2;

#define M_osd_play_sample(chan,num,loop) { \
	if (Machine->samples->sample[num] != 0) \
		osd_play_sample(chan,Machine->samples->sample[num]->data, \
			Machine->samples->sample[num]->length, \
			Machine->samples->sample[num]->smpfreq, \
			Machine->samples->sample[num]->volume,loop); }\

void mario_sh1_w(int offset,int data)
{
	static state[8];


	if (Machine->samples == 0) return;

	if (offset == 1) port2 = data;
	if (offset == 4) test1 = data;
	if (offset == 5) test0 = data;

	if (state[offset] != data)
	{
		if ((Machine->samples->sample[offset] != 0) && (data))
			osd_play_sample(offset,Machine->samples->sample[offset]->data,
					Machine->samples->sample[offset]->length,
                                        Machine->samples->sample[offset]->smpfreq,
                                        Machine->samples->sample[offset]->volume,0);

		state[offset] = data;
	}
	if (data != 0x00)
		if (errorlog) fprintf (errorlog, "*1 offset: %02x, data: %02x\n",offset, data);
}


void mario_sh3_w(int offset,int data)
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


void mario_sh2_w(int offset,int data)
{
	static last;


	if (Machine->samples == 0) return;

//	if (last != data)
	{
		last = data;

		switch (data & 0xf0) {
			case 0x10:
				M_osd_play_sample(8,8,0);
				break;
			case 0x20:
				M_osd_play_sample(8,9,0);
				break;
			case 0x40:
				M_osd_play_sample(8,10,0);
				break;
			case 0x80:
				M_osd_play_sample(8,11,0);
				break;
			}

		data &= 0x0f;
		switch (data)
		{
			/* these samples are longer - play on diff channel */
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0d:
				if (Machine->samples->sample[data+11] != 0)
				{
					osd_play_sample(9,Machine->samples->sample[data+11]->data,
						Machine->samples->sample[data+11]->length,
						Machine->samples->sample[data+11]->smpfreq,
						Machine->samples->sample[data+11]->volume,0);
				}
				break;

			default:
				if (data != 0)
					if (Machine->samples->sample[data+11] != 0)
					{
						osd_play_sample(10,Machine->samples->sample[data+11]->data,
							Machine->samples->sample[data+11]->length,
							Machine->samples->sample[data+11]->smpfreq,
							Machine->samples->sample[data+11]->volume,0);
					}
				break;
		}
	}
	if (data != 0x00)
		if (errorlog) fprintf (errorlog, "*2 offset: %02x, data: %02x\n",offset, data);
}



void mario_sh_update(void)
{
}

int mario_port2_r (int offset) {
	return port2 ^ 0xff;
	}
