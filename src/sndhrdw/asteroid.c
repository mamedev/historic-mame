#include "driver.h"
#include "sndhrdw/pokyintf.h"

/*
	Asteroids Voice breakdown:
	0 - thump
	1 - saucer
	2 - player fire
	3 - saucer fire
	4 - player thrust
	5 - extra life
	6 - explosions
*/

/* Constants for the sound names in the asteroid sample array */
/* Move the sounds Astdelux and Asteroid have in common to the */
/* beginning. BW */
#define kExplode1	0
#define kExplode2	1
#define kExplode3	2
#define kThrust		3
#define kHighThump	4
#define kLowThump	5
#define kFire		6
#define kLargeSaucer	7
#define kSmallSaucer	8
#define kSaucerFire	9
#define kLife		10

/* A nice macro which saves us a lot of typing */
#define OSD_PLAY_SAMPLE(channel, soundnum, loop) {			\
	if (Machine->samples->sample[soundnum] != 0)	 	   	\
	osd_play_sample(channel,					\
		Machine->samples->sample[soundnum]->data,      		\
		Machine->samples->sample[soundnum]->length,    		\
		Machine->samples->sample[soundnum]->smpfreq,   		\
		Machine->samples->sample[soundnum]->volume,loop); }


/* Misc sound code */

static struct POKEYinterface interface =
{
	1,	/* 1 chip */
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
	{ input_port_3_r }
};


int astdelux_sh_start(void)
{
	return pokey_sh_start (&interface);
}

void asteroid_explode_w (int offset,int data) {

	static int explosion = -1;
	int explosion2;
	int sound = -1;

	if (Machine->samples == 0) return;

	if (data & 0x3c) {
#ifdef DEBUG
		if (errorlog) fprintf (errorlog, "data: %02x, old explosion %02x\n",data, explosion);
#endif
		explosion2 = data >> 6;
		if (explosion2 != explosion) {
			osd_stop_sample (6);
			switch (explosion2) {
				case 0:
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

				OSD_PLAY_SAMPLE (6,sound,0)
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
		OSD_PLAY_SAMPLE(0,sound,0)
		}
	}

void asteroid_sounds_w (int offset,int data) {
	static int fire = 0;
	static int sfire = 0;
	static int saucer = 0;
	static int lastsaucer = 0;
	static int lastthrust = 0;
	int sound;
	int fire2;
	int sfire2;

	if (Machine->samples == 0) return;

	switch (offset) {
		case 0: /* Saucer sounds */
			if ((data&0x80) && !(lastsaucer&0x80)) {
				if (saucer)
					sound = kLargeSaucer;
				else
					sound = kSmallSaucer;
				OSD_PLAY_SAMPLE(1, sound, 1)
				}
			if (!(data&0x80) && (lastsaucer&0x80))
				osd_stop_sample(1);
			lastsaucer=data;
			break;
		case 1: /* Saucer fire */
			sfire2 = data & 0x80;
			if (sfire2!=sfire) {
				osd_stop_sample(3);
				if (sfire2)
					OSD_PLAY_SAMPLE(3,kSaucerFire,0);
				}
			sfire=sfire2;
			break;
		case 2: /* Saucer sound select */
			saucer = data & 0x80;
			break;
		case 3: /* Player thrust */
			if ((data&0x80) && !(lastthrust&0x80))
				OSD_PLAY_SAMPLE(4,kThrust,1);
			if (!(data&0x80) && (lastthrust&0x80))
				osd_stop_sample(4);
			lastthrust=data;
			break;
		case 4: /* Player fire */
			fire2 = data & 0x80;
			if (fire2 != fire) {
				osd_stop_sample (2);
				if (fire2)
					OSD_PLAY_SAMPLE(2,kFire,0);
				}
			fire = fire2;
			break;
		case 5: /* life sound */
			if (data & 0x80)
				OSD_PLAY_SAMPLE(5,kLife,0);
			break;
		}
	}

void astdelux_sounds_w (int offset,int data)
{
	static int lastthrust = 0;

	if (Machine->samples == 0) return;

	if (!(data&0x80) && (lastthrust&0x80))
		OSD_PLAY_SAMPLE(4,kThrust,1);
	if ((data&0x80) && !(lastthrust&0x80))
		osd_stop_sample(4);
	lastthrust=data;
}

void asteroid_sh_update (void)
{
}

/*
	Lunar Lander sound breakdown:

	bits 0-2 = volume
	bit 3 = explosion enable
	bit 4 = beeper enable
	If volume > 0 and bits 3 & 4 are off, the thrust noise is played
*/

/* Constants for the sound samples in the llander sample array */
#define kLLthrust		0
#define kLLbeeper		1
#define kLLexplosion	2

void llander_sounds_w (int offset,int data) {

	static int explosionPlaying = 0;
	int volume;
	int beeper;
	int explosion;

	if (Machine->samples == 0) return;

	volume = data & 0x07;
	beeper = data & 0x10;
	explosion = data & 0x08;

//	if (errorlog) fprintf (errorlog,"*** volume %02x   beeper %02x  explosion %02x\n",volume, beeper, explosion);

	if (beeper && volume) {
		/* Play the beeper sample on voice 1 and loop it */
		OSD_PLAY_SAMPLE(1, kLLbeeper, 1)
		}
	else if (explosion && volume) {
		if (Machine->samples->sample[kLLexplosion] != 0 && !explosionPlaying) {
			/* Play the explosion sample on voice 2 */
			explosionPlaying = 1;
			OSD_PLAY_SAMPLE(2,kLLexplosion,0);
			}
		}
	/* We cheat for the thrust - during gameplay it's always at least 1 to simulate a very low */
	/* rumble. By making it only play with a volume >= 3, we avoid always playing the sample. */
	else if (volume >= 3) {
		OSD_PLAY_SAMPLE(0,kLLthrust,0);
		}
	else {
		explosionPlaying = 0;
		}
	}
