/***************************************************************************

	Cyberball 68000 sound simulator

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"

#include <math.h>


static UINT8 *bank_base;
static UINT8 fast_68k_int, io_68k_int;
static UINT8 sound_data_from_68k, sound_data_from_6502;
static UINT8 sound_data_from_68k_ready, sound_data_from_6502_ready;


static void handle_68k_sound_command(int command);
static void update_sound_68k_interrupts(void);



void cyberbal_sound_reset(void)
{
	/* reset the sound system */
	bank_base = &memory_region(REGION_CPU2)[0x10000];
	cpu_setbank(8, &bank_base[0x0000]);
	fast_68k_int = io_68k_int = 0;
	sound_data_from_68k = sound_data_from_6502 = 0;
	sound_data_from_68k_ready = sound_data_from_6502_ready = 0;
}



/*************************************
 *
 *	6502 Sound Interface
 *
 *************************************/

READ_HANDLER( cyberbal_special_port3_r )
{
	int temp = readinputport(3);
	if (!(readinputport(0) & 0x8000)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;
	return temp;
}


READ_HANDLER( cyberbal_sound_6502_stat_r )
{
	int temp = 0xff;
	if (sound_data_from_6502_ready) temp ^= 0x80;
	if (sound_data_from_68k_ready) temp ^= 0x40;
	return temp;
}


WRITE_HANDLER( cyberbal_sound_bank_select_w )
{
	cpu_setbank(8, &bank_base[0x1000 * ((data >> 6) & 3)]);
}


READ_HANDLER( cyberbal_sound_68k_6502_r )
{
	sound_data_from_68k_ready = 0;
	return sound_data_from_68k;
}


WRITE_HANDLER( cyberbal_sound_68k_6502_w )
{
	sound_data_from_6502 = data;
	sound_data_from_6502_ready = 1;

#ifdef EMULATE_SOUND_68000
	if (!io_68k_int)
	{
		io_68k_int = 1;
		update_sound_68k_interrupts();
	}
#else
	handle_68k_sound_command(data);
#endif
}



/*************************************
 *
 *	68000 Sound Interface
 *
 *************************************/

static void update_sound_68k_interrupts(void)
{
#ifdef EMULATE_SOUND_68000
	int newstate = 0;

	if (fast_68k_int)
		newstate |= 6;
	if (io_68k_int)
		newstate |= 2;

	if (newstate)
		cpu_set_irq_line(3, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(3, 7, CLEAR_LINE);
#endif
}


int cyberbal_sound_68k_irq_gen(void)
{
	if (!fast_68k_int)
	{
		fast_68k_int = 1;
		update_sound_68k_interrupts();
	}
	return 0;
}


WRITE16_HANDLER( cyberbal_io_68k_irq_ack_w )
{
	if (io_68k_int)
	{
		io_68k_int = 0;
		update_sound_68k_interrupts();
	}
}


READ16_HANDLER( cyberbal_sound_68k_r )
{
	int temp = (sound_data_from_6502 << 8) | 0xff;

	sound_data_from_6502_ready = 0;

	if (sound_data_from_6502_ready) temp ^= 0x08;
	if (sound_data_from_68k_ready) temp ^= 0x04;
	return temp;
}


WRITE16_HANDLER( cyberbal_sound_68k_w )
{
	if (ACCESSING_MSB)
	{
		sound_data_from_68k = (data >> 8) & 0xff;
		sound_data_from_68k_ready = 1;
	}
}


WRITE16_HANDLER( cyberbal_sound_68k_dac_w )
{
	DAC_signed_data_w((offset >> 4) & 1, (INT16)data >> 8);

	if (fast_68k_int)
	{
		fast_68k_int = 0;
		update_sound_68k_interrupts();
	}
}



/*************************************
 *
 *	68000 Sound Simulator
 *
 *************************************/

#define SAMPLE_RATE 10000

struct sound_descriptor
{
	/*00*/UINT16 start_address_h;
	/*02*/UINT16 start_address_l;
	/*04*/UINT16 end_address_h;
	/*06*/UINT16 end_address_l;
	/*08*/UINT16 reps;
	/*0a*/INT16 volume;
	/*0c*/INT16 delta_volume;
	/*0e*/INT16 target_volume;
	/*10*/UINT16 voice_priority;	/* voice high, priority low */
	/*12*/UINT16 buffer_number;		/* buffer high, number low */
	/*14*/UINT16 continue_unused;	/* continue high, unused low */
};

struct voice_descriptor
{
	UINT8 playing;
	UINT8 *start;
	UINT8 *current;
	UINT8 *end;
	UINT16 reps;
	INT16 volume;
	INT16 delta_volume;
	INT16 target_volume;
	UINT8 priority;
	UINT8 number;
	UINT8 buffer;
	UINT8 cont;
	INT16 chunk[48];
	UINT8 chunk_remaining;
};


static INT16 *volume_table;
static struct voice_descriptor voices[6];
static UINT8 sound_enabled;
static int stream_channel;


static void decode_chunk(UINT8 *memory, INT16 *output, int overall)
{
	UINT16 volume_bits = READ_WORD(memory);
	int volume, i, j;

	memory += 2;
	for (i = 0; i < 3; i++)
	{
		/* get the volume */
		volume = ((overall & 0x03e0) + (volume_bits & 0x3e0)) >> 1;
		volume_bits = ((volume_bits >> 5) & 0x07ff) | ((volume_bits << 11) & 0xf800);

		for (j = 0; j < 4; j++)
		{
			UINT16 data = READ_WORD(memory);
			memory += 2;
			*output++ = volume_table[volume | ((data >>  0) & 0x000f)];
			*output++ = volume_table[volume | ((data >>  4) & 0x000f)];
			*output++ = volume_table[volume | ((data >>  8) & 0x000f)];
			*output++ = volume_table[volume | ((data >> 12) & 0x000f)];
		}
	}
}


static void sample_stream_update(int param, INT16 **buffer, int length)
{
	INT16 *buf_left = buffer[0];
	INT16 *buf_right = buffer[1];
	int i;

	(void)param;

	/* reset the buffers so we can add into them */
	memset(buf_left, 0, length * sizeof(INT16));
	memset(buf_right, 0, length * sizeof(INT16));

	/* loop over voices */
	for (i = 0; i < 6; i++)
	{
		struct voice_descriptor *voice = &voices[i];
		int left = length;
		INT16 *output;

		/* bail if not playing */
		if (!voice->playing || !voice->buffer)
			continue;

		/* pick a buffer */
		output = (voice->buffer == 0x10) ? buf_left : buf_right;

		/* loop until we're done */
		while (left)
		{
			INT16 *source;
			int this_batch;

			if (!voice->chunk_remaining)
			{
				/* loop if necessary */
				if (voice->current >= voice->end)
				{
					if (--voice->reps == 0)
					{
						voice->playing = 0;
						break;
					}
					voice->current = voice->start;
				}

				/* decode this chunk */
				decode_chunk(voice->current, voice->chunk, voice->volume);
				voice->current += 26;
				voice->chunk_remaining = 48;

				/* update volumes */
				voice->volume += voice->delta_volume;
				if ((voice->volume & 0xffe0) == (voice->target_volume & 0xffe0))
					voice->delta_volume = 0;
			}

			/* determine how much to copy */
			this_batch = (left > voice->chunk_remaining) ? voice->chunk_remaining : left;
			source = voice->chunk + 48 - voice->chunk_remaining;
			voice->chunk_remaining -= this_batch;
			left -= this_batch;

			while (this_batch--)
				*output++ += *source++;
		}
	}
}


int cyberbal_samples_start(const struct MachineSound *msound)
{
	const char *names[] =
	{
		"68000 Simulator left",
		"68000 Simulator right"
	};
	int vol[2], i, j;

	(void)msound;

	/* allocate volume table */
	volume_table = malloc(sizeof(INT16) * 64 * 16);
	if (!volume_table)
		return 1;

	/* build the volume table */
	for (j = 0; j < 64; j++)
	{
		double factor = pow(0.5, (double)j * 0.25);
		for (i = 0; i < 16; i++)
			volume_table[j * 16 + i] = (INT16)(factor * (double)((INT16)(i << 12)));
	}

	/* get stream channels */
#if USE_MONO_SOUND
	vol[0] = MIXER(50, MIXER_PAN_CENTER);
	vol[1] = MIXER(50, MIXER_PAN_CENTER);
#else
	vol[0] = MIXER(100, MIXER_PAN_LEFT);
	vol[1] = MIXER(100, MIXER_PAN_RIGHT);
#endif
	stream_channel = stream_init_multi(2, names, vol, SAMPLE_RATE, 0, sample_stream_update);

	/* reset voices */
	memset(voices, 0, sizeof(voices));
	sound_enabled = 1;

	return 0;
}


void cyberbal_samples_stop(void)
{
	if (volume_table)
		free(volume_table);
	volume_table = NULL;
}


static void handle_68k_sound_command(int command)
{
	struct sound_descriptor *sound;
	struct voice_descriptor *voice;
	UINT16 offset;
	int actual_delta, actual_volume;
	int temp;

	/* read the data to reset the latch */
	cyberbal_sound_68k_r(0,0);

	switch (command)
	{
		case 0:		/* reset */
			break;

		case 1:		/* self-test */
			cyberbal_sound_68k_w(0, 0x40 << 8, 0);
			break;

		case 2:		/* status */
			cyberbal_sound_68k_w(0, 0x00 << 8, 0);
			break;

		case 3:
			sound_enabled = 0;
			break;

		case 4:
			sound_enabled = 1;
			break;

		default:
			/* bail if we're not enabled or if we get a bogus voice */
			offset = READ_WORD(&memory_region(REGION_CPU4)[0x1e2a + 2 * command]);
			sound = (struct sound_descriptor *)&memory_region(REGION_CPU4)[offset];

			/* check the voice */
			temp = sound->voice_priority >> 8;
			if (!sound_enabled || temp > 5)
				break;
			voice = &voices[temp];

			/* see if we're allowed to take over */
			actual_volume = sound->volume;
			actual_delta = sound->delta_volume;
			if (voice->playing && voice->cont)
			{
				temp = sound->buffer_number & 0xff;
				if (voice->number != temp)
					break;

				/* if we're ramping, adjust for the current volume */
				if (actual_delta != 0)
				{
					actual_volume = voice->volume;
					if ((actual_delta < 0 && voice->volume <= sound->target_volume) ||
						(actual_delta > 0 && voice->volume >= sound->target_volume))
						actual_delta = 0;
				}
			}
			else if (voice->playing)
			{
				temp = sound->voice_priority & 0xff;
				if (voice->priority > temp ||
					(voice->priority == temp && (temp & 1) == 0))
					break;
			}

			/* fill in the voice; we're taking over */
			voice->playing = 1;
			voice->start = &memory_region(REGION_CPU4)[(sound->start_address_h << 16) | sound->start_address_l];
			voice->current = voice->start;
			voice->end = &memory_region(REGION_CPU4)[(sound->end_address_h << 16) | sound->end_address_l];
			voice->reps = sound->reps;
			voice->volume = actual_volume;
			voice->delta_volume = actual_delta;
			voice->target_volume = sound->target_volume;
			voice->priority = sound->voice_priority & 0xff;
			voice->number = sound->buffer_number & 0xff;
			voice->buffer = sound->buffer_number >> 8;
			voice->cont = sound->continue_unused >> 8;
			voice->chunk_remaining = 0;
			break;
	}
}
