/**********************************************************************************************
 *
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
 *   R. Belmont 31/10/2003
 *   Updated to allow a driver to use both MSM6295s and "raw" ADPCM voices (gcpinbal)
 *   Also added some error trapping for MAME_DEBUG builds
 *
 **********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "state.h"
#include "okim6295.h"

#define MAX_SAMPLE_CHUNK	10000

#define FRAC_BITS			14
#define FRAC_ONE			(1 << FRAC_BITS)
#define FRAC_MASK			(FRAC_ONE - 1)


/* struct describing a single playing ADPCM voice */
struct ADPCMVoice
{
	UINT8 playing;			/* 1 if we are actively playing */

	UINT32 base_offset;		/* pointer to the base memory location */
	UINT32 sample;			/* current sample number */
	UINT32 count;			/* total samples to play */

	UINT32 signal;			/* current ADPCM signal */
	UINT32 step;			/* current ADPCM step */
	UINT32 volume;			/* output volume */

	INT16 last_sample;		/* last sample output */
	INT16 curr_sample;		/* current sample target */
	UINT32 source_step;		/* step value for frequency conversion */
	UINT32 source_pos;		/* current fractional position */
};

struct okim6295
{
	#define OKIM6295_VOICES		4
	struct ADPCMVoice voice[OKIM6295_VOICES];
	INT32 command;
	INT32 bank_offset;
	UINT8 *region_base;		/* pointer to the base of the region */
	sound_stream *stream;	/* which stream are we playing on? */
};

/* step size index shift table */
static const int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

/* volume lookup table */
static UINT32 volume_table[16];

/* useful interfaces */
const struct OKIM6295interface okim6295_interface_region_1 = { REGION_SOUND1 };
const struct OKIM6295interface okim6295_interface_region_2 = { REGION_SOUND2 };
const struct OKIM6295interface okim6295_interface_region_3 = { REGION_SOUND3 };
const struct OKIM6295interface okim6295_interface_region_4 = { REGION_SOUND4 };


/**********************************************************************************************

     compute_tables -- compute the difference tables

***********************************************************************************************/

static void compute_tables(void)
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
		int stepval = floor(16.0 * pow(11.0 / 10.0, (double)step));

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

	/* generate the OKI6295 volume table */
	for (step = 0; step < 16; step++)
	{
		double out = 256.0;
		int vol = step;

		/* 3dB per step */
		while (vol-- > 0)
			out /= 1.412537545;	/* = 10 ^ (3/20) = 3dB */
		volume_table[step] = (UINT32)out;
	}
}



/**********************************************************************************************

     generate_adpcm -- general ADPCM decoding routine

***********************************************************************************************/

static void generate_adpcm(struct okim6295 *chip, struct ADPCMVoice *voice, INT16 *buffer, int samples)
{
	/* if this voice is active */
	if (voice->playing)
	{
		UINT8 *base = chip->region_base + chip->bank_offset + voice->base_offset;
		int sample = voice->sample;
		int signal = voice->signal;
		int count = voice->count;
		int step = voice->step;
		int val;

		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			val = base[sample / 2] >> (((sample & 1) << 2) ^ 4);
			signal += diff_lookup[step * 16 + (val & 15)];

			/* clamp to the maximum */
			if (signal > 2047)
				signal = 2047;
			else if (signal < -2048)
				signal = -2048;

			/* adjust the step size and clamp */
			step += index_shift[val & 7];
			if (step > 48)
				step = 48;
			else if (step < 0)
				step = 0;

			/* output to the buffer, scaling by the volume */
			*buffer++ = signal * voice->volume / 16;
			samples--;

			/* next! */
			if (++sample >= count)
			{
				voice->playing = 0;
				break;
			}
		}

		/* update the parameters */
		voice->sample = sample;
		voice->signal = signal;
		voice->step = step;
	}

	/* fill the rest with silence */
	while (samples--)
		*buffer++ = 0;
}



/**********************************************************************************************
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
***********************************************************************************************/


