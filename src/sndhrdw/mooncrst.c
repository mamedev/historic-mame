#include "driver.h"
#include <math.h>


#define SOUND_CLOCK 1536000 /* 1.536 Mhz */

#define TONE_LENGTH 2000
#define TONE_PERIOD 4
#define NOISE_LENGTH 10000
#define NOISE_RATE 15000
#define WAVE_AMPLITUDE 70


static char *tone;
static char *noise;



void mooncrst_sound_freq_w(int offset,int data)
{
	if (data && data != 0xff) osd_adjust_sample(0,(SOUND_CLOCK/16)/(256-data)*TONE_PERIOD,255);
	else osd_adjust_sample(0,1000,0);
}



void mooncrst_noise_w(int offset,int data)
{
	if (data & 1) osd_adjust_sample(1,NOISE_RATE,255);
	else osd_adjust_sample(1,NOISE_RATE,0);
}



int mooncrst_sh_start(void)
{
	int i;


	if ((tone = malloc(TONE_LENGTH)) == 0)
		return 1;
	if ((noise = malloc(NOISE_LENGTH)) == 0)
	{
		free(tone);
		return 1;
	}

	for (i = 0;i < NOISE_LENGTH;i++)
		noise[i] = (rand() % (2*WAVE_AMPLITUDE)) - WAVE_AMPLITUDE;
	for (i = 0;i < TONE_LENGTH;i++)
		tone[i] = WAVE_AMPLITUDE * sin(2*PI*i/TONE_PERIOD);

	osd_play_sample(0,tone,TONE_LENGTH,1000,0,1);
	osd_play_sample(1,noise,NOISE_LENGTH,NOISE_RATE,0,1);

	return 0;
}



void mooncrst_sh_stop(void)
{
	free(noise);
	free(tone);
}



void mooncrst_sh_update(void)
{
}
