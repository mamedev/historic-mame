/***************************************************************************

  mixer.c

  Manage audio channels allocation, with volume and panning control

***************************************************************************/

#include "driver.h"


static int first_free_channel = 0;
static char channel_name[MIXER_MAX_CHANNELS][40];
static int channel_volume[MIXER_MAX_CHANNELS];
static unsigned char channel_default_mixing_level[MIXER_MAX_CHANNELS];
static unsigned char channel_mixing_level[MIXER_MAX_CHANNELS];
static int channel_gain[MIXER_MAX_CHANNELS];
static int channel_pan[MIXER_MAX_CHANNELS];
static int channel_is_stream[MIXER_MAX_CHANNELS];

static unsigned char config_default_mixing_level[MIXER_MAX_CHANNELS];
static unsigned char config_mixing_level[MIXER_MAX_CHANNELS];
static int config_invalid;

int mixer_sh_start(void)
{
	int i;

	first_free_channel = 0;
	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		channel_name[i][0] = 0;
		channel_default_mixing_level[i] = 0xff;
		channel_mixing_level[i] = 0xff;
		channel_is_stream[i] = 0;
	}

	return 0;
}

void mixer_sh_stop(void)
{
}

int mixer_allocate_channel(int default_mixing_level)
{
	int j,ch;

	ch = first_free_channel;

	if (ch >= MIXER_MAX_CHANNELS)
	{
if (errorlog) fprintf(errorlog,"Too many mixer channels (requested %d, available %d)\n",ch+1,MIXER_MAX_CHANNELS);
		exit(1);
	}

	channel_volume[ch] = 100;
	channel_default_mixing_level[ch] = MIXER_GET_LEVEL(default_mixing_level);
	/* backwards compatibility with old 0-255 volume range */
	if (channel_default_mixing_level[ch] > 100) channel_default_mixing_level[ch] = channel_default_mixing_level[ch] * 25 / 255;

	if (!config_invalid)
	{
		if (channel_default_mixing_level[ch] == config_default_mixing_level[ch])
		{
			channel_mixing_level[ch] = config_mixing_level[ch];
		}
		else
		{
			/* defaults are different from the saved config, set everything back */
			/* to the new defaults */
			config_invalid = 1;
			for (j = 0;j < ch;j++)
				mixer_set_mixing_level(j,channel_default_mixing_level[j]);
		}
	}

	if (config_invalid)
		channel_mixing_level[ch] = channel_default_mixing_level[ch];
	channel_pan[ch] = MIXER_GET_PAN(default_mixing_level);
	channel_gain[ch] = MIXER_GET_GAIN(default_mixing_level);
	mixer_set_name(ch,0);

	first_free_channel++;

	return ch;
}

int mixer_allocate_channels(int channels,const int *default_mixing_levels)
{
	int i,j,ch;

	ch = first_free_channel;

	if (ch + channels > MIXER_MAX_CHANNELS)
	{
if (errorlog) fprintf(errorlog,"Too many mixer channels (requested %d, available %d)\n",ch+channels,MIXER_MAX_CHANNELS);
		exit(1);
	}

	for (i = 0;i < channels;i++)
	{
		channel_volume[ch + i] = 100;
		channel_mixing_level[ch + i] = channel_default_mixing_level[ch + i] = MIXER_GET_LEVEL(default_mixing_levels[i]);
		/* backwards compatibility with old 0-255 volume range */
		if (channel_default_mixing_level[ch + i] > 100) channel_default_mixing_level[ch + i] = channel_default_mixing_level[ch + i] * 25 / 255;

		if (!config_invalid)
		{
			if (channel_default_mixing_level[ch + i] == config_default_mixing_level[ch + i])
			{
				channel_mixing_level[ch + i] = config_mixing_level[ch + i];
			}
			else
			{
				/* defaults are different from the saved config, set everything back */
				/* to the new defaults */
				config_invalid = 1;
				for (j = 0;j < ch + i;j++)
					mixer_set_mixing_level(j,channel_default_mixing_level[j]);
			}
		}

		if (config_invalid)
			channel_mixing_level[ch + i] = channel_default_mixing_level[ch + i];
		channel_pan[ch + i] = MIXER_GET_PAN(default_mixing_levels[i]);
		channel_gain[ch + i] = MIXER_GET_GAIN(default_mixing_levels[i]);
		mixer_set_name(ch+i,0);
	}

	first_free_channel += channels;

	return ch;
}

void mixer_set_name(int channel,const char *name)
{
	if (name)
		strcpy(channel_name[channel],name);
	else
		sprintf(channel_name[channel],"<channel #%d>",channel);

	if (channel_pan[channel] == MIXER_PAN_LEFT)
		strcat(channel_name[channel]," (Lt)");
	else if (channel_pan[channel] == MIXER_PAN_RIGHT)
		strcat(channel_name[channel]," (Rt)");
}

const char *mixer_get_name(int channel)
{
	if (channel_name[channel][0])
		return channel_name[channel];
	else return 0;	/* unused channel */
}

void mixer_set_volume(int channel,int volume)
{
	channel_volume[channel] = volume;
	/* for streams, the volume will be adjusted on next update (avoids distortion) */
	if (!channel_is_stream[channel])
		osd_set_sample_volume(channel,volume * channel_mixing_level[channel] / 100);
}

void mixer_set_mixing_level(int channel,int level)
{
	channel_mixing_level[channel] = level;
	/* for streams, the volume will be adjusted on next update (avoids distortion) */
	if (!channel_is_stream[channel])
		osd_set_sample_volume(channel,channel_volume[channel] * level / 100);
}

int mixer_get_mixing_level(int channel)
{
	return channel_mixing_level[channel];
}



void mixer_play_sample(int channel,signed char *data,int len,int freq,int loop)
{
	osd_play_sample(channel,data,len,freq,channel_volume[channel] * channel_mixing_level[channel] / 100,loop);
}

void mixer_play_sample_16(int channel,INT16 *data,int len,int freq,int loop)
{
	osd_play_sample_16(channel,data,len,freq,channel_volume[channel] * channel_mixing_level[channel] / 100,loop);
}

void mixer_play_streamed_sample_16(int channel,INT16 *data,int len,int freq)
{
	channel_is_stream[channel] = 1;

	if (channel_gain[channel])
	{
		int i;
		int mul = 1 << channel_gain[channel];
		int min = -0x8000 / mul;
		int max =  0x7fff / mul;

		for (i = len/2;i >= 0;i--)
		{
			if (data[i] <= min) data[i] = -0x8000;
			else if (data[i] >= max) data[i] = 0x7fff;
			else data[i] *= mul;
		}
	}

	osd_play_streamed_sample_16(channel,data,len,freq,channel_volume[channel] * channel_mixing_level[channel] / 100,channel_pan[channel]);
}

void mixer_read_config(void *f)
{
	memset(config_default_mixing_level,0xff,MIXER_MAX_CHANNELS);
	memset(config_mixing_level,0xff,MIXER_MAX_CHANNELS);
	config_invalid = 0;
	osd_fread(f,config_default_mixing_level,MIXER_MAX_CHANNELS);
	osd_fread(f,config_mixing_level,MIXER_MAX_CHANNELS);
}

void mixer_write_config(void *f)
{
	osd_fwrite(f,channel_default_mixing_level,MIXER_MAX_CHANNELS);
	osd_fwrite(f,channel_mixing_level,MIXER_MAX_CHANNELS);
}
