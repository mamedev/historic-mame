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
static int portBstatus;

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
		osd_adjust_sample (0, MAXFREQ_A/2/(16-freq), VOLUME_A);
	}
	else
	{
		osd_adjust_sample (0,SAFREQ,0);
	}

	if (noisemulate)
	{
		if (noise_freq != 1750*(4-noise))
		{
			noise_freq = 1750*(4-noise);
			noise_vol = 85*noise;
		}

		if (noise) osd_adjust_sample (3,noise_freq,noise_vol/4);
		else
		{
			osd_adjust_sample (3,1000,0);
			noise_vol = 0;
		}
	}
	else
	{
		switch (noise)
		{
			case 1 :
				if (lastnoise != noise)
					osd_play_sample(3,Machine->samples->sample[0]->data,
						Machine->samples->sample[0]->length,
						Machine->samples->sample[0]->smpfreq,
						Machine->samples->sample[0]->volume,0);
				break;
			case 2 :
		 		if (lastnoise != noise)
					osd_play_sample(3,Machine->samples->sample[1]->data,
						Machine->samples->sample[1]->length,
						Machine->samples->sample[1]->smpfreq,
						Machine->samples->sample[1]->volume,0);
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
		osd_adjust_sample (portBstatus + 1, (1<<pitch) * TMS3615_freq[freq], VOLUME_B);
	}
	else
	{
		osd_adjust_sample (portBstatus + 1, SBFREQ, 0);
	}
	portBstatus ^= 0x01;
	if (errorlog) fprintf(errorlog,"B:%X freq: %02x vol: %02x\n",data, data & 0x0f, (data & 0x30) >> 4);
}



int pleiads_sh_start (void)
{
	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)
		noisemulate = 0;
	else
		noisemulate = 1;

	/* Clear all the variables */
	{
		noise_vol = 0;
		noise_freq = 1000;
		portBstatus = 0;
	}

	osd_play_sample (0,(signed char *)waveform0,2,1000,0,1);
	osd_play_sample (1,(signed char *)waveform1,32,1000,0,1);
	osd_play_sample (2,(signed char *)waveform1,32,1000,0,1);
	osd_play_sample (3,(signed char *)waveform2,128,1000,0,1);

	return 0;
}



void pleiads_sh_update (void)
{
	if ((noise_vol) && (noisemulate))
	{
		osd_adjust_sample (3,noise_freq,noise_vol/4);
		noise_vol-=3;
	}
}

