/*
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *	 HJB 08/31/98
 *	 modified to use an automatically selected oversampling factor
 *	 for the current Machine->sample_rate
 *
 *   Mish 21/7/99
 *   Updated to allow multiple OKI chips with different sample rates
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "adpcm.h"

#define OVERSAMPLING	0	/* breaks 10 Yard Fight */


/* struct describing a single playing ADPCM voice */
struct ADPCMVoice
{
	int playing;            /* 1 if we are actively playing */
	int channel;            /* which channel are we playing on? */
	unsigned char *base;    /* pointer to the base memory location */
	unsigned char *stream;  /* input stream data (if any) */
	void *buffer;           /* output buffer (could be 8 or 16 bits) */
	int bufpos;             /* current position in the buffer */
	int mask;               /* mask to keep us within the buffer */
	int sample;             /* current sample number */
	int count;              /* total samples to play */
	int signal;             /* current ADPCM signal */
	int step;               /* current ADPCM step */
	int volume;             /* output volume */

	/* Voice buffer length and emulation output rate */
	int buffer_len;
	int emulation_rate;
	#if OVERSAMPLING
	int oversampling;
	#endif
};

/* global pointer to the current interface */
static const struct ADPCMinterface *adpcm_intf;

/* global pointer to the current array of samples */
static struct ADPCMsample *sample_list;

/* array of ADPCM voices */
static struct ADPCMVoice adpcm[MAX_ADPCM];

/* sound channel info */
static int channel;

/* step size index shift table */
static int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];



/*
 *   Compute the difference table
 */

static void ComputeTables (void)
{
	/* nibble to bit map */
	static int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};

	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= 48; step++)
	{
		/* compute the step value */
		int stepval = floor (16.0 * pow (11.0 / 10.0, (double)step));

		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
		}
	}
}



/*
 *   Start emulation of several ADPCM output streams
 */

int ADPCM_sh_start (const struct MachineSound *msound)
{
	int i;

	/* compute the difference tables */
	ComputeTables ();

	/* copy the interface pointer to a global */
	adpcm_intf = msound->sound_interface;

	/* set the default sample table */
	sample_list = (struct ADPCMsample *)Machine->gamedrv->sound_prom;

	/* if there's an init function, call it now to generate the table */
	if (adpcm_intf->init)
	{
		sample_list = malloc (257 * sizeof (struct ADPCMsample));
		if (!sample_list)
			return 1;
		memset (sample_list, 0, 257 * sizeof (struct ADPCMsample));
		(*adpcm_intf->init) (adpcm_intf, sample_list, 256);
	}

	/* reserve sound channels */
	channel = mixer_allocate_channels(adpcm_intf->num,adpcm_intf->mixing_level);

	/* initialize the voices */
	memset (adpcm, 0, sizeof (adpcm));
	for (i = 0; i < adpcm_intf->num; i++)
	{
		char name[40];

		sprintf(name,"%s #%d",sound_name(msound),i);
		mixer_set_name(channel+i,name);

		/* compute the emulation rate and buffer size */
		#if OVERSAMPLING
		adpcm[i].oversampling = (adpcm_intf->frequency) ? Machine->sample_rate / adpcm_intf->frequency + 1 : 1;
		if (errorlog) fprintf(errorlog, "adpcm voice %d: using %d times oversampling\n", i, adpcm[i].oversampling);
	    adpcm[i].buffer_len = adpcm_intf->frequency * adpcm[i].oversampling / Machine->drv->frames_per_second;
		#else
		adpcm[i].buffer_len = adpcm_intf->frequency / Machine->drv->frames_per_second;
		#endif
		adpcm[i].emulation_rate = adpcm[i].buffer_len * Machine->drv->frames_per_second;

		adpcm[i].channel = channel + i;
		adpcm[i].mask = 0xffffffff;
		adpcm[i].signal = -2;
		adpcm[i].volume = 255;

		/* allocate an output buffer */
		adpcm[i].buffer = malloc (adpcm[i].buffer_len * Machine->sample_bits / 8);
		if (!adpcm[i].buffer)
			return 1;
	}

	/* success */
	return 0;
}



/*
 *   Stop emulation of several ADPCM output streams
 */

