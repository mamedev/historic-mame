#include "driver.h"


unsigned char *mappy_soundregs;
static int sound_enable;
static int sound_changed;


void mappy_sound_enable_w(int offset,int data)
{
	sound_enable = offset;
	if (sound_enable == 0)
	{
		osd_adjust_sample(0,1000,0);
		osd_adjust_sample(1,1000,0);
		osd_adjust_sample(2,1000,0);
		osd_adjust_sample(3,1000,0);
		osd_adjust_sample(4,1000,0);
		osd_adjust_sample(5,1000,0);
		osd_adjust_sample(6,1000,0);
		osd_adjust_sample(7,1000,0);
	}
}


void mappy_sound_w(int offset,int data)
{
	if (mappy_soundregs[offset] != data)
	{
		sound_changed = 1;
		mappy_soundregs[offset] = data;
	}
}


void mappy_sh_update(void)
{
	if (play_sound == 0) return;

	if (sound_enable && sound_changed)
	{
		int voice;
		static int currwave[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };

		sound_changed = 0;

		for (voice = 0;voice < 8;voice++)
		{
			int freq,volume,wave;

			freq = mappy_soundregs[0x06 + 8 * voice] & 15;	/* high bits are from here */
			freq = freq * 256 + mappy_soundregs[0x05 + 8 * voice];
			freq = freq * 256 + mappy_soundregs[0x04 + 8 * voice];
			freq = freq * 730 / 1000;	/* thanks to jrok for this */

			volume = mappy_soundregs[0x03 + 8 * voice];
			volume = (volume << 4) | volume;

			if (freq == 0)
			{
				freq = 1000;
				volume = 0;
			}

			wave = (mappy_soundregs[0x06 + 8 * voice] >> 4) & 7;
			if (wave != currwave[voice])
			{
				osd_play_sample(voice,&Machine->drv->samples[wave * 32],32,freq,volume,1);
				currwave[voice] = wave;
			}
			else 
				osd_adjust_sample(voice,freq,volume);
		}
	}
}
