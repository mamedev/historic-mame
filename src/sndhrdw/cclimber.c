#include "driver.h"
#include "sndhrdw/8910intf.h"



#define SND_CLOCK 3072000	/* 3.072 Mhz */


static unsigned char samples[0x4000];	/* 16k for samples */
static int sample_freq,sample_volume;
static int porta;



static void cclimber_portA_w(int offset,int data)
{
	porta = data;
}



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	1536000000,	/* 1.536 MHZ */
	{ 255 },
	{ 0 },
	{ 0 },
	{ cclimber_portA_w },
	{ 0 }
};



int cclimber_sh_start(void)
{
	int i;
	unsigned char bits;


	/* decode the rom samples */
	for (i = 0;i < 0x2000;i++)
	{
		bits = Machine->memory_region[2][i] & 0xf0;
		samples[2 * i] = (bits | (bits >> 4)) + 0x80;

		bits = Machine->memory_region[2][i] & 0x0f;
		samples[2 * i + 1] = ((bits << 4) | bits) + 0x80;
	}

	return AY8910_sh_start(&interface);
}



void cclimber_sample_rate_w(int offset,int data)
{
	/* calculate the sampling frequency */
	sample_freq = SND_CLOCK / 4 / (256 - data);
}



void cclimber_sample_volume_w(int offset,int data)
{
	sample_volume = data & 0x1f;
	sample_volume = (sample_volume << 3) | (sample_volume >> 2);
}



void cclimber_sample_trigger_w(int offset,int data)
{
	int start,end;


	if (data == 0 || play_sound == 0)
		return;

	start = 64 * porta;
	end = start;

	/* find end of sample */
	while (end < 0x4000 && (samples[end] != 0xf7 || samples[end+1] != 0x80))
		end += 2;

	osd_play_sample(1,samples + start,end - start,sample_freq,sample_volume,0);
}
