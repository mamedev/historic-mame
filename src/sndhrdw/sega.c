#include "driver.h"

/*
	The speech samples are queued in the order they are received in sega_sh_speech_w ().
	sega_sh_update () takes care of playing and updating the sounds in the order they
	were queued.
*/

#define MAX_SPEECH	4		/* Number of speech samples which can be queued */
#define NOT_PLAYING	-1		/* The queue holds the sample # so -1 will indicate no sample */
#define DEBUG

static int queue[MAX_SPEECH];
static int queuePtr = 0;

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
