#include "driver.h"

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
/* Swapped High and Low thump. Corrected saucer sound stop */

#define kExplode1    0
#define kExplode2    1
#define kExplode3    2
#define kThrust      3
#define kHighThump   4
#define kLowThump    5
#define kFire        6
#define kLargeSaucer 7
#define kSmallSaucer 8
#define kSaucerFire	 9
#define kLife        10



void asteroid_explode_w (int offset,int data)
{
	static int explosion = -1;
	int explosion2;
	int sound = -1;


	if (data & 0x3c)
	{
#ifdef DEBUG
		if (errorlog) fprintf (errorlog, "data: %02x, old explosion %02x\n",data, explosion);
#endif
		explosion2 = data >> 6;
		if (explosion2 != explosion)
		{
			sample_stop(0);
			switch (explosion2)
			{
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

			sample_start(0,sound,0);
		}
		explosion = explosion2;
	}
	else explosion = -1;
}



void asteroid_thump_w (int offset,int data)
{
	/* is the volume bit on? */
	if (data & 0x10)
	{
		int sound;

		if (data & 0x0f)
			sound = kLowThump;
		else
			sound = kHighThump;
		sample_start(1,sound,0);
	}
}



void asteroid_sounds_w (int offset,int data)
{
	static int fire = 0;
	static int sfire = 0;
	static int saucer = 0;
	static int lastsaucer = 0;
	static int lastthrust = 0;
	int sound;
	int fire2;
	int sfire2;


	switch (offset)
	{
		case 0: /* Saucer sounds */
			if ((data&0x80) && !(lastsaucer&0x80))
			{
				if (saucer)
					sound = kLargeSaucer;
				else
					sound = kSmallSaucer;
				sample_start(2,sound,1);
			}
			if (!(data&0x80) && (lastsaucer&0x80))
				sample_stop(2);
			lastsaucer=data;
			break;
		case 1: /* Saucer fire */
			sfire2 = data & 0x80;
			if (sfire2!=sfire)
			{
				sample_stop(3);
				if (sfire2) sample_start(3,kSaucerFire,0);
			}
			sfire=sfire2;
			break;
		case 2: /* Saucer sound select */
			saucer = data & 0x80;
			break;
		case 3: /* Player thrust */
			if ((data&0x80) && !(lastthrust&0x80)) sample_start(4,kThrust,1);
			if (!(data&0x80) && (lastthrust&0x80)) sample_stop(4);
			lastthrust=data;
			break;
		case 4: /* Player fire */
			fire2 = data & 0x80;
			if (fire2 != fire)
			{
				sample_stop(5);
				if (fire2) sample_start(5,kFire,0);
			}
			fire = fire2;
			break;
		case 5: /* life sound */
			if (data & 0x80) sample_start(6,kLife,0);
			break;
	}
}



void astdelux_sounds_w (int offset,int data)
{
	static int lastthrust = 0;

	if (!(data&0x80) && (lastthrust&0x80)) sample_start (1,kThrust,1);
	if ((data&0x80) && !(lastthrust&0x80)) sample_stop (1);
	lastthrust=data;
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

void llander_sounds_w (int offset,int data)
{
	static int explosionPlaying = 0;
	int volume;
	int beeper;
	int explosion;


	volume = data & 0x07;
	beeper = data & 0x10;
	explosion = data & 0x08;

//	if (errorlog) fprintf (errorlog,"*** volume %02x   beeper %02x  explosion %02x\n",volume, beeper, explosion);

	if (beeper && volume)
	{
		/* Play the beeper sample on voice 0 and loop it */
		sample_start(0,kLLbeeper,1);
	}
	else if (!beeper)
	{
		/* Stop the beeper sample on voice 0 */
		sample_stop(0);
	}

	if (explosion && volume)
	{
		if (!explosionPlaying)
		{
			/* Play the explosion sample on voice 2 */
			explosionPlaying = 1;
			sample_start(1,kLLexplosion,0);
		}
	}
	/* We cheat for the thrust - during gameplay it's always at least 1 to simulate a very low */
	/* rumble. By making it only play with a volume >= 3, we avoid always playing the sample. */
	else if (volume >= 3) sample_start(2,kLLthrust,0);
	else explosionPlaying = 0;
}
