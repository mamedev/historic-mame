#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/waveform.h"


/* note: we work with signed samples here, unlike other drivers */
#define AUDIO_CONV(A) (A)

static int emulation_rate;
static struct waveform_interface *interface;
static int buffer_len;
static int sample_pos;
static unsigned char *buffer;

static int channel;

unsigned char *pengo_soundregs;
unsigned char *mappy_soundregs;
static int sound_enable;

#define MAX_VOICES 8

static int freq[MAX_VOICES],volume[MAX_VOICES];
static const unsigned char *wave[MAX_VOICES];
static int counter[MAX_VOICES];


static unsigned char *mixer_table;
static unsigned char *mixer_lookup;
static unsigned short *mixer_buffer;


/* note: gain is specified as gain*16 */
static int make_mixer_table (int voices, int gain)
{
	int count = voices * 128;
	int i;

	/* allocate memory */
	mixer_table = malloc (256 * voices);
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
	mixer_lookup = mixer_table + (voices * 128);

	/* fill in the table */
	for (i = 0; i < count; i++)
	{
		int val = i * gain / (voices * 16);
		if (val > 127) val = 127;
		mixer_lookup[ i] = AUDIO_CONV(val);
		mixer_lookup[-i] = AUDIO_CONV(-val);
	}

	return 0;
}


static void waveform_update(signed char *buffer,int len)
{
	int i;
	int voice;

	short *mix;

	/* if no sound, fill with silence */
	if (sound_enable == 0)
	{
		for (i = 0; i < len; i++)
			*buffer++ = AUDIO_CONV (0x00);
		return;
	}


	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*mix++ = 0;

	for (voice = 0; voice < interface->voices; voice++)
	{
		int f = freq[voice];
		int v = volume[voice];

		if (v && f)
		{
			const unsigned char *w = wave[voice];
			int c = counter[voice];

			mix = mixer_buffer;
			for (i = 0; i < len; i++)
			{
				c += f;
				*mix++ += ((w[(c >> 15) & 0x1f] & 0x0f) - 8) * v;
			}

			counter[voice] = c;
		}
	}

	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*buffer++ = mixer_lookup[*mix++];
}


int waveform_sh_start(struct waveform_interface *intf)
{
	int voice;


	buffer_len = intf->samplerate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	channel = get_play_channels(1);

	if ((buffer = malloc(buffer_len)) == 0)
		return 1;

	if ((mixer_buffer = malloc(sizeof(short) * buffer_len)) == 0)
	{
		free (buffer);
		return 1;
	}

	if (make_mixer_table (intf->voices,intf->gain))
	{
		free (mixer_buffer);
		free (buffer);
		return 1;
	}

	sample_pos = 0;
	interface = intf;
	sound_enable = 1;	/* start with sound enabled, many games don't have a sound enable register */

	for (voice = 0;voice < intf->voices;voice++)
	{
		freq[voice] = 0;
		volume[voice] = 0;
		wave[voice] = &Machine->gamedrv->sound_prom[0];
		counter[voice] = 0;
	}

	return 0;
}


void waveform_sh_stop(void)
{
	free (mixer_table);
	free (mixer_buffer);
	free (buffer);
}


void waveform_sh_update(void)
{
	if (Machine->sample_rate == 0) return;


	waveform_update(&buffer[sample_pos],buffer_len - sample_pos);
	sample_pos = 0;

	osd_play_streamed_sample(channel,buffer,buffer_len,emulation_rate,interface->volume);
}


/********************************************************************************/


static void doupdate()
{
	int totcycles,leftcycles,newpos;


	totcycles = cpu_getfperiod();
	leftcycles = cpu_getfcount();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;
	if (newpos >= buffer_len) newpos = buffer_len - 1;

	waveform_update(&buffer[sample_pos],newpos - sample_pos);
	sample_pos = newpos;
}


/********************************************************************************/


static struct waveform_interface pengo_interface =
{
   3072000/32,      /* sample rate */
   3,               /* number of voices */
   32,              /* gain adjustment */
	255              /* playback volume */
};

int pengo_sh_start(void)
{
	return waveform_sh_start(&pengo_interface);	/* 3.072Mhz sound clock, 3 voices */
}

void pengo_sound_enable_w(int offset,int data)
{
	sound_enable = data;
}

void pengo_sound_w(int offset,int data)
{
	int voice;


	doupdate();	/* update output buffer before changing the registers */

	pengo_soundregs[offset] = data & 0x0f;


	for (voice = 0;voice < interface->voices;voice++)
	{
		freq[voice] = pengo_soundregs[0x14 + 5 * voice];	/* always 0 */
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x13 + 5 * voice];
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x12 + 5 * voice];
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x11 + 5 * voice];
		if (voice == 0)
			freq[voice] = freq[voice] * 16 + pengo_soundregs[0x10 + 5 * voice];
		else freq[voice] = freq[voice] * 16;

		volume[voice] = pengo_soundregs[0x15 + 5 * voice];

		wave[voice] = &Machine->gamedrv->sound_prom[32 * (pengo_soundregs[0x05 + 5 * voice] & 7)];
	}
}


/********************************************************************************/


static struct waveform_interface mappy_interface =
{
   23920,           /* sample rate */
   8,               /* number of voices */
   48,              /* gain adjustment */
	255              /* playback volume */
};

int mappy_sh_start(void)
{
	return waveform_sh_start(&mappy_interface);	/* approximated clock, 8 voices */
}

void mappy_sound_enable_w(int offset,int data)
{
	sound_enable = offset;
}

void mappy_sound_w(int offset,int data)
{
	int voice;


	doupdate();	/* update output buffer before changing the registers */

	mappy_soundregs[offset] = data;


	for (voice = 0;voice < interface->voices;voice++)
	{
		freq[voice] = mappy_soundregs[0x06 + 8 * voice] & 15;	/* high bits are from here */
		freq[voice] = freq[voice] * 256 + mappy_soundregs[0x05 + 8 * voice];
		freq[voice] = freq[voice] * 256 + mappy_soundregs[0x04 + 8 * voice];

		volume[voice] = mappy_soundregs[0x03 + 8 * voice];

		wave[voice] = &Machine->gamedrv->sound_prom[32 * ((mappy_soundregs[0x06 + 8 * voice] >> 4) & 7)];
	}
}

