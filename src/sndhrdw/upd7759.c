/************************************************************

 NEC uPD7759 ADPCM Speech Processor
 by: Juergen Buchmueller, Mike Balfour and Howie Cohen


 Description:
 The uPD7759 is a speech processing LSI that, with an external
 ROM, utilizes ADPCM to produce speech.  The uPD7759 can
 directly address up to 1Mbits of external data ROM, or the
 host CPU can control the speech data transfer.  Three sample
 frequencies are selectable - 5, 6, or 8 kHz.  The external
 ROM can store a maximum of 256 different messages and up to
 50 seconds of speech.

 The uPD7759 should always be hooked up to a 640 kHz clock.

 TODO:
 1) find bugs
 2) fix bugs
 3) Bankswitching and frequency selection may not be 100%

NOTES:

There are 2 types of upd7759 sound roms, master and slave.

A master rom has a header at the beginning of the rom
for example : 15 5A A5 69 55 (this is the POW header)

-the 1st byte (15) is the number of samples stored in the rom
 (actually the number of samples minus 1 - NS)
-the next 4 bytes seems standard in every upd7759 rom used in
master mode (5A A5 69 55)
-after that there is table of (sample offsets)/2 we use this to
calculate the sample table on the fly. Then the samples start,
each sample has a short header with sample rate info in it.
A master rom can have up to 256 samples , and there should be
only one rom per upd7759.

a slave rom has no header... but each sample starts with
FF 00 00 00 00 10 (followed by a few other bytes that are
usually the same but not always)

Clock rates:
in master mode the clock rate should always be 64000000
sample frequencies are selectable - 5, 6, or 8 kHz. This
info is coded in the uPD7759 roms, it selects the sample
frequency for you.

slave mode is still some what of a mystery.  Everything
we know about slave mode (and 1/2 of everything in master
mode) is from guesswork. As far as we know the clock rate
is the same for all samples in slave mode on a per game basis
so valid clock rates are: 40000000, 48000000, 64000000

Differances between master/slave mode.
(very basic explanation based on my understanding)

Master mode: the sound cpu sends a sample number to the upd7759
it then sends a trigger command to it, and the upd7759 plays the
sample directly from the rom (like an adpcm chip)

Slave mode:  the sound cpu sends data to the upd7759 to select
the bank for the sound data, each bank is 0x4000 in size, and
the sample offset. The sound cpu then sends the data for the sample
a byte(?) at a time (dac / cvsd like) which the upd7759 plays till
it reaches the header of the next sample (FF 00 00 00 00)



 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "upd7759.h"
#include "streams.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(n,x)  if( (n>=VERBOSE) && errorlog ) fprintf x
#else
#define LOG(n,x)
#endif


/* signed/unsigned 8-bit conversion macros */
#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) ((A))
#else
	#define AUDIO_CONV(A) ((A)+0x80)
#endif

/* number of samples stuffed into the rom */
static unsigned char numsam;

/* bits to use for the streams interface */
static int bits;

/* playback rate for the streams interface */
/* BASE_CLOCK or a multiple (if oversampling is active) */
static int emulation_rate;

static int base_rate;
/* define the output rate */
#define CLOCK_DIVIDER 80

#define OVERSAMPLING	0	/* 1 use oversampling, 0 don't */

#define SIGNAL_BITS 	13			/* signal range */
#define SIGNAL_SHIFT_L	1			/* adjustment for 16bit samples */
#define SIGNAL_SHIFT_R  (8-SIGNAL_SHIFT_L)           /* adjustment for 8bit samples */
#define SIGNAL_MAX      (0x7fff >> (15-(SIGNAL_SHIFT_L+SIGNAL_BITS)))
#define SIGNAL_MIN		-(SIGNAL_MAX+1)
#define SIGNAL_ZERO 	-2

#define STEP_MAX		48
#define STEP_MIN		0
#define STEP_ZERO		0



