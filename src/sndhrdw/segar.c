/* sound: segar.c */

/*
 * Astro Blaster:
 * Port 076:
 * D7 - <Warp Modifier>
 * D6 - Refill
 * D5 - Mute
 * D4 - Asteroids
 * D3 - Invader 4
 * D2 - Invader 3
 * D1 - Invader 2
 * D0 - Invader 1
 *
 * Port 077:
 * D7 - Sonar
 * D6 - Bonus
 * D5 - <Rate Reset>
 * D4 - <Attack Rate>
 * D3 - Long Explosion
 * D2 - Short Explosion
 * D1 - Laser #2
 * D0 - Laser #1
 */

/*
 * Space Odyssey:
 * Port 0x0E:
 * D7 - Long Explosion
 * D6 - D. Bomb
 * D5 - Battle Star
 * D4 - Accelerate
 * D3 -
 * D2 - Short Explosion
 * D1 -
 * D0 - Back Ground
 *
 * Port 0x0F:
 * D7 - Black Hole
 * D6 - Appearance UFO
 * D5 -
 * D4 -
 * D3 - Warp
 * D2 -
 * D1 - Bonus Up
 * D0 - Shot
 */

#include "sndhrdw/generic.h"
#include "driver.h"

/* Number of simultaneous sound effect tracks */
#define MAX_TRACKS       6
#define MAX_QUEUE        10

/* Dedicate one track to speech */
#define SPEECH_TRACK     MAX_TRACKS+1
#define MAX_SPEECH_QUEUE 10

/* Dedicate one track to repeating sounds */
#define REPEAT_TRACK     SPEECH_TRACK+1

#define NOT_PLAYING	-1		/* The queue holds the sample # so -1 will indicate no sample */

static int queue[MAX_QUEUE];
static int queuePtr = 0;
static int speechQueue[MAX_SPEECH_QUEUE];
static int speechQueuePtr = 0;
static int curSample[MAX_TRACKS];
static int channel = 0;


const char *astrob_sample_names[] =
{
	"*astrob",
        "invadr1.sam","invadr2.sam","invadr3.sam","invadr4.sam",
        "asteroid.sam","refuel.sam",
        "winvadr1.sam","winvadr2.sam","winvadr3.sam","winvadr4.sam",

        "pbullet.sam","ebullet.sam","eexplode.sam","pexplode.sam",
        "deedle.sam","sonar.sam",

        "01.sam","02.sam","03.sam","04.sam","05.sam","06.sam","07.sam","08.sam",
        "09.sam","0a.sam","0b.sam","0c.sam","0d.sam","0e.sam","0f.sam","10.sam",
        "11.sam","12.sam","13.sam","14.sam","15.sam","16.sam","17.sam","18.sam",
        "19.sam","1a.sam","1b.sam","1c.sam","1d.sam","1e.sam","1f.sam","20.sam",
        "22.sam","23.sam",
	0
};

const char *s005_sample_names[] =
{
	0
};

const char *monsterb_sample_names[] =
{
        "zap.sam","jumpdown.sam","laughter.sam","wolfman.sam","warping.sam","tongue.sam",
	0
};

const char *spaceod_sample_names[] =
{
        "fire.sam", "bomb.sam", "eexplode.sam", "pexplode.sam",
        "warp.sam", "birth.sam", "scoreup.sam", "ssound.sam",
        "accel.sam", "damaged.sam", "erocket.sam",
	0
};

/* Astro Blaster */
enum
{
        invadr1 = 0,invadr2,invadr3,invadr4,
        asteroid,refuel,
        winvadr1,winvadr2,winvadr3,winvadr4,
        pbullet,ebullet,eexplode,pexplode,
        deedle,sonar,
        v01,v02,v03,v04,v05,v06,v07,v08,
        v09,v0a,v0b,v0c,v0d,v0e,v0f,v10,
        v11,v12,v13,v14,v15,v16,v17,v18,
        v19,v1a,v1b,v1c,v1d,v1e,v1f,v20,
        v22,v23
};

/* Monster Bash */
enum
{
        mbzap = 0,mbjumpdown,mblaughter,mbwolfman,mbwarping,mbtongue
};

/* Space Odyssey */
enum
{
        sofire = 0,sobomb,soeexplode,sopexplode,
        sowarp,sobirth,soscoreup,sossound,
        soaccel,sodamaged,soerocket
};


int segar_sh_start (void)
{
	int i;

        channel=get_play_channels(MAX_TRACKS + 2);

        for (i = 0; i < MAX_QUEUE; i ++)
		queue[i] = NOT_PLAYING;
        for (i = 0; i < MAX_SPEECH_QUEUE; i ++)
                speechQueue[i] = NOT_PLAYING;
        for (i = 0; i < MAX_TRACKS; i++)
                curSample[i] = NOT_PLAYING;
	return 0;
}

