/* some of the missing sounds are now audible,
   but there is no modulation of any kind yet.
   Andrew Scott (ascott@utkux.utcc.utk.edu) */

#include "driver.h"
#include <math.h>

#define SAFREQ  1400
#define SBFREQ  1400
#define MAXFREQ_A 44100*7
#define MAXFREQ_B1 44100*4
#define MAXFREQ_B2 44100*4

/* for voice A effects */
#define SW_INTERVAL 4
#define MOD_RATE 0.14
#define MOD_DEPTH 0.1

/* for voice B effect */
#define SWEEP_RATE 0.14
#define SWEEP_DEPTH 0.24

/* values needed by phoenix_sh_update */
static int sound_a_play = 0;
static int sound_a_vol = 0;
static int sound_a_freq = SAFREQ;
static int sound_a_adjust=1;
static double x;

static int sound_b1_play = 0;
static int sound_b1_vol = 0;
static int sound_b1_freq = SBFREQ;
static int sound_b2_play = 0;
static int sound_b2_vol = 0;
static int sound_b2_freq = SBFREQ;
static int sound_b_adjust=1;

static int noise_vol = 0;
static int noise_freq = 1000;
static int pitch_a = 0;
static int pitch_b1 = 0;
static int pitch_b2 = 0;

static int noisemulate=0;
static int portBstatus=0;



/* waveforms for the audio hardware */
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


int pleiads_sh_init(const char *gamename)
{
	x = PI/2;

        if (Machine->samples != 0 && Machine->samples->sample[0] != 0)    /* We should check also that Samplename[0] = 0 */
          noisemulate = 0;
        else
          noisemulate = 1;

        return 0;
}


void pleiads_sound_control_a_w(int offset,int data)
{
        static int lastnoise;

        /* voice a */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* noise */
        int noise = (data & 0xc0) >> 6;

        if (freq != sound_a_freq) sound_a_adjust = 1;
        else sound_a_adjust=0;

        sound_a_freq = freq;
        sound_a_vol = vol;

        if (freq != 0x0f)
        {
                osd_adjust_sample(0,MAXFREQ_A/(16-sound_a_freq),85*(3-vol));
                sound_a_play = 1;
        }
        else
        {
                osd_adjust_sample(0,SAFREQ,0);
                sound_a_play = 0;
        }

        if (noisemulate) {
           if (noise_freq != 1750*(4-noise))
           {
                   noise_freq = 1750*(4-noise);
                   noise_vol = 85*noise;
           }

           if (noise) osd_adjust_sample(3,noise_freq,noise_vol);
           else
           {
                   osd_adjust_sample(3,1000,0);
                   noise_vol = 0;
           }
         }
        else
         {
           switch (noise) {
             case 1 : if (lastnoise != noise)
                        osd_play_sample(3,Machine->samples->sample[0]->data,
                                          Machine->samples->sample[0]->length,
                                          Machine->samples->sample[0]->smpfreq,
                                          Machine->samples->sample[0]->volume,0);
                      break;
             case 2 : if (lastnoise != noise)
                        osd_play_sample(3,Machine->samples->sample[1]->data,
                                          Machine->samples->sample[1]->length,
                                          Machine->samples->sample[1]->smpfreq,
                                          Machine->samples->sample[1]->volume,0);
                      break;
           }
           lastnoise = noise;
         }
        if (errorlog) fprintf(errorlog,"A:%X ",data);
}



void pleiads_sound_control_b_w(int offset,int data)
{
    if (portBstatus==0) {
        /* voice b1 */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* melody - osd_play_midi anyone? */
        /* 0 - no tune, 1 - alarm beep?, 2 - even level tune, 3 - odd level tune */
        /*int tune = (data & 0xc0) >> 6;*/

        if (freq != sound_b1_freq) sound_b_adjust = 1;
        else sound_b_adjust=0;

        sound_b1_freq = freq;
        sound_b1_vol = vol;

        if (freq != 0x0f)
        {
                osd_adjust_sample(1,MAXFREQ_B1/(16-sound_b1_freq),85*(3-vol));
                sound_b1_play = 1;
        }
        else
        {
                osd_adjust_sample(1,SBFREQ,0);
                sound_b1_play = 0;
        }
   }

else {
        /* voice b2 */
        int freq = data & 0x0f;
        int vol = (data & 0x30) >> 4;

        /* melody - osd_play_midi anyone? */
        /* 0 - no tune, 1 - alarm beep?, 2 - even level tune, 3 - odd level tune */
        /*int tune = (data & 0xc0) >> 6;*/

        if (freq != sound_b2_freq) sound_b_adjust = 1;
        else sound_b_adjust=0;

        sound_b2_freq = freq;
        sound_b2_vol = vol;

        if (freq != 0x0f)
        {
                osd_adjust_sample(2,MAXFREQ_B2/(16-sound_b2_freq),85*(3-vol));
                sound_b2_play = 1;
        }
        else
        {
                osd_adjust_sample(2,SBFREQ,0);
                sound_b2_play = 0;
        }
   }

     portBstatus=!portBstatus;
        if (errorlog) fprintf(errorlog,"B:%X ",data);
}



int pleiads_sh_start(void)
{
        osd_play_sample(0,waveform1,32,1000,0,1);
        osd_play_sample(1,waveform1,32,1000,0,1);
        osd_play_sample(2,waveform1,32,1000,0,1);
        osd_play_sample(3,waveform2,128,1000,0,1);
	return 0;
}



void pleiads_sh_update(void)
{
        pitch_a=MAXFREQ_A/(16-sound_a_freq);
        pitch_b1=MAXFREQ_B1/(16-sound_b1_freq);
        pitch_b2=MAXFREQ_B2/(16-sound_b2_freq);
/*
        if (hifreq)
            pitch_a=pitch_a*5/4;

        pitch_a+=((double)pitch_a*MOD_DEPTH*sin(t));

        sound_a_sw++;

        if (sound_a_sw==SW_INTERVAL)
        {
            hifreq=!hifreq;
            sound_a_sw=0;
        }

        t+=MOD_RATE;

        if (t>2*PI)
            t=0;

        pitch_b+=((double)pitch_b*SWEEP_DEPTH*sin(x));

if (sound_b_vol==3 || (last_b_freq==15&&sound_b_freq==12) || (last_b_freq!=14&&sound_b_freq==6))
            x=0;

        x+=SWEEP_RATE;

        if (x>3*PI/2)
            x=3*PI/2;

  */

        if (sound_a_play)
                osd_adjust_sample(0,pitch_a,85*(3-sound_a_vol));
        if (sound_b1_play)
                osd_adjust_sample(1,pitch_b1,85*(3-sound_b1_vol));
        if (sound_b2_play)
                osd_adjust_sample(2,pitch_b2,85*(3-sound_b2_vol));

        if ((noise_vol) && (noisemulate))
        {
                osd_adjust_sample(3,noise_freq,noise_vol);
                noise_vol-=3;
        }

}