struct UPD7759sample
{
  	unsigned int offset;    	/* offset in that region */
	unsigned int length;    /* length of the sample */
	unsigned int freq;	   /* play back freq of sample */
};


/* struct describing a single playing ADPCM voice */
struct UPD7759voice
{
	int playing;            /* 1 if we are actively playing */
	unsigned char *base;    /* pointer to the base memory location */
	unsigned char *stream;  /* input stream data (if any) */
	int mask;               /* mask to keep us within the buffer */
	int sample; 			/* current sample number (sample data in slave mode) */
	int freq;				/* current sample playback freq */
	int count;              /* total samples to play */
	int signal;             /* current ADPCM signal */
#if OVERSAMPLING
	int old_signal; 		/* last ADPCM signal */
#endif
    int step;               /* current ADPCM step */
	int counter;			/* over sampleing counter */
	void *timer;			/* timer used in slave mode */
};

/* global pointer to the current interface */
static struct UPD7759_interface *upd7759_intf;

/* array of ADPCM voices */
static struct UPD7759voice updadpcm[MAX_UPD7759];

#if OVERSAMPLING
/* oversampling factor, ie. playback_rate / BASE_CLOCK */
static int oversampling;
#endif
/* array of channels returned by streams.c */
static int channel[MAX_UPD7759];

/* stores the current sample number */
static int sampnum[MAX_UPD7759];

/* step size index shift table */
//static int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static int index_shift[8] = {-1, 1, 1, 2, 2, 3, 5, 7 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

static void UPD7759_update (int chip, void *buffer, int left);

/*
 *   Compute the difference table
 */

static void ComputeTables (void)
{
	/* nibble to bit map */
    static int nbl2bit[16][4] = {
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};
	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= STEP_MAX; step++) {

        /* compute the step value */
		int stepval = floor(16.0 * pow (11.0 / 10.0, (double)step));
		LOG(1,(errorlog, "step %2d:", step));
		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++) {
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
			LOG(1,(errorlog, " %+5d", diff_lookup[step*16 + nib]));
        }
		LOG(1,(errorlog, "\n"));
    }
}



static int find_sample(int sample_num,struct UPD7759sample *sample)
{
	int j;
	int nextoff = 0;
	unsigned char *memrom;
	unsigned char *header;   /* upd7759 has a 4 byte what we assume is an identifier (bytes 1-4)*/
	unsigned char *data;


	memrom = Machine->memory_region[upd7759_intf->region];

	numsam = (unsigned int)memrom[0]; /* get number of samples from sound rom */
	header = &(memrom[1]);

	if (memcmp (header, "\x5A\xA5\x69\x55",4) == 0) {
		LOG(1,(errorlog,"uPD7759 header verified\n"));
	} else {
		LOG(1,(errorlog,"uPD7759 header verification failed\n"));
	}

	LOG(1,(errorlog,"Number of samples in UPD7759 rom = %d\n",numsam));

	/* move the header pointer to the start of the sample offsets */
	header = &(memrom[5]);


	if (sample_num > numsam) return 0;	/* sample out of range */


	nextoff = 2 * sample_num;
	sample->offset = ((((unsigned int)(header[nextoff]))<<8)+(header[nextoff+1]))*2;
	data = &Machine->memory_region[upd7759_intf->region][sample->offset];
	/* guesswork, probably wrong */
	j = 0;
	if (!data[j]) j++;
	if ((data[j] & 0xf0) != 0x50) j++;
	switch (data[j]) {
		case 0x53: sample->freq = 8000; break;
		case 0x59: sample->freq = 6000; break;
		default:
			sample->freq = 5000;
	}

	if (sample_num == numsam)
	{
		sample->length = 0x20000 - sample->offset;
	}
	else
		sample->length = ((((unsigned int)(header[nextoff+2]))<<8)+(header[nextoff+3]))*2 -
							((((unsigned int)(header[nextoff]))<<8)+(header[nextoff+1]))*2;

if (errorlog)
{
	data = &Machine->memory_region[upd7759_intf->region][sample->offset];
	fprintf( errorlog,"play sample %3d, offset $%06x, length %5d, freq = %4d [data $%02x $%02x $%02x]\n",
		sample_num,
		sample->offset,
		sample->length,
		sample->freq,
		data[0],data[1],data[2]);
}

	return 1;
}