/**********************************************************************************************

     okim6295_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void okim6295_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	struct okim6295 *chip = param;
	int i;
	
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	
	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		struct ADPCMVoice *voice = &chip->voice[i];
		INT16 sample_data[MAX_SAMPLE_CHUNK], *curr_data = sample_data;
		INT16 prev = voice->last_sample, curr = voice->curr_sample;
		UINT32 final_pos;
		UINT32 new_samples;
		stream_sample_t *buffer = outputs[0];
		int length = samples;

		/* finish off the current sample */
		if (voice->source_pos > 0)
		{
			/* interpolate */
			while (length > 0 && voice->source_pos < FRAC_ONE)
			{
				*buffer++ += (((INT32)prev * (INT32)(FRAC_ONE - voice->source_pos)) + ((INT32)curr * (INT32)voice->source_pos)) >> FRAC_BITS;
				voice->source_pos += voice->source_step;
				length--;
			}

			/* if we're over, continue; otherwise, we're done */
			if (voice->source_pos >= FRAC_ONE)
				voice->source_pos -= FRAC_ONE;
			else
				continue;
		}

		/* compute how many new samples we need */
		final_pos = voice->source_pos + length * voice->source_step;
		new_samples = (final_pos + FRAC_ONE - 1) >> FRAC_BITS;
		if (new_samples > MAX_SAMPLE_CHUNK)
			new_samples = MAX_SAMPLE_CHUNK;

		/* generate them into our buffer */
		generate_adpcm(chip, voice, sample_data, new_samples);
		prev = curr;
		curr = *curr_data++;

		/* then sample-rate convert with linear interpolation */
		while (length > 0)
		{
			/* interpolate */
			while (length > 0 && voice->source_pos < FRAC_ONE)
			{
				*buffer++ += (((INT32)prev * (INT32)(FRAC_ONE - voice->source_pos)) + ((INT32)curr * (INT32)voice->source_pos)) >> FRAC_BITS;
				voice->source_pos += voice->source_step;
				length--;
			}

			/* if we're over, grab the next samples */
			if (voice->source_pos >= FRAC_ONE)
			{
				voice->source_pos -= FRAC_ONE;
				prev = curr;
				curr = *curr_data++;
			}
		}

		/* remember the last samples */
		voice->last_sample = prev;
		voice->curr_sample = curr;
	}
}



/**********************************************************************************************

     state save support for MAME

***********************************************************************************************/

static void adpcm_state_save_register(struct ADPCMVoice *voice, int i)
{
	char buf[20];

	sprintf(buf,"ADPCM");

	state_save_register_UINT8  (buf, i, "playing", &voice->playing, 1);
	state_save_register_UINT32 (buf, i, "sample" , &voice->sample,  1);
	state_save_register_UINT32 (buf, i, "count"  , &voice->count,   1);
	state_save_register_UINT32 (buf, i, "signal" , &voice->signal,  1);
	state_save_register_UINT32 (buf, i, "step"   , &voice->step,    1);
	state_save_register_UINT32 (buf, i, "volume" , &voice->volume,  1);

	state_save_register_INT16  (buf, i, "last_sample", &voice->last_sample, 1);
	state_save_register_INT16  (buf, i, "curr_sample", &voice->curr_sample, 1);
	state_save_register_UINT32 (buf, i, "source_step", &voice->source_step, 1);
	state_save_register_UINT32 (buf, i, "source_pos" , &voice->source_pos,  1);
	state_save_register_UINT32 (buf, i, "base_offset" , &voice->base_offset,  1);
}

static void okim6295_state_save_register(struct okim6295 *info, int sndindex)
{
	int j;
	char buf[20];

	sprintf(buf,"OKIM6295");

	state_save_register_INT32  (buf, sndindex, "command", &info->command, 1);
	state_save_register_INT32  (buf, sndindex, "bank_offset", &info->bank_offset, 1);
	for (j = 0; j < OKIM6295_VOICES; j++)
		adpcm_state_save_register(&info->voice[j], sndindex * 4 + j);
}



/**********************************************************************************************

     OKIM6295_start -- start emulation of an OKIM6295-compatible chip

***********************************************************************************************/

static void *okim6295_start(int sndindex, int clock, const void *config)
{
	const struct OKIM6295interface *intf = config;
	struct okim6295 *info;
	int voice;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	compute_tables();

	info->command = -1;
	info->bank_offset = 0;
	info->region_base = memory_region(intf->region);

	/* generate the name and create the stream */
	info->stream = stream_create(0, 1, Machine->sample_rate, info, okim6295_update);

	/* initialize the voices */
	for (voice = 0; voice < OKIM6295_VOICES; voice++)
	{
		/* initialize the rest of the structure */
		info->voice[voice].volume = 255;
		info->voice[voice].signal = -2;
		if (Machine->sample_rate)
			info->voice[voice].source_step = (UINT32)((double)clock * (double)FRAC_ONE / (double)Machine->sample_rate);
	}

	okim6295_state_save_register(info, sndindex);

	/* success */
	return info;
}



/**********************************************************************************************

     OKIM6295_stop -- stop emulation of an OKIM6295-compatible chip

***********************************************************************************************/

static void okim6295_reset(void *chip)
{
	struct okim6295 *info = chip;
	int i;
	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		stream_update(info->stream, 0);
		info->voice[i].playing = 0;
	}
}



/**********************************************************************************************

     OKIM6295_set_bank_base -- set the base of the bank for a given voice on a given chip

***********************************************************************************************/

void OKIM6295_set_bank_base(int which, int base)
{
	struct okim6295 *info = sndti_token(SOUND_OKIM6295, which);
	stream_update(info->stream, 0);
	info->bank_offset = base;
}



/**********************************************************************************************

     OKIM6295_set_frequency -- dynamically adjusts the frequency of a given ADPCM voice

***********************************************************************************************/

void OKIM6295_set_frequency(int which, int frequency)
{
	struct okim6295 *info = sndti_token(SOUND_OKIM6295, which);
	int channel;

	stream_update(info->stream, 0);
	for (channel = 0; channel < OKIM6295_VOICES; channel++)
	{
		struct ADPCMVoice *voice = &info->voice[channel];

		/* update the stream and set the new base */
		if (Machine->sample_rate)
			voice->source_step = (UINT32)((double)frequency * (double)FRAC_ONE / (double)Machine->sample_rate);
	}
}


