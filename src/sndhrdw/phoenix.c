#include "driver.h"


/* I played around with these until they sounded ok */
static max_freq = 22500;
static dec_freq = 1500;

/* lfo - low fequency oscillator for the 'screaming' birds! (voice a only)*/
static int lfo_adjust_count = 0;
static int lfo_sine[4] = { 250, 0, -250, 0 };

/* values needed by phoenix_sh_update */
static int sound_a_play = 0;
static int sound_a_vol = 0;
static int sound_a_freq = 1000;


void phoenix_sound_control_a_w(int offset,int data)
{
        /* voice a */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* melody - osd_play_midi anyone? */
        //int tune = (data & 0xc0) >> 6;

        if (freq && freq != 0x0f)
        {
                osd_adjust_sample(0,max_freq-(dec_freq*freq)-lfo_sine[lfo_adjust_count],85*(3-vol));
                sound_a_play = 1;
        }
        else
        {
                osd_adjust_sample(0,1000,0);
                sound_a_play = 0;
        }

        sound_a_freq = freq;
        sound_a_vol = vol;
}



void phoenix_sound_control_b_w(int offset,int data)
{
        /* voice b */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* noise volume - I have no idea how to implement this! */
        //int noise = (data & 0xc0) >> 6;

        if (freq && freq != 0x0f)
                osd_adjust_sample(1,max_freq-(dec_freq*freq),85*(3-vol));
        else
                osd_adjust_sample(1,1000,0);
}



int phoenix_sh_start(void)
{
        osd_play_sample(0,Machine->drv->samples,32,1000,0,1);
        osd_play_sample(1,Machine->drv->samples,32,1000,0,1);
	return 0;
}



void phoenix_sh_update(void)
{
        if (lfo_adjust_count == 4) lfo_adjust_count = 0;

        if (sound_a_play)
                osd_adjust_sample(0,max_freq-(dec_freq*sound_a_freq)-lfo_sine[lfo_adjust_count],85*(3-sound_a_vol));

        lfo_adjust_count++;
}
