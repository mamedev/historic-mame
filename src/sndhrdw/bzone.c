#include "driver.h"
#include "sndhrdw/pokyintf.h"

/*

Battlezone sound info, courtesy of Al Kossow:

D7	motor enable			this enables the engine sound
D6	start LED
D5	sound enable			this enables ALL sound outputs
							including the POKEY output
D4	engine rev en			this controls the engine speed
							the engine sound is an integrated square
							wave (saw tooth) that is frequency modulated
							by engine rev.
D3	shell loud, soft/		explosion volume
D2	shell enable
D1	explosion loud, soft/	explosion volume
D0	explosion enable		gates a noise generator

*/

/* Constants for the sound names in the bzone sample array */
#define kFire1			0
#define kFire2			1
#define kEngine1		2
#define kEngine2		3
#define kExplode1		4
#define kExplode2		5

static int	soundEnable = 0;

int bzone_IN3_r(int offset);

static struct POKEYinterface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	FREQ_17_APPROX,	/* 1.7 Mhz */
	255,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ bzone_IN3_r },
};


int bzone_sh_start(void)
{
	return pokey_sh_start (&interface);
}

void bzone_sounds_w (int offset,int data) {

	static int lastValue = 0;

	/* Enable/disable all sound output */
	if (data & 0x20)
		soundEnable = 1;
	else soundEnable = 0;

	if (Machine->samples == 0) return;

	/* If sound is off, don't bother playing samples */
	if (!soundEnable) {
//		osd_stop_sample (0);	/* Turn off pokey */
		osd_stop_sample (1);	/* Turn off explosion */
		osd_stop_sample (2);	/* Turn off engine noise */
		return;
		}

	if (lastValue == data) return;
	lastValue = data;

	/* Enable explosion output */
	if (data & 0x01) {
		if (data & 0x02) {
			if (Machine->samples->sample[kExplode1] != 0)
				osd_play_sample (1,Machine->samples->sample[kExplode1]->data,
					Machine->samples->sample[kExplode1]->length,
					Machine->samples->sample[kExplode1]->smpfreq,
					Machine->samples->sample[kExplode1]->volume,0);
			}
		else {
			if (Machine->samples->sample[kExplode2] != 0)
				osd_play_sample (1,Machine->samples->sample[kExplode2]->data,
					Machine->samples->sample[kExplode2]->length,
					Machine->samples->sample[kExplode2]->smpfreq,
					Machine->samples->sample[kExplode2]->volume,0);
			}
		}

	/* Enable shell output */
	if (data & 0x04) {
		/* loud shell */
		if (data & 0x08) {
			if (Machine->samples->sample[kFire1] != 0)
				osd_play_sample (1,Machine->samples->sample[kFire1]->data,
					Machine->samples->sample[kFire1]->length,
					Machine->samples->sample[kFire1]->smpfreq,
					Machine->samples->sample[kFire1]->volume,0);
				}
		/* soft shell */
		else {
			if (Machine->samples->sample[kFire2] != 0)
				osd_play_sample (1,Machine->samples->sample[kFire2]->data,
					Machine->samples->sample[kFire2]->length,
					Machine->samples->sample[kFire2]->smpfreq,
					Machine->samples->sample[kFire2]->volume,0);
				}
		}
	/* Enable engine output */
	if (data & 0x80) {
		/* Fast rumble */
		if (data & 0x10) {
			if (Machine->samples->sample[kEngine2] != 0)
				osd_play_sample (2,Machine->samples->sample[kEngine2]->data,
					Machine->samples->sample[kEngine2]->length,
					Machine->samples->sample[kEngine2]->smpfreq,
					Machine->samples->sample[kEngine2]->volume,1);
			}
		/* Slow rumble */
		else {
			if (Machine->samples->sample[kEngine1] != 0)
				osd_play_sample (2,Machine->samples->sample[kEngine1]->data,
					Machine->samples->sample[kEngine1]->length,
					Machine->samples->sample[kEngine1]->smpfreq,
					Machine->samples->sample[kEngine1]->volume,1);
			}
		}
	}

void bzone_pokey_w (int offset,int data) {

	if (soundEnable)
		pokey1_w (offset, data);
	}
