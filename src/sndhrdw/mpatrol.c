#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "m6808/m6808.h"
#include "adpcm.h"


extern int signalf;


unsigned char *mpatrol_io_ram;
unsigned char *mpatrol_sample_data;
unsigned char *mpatrol_sample_table;

static int sample_channel;

static int command[2];

static int port1;
static int port2;
static int ddr1;
static int ddr2;


static struct AY8910interface interface =
{
	2,	/* 2 chips */
	910000,	/* .91 MHZ ?? */
	{ 160, 160 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


/* sound commands from the main CPU come through here; we generate an IRQ and/or sample */
void mpatrol_sound_cmd_w(int offset, int value)
{
	/* generate IRQ on offset 0 only; should we also do it on offset 1? */
	command[offset] = value;
	if (offset == 0)
	{
		cpu_cause_interrupt (1, M6808_INT_IRQ);
		cpu_seticount (0);
	}
}


void mpatrol_io_w(int offset, int data)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			ddr1 = data;
			break;

		/* port 2 DDR */
		case 1:
			ddr2 = data;
			break;

		/* port 1 */
		case 2:
			data = (port1 & ~ddr1) | (data & ddr1);
			port1 = data;
			break;

		/* port 2 */
		case 3:
			data = (port2 & ~ddr2) | (data & ddr2);

			/* write latch */
			if ((port2 & 0x01) && !(data & 0x01))
			{
				/* control or data port? */
				if (port2 & 0x04)
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_control_port_0_w (0, port1);
					else if (port2 & 0x08)
						AY8910_control_port_1_w (0, port1);
				}
				else
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_write_port_0_w (0, port1);
					else if (port2 & 0x08)
						AY8910_write_port_1_w (0, port1);
				}
			}
			port2 = data;
			break;
	}
}


int mpatrol_io_r(int offset)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			return ddr1;

		/* port 2 DDR */
		case 1:
			return ddr2;

		/* port 1 */
		case 2:

			/* input 0 or 1? */
			if (port2 & 0x10)
				return (command[1] & ~ddr1) | (port1 & ddr1);
			else if (port2 & 0x08)
				return (command[0] & ~ddr1) | (port1 & ddr1);
			return (port1 & ddr1);

		/* port 2 */
		case 3:
			return (port2 & ddr2);
	}

	return 0;
}


/* ADPCM sample initialization -- these games use a 16 entry table with 2-byte offset/length pairs */
int mpatrol_sh_init(const char *gamename)
{
	unsigned char *table = mpatrol_sample_table;
	struct GameSamples *samples;
	int i, j;

	/* allocate the sample array; it will be automatically freed */
	samples = malloc (sizeof (struct GameSamples) + 16 * sizeof (struct GameSample));
	if (!samples)
		return 1;

	/* generate the samples */
	samples->total = 16;
	for (i = 0; i < samples->total; i++)
	{
		unsigned char *adpcm;
		int smplen;

		adpcm = &Machine->memory_region[2][(table[0] << 8) + table[1]];
		smplen = (table[2] << 8) + table[3];
		table += 4;

		if (smplen > 1)
		{
			samples->sample[i] = malloc (sizeof (struct GameSample) + smplen * 2 * sizeof (char));
			if (samples->sample[i])
			{
				unsigned char *decoded = &samples->sample[i]->data[0];

				samples->sample[i]->length = smplen * 2;
				samples->sample[i]->volume = 255;
				samples->sample[i]->smpfreq = 4000;   /* standard ADPCM voice freq */
				samples->sample[i]->resolution = 8;

				InitDecoder ();
				for (j = 0; j < smplen; j++)
				{
					int val = *adpcm++;

					signalf += DecodeAdpcm (val >> 4);
					if (signalf > 2047) signalf = 2047;
					if (signalf < -2047) signalf = -2047;
					*decoded++ = (signalf / 16);     /* for 16-bit samples multiply by 16 */

					signalf += DecodeAdpcm (val & 0x0f);
					if (signalf > 2047) signalf = 2047;
					if (signalf < -2047) signalf = -2047;
					*decoded++ = (signalf / 16);     /* for 16-bit samples multiply by 16 */
				}
		   }
		}
		else
			samples->sample[i] = NULL;
	}

	Machine->samples = samples;
	return 0;
}


static int get_sample_num (int offset)
{
	unsigned char *table = mpatrol_sample_table;
	int test, i;

	for (i = 0; i < 16; i++)
	{
		test = (table[0] << 8) + table[1];
		if (test == offset)
			return i;
		table += 4;
	}

	if (errorlog)
		fprintf (errorlog, "Unknown sample at %04X\n", offset);
	return 0;
}


/* These counters are decremented constantly, so we need to watch for a write here that isn't
   a simple increment/decrement */
void mpatrol_sample_trigger_w(int offset, int value)
{
	static int lastword, oldval;
	int word = offset >> 1;
	int channel = -1;
	int sample = 0;

	/* 4 16-bit words here: length1, length2, offset1, offset2 */
	if (word != lastword)
		oldval = (mpatrol_sample_data[(word<<1)] << 8) + mpatrol_sample_data[(word<<1) + 1];

	mpatrol_sample_data[offset] = value;

	if (word == lastword)
	{
		int newval = (mpatrol_sample_data[(word<<1)] << 8) + mpatrol_sample_data[(word<<1) + 1];

		switch (word)
		{
			case 0:	/* channel 2 length */
				if (newval != oldval - 1)
				{
					channel = sample_channel + 0;
					sample = get_sample_num ((mpatrol_sample_data[4] << 8) + mpatrol_sample_data[5]);
				}
				break;

			case 1:	/* channel 3 length */
				if (newval != oldval - 1)
				{
					channel = sample_channel + 1;
					sample = get_sample_num ((mpatrol_sample_data[6] << 8) + mpatrol_sample_data[7]);
				}
				break;
		}
	}
	lastword = word;


	if (channel != -1)
	{
		if (Machine->samples->sample[sample])
		{
			osd_play_sample(channel,
				Machine->samples->sample[sample]->data,
				Machine->samples->sample[sample]->length,
				Machine->samples->sample[sample]->smpfreq,
				Machine->samples->sample[sample]->volume,0);
		}
		else
			osd_stop_sample(channel);
	}
}


/* Standard sound startup */
int mpatrol_sh_start(void)
{
	sample_channel = get_play_channels(2);
	return AY8910_sh_start(&interface);
}
