/***************************************************************************

	Exidy 440 sound system

	Special thanks to Zonn Moore and Neil Bradley for letting me hack
	their Retrocade CVSD decoder into the sound system here.

***************************************************************************/

#include "driver.h"
#include <math.h>


#define	SOUND_LOG		0
#define	CHECK_GAIN		0
#define	FADE_TO_ZERO	1


/* signed/unsigned 8-bit conversion macros */
#define AUDIO_CONV(A) ((unsigned char)(A))
#define AUDIO_UNCONV(A) ((signed char)(A))


/* sample rates for each chip */
#define	SAMPLE_RATE_FAST		(12979200/256)	/* FCLK */
#define	SAMPLE_RATE_SLOW		(12979200/512)	/* SCLK */

/* internal caching */
#define	MAX_CACHE_ENTRIES		1024				/* maximum separate samples we expect to ever see */
#define	SAMPLE_BUFFER_LENGTH	1024				/* size of temporary decode buffer on the stack */

/* FIR digital filter parameters */
#define	FIR_HISTORY_LENGTH		57					/* number of FIR coefficients */

/* CVSD decoding parameters */
#define	INTEGRATOR_LEAK_TC		(10e3 * 0.1e-6)
#define	FILTER_DECAY_TC			((18e3 + 3.3e3) * 0.33e-6)
#define	FILTER_CHARGE_TC		(18e3 * 0.33e-6)
#define	FILTER_MIN				0.0416
#define	FILTER_MAX				1.0954
#define	SAMPLE_GAIN				1000.0


/* channel_data structure holds info about each 6844 DMA channel */
typedef struct m6844_channel_data
{
	int active;
	int address;
	int counter;
	unsigned char control;
	int start_address;
	int start_counter;
} m6844_channel_data;


/* channel_data structure holds info about each active sound channel */
typedef struct sound_channel_data
{
	int stream;
	void *base;
	int offset;
	int remaining;
} sound_channel_data;


/* sound_cache_entry structure contains info on each decoded sample */
typedef struct sound_cache_entry
{
	struct sound_cache_entry *next;
	int address;
	int length;
	int bits;
	int frequency;
	unsigned char data[1];
} sound_cache_entry;



/* globals */
unsigned char *exidy440_m6844_data;
unsigned char *exidy440_sound_banks;
unsigned char *exidy440_sound_volume;
unsigned char exidy440_sound_command;
unsigned char exidy440_sound_command_ack;
double exidy440_sound_gain;

/* local allocated storage */
static sound_cache_entry *sound_cache;
static sound_cache_entry *sound_cache_end;
static sound_cache_entry *sound_cache_max;

/* 6844 description */
static m6844_channel_data m6844_channel[4];
static int m6844_priority;
static int m6844_interrupt;
static int m6844_chain;

/* sound interface parameters */
static int sample_bits;
static sound_channel_data sound_channel[4];

/* debugging */
static FILE *debuglog;

/* constant channel parameters */
static const int channel_frequency[4] =
{
	SAMPLE_RATE_FAST, SAMPLE_RATE_FAST,		/* channels 0 and 1 are run by FCLK */
	SAMPLE_RATE_SLOW, SAMPLE_RATE_SLOW		/* channels 2 and 3 are run by SCLK */
};
static const int channel_bits[4] =
{
	4, 4,									/* channels 0 and 1 are MC3418s, 4-bit CVSD */
	3, 3									/* channels 2 and 3 are MC3417s, 3-bit CVSD */
};


/* function prototypes */
static void channel_update(int ch, void **buffer, int length);
static void m6844_finished(int ch);
static void play_cvsd(int ch);
static void stop_cvsd(int ch);

static void reset_sound_cache(void);
static void *add_to_sound_cache(unsigned char *input, int address, int length, int bits, int frequency);
static void *find_or_add_to_sound_cache(int address, int length, int bits, int frequency);

static void decode_and_filter_cvsd(unsigned char *data, int bytes, int maskbits, int frequency, void *dest);
static void fir_filter_16(short *input, short *output, int count);
static void fir_filter_8(short *input, unsigned char *output, int count);


/************************************
	Initialize the sound system
*************************************/