/**********************************************************************************************

     OKIM6295_status_r -- read the status port of an OKIM6295-compatible chip

***********************************************************************************************/

static int OKIM6295_status_r(int num)
{
	struct okim6295 *info = sndti_token(SOUND_OKIM6295, num);
	int i, result;

	result = 0xf0;	/* naname expects bits 4-7 to be 1 */

	/* set the bit to 1 if something is playing on a given channel */
	stream_update(info->stream, 0);
	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		struct ADPCMVoice *voice = &info->voice[i];

		/* set the bit if it's playing */
		if (voice->playing)
			result |= 1 << i;
	}

	return result;
}



/**********************************************************************************************

     OKIM6295_data_w -- write to the data port of an OKIM6295-compatible chip

***********************************************************************************************/

static void OKIM6295_data_w(int num, int data)
{
	struct okim6295 *info = sndti_token(SOUND_OKIM6295, num);

	/* if a command is pending, process the second half */
	if (info->command != -1)
	{
		int temp = data >> 4, i, start, stop;
		unsigned char *base;

		/* update the stream */
		stream_update(info->stream, 0);

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte) */
		for (i = 0; i < OKIM6295_VOICES; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &info->voice[i];

				if (Machine->sample_rate == 0) return;

				/* determine the start/stop positions */
				base = &info->region_base[info->bank_offset + info->command * 8];
				start = ((base[0] << 16) + (base[1] << 8) + base[2]) & 0x3ffff;
				stop  = ((base[3] << 16) + (base[4] << 8) + base[5]) & 0x3ffff;

				/* set up the voice to play this sample */
				if (start < stop)
				{
					if (!voice->playing) /* fixes Got-cha and Steel Force */
					{
						voice->playing = 1;
						voice->base_offset = start;
						voice->sample = 0;
						voice->count = 2 * (stop - start + 1);

						/* also reset the ADPCM parameters */
						voice->signal = -2;
						voice->step = 0;
						voice->volume = volume_table[data & 0x0f];
					}
					else
					{
						logerror("OKIM6295:%d requested to play sample %02x on non-stopped voice\n",num,info->command);
					}
				}
				/* invalid samples go here */
				else
				{
					logerror("OKIM6295:%d requested to play invalid sample %02x\n",num,info->command);
					voice->playing = 0;
				}
			}
		}

		/* reset the command */
		info->command = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		info->command = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i;

		/* update the stream, then turn it off */
		stream_update(info->stream, 0);

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < 4; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &info->voice[i];

				voice->playing = 0;
			}
		}
	}
}



/**********************************************************************************************

     OKIM6295_status_0_r -- generic status read functions
     OKIM6295_status_1_r

***********************************************************************************************/

READ8_HANDLER( OKIM6295_status_0_r )
{
	return OKIM6295_status_r(0);
}

READ8_HANDLER( OKIM6295_status_1_r )
{
	return OKIM6295_status_r(1);
}

READ8_HANDLER( OKIM6295_status_2_r )
{
	return OKIM6295_status_r(2);
}

READ16_HANDLER( OKIM6295_status_0_lsb_r )
{
	return OKIM6295_status_r(0);
}

READ16_HANDLER( OKIM6295_status_1_lsb_r )
{
	return OKIM6295_status_r(1);
}

READ16_HANDLER( OKIM6295_status_2_lsb_r )
{
	return OKIM6295_status_r(2);
}

READ16_HANDLER( OKIM6295_status_0_msb_r )
{
	return OKIM6295_status_r(0) << 8;
}

READ16_HANDLER( OKIM6295_status_1_msb_r )
{
	return OKIM6295_status_r(1) << 8;
}

READ16_HANDLER( OKIM6295_status_2_msb_r )
{
	return OKIM6295_status_r(2) << 8;
}



/**********************************************************************************************

     OKIM6295_data_0_w -- generic data write functions
     OKIM6295_data_1_w

***********************************************************************************************/

WRITE8_HANDLER( OKIM6295_data_0_w )
{
	OKIM6295_data_w(0, data);
}

WRITE8_HANDLER( OKIM6295_data_1_w )
{
	OKIM6295_data_w(1, data);
}

WRITE8_HANDLER( OKIM6295_data_2_w )
{
	OKIM6295_data_w(2, data);
}

WRITE16_HANDLER( OKIM6295_data_0_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(0, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_1_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(1, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_2_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(2, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_0_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(0, data >> 8);
}

WRITE16_HANDLER( OKIM6295_data_1_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(1, data >> 8);
}

WRITE16_HANDLER( OKIM6295_data_2_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(2, data >> 8);
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void okim6295_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void okim6295_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = okim6295_set_info;		break;
		case SNDINFO_PTR_START:							info->start = okim6295_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = okim6295_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "OKI6295";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "OKI ADPCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