/*
 *   Start emulation of several ADPCM output streams
 */


int UPD7759_sh_start (struct UPD7759_interface *intf)
{
	int i, j;

	/* compute the difference tables */
	ComputeTables ();

    bits = Machine->sample_bits;
	LOG(1,(errorlog,"UPD7759 using %d bit samples\n", bits));


    /* copy the interface pointer to a global */
	upd7759_intf = intf;
	base_rate=intf->clock_rate/CLOCK_DIVIDER;

#if OVERSAMPLING
	oversampling = (Machine->sample_rate / base_rate);
	if (!oversampling) oversampling = 1;
	emulation_rate = base_rate * oversampling;
#else
	emulation_rate = base_rate;
#endif


	memset(updadpcm,0,sizeof(updadpcm));
	for (i = 0; i < intf->num; i++)
	{
		char name[20];

		updadpcm[i].mask = 0xffffffff;
		updadpcm[i].signal = SIGNAL_ZERO;
		updadpcm[i].step = STEP_ZERO;
		updadpcm[i].counter = emulation_rate / 2;

		sprintf(name,"uPD7759 #%d",i);

		channel[i] = stream_init(name, emulation_rate, bits, i, UPD7759_update);
		stream_set_volume(channel[i], intf->volume[i]);
	}
	return 0;
}


/*
 *   Stop emulation of several UPD7759 output streams
 */

void UPD7759_sh_stop (void)
{
	int i;

    /* free any output and streaming buffers */
	for (i = 0; i < upd7759_intf->num; i++) {
		if (updadpcm[i].stream)
			free (updadpcm[i].stream);
		updadpcm[i].stream = 0;
	}
}

void UPD7759_sh_update (void)
{
}


/*
 *	 Update emulation of an ADPCM output stream - 8 bit case
 */
static void update_8bit (struct UPD7759voice *voice, unsigned char *buffer, int left)
{
	unsigned char *base = voice->base;
	int sample = voice->sample;
	int signal = voice->signal;
#if OVERSAMPLING
	int old_signal = voice->old_signal;
	int i, delta;
#endif
    int count = voice->count;
	int step = voice->step;
	int counter = voice->counter;
	int mask = voice->mask;
	int val;

	while (left) {
		/* compute the new amplitude and update the current step */
		val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
		signal += diff_lookup[step * 16 + (val & 15)];
		if (signal > SIGNAL_MAX) signal = SIGNAL_MAX;
		else if (signal < SIGNAL_MIN) signal = SIGNAL_MIN;
		step += index_shift[val & 7];
		if (step > STEP_MAX) step = STEP_MAX;
		else if (step < STEP_MIN) step = STEP_MIN;
#if OVERSAMPLING
		i = 0;
		delta = (signal  << SIGNAL_SHIFT_L) - old_signal;
        while (counter > 0 && left > 0) {
			*buffer++ = (old_signal + delta * i / oversampling) >> 8;
			if (++i == oversampling) i = 0;
			counter -= voice->freq;
			left--;
		}
		old_signal = (signal  << SIGNAL_SHIFT_L);
#else
        while (counter > 0 && left > 0) {
			*buffer++ = AUDIO_CONV(signal >> SIGNAL_SHIFT_R);
			counter -=voice->freq;
			left--;
		}
#endif
        counter += emulation_rate;

        /* next! */
		if (++sample > count) {
			/* if we're not streaming, fill with silence and stop */
			if (voice->base != voice->stream) {
				while (left--)
					*buffer++ = AUDIO_CONV(voice->signal >> SIGNAL_SHIFT_R);
				voice->playing = 0;
			} else {
				/* if we are streaming, pad with the last sample */
				unsigned char last = buffer[-1];
				while (left--)
					*buffer++ = last;
			}
			break;
		}
	}
    /* update the parameters */
	voice->sample = sample;
	voice->signal = signal;
#if OVERSAMPLING
	voice->old_signal = old_signal;
#endif
    voice->step = step;
	voice->counter = counter;
}