int exidy440_sh_start(const struct MachineSound *msound)
{
	const char *names[] =
	{
		"MC3417 #1 left",
		"MC3417 #1 right",
		"MC3417 #2 left",
		"MC3417 #2 right",
		"MC3418 #1 left",
		"MC3418 #1 right",
		"MC3418 #2 left",
		"MC3418 #2 right"
	};
	int i, length;

	/* reset the system */
	exidy440_sound_command = 0;
	exidy440_sound_command_ack = 1;

	/* reset the 6844 */
	for (i = 0; i < 4; i++)
	{
		m6844_channel[i].active = 0;
		m6844_channel[i].control = 0x00;
	}
	m6844_priority = 0x00;
	m6844_interrupt = 0x00;
	m6844_chain = 0x00;

	/* get stream channels */
	sample_bits = Machine->sample_bits;
	for (i = 0; i < 4; i++)
	{
		sound_channel[i].stream = stream_init_multi(msound,2, &names[i * 2], (i & 2) ? SAMPLE_RATE_SLOW : SAMPLE_RATE_FAST, sample_bits, i, channel_update);
		stream_set_pan(sound_channel[i].stream + 0, OSD_PAN_LEFT);
		stream_set_pan(sound_channel[i].stream + 1, OSD_PAN_RIGHT);
	}

	/* allocate the sample cache */
	length = Machine->memory_region_length[2] * sample_bits + MAX_CACHE_ENTRIES * sizeof(sound_cache_entry);
	sound_cache = malloc(length);
	if (!sound_cache)
		return 1;

	/* determine the hard end of the cache and reset */
	sound_cache_max = (sound_cache_entry *)((unsigned char *)sound_cache + length);
	reset_sound_cache();

	if (SOUND_LOG)
		debuglog = fopen("sound.log", "w");

	return 0;
}


/************************************
	Tear down the sound system
*************************************/

void exidy440_sh_stop(void)
{
	if (SOUND_LOG && debuglog)
		fclose(debuglog);

	if (sound_cache)
		free(sound_cache);
	sound_cache = NULL;
}


/************************************
	Periodic sound update
*************************************/

void exidy440_sh_update(void)
{
}


/************************************
	update the two sound streams
*************************************/

unsigned char *copy_samples_8(unsigned char *dst, unsigned char *src, int count)
{
	memcpy(dst, src, count);
	return dst + count;
}

short *copy_samples_16(short *dst, short *src, int count)
{
	memcpy(dst, src, count * 2);
	return dst + count;
}

void channel_update(int ch, void **buffer, int length)
{
	sound_channel_data *channel = &sound_channel[ch];
	void *buf_left = buffer[0];
	void *buf_right = buffer[1];
	void *srcdata;
	int samples;

	/* if we're not active, bail */
	if (channel->remaining <= 0)
	{
		memset(buf_left, 0, length * sample_bits / 8);
		memset(buf_right, 0, length * sample_bits / 8);
		return;
	}

	/* if we're not done yet, process the sample */
	if (channel->remaining > 0)
	{
		/* see how many samples to copy */
		samples = (length > channel->remaining) ? channel->remaining : length;

		/* get a pointer to the sample data and copy to the left/right */
		if (sample_bits == 16)
		{
			srcdata = (short *)channel->base + channel->offset;
			buf_left = copy_samples_16(buf_left, srcdata, samples);
			buf_right = copy_samples_16(buf_right, srcdata, samples);
		}
		else
		{
			srcdata = (unsigned char *)channel->base + channel->offset;
			buf_left = copy_samples_8(buf_left, srcdata, samples);
			buf_right = copy_samples_8(buf_right, srcdata, samples);
		}

		/* update our counters */
		channel->offset += samples;
		channel->remaining -= samples;
		length -= samples;

		/* update the MC6844 */
		m6844_channel[ch].address = m6844_channel[ch].start_address + channel->offset / 8;
		m6844_channel[ch].counter = m6844_channel[ch].start_counter - channel->offset / 8;
		if (m6844_channel[ch].counter <= 0)
		{
			if (SOUND_LOG && debuglog)
				fprintf(debuglog, "Channel %d finished\n", ch);
			m6844_finished(ch);
		}
	}

	/* fill in the rest if we need to */
	if (length)
	{
		memset(buf_left, 0, length * sample_bits / 8);
		memset(buf_right, 0, length * sample_bits / 8);
	}
}


