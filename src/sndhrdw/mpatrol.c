#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "m6808/m6808.h"
#include "adpcm.h"


extern int signalf;


unsigned char *mpatrol_io_ram;
unsigned char *mpatrol_sample_data;
unsigned char *mpatrol_sample_table;


static int command[2];


static struct AY8910interface interface =
{
	2,	/* 2 chips */
	17,	/* 17 updates per video frame (good quality) */
	910000000,	/* .91 MHZ ?? */
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


/* update the AY8910 only 1/4 of the time (17 times/frame should be plenty) */
int mpatrol_sh_interrupt(void)
{
	if (cpu_getiloops () % 4 == 0)
		AY8910_update ();

	return nmi_interrupt();
}


/* although I cannot find documentation on the 6803, it appears that you write to
   address location $03 to specify the port you wish to output to, and then write
   the value to be sent to location $02.  Finally, you write to a trigger port to
   actually send the data out.  I also think you have to put a $ff into location
   0 as well to indicate that you are writing instead of reading, but I'm
   not checking for that. */
void mpatrol_io_w(int offset, int value)
{
	static int cmd[2], val[2], trigger[2];

	mpatrol_io_ram[offset] = value;
	if (offset == 2)
	{
		switch (mpatrol_io_ram[3])
		{
			case 0x15:
				cmd[0] = value & 0x0f;
				break;
			case 0x10:
				val[0] = value;
				break;
			case 0x0d:
				cmd[1] = value & 0x0f;
				break;
			case 0x08:
				val[1] = value;
				break;
			default:
				if (errorlog)
					fprintf (errorlog, "Unknown I/O write to port %02X\n", mpatrol_io_ram[3]);
				break;
		}
	}
	else if (offset == 3)
	{
		switch (value)
		{
			case 0x11:
				trigger[0] = 1;
				break;
			case 0x10:
				if (trigger[0])
				{
					AY8910_control_port_0_w (0, cmd[0]);
					AY8910_write_port_0_w (0, val[0]);
					trigger[0] = 0;
				}
				break;
			case 0x09:
				trigger[1] = 1;
				break;
			case 0x08:
				if (trigger[1])
				{
					/* the ultimate Moon Patrol hack: the envelope for this chip only sounds close
					   to correct if it's divided by about 6. It doesn't seem to hurt the other
					   games, so I'm leaving it in! ASG */
/*					if (cmd[1] == 11 || cmd[1] == 12)
					{
						static int total;
						if (cmd[1] == 11) total = (total & 0xff00) | (val[1] & 0xff);
						else total = (total & 0x00ff) | ((val[1] << 8) & 0xff00);
						AY8910_control_port_1_w (0, 11);
						AY8910_write_port_1_w (0, (total/6) & 0xff);
						AY8910_control_port_1_w (0, 12);
						AY8910_write_port_1_w (0, (total/6) >> 8);
					}
					else*/
					{
						AY8910_control_port_1_w (0, cmd[1]);
						AY8910_write_port_1_w (0, val[1]);
					}

					trigger[1] = 0;
				}
				break;
		}
	}
}


/* although I cannot find documentation on the 6803, it appears that you write to
   address location $03 to specify the port you wish to read from, and then read
   the value from location $02.  I also think you have to put a $00 into
   location 0 as well to indicate that you are reading instead of writing, but I'm
   not checking for that. */
int mpatrol_io_r(int offset)
{
	if (mpatrol_io_ram[0] == 0x00)
	{
		switch (mpatrol_io_ram[3])
		{
			case 0x10:
				return AY8910_read_port_0_r (0);
			case 0x08:
				return AY8910_read_port_1_r (0);
			case 0x14:
				return command[1];	/* return the current sound command */
			case 0x0c:
				return command[0];	/* return the current sound command */
			default:
				if (errorlog)
					fprintf (errorlog, "Unknown I/O read from port %02X\n", mpatrol_io_ram[3]);
				return mpatrol_io_ram[offset];
		}
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
	int channel = 0;
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
					channel = 2;
					sample = get_sample_num ((mpatrol_sample_data[4] << 8) + mpatrol_sample_data[5]);
				}
				break;

			case 1:	/* channel 3 length */
				if (newval != oldval - 1)
				{
					channel = 3;
					sample = get_sample_num ((mpatrol_sample_data[6] << 8) + mpatrol_sample_data[7]);
				}
				break;
		}
	}
	lastword = word;	
	
	
	if (channel)
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
	return AY8910_sh_start(&interface);
}