/*
 *	 Update emulation of an ADPCM output stream - 16 bit case
 */
static void update_16bit (struct UPD7759voice *voice, short *buffer, int left)
{
	unsigned char *base = voice->base;
	int sample = voice->sample;
	int signal = voice->signal;
#if OVERSAMPLING
	int old_signal = voice->old_signal;
	int i, delta;
#endif
    int count = voice->count;
	int step = voice->step;
	int counter = voice->counter;
    int mask = voice->mask;
	int val;

	while (left) {

		/* compute the new amplitude and update the current step */
		val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
		signal += diff_lookup[step * 16 + (val & 15)];
		if (signal > SIGNAL_MAX) signal = SIGNAL_MAX;
		else if (signal < SIGNAL_MIN) signal = SIGNAL_MIN;
		step += index_shift[val & 7];
		if (step > STEP_MAX) step = STEP_MAX;
		else if (step < STEP_MIN) step = STEP_MIN;
#if OVERSAMPLING
		i = 0;
		delta = (signal  << SIGNAL_SHIFT_L) - old_signal;
        while (counter > 0 && left > 0) {
			*buffer++ = old_signal + delta * i / oversampling;
			if (++i == oversampling) i = 0;
			counter -= voice->freq;
			left--;
		}
		old_signal = (signal  << SIGNAL_SHIFT_L);
#else
		while (counter > 0 && left > 0) {
			*buffer++ = (signal << SIGNAL_SHIFT_L);
			counter -= voice->freq;
            left--;
        }
#endif
		counter += emulation_rate;

        /* next! */
        if (++sample > count) {
			/* if we're not streaming, fill with silence and stop */
			if (voice->base != voice->stream) {
				while (left--)
					*buffer++ = (voice->signal << SIGNAL_SHIFT_L);
				voice->playing = 0;
			} else {
				/* if we are streaming, pad with the last sample */
				short last = buffer[-1];
				while (left--)
					*buffer++ = last;
			}
			break;
		}
	}
	/* update the parameters */
	voice->sample = sample;
	voice->signal = signal;
#if OVERSAMPLING
	voice->old_signal = old_signal;
#endif
    voice->step = step;
	voice->counter = counter;
}

/*
 *   Update emulation of an uPD7759 output stream
 */
static void UPD7759_update (int chip, void *buffer, int left)
{
	struct UPD7759voice *voice = &updadpcm[chip];
	int i;

	/* see if there's actually any need to generate samples */

	if (left > 0) {
        /* if this voice is active */
		if (voice->playing) {
			if (upd7759_intf->mode == UPD7759_SLAVE_MODE) {
				if (bits == 16) {
					short *p = buffer;
					while (left--)
						*p++ = (voice->signal << SIGNAL_SHIFT_L);
				} else {
					 memset(buffer, AUDIO_CONV(voice->signal >> SIGNAL_SHIFT_R), left);
                }
            } else {
				if (bits == 16)
					update_16bit(voice, buffer, left);
				else
					update_8bit(voice, buffer, left);
			}
		} else {
			/* voice is not playing */
			if (bits == 16) {
				short *p = buffer;
				for (i = 0; i < left; i++)
					*p++ = (voice->signal << SIGNAL_SHIFT_L);
			} else {
				memset ((unsigned char *)buffer, AUDIO_CONV(voice->signal >> SIGNAL_SHIFT_R), left);

			}
		}
	}
}

