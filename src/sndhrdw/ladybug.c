#include "driver.h"
#include "sn76496.h"
#include <math.h>


#define SND_CLOCK 4000000	/* 4 Mhz */
#define CHIPS 2


#define TONE_LENGTH 2000
#define TONE_PERIOD 4
#define NOISE_LENGTH 10000
#define WAVE_AMPLITUDE 70


static char *tone;
static char *noise;

static struct SN76496 sn[CHIPS];



void ladybug_sound1_w(int offset,int data)
{
	SN76496Write(&sn[0],data);
}

void ladybug_sound2_w(int offset,int data)
{
	SN76496Write(&sn[1],data);
}



int ladybug_sh_start(void)
{
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
}



void ladybug_sh_stop(void)
{
	free(noise);
	free(tone);
}



void ladybug_sh_update(void)
{
	int i,j;


	if (play_sound == 0) return;

	for (j = 0;j < CHIPS;j++)
	{
		SN76496Update(&sn[j]);

		for (i = 0;i < 3;i++)
			osd_adjust_sample(4*j+i,TONE_PERIOD * sn[j].Frequency[i],sn[j].Volume[i]);

		osd_adjust_sample(4*j+3,sn[j].NoiseShiftRate,sn[j].Volume[3]);
	}
}
