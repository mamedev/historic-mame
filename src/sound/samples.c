#include "driver.h"



static int firstchannel,numchannels;


/* Start one of the samples loaded from disk. Note: channel must be in the range */
/* 0 .. Samplesinterface->channels-1. It is NOT the discrete channel to pass to */
/* mixer_play_sample() */
void sample_start(int channel,int samplenum,int loop)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (Machine->samples->sample[samplenum] == 0) return;
	if (channel >= numchannels)
	{
		if (errorlog) fprintf(errorlog,"error: sample_start() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}
	if (samplenum >= Machine->samples->total)
	{
		if (errorlog) fprintf(errorlog,"error: sample_start() called with samplenum = %d, but only %d samples available\n",samplenum,Machine->samples->total);
		return;
	}

	if ( Machine->samples->sample[samplenum]->resolution == 8 )
	{
		if (errorlog) fprintf(errorlog,"play 8 bit sample %d, channel %d\n",samplenum,channel);
		mixer_play_sample(firstchannel + channel,
				Machine->samples->sample[samplenum]->data,
				Machine->samples->sample[samplenum]->length,
				Machine->samples->sample[samplenum]->smpfreq,
				loop);
	}
	else
	{
		if (errorlog) fprintf(errorlog,"play 16 bit sample %d, channel %d\n",samplenum,channel);
		mixer_play_sample_16(firstchannel + channel,
				(short *) Machine->samples->sample[samplenum]->data,
				Machine->samples->sample[samplenum]->length,
				Machine->samples->sample[samplenum]->smpfreq,
				loop);
	}
}

void sample_set_freq(int channel,int freq)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		if (errorlog) fprintf(errorlog,"error: sample_adjust() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	osd_set_sample_freq(channel + firstchannel,freq);
}

void sample_set_volume(int channel,int volume)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		if (errorlog) fprintf(errorlog,"error: sample_adjust() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_volume(channel + firstchannel,volume * 100 / 255);
}

void sample_stop(int channel)
{
	if (Machine->sample_rate == 0) return;
	if (channel >= numchannels)
	{
		if (errorlog) fprintf(errorlog,"error: sample_stop() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	osd_stop_sample(channel + firstchannel);
}

int sample_playing(int channel)
{
	if (Machine->sample_rate == 0) return 0;
	if (channel >= numchannels)
	{
		if (errorlog) fprintf(errorlog,"error: sample_playing() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return 0;
	}

	return !osd_get_sample_status(channel + firstchannel);
}



/***************************************************************************

  Read samples into memory.
  This function is different from readroms() because it doesn't fail if
  it doesn't find a file: it will load as many samples as it can find.

***************************************************************************/

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

static struct GameSample *read_wav_sample(void *f)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize, temp32;
	UINT16 bits, temp16;
	char buf[32];
	struct GameSample *result;

	/* read the core header and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 4)
		return NULL;
	if (memcmp(&buf[0], "RIFF", 4) != 0)
		return NULL;

	/* get the total size */
	offset += osd_fread(f, &filesize, 4);
	if (offset < 8)
		return NULL;
	filesize = intelLong(filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 12)
		return NULL;
	if (memcmp(&buf[0], "WAVE", 4) != 0)
		return NULL;

	/* seek until we find a format tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "fmt ", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* read the format -- make sure it is PCM */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* number of channels -- only mono is supported */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* sample rate */
	offset += osd_fread(f, &rate, 4);
	rate = intelLong(rate);

	/* bytes/second and block alignment are ignored */
	offset += osd_fread(f, buf, 6);

	/* bits/sample */
	offset += osd_fread_lsbfirst(f, &bits, 2);
	if (bits != 8 && bits != 16)
		return NULL;

	/* seek past any extra data */
	osd_fseek(f, length - 16, SEEK_CUR);
	offset += length - 16;

	/* seek until we find a data tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "data", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* allocate the game sample */
	result = malloc(sizeof(struct GameSample) + length);
	if (result == NULL)
		return NULL;

	/* fill in the sample data */
	result->length = length;
	result->smpfreq = rate;
	result->resolution = bits;

	/* read the data in */
	if (bits == 8)
	{
		osd_fread(f, result->data, length);

		/* convert 8-bit data to signed samples */
		for (temp32 = 0; temp32 < length; temp32++)
			result->data[temp32] ^= 0x80;
	}
	else
	{
		/* 16-bit data is fine as-is */
		osd_fread_lsbfirst(f, result->data, length);
	}

	return result;
}

struct GameSamples *readsamples(const char **samplenames,const char *basename)
/* V.V - avoids samples duplication */
/* if first samplename is *dir, looks for samples into "basename" first, then "dir" */
{
	int i;
	struct GameSamples *samples;
	int skipfirst = 0;

	/* if the user doesn't want to use samples, bail */
	if (!options.use_samples) return 0;

	if (samplenames == 0 || samplenames[0] == 0) return 0;

	if (samplenames[0][0] == '*')
		skipfirst = 1;

	i = 0;
	while (samplenames[i+skipfirst] != 0) i++;

	if ((samples = malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;
	for (i = 0;i < samples->total;i++)
		samples->sample[i] = 0;

	for (i = 0;i < samples->total;i++)
	{
		void *f;

		if (samplenames[i+skipfirst][0])
		{
			if ((f = osd_fopen(basename,samplenames[i+skipfirst],OSD_FILETYPE_SAMPLE,0)) == 0)
				if (skipfirst)
					f = osd_fopen(samplenames[0]+1,samplenames[i+skipfirst],OSD_FILETYPE_SAMPLE,0);
			if (f != 0)
			{
				samples->sample[i] = read_wav_sample(f);
				osd_fclose(f);
			}
		}
	}

	return samples;
}


void freesamples(struct GameSamples *samples)
{
	int i;


	if (samples == 0) return;

	for (i = 0;i < samples->total;i++)
		free(samples->sample[i]);

	free(samples);
}



int samples_sh_start(const struct MachineSound *msound)
{
	int i;
	int vol[MIXER_MAX_CHANNELS];
	const struct Samplesinterface *intf = msound->sound_interface;

	/* read audio samples if available */
	Machine->samples = readsamples(intf->samplenames,Machine->gamedrv->name);

	numchannels = intf->channels;
	for (i = 0;i < numchannels;i++)
		vol[i] = intf->volume;
	firstchannel = mixer_allocate_channels(numchannels,vol);
	for (i = 0;i < numchannels;i++)
	{
		char buf[40];

		sprintf(buf,"Sample #%d",i);
		mixer_set_name(firstchannel + i,buf);
	}
	return 0;
}