void ADPCM_sh_stop (void)
{
	int i;

	/* free the temporary table if we created it */
	if (sample_list && sample_list != (struct ADPCMsample *)Machine->gamedrv->sound_prom)
		free (sample_list);
	sample_list = 0;

	/* free any output and streaming buffers */
	for (i = 0; i < adpcm_intf->num; i++)
	{
		if (adpcm[i].stream)
			free (adpcm[i].stream);
		if (adpcm[i].buffer)
			free (adpcm[i].buffer);
	}
}



/*
 *   Update emulation of an ADPCM output stream
 */

static void ADPCM_update (struct ADPCMVoice *voice, int finalpos)
{
	int left = finalpos - voice->bufpos;

	/* see if there's actually any need to generate samples */
	if (left > 0)
	{
		/* if this voice is active */
		if (voice->playing)
		{
			unsigned char *base = voice->base;
			int sample = voice->sample;
			int signal = voice->signal;
			int count = voice->count;
			int step = voice->step;
			int mask = voice->mask;
			int val;
#if OVERSAMPLING
            int oldsignal = signal;
			int delta, i;
#endif

			/* 16-bit case */
			if (Machine->sample_bits == 16)
			{
				short *buffer = (short *)voice->buffer + voice->bufpos;

				while (left)
				{
					/* compute the new amplitude and update the current step */
					val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
					signal += diff_lookup[step * 16 + (val & 15)];
					if (signal > 2047) signal = 2047;
					else if (signal < -2048) signal = -2048;
					step += index_shift[val & 7];
					if (step > 48) step = 48;
					else if (step < 0) step = 0;

#if OVERSAMPLING
                    /* antialiasing samples */
                    delta = signal - oldsignal;
					for (i = 1; left && i <= voice->oversampling; left--, i++)
						*buffer++ = (oldsignal + delta * i / voice->oversampling) * voice->volume / 16;
					oldsignal = signal;
#else
					*buffer++ = signal * voice->volume / 16;
					left--;
#endif

                    /* next! */
					if (++sample > count)
					{
						/* if we're not streaming, fill with silence and stop */
						if (voice->base != voice->stream)
						{
							while (left--)
								*buffer++ = 0;
							voice->playing = 0;
						}

						/* if we are streaming, pad with the last sample */
						else
						{
							short last = buffer[-1];
							while (left--)
								*buffer++ = last;
						}
						break;
					}
				}
			}

			/* 8-bit case */
			else
			{
				unsigned char *buffer = (unsigned char *)voice->buffer + voice->bufpos;

				while (left)
				{
					/* compute the new amplitude and update the current step */
					val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
					signal += diff_lookup[step * 16 + (val & 15)];
					if (signal > 2047) signal = 2047;
					else if (signal < -2048) signal = -2048;
					step += index_shift[val & 7];
					if (step > 48) step = 48;
					else if (step < 0) step = 0;

#if OVERSAMPLING
                    delta = signal - oldsignal;
					for (i = 1; left && i <= voice->oversampling; left--, i++)
						*buffer++ = (oldsignal + delta * i / voice->oversampling) * voice->volume / (16 * 256);
                    oldsignal = signal;
#else
					*buffer++ = signal * voice->volume / (16 * 256);
					left--;
#endif
                    /* next! */
					if (++sample > count)
					{
						/* if we're not streaming, fill with silence and stop */
						if (voice->base != voice->stream)
						{
							while (left--)
								*buffer++ = 0;
							voice->playing = 0;
						}

						/* if we are streaming, pad with the last sample */
						else
						{
							unsigned char last = buffer[-1];
							while (left--)
								*buffer++ = last;
						}
						break;
					}
				}
			}

			/* update the parameters */
			voice->sample = sample;
			voice->signal = signal;
			voice->step = step;
		}

		/* voice is not playing */
		else
		{
			if (Machine->sample_bits == 16)
				memset ((short *)voice->buffer + voice->bufpos, 0, left * 2);
			else
				memset ((unsigned char *)voice->buffer + voice->bufpos, 0, left);
		}

		/* update the final buffer position */
		voice->bufpos = finalpos;
	}
}



/*
 *   Update emulation of several ADPCM output streams
 */

