#include "driver.h"
#include <math.h>


#define AUDIO_CONV(A) (A-0x80)

#define SOUND_CLOCK (18432000/6/2) /* 1.536 Mhz */

#define NOISE_LENGTH 8000
#define NOISE_RATE 1000
#define TOOTHSAW_LENGTH 16
#define TOOTHSAW_VOLUME 36
#define STEPS 16
#define LFO_VOLUME 8
#define SHOOT_VOLUME 50
#define NOISE_VOLUME 50
#define WAVE_AMPLITUDE 70
#define MAXFREQ 200
#define MINFREQ 80

#define STEP 1

static signed char *noise;

static int shootsampleloaded = 0;
static int deathsampleloaded = 0;
static int t=0;
static int LastPort1=0;
static int LastPort2=0;
static int lfo_rate=0;
static int lfo_active[3];
static int freq=MAXFREQ;

static signed char waveform1[4][TOOTHSAW_LENGTH];
static int pitch,vol;

static signed char waveform2[32] =
{
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  -0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,
  -0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,
};

static int channelnoise,channelshoot,channellfo;
static int tone_stream;



static void tone_update(int ch, void *buffer, int length)
{
	int i,j;
	signed char *w = waveform1[vol];
	signed char *dest = buffer;
	static int counter,countdown;

	/* only update if we have non-zero volume and frequency */
	if (pitch!=0xff)
	{
		for (i = 0; i < length; i++)
		{
			int mix = 0;

			for (j = 0;j < STEPS;j++)
			{
				if (countdown >= 256)
				{
					counter = (counter + 1) % TOOTHSAW_LENGTH;
					countdown = pitch;
				}
				countdown++;

				mix += w[counter];
			}

			*dest++ = mix / STEPS;
		}
	}
	else
	{
		for (i = 0; i < length; i++)
			*dest++ = 0;
	}
}

void mooncrst_pitch_w(int offset,int data)
{
	stream_update(tone_stream,0);

	pitch = data;
}

void mooncrst_vol_w(int offset,int data)
{
	stream_update(tone_stream,0);

	/* offset 0 = bit 0, offset 1 = bit 1 */
	vol = (vol & ~(1 << offset)) | ((data & 1) << offset);
}



void mooncrst_noise_w(int offset,int data)
{
	if (deathsampleloaded)
	{
		if (data & 1 && !(LastPort1 & 1))
			mixer_play_sample(channelnoise,Machine->samples->sample[1]->data,
					Machine->samples->sample[1]->length,
					Machine->samples->sample[1]->smpfreq,
					0);
		LastPort1=data;
	}
	else
	{
		if (data & 1) mixer_set_volume(channelnoise,100);
		else mixer_set_volume(channelnoise,0);
	}
}

void mooncrst_shoot_w(int offset,int data)
{
	if (data & 1 && !(LastPort2 & 1) && shootsampleloaded)
		mixer_play_sample(channelshoot,Machine->samples->sample[0]->data,
				Machine->samples->sample[0]->length,
				Machine->samples->sample[0]->smpfreq,
				0);
	LastPort2=data;
}


int mooncrst_sh_start(const struct MachineSound *msound)
{
	int i;
	int lfovol[3] = {LFO_VOLUME,LFO_VOLUME,LFO_VOLUME};

	channelnoise = mixer_allocate_channel(NOISE_VOLUME);
	mixer_set_name(channelnoise,"Noise");
	channelshoot = mixer_allocate_channel(SHOOT_VOLUME);
	mixer_set_name(channelshoot,"Shoot");
	channellfo = mixer_allocate_channels(3,lfovol);
	mixer_set_name(channellfo+0,"Background #0");
	mixer_set_name(channellfo+1,"Background #1");
	mixer_set_name(channellfo+2,"Background #2");

	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)    /* We should check also that Samplename[0] = 0 */
	  shootsampleloaded = 1;
	else
	  shootsampleloaded = 0;

	if (Machine->samples != 0 && Machine->samples->sample[1] != 0)    /* We should check also that Samplename[0] = 0 */
	  deathsampleloaded = 1;
	else
	  deathsampleloaded = 0;

	if ((noise = malloc(NOISE_LENGTH)) == 0)
	{
		return 1;
	}

	for (i = 0;i < NOISE_LENGTH;i++)
		noise[i] = (rand() % (2*WAVE_AMPLITUDE)) - WAVE_AMPLITUDE;
	for (i = 0;i < TOOTHSAW_LENGTH;i++)
	{
		int bit0,bit2,bit3;

		bit0 = (i >> 0) & 1;
		bit2 = (i >> 2) & 1;
		bit3 = (i >> 3) & 1;

		/* relative weigths derived from the schematics. There are 4 possible */
		/* "volume" settings which also affect the pitch. */
		waveform1[0][i] = AUDIO_CONV(0x20 * bit0 + 0x30 * bit2);
		waveform1[1][i] = AUDIO_CONV(0x20 * bit0 + 0x99 * bit2);
		waveform1[2][i] = AUDIO_CONV(0x20 * bit0 + 0x30 * bit2 + 0x46 * bit3);
		waveform1[3][i] = AUDIO_CONV(0x20 * bit0 + 0x99 * bit2 + 0x46 * bit3);
	}

	pitch = 0;
	vol = 0;

	tone_stream = stream_init("Tone",TOOTHSAW_VOLUME,SOUND_CLOCK/STEPS,8,0,tone_update);

	if (!deathsampleloaded)
	{
		mixer_set_volume(channelnoise,0);
		mixer_play_sample(channelnoise,noise,NOISE_LENGTH,NOISE_RATE,1);
	}

	mixer_set_volume(channellfo+0,0);
	mixer_play_sample(channellfo+0,waveform2,32,1000,1);
	mixer_set_volume(channellfo+1,0);
	mixer_play_sample(channellfo+1,waveform2,32,1000,1);
	mixer_set_volume(channellfo+2,0);
	mixer_play_sample(channellfo+2,waveform2,32,1000,1);

	return 0;
}



void mooncrst_sh_stop(void)
{
	free(noise);
	osd_stop_sample(channelnoise);
	osd_stop_sample(channelshoot);
	osd_stop_sample(channellfo+0);
	osd_stop_sample(channellfo+1);
	osd_stop_sample(channellfo+2);
}

void mooncrst_background_w(int offset,int data)
{
	lfo_active[offset] = data & 1;

	if (lfo_active[offset]) mixer_set_volume(channellfo+offset,100);
	else mixer_set_volume(channellfo+offset,0);
}

void mooncrst_lfo_freq_w(int offset,int data)
{
	static int lforate[4];

	lforate[offset] = data & 1;
	lfo_rate = lforate[3]*8 + lforate[2]*4 + lforate[1]*2 + lforate[0];
	lfo_rate = 16 - lfo_rate;
}

void mooncrst_sh_update(void)
{
	osd_set_sample_freq(channellfo+0,freq*32);
	osd_set_sample_freq(channellfo+1,(freq*1040/760)*32);
	osd_set_sample_freq(channellfo+2,(freq*1040/540)*32);

	if (t == 0) freq -= lfo_rate;
	if (freq <= MINFREQ) freq = MAXFREQ;

	t = (t + 1) % 3;
}