/************************************
	Sound command register
*************************************/

int exidy440_sound_command_r(int offset)
{
	/* clear the FIRQ that got us here and acknowledge the read to the main CPU */
	cpu_set_irq_line(1, 1, CLEAR_LINE);
	exidy440_sound_command_ack = 1;

	return exidy440_sound_command;
}


/************************************
	Sound volume registers
*************************************/

void exidy440_sound_volume_w(int offset, int data)
{
	int stream = sound_channel[offset / 2].stream;

	if (SOUND_LOG && debuglog)
		fprintf(debuglog, "Volume %02X=%02X\n", offset, data);

	/* update the stream */
	stream_update(stream, 0);
	stream_set_volume(stream + (offset & 1), (~data & 0xff) * 100 / 255);

	/* set the new volume */
	exidy440_sound_volume[offset] = data;
}


/************************************
	Sound interrupt handling
*************************************/

int exidy440_sound_interrupt(void)
{
	cpu_set_irq_line(1, 0, ASSERT_LINE);
	return 0;
}

void exidy440_sound_interrupt_clear(int offset, int data)
{
	cpu_set_irq_line(1, 0, CLEAR_LINE);
}


/************************************
	MC6844 DMA controller interface
*************************************/

void exidy440_m6844_update(void)
{
	int i;

	/* update all the streams */
	for (i = 0; i < 4; i++)
		stream_update(sound_channel[i].stream, 0);
}

void m6844_finished(int ch)
{
	m6844_channel_data *channel = &m6844_channel[ch];

	/* mark us inactive */
	channel->active = 0;

	/* set the final address and counter */
	channel->counter = 0;
	channel->address = channel->start_address + channel->start_counter;

	/* clear the DMA busy bit and set the DMA end bit */
	channel->control &= ~0x40;
	channel->control |= 0x80;
}


/************************************
	MC6844 DMA controller I/O
*************************************/

int exidy440_m6844_r(int offset)
{
	int result = 0;

	/* first update the current state of the DMA transfers */
	exidy440_m6844_update();

	/* switch off the offset we were given */
	switch (offset)
	{
		/* upper byte of address */
		case 0x00:
		case 0x04:
		case 0x08:
		case 0x0c:
			result = m6844_channel[offset / 4].address >> 8;
			break;

		/* lower byte of address */
		case 0x01:
		case 0x05:
		case 0x09:
		case 0x0d:
			result = m6844_channel[offset / 4].address & 0xff;
			break;

		/* upper byte of counter */
		case 0x02:
		case 0x06:
		case 0x0a:
		case 0x0e:
			result = m6844_channel[offset / 4].counter >> 8;
			break;

		/* lower byte of counter */
		case 0x03:
		case 0x07:
		case 0x0b:
		case 0x0f:
			result = m6844_channel[offset / 4].counter & 0xff;
			break;

		/* channel control */
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			result = m6844_channel[offset - 0x10].control;

			/* a read here clears the DMA end flag */
			m6844_channel[offset - 0x10].control &= ~0x80;
			break;

		/* priority control */
		case 0x14:
			result = m6844_priority;
			break;

		/* interrupt control */
		case 0x15:

			/* update the global DMA end flag */
			m6844_interrupt &= ~0x80;
			m6844_interrupt |= (m6844_channel[0].control & 0x80) |
			                   (m6844_channel[1].control & 0x80) |
			                   (m6844_channel[2].control & 0x80) |
			                   (m6844_channel[3].control & 0x80);

			result = m6844_interrupt;
			break;

		/* chaining control */
		case 0x16:
			result = m6844_chain;
			break;
	}

	return result;
}

