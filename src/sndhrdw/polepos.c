/***************************************************************************
	polepos.c
	Sound handler
****************************************************************************/
#include "driver.h"

static int sample_msb = 0;
static int sample_lsb = 0;
static int sample_enable = 0;

static int current_position;
static int sound_stream;

/* speech section */
static int channel;
static INT8 *speech;
/* macro to convert 4-bit unsigned samples to 8-bit signed samples */
#define SAMPLE_CONV4(a) (0x11*((a&0x0f))-0x80)
#define SAMPLE_SIZE 0x2000

#define AMP(r)	(r*128/10100)
static int volume_table[8] = {
	AMP(2200), AMP(3200), AMP(4400), AMP(5400),
	AMP(6900), AMP(7900), AMP(9100), AMP(10100)
};

/************************************/
/* Stream updater                   */
/************************************/
static void engine_sound_update(int num, void *buffer, int length)
{
	UINT32 current = current_position, step, clock, slot, volume;
	UINT8 *output = buffer, *base;

	/* if we're not enabled, just fill with 0 */
	if (!sample_enable || Machine->sample_rate == 0)
	{
		memset(output, 0, length);
		return;
	}

	/* determine the effective clock rate */
	clock = (Machine->drv->cpu[0].cpu_clock / 64) * ((sample_msb + 1) * 64 + sample_lsb + 1) / (16*64);
	step = (clock << 12) / Machine->sample_rate;

	/* determine the volume */
	slot = (sample_msb >> 3) & 7;
	volume = volume_table[slot];
	base = &Machine->memory_region[5][0x1000 + slot * 0x800];

	/* fill in the sample */
	while (length--)
	{
		*output++ = (base[(current >> 12) & 0x7ff] * volume) / 256;
		current += step;
	}

	current_position = current;
}

/************************************/
/* Sound handler start              */
/************************************/
int polepos_sh_start(const struct MachineSound *msound)
{
	int i, bits;

	channel = mixer_allocate_channel(25);
	mixer_set_name(channel,"Speech");

	speech = malloc(2*SAMPLE_SIZE);
	if (!speech)
		return 1;

	/* decode the rom samples */
	for (i = 0;i < SAMPLE_SIZE;i++)
	{
		bits = Machine->memory_region[5][0x5000+i] & 0x0f;
		speech[2*i] = SAMPLE_CONV4(bits);

		bits = (Machine->memory_region[5][0x5000+i] & 0xf0) >> 4;
		speech[2*i + 1] = SAMPLE_CONV4(bits);
	}

	sound_stream = stream_init("Engine Sound", 50, Machine->sample_rate, 8, 0, engine_sound_update);
	current_position = 0;
	sample_msb = sample_lsb = 0;
	sample_enable = 0;
    return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void polepos_sh_stop(void)
{
	if (speech)
		free(speech);
	speech = NULL;
}

/************************************/
/* Sound handler update 			*/
/************************************/
void polepos_sh_update(void)
{
}

/************************************/
/* Write LSB of engine sound		*/
/************************************/
void polepos_engine_sound_lsb_w(int offset, int data)
{
	stream_update(sound_stream, 0);
	sample_lsb = data & 62;
    sample_enable = data & 1;
}

/************************************/
/* Write MSB of engine sound		*/
/************************************/
void polepos_engine_sound_msb_w(int offset, int data)
{
	stream_update(sound_stream, 0);
	sample_msb = data & 63;
}

/************************************/
/* Play speech sample				*/
/************************************/
void polepos_sample_play(int start, int end)
{
	int len = end - start;

	if (Machine->sample_rate == 0)
		return;

	mixer_play_sample(channel, speech + start, len, 4000, 0);
}