/* This function is currently not used... */
int segar_sh_r (int offset)
{

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
	return (retVal);
}

void segar_sh_update (void)
{
	int sound;
        int track;

	if( Machine->samples == 0 )
                return;

        for (track=0; track < MAX_TRACKS; track++)
        {
                if (osd_get_sample_status(channel + track))
                        curSample[track]=NOT_PLAYING;
        }

        /* Check the queue position. If a sound is scheduled, try to play it */
        if (queue[queuePtr] != NOT_PLAYING)
        {
		sound = queue[queuePtr];

                track=0;
                /* We'll take either an empty track or a track where this sound is already playing */
                while ((track<MAX_TRACKS) && (!osd_get_sample_status(channel + track)) && (curSample[track]!=sound))
                        track++;

                if (track!=MAX_TRACKS)
                {
                        if (curSample[track]==sound)
                                osd_stop_sample(channel + track);

                        if (Machine->samples->sample[sound])
                        osd_play_sample (channel + track,Machine->samples->sample[sound]->data,
                                Machine->samples->sample[sound]->length,
                                Machine->samples->sample[sound]->smpfreq,
                                Machine->samples->sample[sound]->volume,0);

                        curSample[track]=sound;

                        /* Update the queue pointer to the next one */
                        queue[queuePtr] = NOT_PLAYING;
                        queuePtr = ((queuePtr + 1) % MAX_QUEUE);
                }
       }

        /* Check the speech queue position. If a sound is scheduled, try to play it */
        if (speechQueue[speechQueuePtr] != NOT_PLAYING)
        {
                sound = speechQueue[speechQueuePtr];

                if (osd_get_sample_status(channel + SPEECH_TRACK))
                {
                        if (Machine->samples->sample[sound])
                        osd_play_sample (channel + SPEECH_TRACK,Machine->samples->sample[sound]->data,
                                Machine->samples->sample[sound]->length,
                                Machine->samples->sample[sound]->smpfreq,
                                Machine->samples->sample[sound]->volume,0);

                        /* Update the queue pointer to the next one */
                        speechQueue[speechQueuePtr] = NOT_PLAYING;
                        speechQueuePtr = ((speechQueuePtr + 1) % MAX_SPEECH_QUEUE);
                }
       }

}

void segar_Mute( void )
{
        osd_stop_sample(channel + REPEAT_TRACK);
}

void segar_PlayLoopSound( int which )
{
        struct GameSample *sample;

	if( Machine->samples == 0 )
                return;

        sample = Machine->samples->sample[which];

   if (sample == 0)
                return;

        osd_stop_sample (channel + REPEAT_TRACK);
        osd_play_sample(channel + REPEAT_TRACK, sample->data, sample->length, sample->smpfreq, sample->volume, 1);
}

void segar_queue_sound(int sound)
{
    int newPtr;

    /* Queue the new sound */
    if (sound!=NOT_PLAYING)
    {
            newPtr = queuePtr;
            while (queue[newPtr] != NOT_PLAYING)
            {
                 newPtr = ((newPtr + 1) % MAX_QUEUE);
                 if (newPtr == queuePtr)
                 {
                    /* The queue has overflowed. Oops. */
                    if (errorlog)
                        fprintf (errorlog, "*** Sound queue overflow!\n");
                    return;
                 }
            }

            queue[newPtr] = sound;
    }

}

void segar_queue_speech(int sound)
{
    int newPtr;

    /* Queue the new sound */
    if (sound!=NOT_PLAYING)
    {
            newPtr = speechQueuePtr;
            while (speechQueue[newPtr] != NOT_PLAYING)
            {
                 newPtr = ((newPtr + 1) % MAX_SPEECH_QUEUE);
                 if (newPtr == speechQueuePtr)
                 {
                    /* The queue has overflowed. Oops. */
                    if (errorlog)
                        fprintf (errorlog, "*** Speech queue overflow!\n");
                    return;
                 }
            }

            speechQueue[newPtr] = sound;
    }

}

void spaceod_audio_port1_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

/* This one's just too annoying */
/*
        if (!(val & 0x01))   segar_queue_sound(sossound);
*/
        if (!(val & 0x04))   segar_queue_sound(soeexplode);
        if (!(val & 0x10))   segar_queue_sound(soaccel);
        if (!(val & 0x20))   segar_queue_sound(soerocket);
        if (!(val & 0x40))   segar_queue_sound(sobomb);
        if (!(val & 0x80))   segar_queue_sound(sopexplode);

}