/************************************************************
 UPD7759_message_w

 Store the inputs to I0-I7 externally to the uPD7759.

 I0-I7 input the message number of the message to be
 reproduced. The inputs are latched at the rising edge of the
 !ST input. Unused pins should be grounded.

 In slave mode it seems like the ADPCM data is stuffed
 here from an external source (eg. Z80 NMI code).
 *************************************************************/

void UPD7759_message_w (int num, int data)
{
	struct UPD7759voice *voice = updadpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= upd7759_intf->num) {
		LOG(1,(errorlog,"error: UPD7759_SNDSELECT() called with channel = %d, but only %d channels allocated\n", num, upd7759_intf->num));
		return;
	}

	if (upd7759_intf->mode == UPD7759_SLAVE_MODE) {
		int offset = -1;

		//LOG(1,(errorlog,"upd7759_message_w $%02x\n", data));
		if (errorlog) fprintf(errorlog,"upd7759_message_w $%2x\n",data);

        switch (data) {

			case 0x00: 							/* roms 0x10000 & 0x20000 in size */
			case 0x38: offset = 0x10000; break; /* roms 0x8000 in size */

			case 0x01: 							/* roms 0x10000 & 0x20000 in size */
			case 0x39: offset = 0x14000; break; /* roms 0x8000 in size */

			case 0x02: 							/* roms 0x10000 & 0x20000 in size */
			case 0x34: offset = 0x18000; break; /* roms 0x8000 in size */

			case 0x03: 							/* roms 0x10000 & 0x20000 in size */
			case 0x35: offset = 0x1c000; break; /* roms 0x8000 in size */

			case 0x04:							/* roms 0x10000 & 0x20000 in size */
			case 0x2c: offset = 0x20000; break; /* roms 0x8000 in size */

			case 0x05: 							/* roms 0x10000 & 0x20000 in size */
			case 0x2d: offset = 0x24000; break; /* roms 0x8000 in size */

			case 0x06:							/* roms 0x10000 & 0x20000 in size */
			case 0x1c: offset = 0x28000; break;	/* roms 0x8000 in size in size */

			case 0x07: 							/* roms 0x10000 & 0x20000 in size */
			case 0x1d: offset = 0x2c000; break;	/* roms 0x8000 in size */

			case 0x08: offset = 0x30000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x09: offset = 0x34000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0a: offset = 0x38000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0b: offset = 0x3c000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0c: offset = 0x40000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0d: offset = 0x44000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0e: offset = 0x48000; break; /* roms 0x10000 & 0x20000 in size */
			case 0x0f: offset = 0x4c000; break; /* roms 0x10000 & 0x20000 in size */

			default:

				//LOG(1,(errorlog, "upd7759_message_w unhandled $%02x\n", data));
				if (errorlog) fprintf (errorlog, "upd7759_message_w unhandled $%02x\n", data);
				if ((data & 0xc0) == 0xc0) {
					if (voice->timer) {
						timer_remove(voice->timer);
						voice->timer = 0;
					}
					voice->playing = 0;
				}
        }
		if (offset > 0) {
			voice->base = &Machine->memory_region[upd7759_intf->region][offset];
			//LOG(1,(errorlog, "upd7759_message_w set base $%08x\n", offset));
		if (errorlog)fprintf(errorlog, "upd7759_message_w set base $%08x\n", offset);
        }

    } else {

		LOG(1,(errorlog,"uPD7759 calling sample : %d\n", data));
		sampnum[num] = data;

    }
}

/************************************************************
 UPD7759_dac

 Called by the timer interrupt at twice the sample rate.
 The first time the external irq callback is called, the
 second time the ADPCM msb is converted and the resulting
 signal is sent to the DAC.
 ************************************************************/
