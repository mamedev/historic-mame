/* Sound hardware for Pleiades, Naughty Boy and Pop Flamer. Note that it's very */
/* similar to Phoenix. */

/* some of the missing sounds are now audible,
   but there is no modulation of any kind yet.
   Andrew Scott (ascott@utkux.utcc.utk.edu) */

#include "driver.h"

#define SAFREQ  1400
#define SBFREQ  1400
#define MAXFREQ_A 40000

#define VOLUME_A 20
#define VOLUME_B 80
#define SAMPLE_VOLUME 25

/* for voice A effects */
#define SW_INTERVAL 4
#define MOD_RATE 0.14
#define MOD_DEPTH 0.1

/* for voice B effect */
#define SWEEP_RATE 0.14
#define SWEEP_DEPTH 0.24

static int noise_vol;
static int noise_freq;

static int noisemulate;

static int channel0,channel1,channel2,channel3,channel23;

/* coin-up */
static void *timer_a;
static void *timer_b;
void timer_a_callback(int param);
void timer_b_callback(int param);


/* waveforms for the audio hardware */
static signed char waveform0[2] =
{
	/* flip-flop */
	-128, 127
};
static unsigned char waveform1[32] =
{
	/* sine-wave */
	0x0F, 0x0F, 0x0F, 0x06, 0x06, 0x09, 0x09, 0x06, 0x06, 0x09, 0x06, 0x0D, 0x0F, 0x0F, 0x0D, 0x00,
	0xE6, 0xDE, 0xE1, 0xE6, 0xEC, 0xE6, 0xE7, 0xE7, 0xE7, 0xEC, 0xEC, 0xEC, 0xE7, 0xE1, 0xE1, 0xE7,
};
static unsigned char waveform2[] =
{
	/* white-noise ? */
	0x79, 0x75, 0x71, 0x72, 0x72, 0x6F, 0x70, 0x71, 0x71, 0x73, 0x75, 0x76, 0x74, 0x74, 0x78, 0x7A,
	0x79, 0x7A, 0x7B, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7D, 0x80, 0x85, 0x88, 0x88, 0x87,
	0x8B, 0x8B, 0x8A, 0x8A, 0x89, 0x87, 0x85, 0x87, 0x89, 0x86, 0x83, 0x84, 0x84, 0x85, 0x84, 0x84,
	0x85, 0x86, 0x87, 0x87, 0x88, 0x88, 0x86, 0x81, 0x7E, 0x7D, 0x7F, 0x7D, 0x7C, 0x7D, 0x7D, 0x7C,
	0x7E, 0x81, 0x7F, 0x7C, 0x7E, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x84, 0x83, 0x82, 0x82, 0x83,
	0x82, 0x84, 0x88, 0x8C, 0x8E, 0x8B, 0x8B, 0x8C, 0x8A, 0x8A, 0x8A, 0x89, 0x85, 0x86, 0x89, 0x89,
	0x86, 0x85, 0x85, 0x85, 0x84, 0x83, 0x82, 0x83, 0x83, 0x83, 0x82, 0x83, 0x83
};



void pleiads_sound_control_a_w (int offset,int data)
{
	static int lastnoise;

	/* voice a */
	int freq = data & 0x0f;
	/* (data & 0x10), (data & 0x20), other analog stuff which I don't understand */

	/* noise */
	int noise = (data & 0xc0) >> 6;

	if (freq != 0x0f)
	{
		mixer_set_sample_frequency(channel0,MAXFREQ_A/2/(16-freq));
		mixer_set_volume(channel0,VOLUME_A);
	}
	else
	{
		mixer_set_volume(channel0,0);
	}

	if (noisemulate)
	{
		if (noise_freq != 1750*(4-noise))
		{
			noise_freq = 1750*(4-noise);
			noise_vol = 85*noise;
		}

		if (noise)
		{
			mixer_set_sample_frequency(channel1,noise_freq);
			mixer_set_volume(channel1,noise_vol*100/255);
		}
		else
		{
			mixer_set_volume(channel1,0);
			noise_vol = 0;
		}
	}
	else
	{
		switch (noise)
		{
			case 1 :
				if (lastnoise != noise)
					mixer_play_sample(channel1,Machine->samples->sample[0]->data,
						Machine->samples->sample[0]->length,
						Machine->samples->sample[0]->smpfreq,
						0);
				break;
			case 2 :
		 		if (lastnoise != noise)
					mixer_play_sample(channel1,Machine->samples->sample[1]->data,
						Machine->samples->sample[1]->length,
						Machine->samples->sample[1]->smpfreq,
						0);
				break;
		}
		lastnoise = noise;
	}
/*	if (errorlog) fprintf(errorlog,"A:%X \n",data);*/
}



