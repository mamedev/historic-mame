#include "driver.h"

/*
	Voice breakdown:
	0 - thump
	1 - saucer
	2 - player fire
	3 - saucer fire
	4 - player thrust
	5 - extra life
	6 - explosions
*/

/* Constants for the sound names in the sample array */
#define kHighThump		0
#define kLowThump		1
#define kFire			2
#define kLargeSaucer	3
#define kSmallSaucer	4
#define kThrust			5
#define kSaucerFire		6
#define kLife			7
#define kExplode1		8
#define kExplode2		9
#define kExplode3		10


void asteroid_explode_w (int offset,int data) {

	static int explosion = -1;
	int explosion2;
	int sound = -1;

	if (Machine->samples == 0) return;

	if (data & 0x3c) {
		if (errorlog) fprintf (errorlog, "data: %02x, old explosion %02x\n",data, explosion);
		explosion2 = data >> 6;
		if (explosion2 != explosion) {
			osd_stop_sample (6);
			switch (explosion2) {
				case 0:
					sound = kExplode1;
					break;
				case 1:
					sound = kExplode1;
					break;
				case 2:
					sound = kExplode2;
					break;
				case 3:
					sound = kExplode3;
					break;
				}

			if (Machine->samples->sample[sound] != 0)
				osd_play_sample (6,Machine->samples->sample[sound]->data,
					Machine->samples->sample[sound]->length,
					Machine->samples->sample[sound]->smpfreq,
					Machine->samples->sample[sound]->volume,0);
			}
		explosion = explosion2;
		}
	else explosion = -1;
	}

void asteroid_thump_w (int offset,int data) {

	if (Machine->samples == 0) return;

	/* is the volume bit on? */
	if (data & 0x10) {
		int sound;

		if (data & 0x0f)
			sound = kHighThump;
		else
			sound = kLowThump;
		if (Machine->samples->sample[sound] != 0)
			osd_play_sample (0,Machine->samples->sample[sound]->data,
				Machine->samples->sample[sound]->length,
				Machine->samples->sample[sound]->smpfreq,
				Machine->samples->sample[sound]->volume,0);
		}
	}

void asteroid_sounds_w (int offset,int data) {
	static int fire = 0;
	static int saucer = 0;
	int sound;
	int fire2;

	if (Machine->samples == 0) return;

	switch (offset) {
		case 0: /* Saucer sounds */
			if (data & 0x80) {
				if (saucer)
					sound = kLargeSaucer;
				else
					sound = kSmallSaucer;

				if (Machine->samples->sample[sound] != 0)
					osd_play_sample (1,Machine->samples->sample[sound]->data,
						Machine->samples->sample[sound]->length,
						Machine->samples->sample[sound]->smpfreq,
						Machine->samples->sample[sound]->volume,0);
				}
			break;
		case 1: /* Saucer fire */
			if (data & 0x80) {
				if (Machine->samples->sample[kSaucerFire] != 0)
					osd_play_sample (3,Machine->samples->sample[kSaucerFire]->data,
						Machine->samples->sample[kSaucerFire]->length,
						Machine->samples->sample[kSaucerFire]->smpfreq,
						Machine->samples->sample[kSaucerFire]->volume,0);
				}
			break;
		case 2: /* Saucer sound select */
			saucer = data & 0x80;
			break;
		case 3: /* Player thrust */
			if (data & 0x80) {
				if (Machine->samples->sample[kThrust] != 0)
					osd_play_sample (4,Machine->samples->sample[kThrust]->data,
						Machine->samples->sample[kThrust]->length,
						Machine->samples->sample[kThrust]->smpfreq,
						Machine->samples->sample[kThrust]->volume,0);
				}
			break;
		case 4: /* Player fire */
			if (errorlog) fprintf (errorlog, "fire: %02x, old fire %02x\n",data & 0x80, fire);
			fire2 = data & 0x80;
			if (fire2 != fire) {
				osd_stop_sample (2);
				if (fire2)
					if (Machine->samples->sample[kFire] != 0)
						osd_play_sample (2,Machine->samples->sample[kFire]->data,
							Machine->samples->sample[kFire]->length,
							Machine->samples->sample[kFire]->smpfreq,
							Machine->samples->sample[kFire]->volume,0);
				}
			fire = fire2;
			break;
		case 5: /* life sound */
			if (data & 0x80) {
				if (Machine->samples->sample[kLife] != 0)
					osd_play_sample (5,Machine->samples->sample[kLife]->data,
						Machine->samples->sample[kLife]->length,
						Machine->samples->sample[kLife]->smpfreq,
						Machine->samples->sample[kLife]->volume,0);
				}
			break;
		}
	}

void asteroid_sh_update (void) {
	}