static void UPD7759_dac(int num)
{
	static int dac_msb = 0;
	struct UPD7759voice *voice = updadpcm + num;

	dac_msb ^= 1;
	if (dac_msb) {

		/* conversion of the ADPCM data to a new signal value */
		stream_update(channel[num], 0);
        /* convert lower nibble */
        voice->signal += diff_lookup[voice->step * 16 + (voice->sample & 15)];
		if (voice->signal > SIGNAL_MAX) voice->signal = SIGNAL_MAX;
		else if (voice->signal < SIGNAL_MIN) voice->signal = SIGNAL_MIN;
		voice->step += index_shift[voice->sample & 7];
		if (voice->step > STEP_MAX) voice->step = STEP_MAX;
		else if (voice->step < STEP_MIN) voice->step = STEP_MIN;

    } else {

        if (upd7759_intf->irqcallback[num])
			(*upd7759_intf->irqcallback[num])(num);

    }
}

/************************************************************
 UPD7759_start_w

 !ST pin:
 Setting the !ST input low while !CS is low will start
 speech reproduction of the message in the speech ROM locations
 addressed by the contents of I0-I7. If the device is in
 standby mode, standby mode will be released.
 NOTE: While !BUSY is low, another !ST will not be accepted.
 *************************************************************/

void UPD7759_start_w (int num, int data)
{
	struct UPD7759voice *voice = updadpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= upd7759_intf->num) {
		LOG(1,(errorlog,"error: UPD7759_play_stop() called with channel = %d, but only %d channels allocated\n", num, upd7759_intf->num));
		return;
	}

	/* handle the slave mode */
    if (upd7759_intf->mode == UPD7759_SLAVE_MODE) {

		if (voice->playing) {

            /* if the chip is busy this should be the ADPCM data */
			data &= 0xff;	/* be sure to use 8 bits value only */
			LOG(3,(errorlog,"UPD7759_data_w: $%02x\n", data));

            /* detect end of a sample by inspection of the last 5 bytes */
			/* FF 00 00 00 00 is the start of the next sample */
			if (voice->count > 5 && voice->sample == 0xff000000 && data == 0x00) {
				/* remove an old timer */
				if (voice->timer) {
					timer_remove(voice->timer);
					voice->timer = 0;
				}
				/* stop playing this sample */
				voice->playing = 0;
				return;
            }

            /* collect the data written in voice->sample */
			voice->sample = (voice->sample << 8) | (data & 0xff);
			voice->count++;

			stream_update(channel[num], 0);
            /* conversion of the ADPCM data to a new signal value */

            /* convert the upper nibble */
            voice->signal += diff_lookup[voice->step * 16 + ((voice->sample >> 4) & 15)];
            if (voice->signal > SIGNAL_MAX) voice->signal = SIGNAL_MAX;
            else if (voice->signal < SIGNAL_MIN) voice->signal = SIGNAL_MIN;
			voice->step += index_shift[(voice->sample >> 4) & 7];
            if (voice->step > STEP_MAX) voice->step = STEP_MAX;
            else if (voice->step < STEP_MIN) voice->step = STEP_MIN;

        } else {

			LOG(2,(errorlog,"UPD7759_start_w: $%02x\n", data));

            /* remove an old timer */
            if (voice->timer) {
                timer_remove(voice->timer);
                voice->timer = 0;
            }
            /* start a new timer */
			voice->timer = timer_pulse( TIME_IN_HZ(base_rate), num, UPD7759_dac );
			/* reset the step width */
            voice->step = STEP_ZERO;
            /* reset count for the detection of an sample ending */
            voice->count = 0;
			/* this voice is now playing */
            voice->playing = 1;

        }

	} else {
		struct UPD7759sample sample;

		/* bail if the chip is busy */
		if (voice->playing)
			return;

		LOG(2,(errorlog,"UPD7759_start_w: %d\n", data));

		/* find a match */
		if (find_sample(sampnum[num],&sample))
		{
			/* update the  voice */
			stream_update(channel[num], 0);
			voice->freq = sample.freq;
			/* set up the voice to play this sample */
			voice->playing = 1;
			voice->base = &Machine->memory_region[upd7759_intf->region][sample.offset];
			voice->sample = 0;
			/* sample length needs to be doubled (counting nibbles) */
			voice->count = sample.length * 2;

			/* also reset the chip parameters */
			voice->step = STEP_ZERO;
			voice->counter = emulation_rate / 2;

			return;
		}

		LOG(1,(errorlog,"warning: UPD7759_playing_w() called with invalid number = %08x\n",data));
	}
}

