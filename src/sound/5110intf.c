/******************************************************************************

     TMS5110 interface

     slightly modified from 5220intf by Jarek Burczynski

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5110.h"
#include "5110intf.h"


#define MAX_SAMPLE_CHUNK	10000

#define FRAC_BITS			14
#define FRAC_ONE			(1 << FRAC_BITS)
#define FRAC_MASK			(FRAC_ONE - 1)


/* the state of the streamed output */
struct tms5110_info
{
	const struct TMS5110interface *intf;
	INT16 last_sample, curr_sample;
	UINT32 source_step;
	UINT32 source_pos;
	sound_stream *stream;
	void *chip;
};


/* static function prototypes */
static void tms5110_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length);



/******************************************************************************

     tms5110_start -- allocate buffers and reset the 5110

******************************************************************************/

static void *tms5110_start(int sndindex, int clock, const void *config)
{
	static const struct TMS5110interface dummy = { 0 };
	struct tms5110_info *info;
	
	info = auto_malloc(sizeof(*info));
	info->intf = config ? config : &dummy;
	
	info->chip = tms5110_create();
	memset(info, 0, sizeof(*info));
	if (!info->chip)
		return NULL;
	sound_register_token(info);
	
	/* initialize a stream */
	info->stream = stream_create(0, 1, Machine->sample_rate, info, tms5110_update);

    if (info->intf->M0_callback==NULL)
    {
	logerror("\n file: 5110intf.c, tms5110_start(), line 53:\n  Missing _mandatory_ 'M0_callback' function pointer in the TMS5110 interface\n  This function is used by TMS5110 to call for a single bits\n  needed to generate the speech\n  Aborting startup...\n");
	return NULL;
    }
    tms5110_set_M0_callback(info->chip, info->intf->M0_callback );

    /* reset the 5110 */
    tms5110_reset_chip(info->chip);

    /* set the initial frequency */
    tms5110_set_frequency(clock);
    info->source_pos = 0;
    info->last_sample = info->curr_sample = 0;

    /* request a sound channel */
    return info;
}



/******************************************************************************

     tms5110_stop -- free buffers

******************************************************************************/

static void tms5110_stop(void *chip)
{
	struct tms5110_info *info = chip;
	tms5110_destroy(info->chip);
}


static void tms5110_reset(void *chip)
{
	struct tms5110_info *info = chip;
	tms5110_reset_chip(info->chip);
}



/******************************************************************************

     tms5110_CTL_w -- write Control Command to the sound chip
commands like Speech, Reset, etc., are loaded into the chip via the CTL pins

******************************************************************************/

WRITE8_HANDLER( tms5110_CTL_w )
{
	struct tms5110_info *info = sndti_token(SOUND_TMS5110, 0);

    /* bring up to date first */
    stream_update(info->stream, 0);
    tms5110_CTL_set(info->chip, data);
}

/******************************************************************************

     tms5110_PDC_w -- write to PDC pin on the sound chip

******************************************************************************/

WRITE8_HANDLER( tms5110_PDC_w )
{
	struct tms5110_info *info = sndti_token(SOUND_TMS5110, 0);

    /* bring up to date first */
    stream_update(info->stream, 0);
    tms5110_PDC_set(info->chip, data);
}



/******************************************************************************

     tms5110_status_r -- read status from the sound chip

******************************************************************************/

READ8_HANDLER( tms5110_status_r )
{
	struct tms5110_info *info = sndti_token(SOUND_TMS5110, 0);

    /* bring up to date first */
    stream_update(info->stream, 0);
    return tms5110_status_read(info->chip);
}



/******************************************************************************

     tms5110_ready_r -- return the not ready status from the sound chip

******************************************************************************/

int tms5110_ready_r(void)
{
	struct tms5110_info *info = sndti_token(SOUND_TMS5110, 0);

    /* bring up to date first */
    stream_update(info->stream, 0);
    return tms5110_ready_read(info->chip);
}



/******************************************************************************

     tms5110_update -- update the sound chip so that it is in sync with CPU execution

******************************************************************************/

static void tms5110_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct tms5110_info *info = param;
	INT16 sample_data[MAX_SAMPLE_CHUNK], *curr_data = sample_data;
	INT16 prev = info->last_sample, curr = info->curr_sample;
	stream_sample_t *buffer = _buffer[0];
	UINT32 final_pos;
	UINT32 new_samples;

	/* finish off the current sample */
	if (info->source_pos > 0)
	{
		/* interpolate */
		while (length > 0 && info->source_pos < FRAC_ONE)
		{
			*buffer++ = (((INT32)prev * (INT32)(FRAC_ONE - info->source_pos)) + ((INT32)curr * (INT32)info->source_pos)) >> FRAC_BITS;
			info->source_pos += info->source_step;
			length--;
		}

		/* if we're over, continue; otherwise, we're done */
		if (info->source_pos >= FRAC_ONE)
			info->source_pos -= FRAC_ONE;
		else
		{
			tms5110_process(info->chip, sample_data, 0);
			return;
		}
	}

	/* compute how many new samples we need */
	final_pos = info->source_pos + length * info->source_step;
	new_samples = (final_pos + FRAC_ONE - 1) >> FRAC_BITS;
	if (new_samples > MAX_SAMPLE_CHUNK)
		new_samples = MAX_SAMPLE_CHUNK;

	/* generate them into our buffer */
	tms5110_process(info->chip, sample_data, new_samples);
	prev = curr;
	curr = *curr_data++;

	/* then sample-rate convert with linear interpolation */
	while (length > 0)
	{
		/* interpolate */
		while (length > 0 && info->source_pos < FRAC_ONE)
		{
			*buffer++ = (((INT32)prev * (INT32)(FRAC_ONE - info->source_pos)) + ((INT32)curr * (INT32)info->source_pos)) >> FRAC_BITS;
			info->source_pos += info->source_step;
			length--;
		}

		/* if we're over, grab the next samples */
		if (info->source_pos >= FRAC_ONE)
		{
			info->source_pos -= FRAC_ONE;
			prev = curr;
			curr = *curr_data++;
		}
	}

	/* remember the last samples */
	info->last_sample = prev;
	info->curr_sample = curr;
}



/******************************************************************************

     tms5110_set_frequency -- adjusts the playback frequency

******************************************************************************/

void tms5110_set_frequency(int frequency)
{
	struct tms5110_info *info = sndti_token(SOUND_TMS5110, 0);

	/* skip if output frequency is zero */
	if (!Machine->sample_rate)
		return;

	/* update the stream and compute a new step size */
	stream_update(info->stream, 0);
	info->source_step = (UINT32)((double)(frequency / 80) * (double)FRAC_ONE / (double)Machine->sample_rate);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void tms5110_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void tms5110_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = tms5110_set_info;		break;
		case SNDINFO_PTR_START:							info->start = tms5110_start;			break;
		case SNDINFO_PTR_STOP:							info->stop = tms5110_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = tms5110_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "TMS5110";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "TI Speech";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

