/***************************************************************************
	pong.c
	Sound handler

	J. Buchmueller, November '99
****************************************************************************/

#include "driver.h"
#include "vidhrdw/pong.h"

/* HJB 99/11/22 corrected HIT_CLOCK and VBLANK_CLOCK */
#define HIT_CLOCK		(PONG_MAX_V-PONG_VBLANK) * PONG_FPS / 16 / 2
#define VBLANK_CLOCK	(PONG_MAX_V-PONG_VBLANK) * PONG_FPS / 16 / 4
#define SCORE_CLOCK 	PONG_MAX_V * PONG_FPS / 32

static	int channel;
static	signed char waveform[] = { -120, -120, 120, 120 };

int pong_hit_sound = 0;
int pong_vblank_sound = 0;
int pong_score_sound = 0;

/************************************/
/* Sound handler start				*/
/************************************/
int pong_sh_start(const struct MachineSound *msound)
{
	int vol[3];

	vol[0]= vol[1]= vol[2]= 20;
	channel = mixer_allocate_channels(3,vol);

	mixer_play_sample(channel,waveform,sizeof(waveform),sizeof(waveform)*HIT_CLOCK,1);
    mixer_set_volume(channel,0);
	mixer_play_sample(channel+1,waveform,sizeof(waveform),sizeof(waveform)*VBLANK_CLOCK,1);
    mixer_set_volume(channel+1,0);
	mixer_play_sample(channel+2,waveform,sizeof(waveform),sizeof(waveform)*SCORE_CLOCK,1);
    mixer_set_volume(channel+2,0);

    return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void pong_sh_stop(void)
{
	mixer_stop_sample(channel);
    mixer_stop_sample(channel+1);
	mixer_stop_sample(channel+2);
}

/************************************/
/* Sound handler update 			*/
/************************************/
void pong_sh_update(void)
{
	mixer_set_volume(channel,pong_hit_sound ? 100 : 0);
	mixer_set_volume(channel+1,pong_vblank_sound ? 100 : 0);
	mixer_set_volume(channel+2,pong_score_sound ? 100 : 0);
}