/************************************************************
 UPD7759_data_r

 External read data from the UPD7759 memory region based
 on voice->base. Used in slave mode to retrieve data to
 stuff into UPD7759_message_w.
 *************************************************************/

int UPD7759_data_r(int num, int offs)
{
	struct UPD7759voice *voice = updadpcm + num;

    /* If there's no sample rate, do nothing */
    if (Machine->sample_rate == 0)
		return 0x00;

    /* range check the numbers */
    if ( num >= upd7759_intf->num ) {
		LOG(1,(errorlog,"error: UPD7759_data_r() called with channel = %d, but only %d channels allocated\n", num, upd7759_intf->num));
		return 0x00;
    }

#if VERBOSE
    if (!(offs&0xff)) LOG(1, (errorlog,"UPD7759#%d sample offset = $%04x\n", num, offs));
#endif

	return voice->base[offs];
}

/* helper functions to be used as memory read handler function pointers */
int UPD7759_0_data_r(int offs) { return UPD7759_data_r(0, offs); }
int UPD7759_1_data_r(int offs) { return UPD7759_data_r(1, offs); }

/************************************************************
 UPD7759_busy_r

 !BUSY pin:
 !BUSY outputs the status of the uPD7759. It goes low during
 speech decode and output operations. When !ST is received,
 !BUSY goes low. While !BUSY is low, another !ST will not be
 accepted. In standby mode, !BUSY becomes high impedance. This
 is an active low output.
 *************************************************************/

int UPD7759_busy_r (int num)
{
	struct UPD7759voice *voice = updadpcm + num;

	/* If there's no sample rate, return not busy */
	if ( Machine->sample_rate == 0 )
		return 1;

	/* range check the numbers */
	if ( num >= upd7759_intf->num ) {
		LOG(1,(errorlog,"error: UPD7759_busy_r() called with channel = %d, but only %d channels allocated\n", num, upd7759_intf->num));
		return 1;
	}

	/* bring the chip in sync with the CPU */
	stream_update(channel[num], 0);

	if ( voice->playing == 0 ) {
		LOG(1,(errorlog,"uPD7759 not busy\n"));
		return 1;
	} else {
		LOG(1,(errorlog,"uPD7759 busy\n"));
		return 0;
	}

	return 1;
}

/************************************************************
 UPD7759_reset_w

 !RESET pin:
 The !RESET input initialized the chip. Use !RESET following
 power-up to abort speech reproduction or to release standby
 mode. !RESET must remain low at least 12 oscillator clocks.
 At power-up or when recovering from standby mode, !RESET
 must remain low at least 12 more clocks after clock
 oscillation stabilizes.
 *************************************************************/

void UPD7759_reset_w (int num, int data)
{
	int i;
	struct UPD7759voice *voice = updadpcm + num;

	/* If there's no sample rate, do nothing */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if ( num >= upd7759_intf->num ) {
		LOG(1,(errorlog,"error: UPD7759_reset_w() called with channel = %d, but only %d channels allocated\n", num, upd7759_intf->num));
		return;
	}

	/* if !RESET is high, do nothing */
	if (data > 0)
		return;

	/* mark the uPD7759 as NOT PLAYING */
	/* (Note: do we need to do anything else?) */
	voice->playing = 0;
}

