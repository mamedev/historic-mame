#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "osdepend.h"


#define SND_CLOCK 3072000	/* 3.072 Mhz */


static unsigned char soundregisters[0x20];
static int sound_enable;
static int sound_changed;



void pengo_sound_enable_w(int address,int offset,int data)
{
	sound_enable = data;
	if (sound_enable == 0)
	{
		osd_stop_sample(0);
		osd_stop_sample(1);
		osd_stop_sample(2);
	}
}



void pengo_sound_w(int address,int offset,int data)
{
	data &= 0x0f;

	if (soundregisters[offset] != data)
	{
		sound_changed = 1;
		soundregisters[offset] = data;
	}
}



void pengo_sh_update(void)
{
	if (play_sound == 0) return;

	if (sound_enable && sound_changed)
	{
		int voice;
		static int currwave[3] = { -1,-1,-1 };


		sound_changed = 0;

		for (voice = 0;voice < 3;voice++)
		{
			int freq,volume,wave;


			freq = soundregisters[0x14 + 5 * voice];	/* always 0 */
			freq = freq * 16 + soundregisters[0x13 + 5 * voice];
			freq = freq * 16 + soundregisters[0x12 + 5 * voice];
			freq = freq * 16 + soundregisters[0x11 + 5 * voice];
			if (voice == 0)
				freq = freq * 16 + soundregisters[0x10 + 5 * voice];
			else freq = freq * 16;

			freq = (SND_CLOCK / 2048) * freq / 512;
			volume = soundregisters[0x15 + 5 * voice];
			volume = (volume << 4) | volume;

			wave = soundregisters[0x05 + 5 * voice] & 7;
			if (wave != currwave[voice])
			{
				osd_play_sample(voice,&Machine->drv->samples[wave * 32],32,freq,volume,1);
				currwave[voice] = wave;
			}
			else osd_adjust_sample(voice,freq,volume);
		}
	}
}
