#include "driver.h"

/*
	Tac/Scan sound constants

	There are some sound that are unknown:
	$09
	$0a
	$0e
	$1c
	$30 - $3f
	$41

	Some sound samples are missing - I use the one bullet and one explosion sound
	for all 3 for example.
*/

#define	shipStop 0x10
#define	shipLaser 0x18
#define	shipExplosion 0x20
#define	shipDocking 0x28
#define	shipRoar 0x40
#define	tunnelHigh 0x48
#define	stingerThrust 0x50
#define	stingerLaser 0x51
#define	stingerStop 0x52
#define	stingerExplosion 0x54
#define	enemyBullet0 0x61
#define	enemyBullet1 0x62
#define	enemyBullet2 0x63
#define	enemyExplosion0 0x6c
#define	enemyExplosion1 0x6d
#define	enemyExplosion2 0x6e

#define	kVoiceShipRoar 0
#define	kVoiceShip 1
#define	kVoiceTunnel 2
#define	kVoiceStinger 3
#define	kVoiceEnemy 4

static int roarPlaying;		/* Is the ship roar noise playing? */

/*
	The speech samples are queued in the order they are received in sega_sh_speech_w ().
	sega_sh_update () takes care of playing and updating the sounds in the order they
	were queued.

*/

#define MAX_SPEECH	4		/* Number of speech samples which can be queued */
#define NOT_PLAYING	-1		/* The queue holds the sample # so -1 will indicate no sample */
/* #define DEBUG */

static int queue[MAX_SPEECH];
static int queuePtr = 0;

#ifdef macintosh
#pragma mark ________Speech
#endif

int sega_sh_start (void) {
	int i;

	for (i = 0; i < MAX_SPEECH; i ++)
		queue[i] = NOT_PLAYING;

	return 0;
	}

int sega_sh_r (int offset) {

	/* 0x80 = universal sound board ready */
	/* 0x01 = speech ready */

	int retVal;

	retVal = 0x81;
/*
	if (osd_get_sample_status(0)) {
		retVal = 0x00;
		}
	else retVal = 0x81;
*/
#ifdef DEBUG
	if (errorlog) fprintf (errorlog, "Check sample status: %02x\n", retVal);
#endif
	return (retVal);
	}

void sega_sh_speech_w (int offset,int data) {

	int sound;

	if (Machine->samples == 0) return;

	sound = data & 0x7f;
	if ((sound >= Machine->samples->total) || (sound == 0))
		return;
	/* The sound numbers start at 1 but the sample name array starts at 0 */
	sound --;
	if (data & 0x80) {
		/* This typically comes immediately after a speech command. Purpose? */
		/* osd_stop_sample (0);*/
		return;
	} else if (Machine->samples->sample[sound] != 0) {
		int newPtr;

#ifdef DEBUG
		if (errorlog) fprintf (errorlog, "Queueing sound #:%02x    raw data:%02x\n", sound, data);
#endif

		/* Queue the new sound */
		newPtr = queuePtr;
		while (queue[newPtr] != NOT_PLAYING) {
			newPtr ++;
			if (newPtr >= MAX_SPEECH)
				newPtr = 0;
			if (newPtr == queuePtr) {
				/* The queue has overflowed. Oops. */
				if (errorlog) fprintf (errorlog, "*** Queue overflow! queuePtr: %02d\n", queuePtr);
				return;
				}
			}

		queue[newPtr] = sound;
		}

	}

void sega_sh_update (void) {
	int sound;

	/* if a sound is playing, return */
	if (!osd_get_sample_status (0)) return;

	/* Check the queue position. If a sound is scheduled, play it */
	if (queue[queuePtr] != NOT_PLAYING) {
		sound = queue[queuePtr];

#ifdef DEBUG
		if (errorlog) fprintf (errorlog, "playing sound: %02d\n", sound);
#endif
		osd_play_sample (0,Machine->samples->sample[sound]->data,
			Machine->samples->sample[sound]->length,
			Machine->samples->sample[sound]->smpfreq,
			Machine->samples->sample[sound]->volume,0);
		/* Update the queue pointer to the next one */
		queue[queuePtr] = NOT_PLAYING;
		++ queuePtr;
		if (queuePtr >= MAX_SPEECH)
			queuePtr = 0;
		}

	}

#ifdef macintosh
#pragma mark ________Tac/Scan Sound
#endif

int tacscan_sh_start (void) {

	roarPlaying = 0;
	return 0;
	}

void tacscan_sh_w (int offset,int data) {

	int sound;	/* index into the sample name array in drivers/sega.c */
	int voice=0;	/* which voice to play the sound on */
	int loop;	/* is this sound continuous? */

	if (Machine->samples == 0) return;

	loop = 0;
	switch (data) {
		case shipRoar:
			/* Play the increasing roar noise */
			voice = kVoiceShipRoar;
			sound = 0;
			roarPlaying = 1;
			break;
		case shipStop:
			/* Play the decreasing roar noise */
			voice = kVoiceShipRoar;
			sound = 2;
			roarPlaying = 0;
			break;
		case shipLaser:
			voice = kVoiceShip;
			sound = 3;
			break;
		case shipExplosion:
			voice = kVoiceShip;
			sound = 4;
			break;
		case shipDocking:
			voice = kVoiceShip;
			sound = 5;
			break;
		case tunnelHigh:
			voice = kVoiceTunnel;
			sound = 6;
			break;
		case stingerThrust:
			voice = kVoiceStinger;
			sound = 7;
			loop = 1;
			break;
		case stingerLaser:
			voice = kVoiceStinger;
			sound = 8;
			loop = 1;
			break;
		case stingerExplosion:
			voice = kVoiceStinger;
			sound = 9;
			break;
		case stingerStop:
			voice = kVoiceStinger;
			sound = -1;
			break;
		case enemyBullet0:
		case enemyBullet1:
		case enemyBullet2:
			voice = kVoiceEnemy;
			sound = 10;
			break;
		case enemyExplosion0:
		case enemyExplosion1:
		case enemyExplosion2:
			voice = kVoiceEnemy;
			sound = 11;
			break;
		default:
#ifdef DEBUG
			if (errorlog) fprintf (errorlog, "Play sample #: %02x\n", data);
#endif
			/* don't play anything */
			sound = -1;
			break;
		}
	if (sound != -1) {
		osd_stop_sample (voice);
		if (Machine->samples->sample[sound] !=0)
			osd_play_sample (voice,Machine->samples->sample[sound]->data,
			Machine->samples->sample[sound]->length,
			Machine->samples->sample[sound]->smpfreq,
			Machine->samples->sample[sound]->volume,loop);
		}
	}

void tacscan_sh_update (void) {

	/* If the ship roar has started playing but the sample stopped */
	/* play the intermediate roar noise */
	if ((roarPlaying) && (osd_get_sample_status (kVoiceShipRoar))) {
		if (Machine->samples->sample[kVoiceShipRoar] !=0)
			osd_play_sample (kVoiceShipRoar, Machine->samples->sample[1]->data,
			Machine->samples->sample[1]->length,
			Machine->samples->sample[1]->smpfreq,
			Machine->samples->sample[1]->volume,1);
		}
	}