void exidy440_m6844_w(int offset, int data)
{
	int i;

	/* first update the current state of the DMA transfers */
	exidy440_m6844_update();

	/* switch off the offset we were given */
	switch (offset)
	{
		/* upper byte of address */
		case 0x00:
		case 0x04:
		case 0x08:
		case 0x0c:
			m6844_channel[offset / 4].address = (m6844_channel[offset / 4].address & 0xff) | (data << 8);
			break;

		/* lower byte of address */
		case 0x01:
		case 0x05:
		case 0x09:
		case 0x0d:
			m6844_channel[offset / 4].address = (m6844_channel[offset / 4].address & 0xff00) | (data & 0xff);
			break;

		/* upper byte of counter */
		case 0x02:
		case 0x06:
		case 0x0a:
		case 0x0e:
			m6844_channel[offset / 4].counter = (m6844_channel[offset / 4].counter & 0xff) | (data << 8);
			break;

		/* lower byte of counter */
		case 0x03:
		case 0x07:
		case 0x0b:
		case 0x0f:
			m6844_channel[offset / 4].counter = (m6844_channel[offset / 4].counter & 0xff00) | (data & 0xff);
			break;

		/* channel control */
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			m6844_channel[offset - 0x10].control = (m6844_channel[offset - 0x10].control & 0xc0) | (data & 0x3f);
			break;

		/* priority control */
		case 0x14:
			m6844_priority = data;

			/* update the sound playback on each channel */
			for (i = 0; i < 4; i++)
			{
				/* if we're going active... */
				if (!m6844_channel[i].active && (data & (1 << i)))
				{
					/* mark us active */
					m6844_channel[i].active = 1;

					/* set the DMA busy bit and clear the DMA end bit */
					m6844_channel[i].control |= 0x40;
					m6844_channel[i].control &= ~0x80;

					/* set the starting address, counter, and time */
					m6844_channel[i].start_address = m6844_channel[i].address;
					m6844_channel[i].start_counter = m6844_channel[i].counter;

					/* generate and play the sample */
					play_cvsd(i);
				}

				/* if we're going inactive... */
				else if (m6844_channel[i].active && !(data & (1 << i)))
				{
					/* mark us inactive */
					m6844_channel[i].active = 0;

					/* stop playing the sample */
					stop_cvsd(i);
				}
			}
			break;

		/* interrupt control */
		case 0x15:
			m6844_interrupt = (m6844_interrupt & 0x80) | (data & 0x7f);
			break;

		/* chaining control */
		case 0x16:
			m6844_chain = data;
			break;
	}
}


/************************************
	Sound cache management
*************************************/

void reset_sound_cache(void)
{
	sound_cache_end = sound_cache;
}

void *add_to_sound_cache(unsigned char *input, int address, int length, int bits, int frequency)
{
	sound_cache_entry *current = sound_cache_end;

	/* compute where the end will be once we add this entry */
	sound_cache_end = (sound_cache_entry *)((unsigned char *)current + sizeof(sound_cache_entry) + length * sample_bits);

	/* if this will overflow the cache, reset and re-add */
	if (sound_cache_end > sound_cache_max)
	{
		reset_sound_cache();
		return add_to_sound_cache(input, address, length, bits, frequency);
	}

	/* fill in this entry */
	current->next = sound_cache_end;
	current->address = address;
	current->length = length;
	current->bits = bits;
	current->frequency = frequency;

	/* decode the data into the cache */
	decode_and_filter_cvsd(input, length, bits, frequency, &current->data);
	return &current->data;
}

void *find_or_add_to_sound_cache(int address, int length, int bits, int frequency)
{
	sound_cache_entry *current;

	for (current = sound_cache; current < sound_cache_end; current = current->next)
		if (current->address == address && current->length == length && current->bits == bits && current->frequency == frequency)
			return &current->data;

	return add_to_sound_cache(&Machine->memory_region[2][address], address, length, bits, frequency);
}


/************************************
	Internal CVSD decoder and player
*************************************/

void play_cvsd(int ch)
{
	sound_channel_data *channel = &sound_channel[ch];
	int address = m6844_channel[ch].address;
	int length = m6844_channel[ch].counter;
	void *base;

	/* add the bank number to the address */
	if (exidy440_sound_banks[ch] & 1)
		address += 0x0000;
	else if (exidy440_sound_banks[ch] & 2)
		address += 0x8000;
	else if (exidy440_sound_banks[ch] & 4)
		address += 0x10000;
	else if (exidy440_sound_banks[ch] & 8)
		address += 0x18000;

	/* compute the base address in the converted samples array */
	base = find_or_add_to_sound_cache(address, length, channel_bits[ch], channel_frequency[ch]);
	if (!base)
		return;

	/* if the length is 0 or 1, just do an immediate end */
	if (length <= 3)
	{
		channel->base = (unsigned char *)base;
		channel->offset = length;
		channel->remaining = 0;
		m6844_finished(ch);
		return;
	}

	if (SOUND_LOG && debuglog)
		fprintf(debuglog, "Sound channel %d play at %02X,%04X, length = %04X, volume = %02X/%02X\n",
				ch, exidy440_sound_banks[ch], m6844_channel[ch].address,
				m6844_channel[ch].counter, exidy440_sound_volume[ch * 2], exidy440_sound_volume[ch * 2 + 1]);

	/* set the pointer and count */
	channel->base = base;
	channel->offset = 0;
	channel->remaining = length * 8;
}

