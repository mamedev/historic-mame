/***************************************************************************
	pong.c
	Sound handler

	J. Buchmueller, November '99
****************************************************************************/

#include "driver.h"

#define HIT_CLOCK		(256+4+1+1-16) * 60 / 16 / 1
#define VBLANK_CLOCK	(256+4+1+1-16) * 60 / 16 / 2
#define SCORE_CLOCK 	(256+4+1+1) * 60 / 32

static	int channel;
static	signed char waveform[2] = { -120, 120 };

int pong_hit_sound = 0;
int pong_vblank_sound = 0;
int pong_score_sound = 0;

/************************************/
/* Sound handler start				*/
/************************************/
int pong_sh_start(const struct MachineSound *msound)
{
	int vol[3];

	vol[0]= vol[1]= vol[2]= 255;
	channel = mixer_allocate_channels(3,vol);

    mixer_set_volume(channel,0);
	mixer_play_sample(channel,waveform,sizeof(waveform),sizeof(waveform)*HIT_CLOCK,1);
	mixer_set_volume(channel+1,0);
	mixer_play_sample(channel+1,waveform,sizeof(waveform),sizeof(waveform)*VBLANK_CLOCK,1);
	mixer_set_volume(channel+2,0);
	mixer_play_sample(channel+2,waveform,sizeof(waveform),sizeof(waveform)*SCORE_CLOCK,1);

    return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void pong_sh_stop(void)
{
	osd_stop_sample(channel);
    osd_stop_sample(channel+1);
	osd_stop_sample(channel+2);
}

/************************************/
/* Sound handler update 			*/
/************************************/
void pong_sh_update(void)
{
	mixer_set_volume(channel,pong_hit_sound ? 33 : 0);
	mixer_set_volume(channel+1,pong_vblank_sound ? 33 : 0);
	mixer_set_volume(channel+2,pong_score_sound ? 33 : 0);
}