void ADPCM_sh_update (void)
{
	int i;

	/* bail if we're not emulating sound */
	if (Machine->sample_rate == 0)
		return;

	/* loop over voices */
	for (i = 0; i < adpcm_intf->num; i++)
	{
		struct ADPCMVoice *voice = adpcm + i;

		/* update to the end of buffer */
		ADPCM_update (voice, voice->buffer_len);

		/* play the result */
		if (Machine->sample_bits == 16)
			mixer_play_streamed_sample_16(voice->channel,voice->buffer,2*(voice->buffer_len),voice->emulation_rate);
		else
			mixer_play_streamed_sample(voice->channel,voice->buffer,voice->buffer_len,voice->emulation_rate);

		/* reset the buffer position */
		voice->bufpos = 0;
	}
}



/*
 *   Handle a write to the ADPCM data stream
 */

void ADPCM_trigger (int num, int which)
{
	struct ADPCMVoice *voice = adpcm + num;
	struct ADPCMsample *sample;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_trigger() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* find a match */
	for (sample = sample_list; sample->length > 0; sample++)
	{
		if (sample->num == which)
		{
			/* update the ADPCM voice */
			ADPCM_update (voice, cpu_scalebyfcount (voice->buffer_len));

			/* set up the voice to play this sample */
			voice->playing = 1;
			voice->base = &Machine->memory_region[adpcm_intf->region][sample->offset];
			voice->sample = 0;
			voice->count = sample->length;

			/* also reset the ADPCM parameters */
			voice->signal = -2;
			voice->step = 0;

			return;
		}
	}

if (errorlog) fprintf(errorlog,"warning: ADPCM_trigger() called with unknown trigger = %08x\n",which);
}



void ADPCM_play (int num, int offset, int length)
{
	struct ADPCMVoice *voice = adpcm + num;


	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_trigger() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (voice->buffer_len));

	/* set up the voice to play this sample */
	voice->playing = 1;
	voice->base = &Machine->memory_region[adpcm_intf->region][offset];
	voice->sample = 0;
	voice->count = length;

	/* also reset the ADPCM parameters */
	voice->signal = -2;
	voice->step = 0;
}



/*
 *   Stop playback on an ADPCM data channel
 */

void ADPCM_stop (int num)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_stop() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (voice->buffer_len));

	/* stop playback */
	voice->playing = 0;
}

/*
 *   Change volume on an ADPCM data channel
 */

void ADPCM_setvol (int num, int vol)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_setvol() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* update the ADPCM voice */
	ADPCM_update(voice, cpu_scalebyfcount (voice->buffer_len));

	voice->volume = vol;
}


/*
 *   Stop playback on an ADPCM data channel
 */

int ADPCM_playing (int num)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return 0;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_playing() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return 0;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (voice->buffer_len));

	/* return true if we're playing */
	return voice->playing;
}



/*
 *
 *	OKIM 6295 ADPCM chip:
 *
 *	Command bytes are sent:
 *
 *		1xxx xxxx = start of 2-byte command sequence, xxxxxxx is the sample number to trigger
 *		abcd vvvv = second half of command; one of the abcd bits is set to indicate which voice
 *		            the v bits seem to be volumed
 *
 *		0abc d000 = stop playing; one or more of the abcd bits is set to indicate which voice(s)
 *
 *	Status is read:
 *
 *		???? abcd = one bit per voice, set to 0 if nothing is playing, or 1 if it is active
 *
 */

/* Mish,  1/1/99: Added support for multiple chips with different playback regions
   Mish, 19/1/99: Implemented bottom 4 bits as volume, 0 = full volume
                  See Midnight Resistance drum track, sly spy bullets, etc
                  This is still relatively untested..
*/


static const struct OKIM6295interface *okim6295_interface;
static int okim6295_command[MAX_OKIM6295];
static int okim6295_base[MAX_OKIM6295][MAX_OKIM6295_VOICES];


/*
 *    Start emulation of an OKIM6295-compatible chip
 */

