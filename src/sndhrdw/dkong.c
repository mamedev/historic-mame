#include "driver.h"
#include "sndhrdw\generic.h"
#include "I8039.h"
#include "dac.h"

/* A nice macro which saves us a lot of typing */
#define M_OSD_PLAY_SAMPLE(channel, soundnum, loop) { \
	if (Machine->samples->sample[soundnum] != 0) \
		osd_play_sample(channel, \
			Machine->samples->sample[soundnum]->data, \
			Machine->samples->sample[soundnum]->length, \
			Machine->samples->sample[soundnum]->smpfreq, \
			Machine->samples->sample[soundnum]->volume,loop); }


static unsigned char dkongDAC;
unsigned char *dkong_dac = &dkongDAC;

static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};



void dkong_sh_w(int offset,int data)
{
	if (data)
        {
	        cpu_cause_interrupt(1,I8039_EXT_INT);
        }
}


void dkong_digital_out_w(int offset,int data)
{
	*dkong_dac = data;
	DAC_data_w(0,data);
}



int dkong_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	return 0;
}

void dkong_sh_stop(void)
{
	DAC_sh_stop();
}

void dkong_sh_update(void)
{
	DAC_sh_update();
}



void dkong_sh1_w(int offset,int data)
{
	static int state[8];


	if (Machine->samples == 0) return;

	if (state[offset] != data)
	{
		if ((Machine->samples->sample[offset] != 0) && (data))
			M_OSD_PLAY_SAMPLE(offset+1,offset,0);

		state[offset] = data;
	}
}


void dkongjr_sh_death_w(int offset,int data)
{
	static int death = 0;


	if (Machine->samples == 0) return;

	if (death != data)
	{
		if ((Machine->samples->sample[0] != 0) && (data))
			M_OSD_PLAY_SAMPLE(8,0,0);

		death = data;
	}
}
void dkongjr_sh_drop_w(int offset,int data)
{
	static int drop = 0;


	if (Machine->samples == 0) return;

	if (drop != data)
	{
		if ((Machine->samples->sample[1] != 0) && (data))
			M_OSD_PLAY_SAMPLE(8,1,0);

		drop = data;
	}
}
void dkongjr_sh_roar_w(int offset,int data)
{
	static int roar = 0;


	if (Machine->samples == 0) return;

	if (roar != data)
	{
		if ((Machine->samples->sample[2] != 0) && (data))
			M_OSD_PLAY_SAMPLE(8,2,0);

		roar = data;
	}
}
