#include "driver.h"
#include "sndhrdw/generic.h"
#include "sn76496.h"
#include <math.h>



int tp84_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 512

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

int tp84_sh_interrupt(void)
{
	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}



//#define MIX		/* SN76496 sound mixing mode  */

#define SND_CLOCK 4000000	/* ?? */
#define CHIPS 3


#ifdef MIX
#define UPDATES_PER_SECOND 60
#define emulation_rate (350*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)
static char *sample;
#else
#define TONE_LENGTH 2000
#define TONE_PERIOD 4
#define NOISE_LENGTH 10000
#define WAVE_AMPLITUDE 70

static char *tone;
static char *noise;
#endif

static struct SN76496 sn[CHIPS];

void tp84_sound1_w(int offset,int data)
{
	SN76496Write(&sn[0],data);
}

void tp84_sound2_w(int offset,int data)
{
	SN76496Write(&sn[1],data);
}

void tp84_sound3_w(int offset,int data)
{
	SN76496Write(&sn[2],data);
}

void tp84_sound4_w(int offset,int data)
{
	SN76496Write(&sn[3],data);
}



int tp84_sh_start(void)
{
#ifdef MIX
	int j;

	if ((sample = malloc(buffer_len)) == 0)
		return 1;

	for (j = 0;j < CHIPS;j++)
	{
		sn[j].Clock = SND_CLOCK;
		SN76496Reset(&sn[j]);
	}
	return 0;
#else
	int i,j;

	if ((tone = malloc(TONE_LENGTH)) == 0)
		return 1;
	if ((noise = malloc(NOISE_LENGTH)) == 0)
	{
		free(tone);
		return 1;
	}

	for (i = 0;i < TONE_LENGTH;i++)
		tone[i] = WAVE_AMPLITUDE * sin(2*PI*i/TONE_PERIOD);
	for (i = 0;i < NOISE_LENGTH;i++)
		noise[i] = (rand() % (2*WAVE_AMPLITUDE)) - WAVE_AMPLITUDE;

	for (j = 0;j < CHIPS;j++)
	{
		sn[j].Clock = SND_CLOCK;
		SN76496Reset(&sn[j]);

		for (i = 0;i < 3;i++)
			osd_play_sample(4*j+i,tone,TONE_LENGTH,TONE_PERIOD * sn[j].Frequency[i],sn[j].Volume[i],1);

		osd_play_sample(4*j+3,noise,NOISE_LENGTH,sn[j].NoiseShiftRate,sn[j].Volume[3],1);
	}

	return 0;
#endif
}



void tp84_sh_stop(void)
{
#ifdef MIX
	free(sample);
#else
	free(noise);
	free(tone);
#endif
}

void tp84_sh_update(void)
{
#ifdef MIX
	int i;

	if (play_sound == 0) return;
	for (i = 0;i < CHIPS;i++)
	{
		SN76496UpdateB(&sn[i] , emulation_rate , sample , buffer_len );
		osd_play_streamed_sample(i,sample,buffer_len,emulation_rate,255 );
	}
#else
	int i,j;

	if (play_sound == 0) return;

	for (j = 0;j < CHIPS;j++)
	{
		SN76496Update(&sn[j]);

		for (i = 0;i < 3;i++)
			osd_adjust_sample(4*j+i,TONE_PERIOD * sn[j].Frequency[i],sn[j].Volume[i]);

		osd_adjust_sample(4*j+3,sn[j].NoiseShiftRate,sn[j].Volume[3]);
	}
#endif
}