static int OKIM6295_sub_start (const struct MachineSound *msound)
{
	int i;

	/* compute the difference tables */
	ComputeTables ();

	/* copy the interface pointer to a global */
	adpcm_intf = msound->sound_interface;

	/* set the default sample table */
	sample_list = (struct ADPCMsample *)Machine->gamedrv->sound_prom;

	/* if there's an init function, call it now to generate the table */
	if (adpcm_intf->init)
	{
		sample_list = malloc (257 * sizeof (struct ADPCMsample));
		if (!sample_list)
			return 1;
		memset (sample_list, 0, 257 * sizeof (struct ADPCMsample));
		(*adpcm_intf->init) (adpcm_intf, sample_list, 256);
	}

	/* reserve sound channels */
	channel = mixer_allocate_channels(adpcm_intf->num,adpcm_intf->mixing_level);

	/* initialize the voices */
	memset (adpcm, 0, sizeof (adpcm));
	for (i = 0; i < adpcm_intf->num; i++)
	{
		char name[40];

		sprintf(name,"%s #%d",sound_name(msound),i);
		mixer_set_name(channel+i,name);

		/* compute the emulation rate and buffer size */
		#if OVERSAMPLING
		adpcm[i].oversampling = (okim6295_interface->frequency[i/4]) ? Machine->sample_rate / okim6295_interface->frequency[i/4] + 1 : 1;
		if (errorlog) fprintf(errorlog, "adpcm voice %d: using %d times oversampling\n", i, adpcm[i].oversampling);
	    adpcm[i].buffer_len = okim6295_interface->frequency[i/4] * adpcm[i].oversampling / Machine->drv->frames_per_second;
		#else
		adpcm[i].buffer_len = okim6295_interface->frequency[i/4] / Machine->drv->frames_per_second;
		#endif
		adpcm[i].emulation_rate = adpcm[i].buffer_len * Machine->drv->frames_per_second;

		adpcm[i].channel = channel + i;
		adpcm[i].mask = 0xffffffff;
		adpcm[i].signal = -2;
		adpcm[i].volume = 255;

		/* allocate an output buffer */
		adpcm[i].buffer = malloc (adpcm[i].buffer_len * Machine->sample_bits / 8);
		if (!adpcm[i].buffer)
			return 1;
	}

	/* success */
	return 0;
}

int OKIM6295_sh_start (const struct MachineSound *msound)
{
	static struct ADPCMinterface generic_interface;
	struct MachineSound msound_copy;
	int i,j;

	/* save a global pointer to our interface */
	okim6295_interface = msound->sound_interface;

	/* create an interface for the generic system here */
	generic_interface.num = 4 * okim6295_interface->num;
	generic_interface.frequency = okim6295_interface->frequency[0];
	generic_interface.region = okim6295_interface->region[0];
	generic_interface.init = 0;
	for (i = 0; i < okim6295_interface->num; i++)
		generic_interface.mixing_level[i*4+0] =
		generic_interface.mixing_level[i*4+1] =
		generic_interface.mixing_level[i*4+2] =
		generic_interface.mixing_level[i*4+3] = okim6295_interface->mixing_level[i];

	/* reset the parameters */
	for (i = 0; i < okim6295_interface->num; i++)
	{
		okim6295_command[i] = -1;
		for (j = 0; j < MAX_OKIM6295_VOICES; j++)
			okim6295_base[i][j] = 0;
	}

	/* initialize it in the standard fashion */
	memcpy(&msound_copy,msound,sizeof(msound));
	msound_copy.sound_interface = &generic_interface;
	return OKIM6295_sub_start (&msound_copy);
}



/*
 *    Stop emulation of an OKIM6295-compatible chip
 */

void OKIM6295_sh_stop (void)
{
	/* standard stop */
	ADPCM_sh_stop ();
}



/*
 *    Update emulation of an OKIM6295-compatible chip
 */

void OKIM6295_sh_update (void)
{
	/* standard update */
	ADPCM_sh_update ();
}



/*
 *    Update emulation of an OKIM6295-compatible chip
 */

void OKIM6295_set_bank_base (int which, int voice, int base)
{
	/* set the base offset */
	if (voice != ALL_VOICES)
	{
		okim6295_base[which][voice] = base;
	}
	else
	{
		int i;
		for (i=0; i < MAX_OKIM6295_VOICES; i++)
			okim6295_base[which][i] = base;
	}
}



/*
 *    Read the status port of an OKIM6295-compatible chip
 */