void stop_cvsd(int ch)
{
	/* the DMA channel is marked inactive; that will kill the audio */
	sound_channel[ch].remaining = 0;
	stream_update(sound_channel[ch].stream, 0);

	if (SOUND_LOG && debuglog)
		fprintf(debuglog, "Channel %d stop\n", ch);
}


/************************************
	FIR digital filters
*************************************/

void fir_filter_16(short *input, short *output, int count)
{
	while (count--)
	{
		int result = (input[-1] - input[-8] - input[-48] + input[-55]) << 2;
		result += (input[0] + input[-18] + input[-38] + input[-56]) << 3;
		result += (-input[-2] - input[-4] + input[-5] + input[-51] - input[-52] - input[-54]) << 4;
		result += (-input[-3] - input[-11] - input[-45] - input[-53]) << 5;
		result += (input[-6] + input[-7] - input[-9] - input[-15] - input[-41] - input[-47] + input[-49] + input[-50]) << 6;
		result += (-input[-10] + input[-12] + input[-13] + input[-14] + input[-21] + input[-35] + input[-42] + input[-43] + input[-44] - input[-46]) << 7;
		result += (-input[-16] - input[-17] + input[-19] + input[-37] - input[-39] - input[-40]) << 8;
		result += (input[-20] - input[-22] - input[-24] + input[-25] + input[-31] - input[-32] - input[-34] + input[-36]) << 9;
		result += (-input[-23] - input[-33]) << 10;
		result += (input[-26] + input[-30]) << 11;
		result += (input[-27] + input[-28] + input[-29]) << 12;
		result >>= 14;

		*output++ = result;
		input++;
	}
}

void fir_filter_8(short *input, unsigned char *output, int count)
{
	while (count--)
	{
		int result = (input[-1] - input[-8] - input[-48] + input[-55]) << 2;
		result += (input[0] + input[-18] + input[-38] + input[-56]) << 3;
		result += (-input[-2] - input[-4] + input[-5] + input[-51] - input[-52] - input[-54]) << 4;
		result += (-input[-3] - input[-11] - input[-45] - input[-53]) << 5;
		result += (input[-6] + input[-7] - input[-9] - input[-15] - input[-41] - input[-47] + input[-49] + input[-50]) << 6;
		result += (-input[-10] + input[-12] + input[-13] + input[-14] + input[-21] + input[-35] + input[-42] + input[-43] + input[-44] - input[-46]) << 7;
		result += (-input[-16] - input[-17] + input[-19] + input[-37] - input[-39] - input[-40]) << 8;
		result += (input[-20] - input[-22] - input[-24] + input[-25] + input[-31] - input[-32] - input[-34] + input[-36]) << 9;
		result += (-input[-23] - input[-33]) << 10;
		result += (input[-26] + input[-30]) << 11;
		result += (input[-27] + input[-28] + input[-29]) << 12;
		result >>= 14;

		*output++ = AUDIO_CONV(result >> 8);
		input++;
	}
}


/************************************
	CVSD decoder
*************************************/

