/*
     buffering DAC driver
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/dac.h"

//#define CR_FILTER		/* support CR filtaling after DAC output */

#ifdef CR_FILTER
#define AUDIO_CONV(A) (((A)<<8)-0x8000)
#else
#define AUDIO_CONV(A) ((A)-0x80)
#endif

static struct DACinterface *intf;

static int emulation_rate;
static int buffer_len;
static unsigned char *buffer[MAX_DAC];
static int sample_pos[MAX_DAC];

static int amplitude_DAC[MAX_DAC];
static int output_DAC[MAX_DAC];

static int volume[MAX_DAC];
static int channel;

#ifdef CR_FILTER
/* C-R filter tables */
static int filter[511];
#endif

void DAC_data_w(int num , int newdata)
{
	int totcycles  = cpu_getfperiod();
	int leftcycles = cpu_getfcount();
	int newpos;
	int data   = amplitude_DAC[num];
#ifdef CR_FILTER
	int output = output_DAC[num];
#endif
	int pos    = sample_pos[num];

	newpos = buffer_len * (totcycles-leftcycles) / totcycles;
	if (newpos >= buffer_len) newpos = buffer_len - 1;

#ifdef CR_FILTER
	while (pos < newpos )
	{
		output += filter[0xff+((data-output)>>8)];
	    buffer[num][pos++] = (output>>8);
	}
	output_DAC[num]    = output;
#else
	while (pos < newpos )
	    buffer[num][pos++] = data;
#endif
	amplitude_DAC[num] = AUDIO_CONV(newdata);
	sample_pos[num]    = pos;
}

/* maximum of 'vol' is 255(default) */
void DAC_volume(int num,int vol)
{
	volume[num]  = intf->volume[num] * vol / 255;
}


int DAC_sh_start(struct DACinterface *interface)
{
	int i;
#ifdef CR_FILTER
	int j,delta;
#endif
	intf = interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	channel = get_play_channels(intf->num);
	/* reserve buffer */
	for (i = 0;i < intf->num;i++)
	{
		if ((buffer[i] = malloc(sizeof(char)*buffer_len)) == 0)
		{
			while (--i >= 0) free(buffer[i]);
			return 1;
		}
		/* reset state */
		volume[i]  = intf->volume[i];
		sample_pos[i] = 0;
		amplitude_DAC[i] = output_DAC[i] = 0;
#ifdef CR_FILTER
		/* make filter table */
		filter[0xff] =  0;
		for (j = 1;j < 256 ;j++)
		{
			delta = (j<<8);
			if( delta < 1 ) delta = 1;
			filter[0xff+j] =  delta;
			filter[0xff-j] = -delta;
		}
#endif
	}
	return 0;
}

void DAC_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++){
		free(buffer[i]);
	}
}

void DAC_sh_update(void)
{
	int i;
	int data , pos;
#ifdef CR_FILTER
	int output;
#endif

	if (Machine->sample_rate == 0 ) return;

	for (i = 0;i < intf->num;i++)
	{
#ifdef CR_FILTER
		data = amplitude_DAC[i];
		output  = output_DAC[i];
		pos = sample_pos[i];
		while (pos < buffer_len )
		{
			output += filter[0xff+((data-output)>>8)];
		    buffer[i][pos++] = (output>>8);
		}
		output_DAC[i] = output;
#else
		data = amplitude_DAC[i];
		pos = sample_pos[i];
		while (pos < buffer_len )
		    buffer[i][pos++] = data;
#endif
		sample_pos[i] = 0;

		osd_play_streamed_sample(channel+i,buffer[i],buffer_len,emulation_rate,volume[i]);
	}
}