void spaceod_audio_port2_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

        if (!(val & 0x01))   segar_queue_sound(sofire);
        if (!(val & 0x02))   segar_queue_sound(soscoreup);
        if (!(val & 0x08))   segar_queue_sound(sowarp);
        if (!(val & 0x40))   segar_queue_sound(sobirth);
        if (!(val & 0x80))   segar_queue_sound(sodamaged);

}

void astrob_speech_port_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

        switch (val)
        {
                case 0x01:      segar_queue_speech(v01); break;
                case 0x02:      segar_queue_speech(v02); break;
                case 0x03:      segar_queue_speech(v03); break;
                case 0x04:      segar_queue_speech(v04); break;
                case 0x05:      segar_queue_speech(v05); break;
                case 0x06:      segar_queue_speech(v06); break;
                case 0x07:      segar_queue_speech(v07); break;
                case 0x08:      segar_queue_speech(v08); break;
                case 0x09:      segar_queue_speech(v09); break;
                case 0x0a:      segar_queue_speech(v0a); break;
                case 0x0b:      segar_queue_speech(v0b); break;
                case 0x0c:      segar_queue_speech(v0c); break;
                case 0x0d:      segar_queue_speech(v0d); break;
                case 0x0e:      segar_queue_speech(v0e); break;
                case 0x0f:      segar_queue_speech(v0f); break;
                case 0x10:      segar_queue_speech(v10); break;
                case 0x11:      segar_queue_speech(v11); break;
                case 0x12:      segar_queue_speech(v12); break;
                case 0x13:      segar_queue_speech(v13); break;
                case 0x14:      segar_queue_speech(v14); break;
                case 0x15:      segar_queue_speech(v15); break;
                case 0x16:      segar_queue_speech(v16); break;
                case 0x17:      segar_queue_speech(v17); break;
                case 0x18:      segar_queue_speech(v18); break;
                case 0x19:      segar_queue_speech(v19); break;
                case 0x1a:      segar_queue_speech(v1a); break;
                case 0x1b:      segar_queue_speech(v1b); break;
                case 0x1c:      segar_queue_speech(v1c); break;
                case 0x1d:      segar_queue_speech(v1d); break;
                case 0x1e:      segar_queue_speech(v1e); break;
                case 0x1f:      segar_queue_speech(v1f); break;
                case 0x20:      segar_queue_speech(v20); break;
                case 0x22:      segar_queue_speech(v22); break;
                case 0x23:      segar_queue_speech(v23); break;

	}

}

void astrob_audio_port1_w( int offset, int val )
{
        int warp=0;

	if( Machine->samples == 0 )
                return;

        if (!(val & 0x80))  warp=1;

        if (!(val & 0x40))           segar_PlayLoopSound(refuel);
        if (!(val & 0x10))           segar_PlayLoopSound(asteroid);
        if (!(val & 0x08) && warp)   segar_PlayLoopSound(winvadr4);
        if (!(val & 0x08) && !warp)  segar_PlayLoopSound(invadr4);
        if (!(val & 0x04) && warp)   segar_PlayLoopSound(winvadr3);
        if (!(val & 0x04) && !warp)  segar_PlayLoopSound(invadr3);
        if (!(val & 0x02) && warp)   segar_PlayLoopSound(winvadr2);
        if (!(val & 0x02) && !warp)  segar_PlayLoopSound(invadr2);
        if (!(val & 0x01) && warp)   segar_PlayLoopSound(winvadr1);
        if (!(val & 0x01) && !warp)  segar_PlayLoopSound(invadr1);

        if (!(val & 0x20))           segar_Mute();
}

void astrob_audio_port2_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

        if (!(val & 0x01))  segar_queue_sound(pbullet);
        if (!(val & 0x02))  segar_queue_sound(ebullet);
        if (!(val & 0x04))  segar_queue_sound(eexplode);
        if (!(val & 0x08))  segar_queue_sound(pexplode);
        if (!(val & 0x40))  segar_queue_sound(deedle);
        if (!(val & 0x80))  segar_queue_sound(sonar);

}

/* This port fires off two discrete circuit sounds */
void monsterb_audio_port2_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

        if (!(val & 0x01))   segar_queue_sound(mbzap);
        if (!(val & 0x02))   segar_queue_sound(mbjumpdown);

}

void monsterb_audio_port3_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;

        switch (val & 0x0F)
        {
                case 0x0F:      segar_queue_sound(mblaughter); break;
                case 0x0E:      segar_queue_sound(mbwolfman); break;
                case 0x0C:      segar_queue_sound(mbwarping); break;
                case 0x0D:      segar_queue_sound(mbtongue); break;
        }
}

void monsterb_audio_port4_w( int offset, int val )
{
	if( Machine->samples == 0 )
                return;
}

int monsterb_audio_port3_r( int offset)
{
	if( Machine->samples == 0 )
                return 0x00;

        return 0xFF;
}