void decode_and_filter_cvsd(unsigned char *input, int bytes, int maskbits, int frequency, void *output)
{
	short buffer[SAMPLE_BUFFER_LENGTH + FIR_HISTORY_LENGTH];
	int total_samples = bytes * 8;
	int mask = (1 << maskbits) - 1;
	double filter, integrator, leak;
	double charge, decay, gain;
	int steps;
	int chunk_start;

#if CHECK_GAIN
#undef printf
	int clipped = 0;
	int min = 0, max = 0;
#endif

	/* compute the charge, decay, and leak constants */
	charge = pow(exp(-1), 1.0 / (FILTER_CHARGE_TC * (double)frequency));
	decay = pow(exp(-1), 1.0 / (FILTER_DECAY_TC * (double)frequency));
	leak = pow(exp(-1), 1.0 / (INTEGRATOR_LEAK_TC * (double)frequency));

	/* compute the gain */
	gain = SAMPLE_GAIN * exidy440_sound_gain;

	/* clear the history words for a start */
	memset(&buffer[0], 0, FIR_HISTORY_LENGTH * sizeof(short));

	/* initialize the CVSD decoder */
	steps = 0xaa;
	filter = FILTER_MIN;
	integrator = 0.0;

	/* loop over chunks */
	for (chunk_start = 0; chunk_start < total_samples; chunk_start += SAMPLE_BUFFER_LENGTH)
	{
		short *bufptr = &buffer[FIR_HISTORY_LENGTH];
		int chunk_bytes;
		int ind;

		/* how many samples do we generate in this chunk? */
		if (chunk_start + SAMPLE_BUFFER_LENGTH > total_samples)
			chunk_bytes = (total_samples - chunk_start) / 8;
		else
			chunk_bytes = SAMPLE_BUFFER_LENGTH / 8;

		/* loop over samples */
		for (ind = 0; ind < chunk_bytes; ind++)
		{
			int databyte = *input++;
			int bit;
			int sample;

			/* loop over bits in the byte, low to high */
			for (bit = 0; bit < 8; bit++)
			{
				/* move the estimator up or down a step based on the bit */
				if (databyte & (1 << bit))
				{
					integrator += filter;
					steps = (steps << 1) | 1;
				}
				else
				{
					integrator -= filter;
					steps <<= 1;
				}

				/* keep track of the last n bits */
				steps &= mask;

				/* simulate leakage */
				integrator *= leak;

				/* if we got all 0's or all 1's in the last n bits, bump the step up */
				if (steps == 0 || steps == mask)
				{
					filter = FILTER_MAX - ((FILTER_MAX - filter) * charge);
					if (filter > FILTER_MAX)
						filter = FILTER_MAX;
				}

				/* simulate decay */
				else
				{
					filter *= decay;
					if (filter < FILTER_MIN)
						filter = FILTER_MIN;
				}

				/* compute the sample as a 16-bit word */
				sample = (int)(integrator * gain);
#if CHECK_GAIN
				if (sample < min)
					min = sample;
				else if (sample > max)
					max = sample;
				if (sample < -32768 || sample > 32767)
					clipped++;
#endif
				if (sample > 32767)
					sample = 32767;
				else if (sample < -32768)
					sample = -32768;

				/* store the result to our temporary buffer */
				*bufptr++ = sample;
			}
		}

		/* all done with this chunk, run the filter on it */
		if (sample_bits == 16)
			fir_filter_16(&buffer[FIR_HISTORY_LENGTH], &((short *)output)[chunk_start], chunk_bytes * 8);
		else
			fir_filter_8(&buffer[FIR_HISTORY_LENGTH], &((unsigned char *)output)[chunk_start], chunk_bytes * 8);

		/* copy the last few input samples down to the start for a new history */
		memcpy(&buffer[0], &buffer[SAMPLE_BUFFER_LENGTH], FIR_HISTORY_LENGTH * sizeof(short));
	}

#if CHECK_GAIN
	printf("Clipped %d samples (min=%d, max=%d)\n", clipped, min, max);
#endif

	/* make sure the volume goes smoothly to 0 over the last 512 samples */
	if (FADE_TO_ZERO)
	{
		chunk_start = (total_samples > 512) ? total_samples - 512 : 0;
		if (sample_bits == 16)
		{
			short *data = (short *)output + chunk_start;
			for ( ; chunk_start < total_samples; chunk_start++)
				*data++ = (*data * ((total_samples - chunk_start) >> 9));
		}
		else
		{
			unsigned char *data = (unsigned char *)output + chunk_start;
			for ( ; chunk_start < total_samples; chunk_start++)
				*data++ = AUDIO_CONV(AUDIO_UNCONV(*data) * ((total_samples - chunk_start) >> 9));
		}
	}
}
