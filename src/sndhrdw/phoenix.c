#include "driver.h"



/* I played around with these until they sounded ok */
static max_freq = 22500;
static dec_freq = 1500;

/* values needed by phoenix_sh_update */
static int sound_a_play = 0;
static int sound_a_adjust = -750;
static int sound_a_sine[4] = { 0, 250, 0, -250 };
static int sound_a_sine_adjust = 0;
static int sound_a_vol = 0;
static int sound_a_freq = 1000;

static int sound_b_play = 0;
static int sound_b_adjust = -750;
static int sound_b_vol = 0;
static int sound_b_freq = 1000;

static int noise_vol = 0;
static int noise_freq = 1000;



void phoenix_sound_control_a_w(int offset,int data)
{
        /* voice a */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* noise */
        int noise = (data & 0xc0) >> 6;

        if (freq != sound_a_freq) sound_a_adjust = 0;
        sound_a_freq = freq;
        sound_a_vol = vol;

        if (freq != 0x0f)
        {
                osd_adjust_sample(0,max_freq-(dec_freq*freq),85*(3-vol));
                sound_a_play = 1;
        }
        else
        {
                osd_adjust_sample(0,1000,0);
                sound_a_play = 0;
        }

        if (noise_freq != 1500*(4-noise))
        {
                noise_freq = 1500*(4-noise);
                noise_vol = 85*noise;
        }

        if (noise) osd_adjust_sample(2,noise_freq,noise_vol);
        else
        {
                osd_adjust_sample(2,1000,0);
                noise_vol = 0;
        }
}



void phoenix_sound_control_b_w(int offset,int data)
{
        /* voice b */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* melody - osd_play_midi anyone? */
        /* 0 - no tune, 1 - alarm beep?, 2 - even level tune, 3 - odd level tune */
        /*int tune = (data & 0xc0) >> 6;*/

        if (freq != sound_b_freq) sound_b_adjust = 0;
        sound_b_freq = freq;
        sound_b_vol = vol;

        if (freq != 0x0f)
        {
                osd_adjust_sample(1,max_freq-(dec_freq*freq),85*(3-vol));
                sound_b_play = 1;
        }
        else
        {
                osd_adjust_sample(1,1000,0);
                sound_b_play = 0;
        }
}



int phoenix_sh_start(void)
{
        osd_play_sample(0,Machine->drv->samples,32,1000,0,1);
        osd_play_sample(1,Machine->drv->samples,32,1000,0,1);
        osd_play_sample(2,&Machine->drv->samples[32],128,1000,0,1);
	return 0;
}



void phoenix_sh_update(void)
{
        sound_a_adjust++;
        sound_b_adjust++;
        sound_a_sine_adjust++;

        if (sound_a_adjust == 750) sound_a_adjust = -750;
        if (sound_b_adjust == 750) sound_b_adjust = -750;
        if (sound_a_sine_adjust == 4) sound_a_sine_adjust = 0;

        if (sound_a_play)
                osd_adjust_sample(0,max_freq-(dec_freq*sound_a_freq)-sound_a_adjust-sound_a_sine[sound_a_sine_adjust],85*(3-sound_a_vol));
        if (sound_b_play)
                osd_adjust_sample(1,max_freq-(dec_freq*sound_b_freq)-sound_b_adjust,85*(3-sound_b_vol));
        if (noise_vol)
        {
                osd_adjust_sample(2,noise_freq,noise_vol);
                noise_vol-=3;
        }
}