static int OKIM6295_status_r (int num)
{
	int i, result, buffer_end;

	/* range check the numbers */
	if (num >= okim6295_interface->num)
	{
		if (errorlog) fprintf(errorlog,"error: OKIM6295_status_r() called with chip = %d, but only %d chips allocated\n", num, okim6295_interface->num);
		return 0x0f;
	}

	/* set the bit to 1 if something is playing on a given channel */
	result = 0;
	for (i = 0; i < 4; i++)
	{
		struct ADPCMVoice *voice = adpcm + num * 4 + i;

		/* precompute the end of buffer */
		buffer_end = cpu_scalebyfcount (voice->buffer_len);


		/* update the ADPCM voice */
		ADPCM_update (voice, buffer_end);

		/* set the bit if it's playing */
		if (voice->playing)
			result |= 1 << i;
	}

	return result;
}



/*
 *    Write to the data port of an OKIM6295-compatible chip
 */

static void OKIM6295_data_w (int num, int data)
{
	/* range check the numbers */
	if (num >= okim6295_interface->num)
	{
		if (errorlog) fprintf(errorlog,"error: OKIM6295_data_w() called with chip = %d, but only %d chips allocated\n", num, okim6295_interface->num);
		return;
	}

	/* if a command is pending, process the second half */
	if (okim6295_command[num] != -1)
	{
		int temp = data >> 4, i, start, stop, buffer_end;
		unsigned char *base;

		/* determine the start/stop positions */
//		base = &Machine->memory_region[okim6295_interface->region[num]][okim6295_base[num] + okim6295_command[num] * 8];
//		start = (base[0] << 16) + (base[1] << 8) + base[2];
//		stop = (base[3] << 16) + (base[4] << 8) + base[5];

		/* precompute the end of buffer */
//		buffer_end = cpu_scalebyfcount (buffer_len);

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte */
		for (i = 0; i < 4; i++)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = adpcm + num * 4 + i;

				/* precompute the end of buffer */
				buffer_end = cpu_scalebyfcount (voice->buffer_len);

				/* determine the start/stop positions */
				base = &Machine->memory_region[okim6295_interface->region[num]][okim6295_base[num][i] + okim6295_command[num] * 8];
				start = (base[0] << 16) + (base[1] << 8) + base[2];
				stop = (base[3] << 16) + (base[4] << 8) + base[5];

				/* update the ADPCM voice */
				ADPCM_update (voice, buffer_end);

				/* set up the voice to play this sample */
				if (start >= 0x40000 || stop >= 0x40000)	/* validity check */
				{
if (errorlog) fprintf(errorlog,"OKIM6295: requested to play invalid sample %02x\n",okim6295_command[num]);
					voice->playing = 0;
				}
				else
				{
					voice->playing = 1;
					voice->base = &Machine->memory_region[okim6295_interface->region[num]][okim6295_base[num][i] + start];
					voice->sample = 0;
					voice->count = 2 * (stop - start + 1);

					/* also reset the ADPCM parameters */
					voice->signal = -2;
					voice->step = 0;

					/* volume control is entirely guesswork */
					{
						double out;
						int vol;


						vol = data & 0x0f;
						out = 256;

						/* assume 2dB per step (most likely wrong!) */
						while (vol-- > 0)
							out /= 1.258925412;	/* = 10 ^ (2/20) = 2dB */

						voice->volume = out;
					}
				}
			}
			temp >>= 1;
		}

#if 0
if (errorlog) fprintf(errorlog,"oki%d(2 byte) %02x %02x (Voice %01x - %01x)\n",num,okim6295_command[num],data,data>>4,data&0xf);
#endif

		/* reset the command */
		okim6295_command[num] = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		okim6295_command[num] = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i, buffer_end;

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < 4; i++)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = adpcm + num * 4 + i;

				/* precompute the end of buffer */
				buffer_end = cpu_scalebyfcount (voice->buffer_len);

				/* update the ADPCM voice */
				ADPCM_update (voice, buffer_end);

				/* stop this guy from playing */
				voice->playing = 0;
			}
			temp >>= 1;
		}
	}
}

int OKIM6295_status_0_r (int num)
{
	return OKIM6295_status_r(0);
}

int OKIM6295_status_1_r (int num)
{
	return OKIM6295_status_r(1);
}

void OKIM6295_data_0_w (int num, int data)
{
	OKIM6295_data_w(0,data);
}

void OKIM6295_data_1_w (int num, int data)
{
	OKIM6295_data_w(1,data);
}
