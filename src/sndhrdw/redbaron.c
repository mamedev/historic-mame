#include "driver.h"
#include "sndhrdw/pokyintf.h"

/*

	Red Baron sound notes:
	$fx - $5x = explosion volume
	$08 = disable all sounds? led?
	$04 = machine gun sound
	$02 = nosedive sound

*/

/* Constants for the sound names in the redbaron sample array */
#define kFire			0
#define kNosedive		1
#define kExplode		2

extern int rb_input_select; 	/* used in drivers/redbaron.c to select joystick pot */

int redbaron_joy_r(int offset);

static struct POKEYinterface interface =
{
	1,	/* 1 chip */
	1,	/* 1 updates per video frame (low quality) */
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
	{ redbaron_joy_r },
};


int redbaron_sh_start(void)
{
	return pokey_sh_start (&interface);
}

void redbaron_sounds_w (int offset,int data) {

	static int lastValue = 0;
	static int explosionPlaying = 0;
	
	rb_input_select = (data & 0x01);

	if (Machine->samples == 0) return;

	if (lastValue == data) return;
	lastValue = data;
	
	/* Enable explosion output */
	if (data & 0xf0) {
		if (Machine->samples->sample[kExplode] != 0)
			osd_play_sample (1,Machine->samples->sample[kExplode]->data,
				Machine->samples->sample[kExplode]->length,
				Machine->samples->sample[kExplode]->smpfreq,
				Machine->samples->sample[kExplode]->volume,0);
		if (!explosionPlaying) explosionPlaying = 1;
		}
	else explosionPlaying = 0;
	
	/* Enable nosedive output */
	if (data & 0x02) {
		if (Machine->samples->sample[kNosedive] != 0)
			osd_play_sample (1,Machine->samples->sample[kNosedive]->data,
				Machine->samples->sample[kNosedive]->length,
				Machine->samples->sample[kNosedive]->smpfreq,
				Machine->samples->sample[kNosedive]->volume,0);
		}

	/* Enable firing output */
	if (data & 0x04) {
		if (Machine->samples->sample[kFire] != 0)
			osd_play_sample (1,Machine->samples->sample[kFire]->data,
				Machine->samples->sample[kFire]->length,
				Machine->samples->sample[kFire]->smpfreq,
				Machine->samples->sample[kFire]->volume,0);
		}

#ifdef DEBUG
	if (errorlog) fprintf (errorlog,"Red Baron sound: %02x\n",data);
#endif
	}