#define BASE_FREQ       246.9416506
#define PITCH_1         BASE_FREQ * 1.0			/* B  */
#define PITCH_2         BASE_FREQ * 1.059463094		/* C  */
#define PITCH_3         BASE_FREQ * 1.122462048		/* C# */
#define PITCH_4         BASE_FREQ * 1.189207115		/* D  */
#define PITCH_5         BASE_FREQ * 1.259921050		/* D# */
#define PITCH_6         BASE_FREQ * 1.334839854		/* E  */
#define PITCH_7         BASE_FREQ * 1.414213562		/* F  */
#define PITCH_8         BASE_FREQ * 1.498307077		/* F# */
#define PITCH_9         BASE_FREQ * 1.587401052		/* G  */
#define PITCH_10        BASE_FREQ * 1.681792830		/* G# */
#define PITCH_11        BASE_FREQ * 1.781797436		/* A = 440 */
#define PITCH_12        BASE_FREQ * 1.887748625		/* A# */
#define PITCH_13        BASE_FREQ * 2.0			/* B  */

#define SHIFT_1 8*1.0

static int TMS3615_freq[] =
{
	PITCH_1*SHIFT_1, PITCH_2*SHIFT_1, PITCH_3*SHIFT_1, PITCH_4*SHIFT_1,
	PITCH_5*SHIFT_1, PITCH_6*SHIFT_1, PITCH_7*SHIFT_1, PITCH_8*SHIFT_1,
	PITCH_9*SHIFT_1, PITCH_10*SHIFT_1, PITCH_11*SHIFT_1, PITCH_12*SHIFT_1,
	PITCH_13*SHIFT_1, 0, 0, 0
};


void pleiads_sound_control_b_w (int offset,int data)
{
	static int portBstatus;
	/* voice b1 & b2 */
	int freq = data & 0x0f;
	int pitch = (data & 0xc0) >> 6;
	/* pitch selects one of 4 possible clock inputs (actually 3, because */
	/* IC2 and IC3 are tied together) */
	if (pitch == 3) pitch = 2;

	/* (data & 0x30) goes to a 556 */

	/* freq == 0x0d and 0x0e do nothing, 0x0f does something different (SAST BIAS?) */
	if (freq <= 0x0c)
	{
		mixer_set_sample_frequency(channel23+portBstatus, (1<<pitch) * TMS3615_freq[freq]);
		mixer_set_volume(channel23+portBstatus,VOLUME_B);
	}
	else
	{
		mixer_set_volume(channel23+portBstatus,0);
	}
	portBstatus ^= 0x01;
	if (errorlog) fprintf(errorlog,"B:%X freq: %02x vol: %02x\n",data, data & 0x0f, (data & 0x30) >> 4);
}


static const char *phoenix_sample_names[] =
{
	"*phoenix",
	"shot8.wav",
	"death8.wav",
	"phoenix1.wav",
	"phoenix2.wav",
	0	/* end of array */
};

int pleiads_sh_start(const struct MachineSound *msound)
{
	int vol[2];

	Machine->samples = readsamples(phoenix_sample_names,Machine->gamedrv->name);

	channel0 = mixer_allocate_channel(VOLUME_A);
	channel1 = mixer_allocate_channel(SAMPLE_VOLUME);
	channel2 = mixer_allocate_channel(5);
	channel3 = mixer_allocate_channel(5);
	vol[0]=vol[1]=VOLUME_B;
	channel23 = mixer_allocate_channels(2,vol);

	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)
		noisemulate = 0;
	else
	{
		noisemulate = 1;
		mixer_set_volume(channel1,0);
		mixer_play_sample(channel1,(signed char *)waveform2,128,1000,1);
	}

	/* Clear all the variables */
	{
		noise_vol = 0;
		noise_freq = 1000;
	}

	mixer_set_volume(channel0,0);
        mixer_play_sample(channel0,waveform0,2,1000,1);
	mixer_set_volume(channel23+0,0);
        mixer_play_sample(channel23+0,(signed char *)waveform1,32,1000,1);
	mixer_set_volume(channel23+1,0);
	mixer_play_sample(channel23+1,(signed char *)waveform1,32,1000,1);

	return 0;
}



void pleiads_sh_update (void)
{
	if ((noise_vol) && (noisemulate))
	{
		mixer_set_volume(channel1,noise_vol*100/255);
		noise_vol-=3;
	}
        if (readinputport(2) & 1)
		{
                mixer_set_volume(channel2,100);
                mixer_play_sample(channel2,waveform0,2,1000,1);
                mixer_set_volume(channel2+1,100);
                mixer_play_sample(channel2+1,waveform0,2,2000,1);

                timer_a = timer_set(TIME_IN_SEC(.2),0, timer_a_callback);
                }
}

void timer_a_callback (int param)
{
        mixer_set_volume(channel2,0);
        mixer_set_volume(channel3,100);
        mixer_play_sample(channel3,waveform0,2,4000,1);

        timer_b = timer_set(TIME_IN_SEC(.08),0, timer_b_callback);
}

void timer_b_callback (int param)
{
        mixer_set_volume(channel3,0);
}
