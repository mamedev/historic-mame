#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"


#define SND_CLOCK 4000000	/* 4 Mhz */


static int sound_changed;
static int freqlo[6],freqhi[6],vol[6];



static void sound_w(int voice,int data)
{
	static int lasttone[2];


	sound_changed = 1;

	if (data & 0x80)
	{
		switch ((data >> 4)& 0x07)
		{
			case 0x0:
				freqlo[3*voice+0] = data & 0x0f;
				lasttone[voice] = 0;
				break;
			case 0x1:
				vol[3*voice+0] = 0x0f - (data & 0x0f);
				break;
			case 0x2:
				freqlo[3*voice+1] = data & 0x0f;
				lasttone[voice] = 1;
				break;
			case 0x3:
				vol[3*voice+1] = 0x0f - (data & 0x0f);
				break;
			case 0x4:
				freqlo[3*voice+2] = data & 0x0f;
				lasttone[voice] = 2;
				break;
			case 0x5:
				vol[3*voice+2] = 0x0f - (data & 0x0f);
				break;
			case 0x6:
				break;
			case 0x7:
				break;
		}
	}
	else freqhi[3*voice+lasttone[voice]] = data & 0x3f;
}



void ladybug_sound1_w(int address,int offset,int data)
{
	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - write output port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	sound_w(0,data);
}



void ladybug_sound2_w(int address,int offset,int data)
{
	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - write output port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	sound_w(1,data);
}



int ladybug_sh_start(void)
{
	int i;


	for (i = 0;i < 6;i++)
		osd_play_sample(i,Machine->drv->samples,32,1000,0,1);
	return 0;
}



void ladybug_sh_update(void)
{
	if (play_sound == 0) return;

	if (sound_changed)
	{
		int voice;


		sound_changed = 0;

		for (voice = 0;voice < 6;voice++)
		{
			int freq,volume;


			freq = freqlo[voice] | (freqhi[voice] << 4);
			if (freq) freq = SND_CLOCK / freq;

			volume = vol[voice];
			volume = (volume << 4) | volume;

			osd_adjust_sample(voice,freq,volume);
		}
	}

	osd_update_audio();
}
