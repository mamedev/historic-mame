/*
     buffering DAC driver
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"

#define STEP 32768 /* frequency steps / sampling rate */

#ifdef SIGNED_SAMPLES
  #define AUDIO_CONV(A) ((A)-0x80)
#else
  #define AUDIO_CONV(A) ((A))
#endif

static struct DACinterface *intf;

static int emulation_rate;
static int buffer_len;
static int buffer_step;
static void *dac_buffer[MAX_DAC];
static int sample_pos[MAX_DAC];
static int sample_step[MAX_DAC];
static int current_count[MAX_DAC];

static int amplitude_DAC[MAX_DAC];

static int volume[MAX_DAC];
static int channel;

static void DAC_update(int num , int newpos )
{
	int newstep;
	int data = amplitude_DAC[num];
	int pos  = sample_pos[num];
	int step = sample_step[num];
	void *buffer = dac_buffer[num];

	/* make new status */
	newstep = newpos &  STEP;
	newpos /= STEP;

	/* get current position based on the timer */
	if( pos == newpos )
	{
		current_count[num] += (newstep-step) * data;
	}
	else
	{
		current_count[num] += (STEP-step) * data;
		if( Machine->sample_bits == 16 )
		{	/* 16bit output */
			int data16 = data<<8;
			((unsigned short *)buffer)[pos++] = current_count[num] / (STEP / 256);
			while (pos < newpos )
				((unsigned short *)buffer)[pos++] = data16;
			current_count[num] = newstep * data;
		}
		else
		{	/* 8bit output */
			((unsigned char *)buffer)[pos++] = current_count[num] / STEP;
			while (pos < newpos )
				((unsigned char *)buffer)[pos++] = data;
			current_count[num] = newstep * data;
		}
	}
	sample_pos[num]    = pos;
	sample_step[num]   = newstep;
}

void DAC_data_w(int num , int data)
{
	/* check current data */
	if( AUDIO_CONV(data) == amplitude_DAC[num] ) return;
	/* update */
	DAC_update(num,cpu_scalebyfcount(buffer_step) );
	/* set new data */
	amplitude_DAC[num] = AUDIO_CONV(data);
}

/* maximum of 'vol' is 255(default) */
void DAC_volume(int num,int vol)
{
	volume[num]  = intf->volume[num] * vol / 255;
}


int DAC_sh_start(struct DACinterface *interface)
{
	int i;

	intf = interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	buffer_step = buffer_len * STEP;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	channel = get_play_channels(intf->num);
	/* reserve buffer */
	for (i = 0;i < intf->num;i++)
	{
		if ((dac_buffer[i] = malloc((Machine->sample_bits/8)*buffer_len)) == 0)
		{
			while (--i >= 0) free(dac_buffer[i]);
			return 1;
		}
		/* reset state */
		volume[i]  = intf->volume[i];
		sample_pos[i] = sample_step[i] = current_count[i] = 0;
		amplitude_DAC[i] = 0;
	}
	return 0;
}

void DAC_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++){
		free(dac_buffer[i]);
	}
}

void DAC_sh_update(void)
{
	int num;

	if (Machine->sample_rate == 0 ) return;

	for (num = 0;num < intf->num;num++)
	{
		DAC_update(num,buffer_step );
		/* reset position , step , count */
		sample_pos[num] = sample_step[num] = current_count[num] = 0;
		/* play sound */
		if( Machine->sample_bits == 16 )
			osd_play_streamed_sample_16(channel+num,dac_buffer[num],2*buffer_len,emulation_rate,volume[num]);
		else
			osd_play_streamed_sample(channel+num,dac_buffer[num],buffer_len,emulation_rate,volume[num]);

	}
}
