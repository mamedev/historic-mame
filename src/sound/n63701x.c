/***************************************************************************

An Hitachi HD637A01X0 MCU programmed to act as a sample player.
Used by some Namco System 86 games.

The MCU has internal ROM which hasn't been dumped, so here we simulate its
simple functions.

The chip can address ROM space up to 8 block of 0x10000 bytes. At the beginning
of each block there's a table listing the start offset of each sample.
Samples are 8 bit unsigned, 0xff marks the end of the sample. 0x00 is used for
silence compression: '00 nn' must be replaced by nn+1 times '80'.

***************************************************************************/

#include "driver.h"

static int stream;		/* channel assigned by the mixer */
static const struct namco_63701x_interface *intf;	/* pointer to our config data */
static UINT8 *rom;		/* pointer to sample ROM */

typedef struct
{
	int select;
	int playing;
	int base_addr;
	int position;
	int volume;
	int silence_counter;
} voice;

static voice voices[2];


/* volume control has three resistors: 22000, 10000 and 3300 Ohm.
   22000 is always enabled, the other two can be turned off.
   Since 0x00 and 0xff samples have special meaning, the available range is
   0x01 to 0xfe, therefore 258 * (0x01 - 0x80) = 0x8002 just keeps us
   inside 16 bits without overflowing.
 */
static int vol_table[4] = { 26, 84, 200, 258 };


static void namco_63701x_update(int channel, INT16 **buffer, int length)
{
	int ch;

	for (ch = 0;ch < 2;ch++)
	{
		INT16 *buf = buffer[ch];
		voice *v = &voices[ch];

		if (v->playing)
		{
			UINT8 *base = rom + v->base_addr;
			int pos = v->position;
			int vol = vol_table[v->volume];
			int p;

			for (p = 0;p < length;p++)
			{
				if (v->silence_counter)
				{
					v->silence_counter--;
					*(buf++) = 0;
				}
				else
				{
					int data = base[(pos++) & 0xffff];

					if (data == 0xff)	/* end of sample */
					{
						v->playing = 0;
						break;
					}
					else if (data == 0x00)	/* silence compression */
					{
						data = base[(pos++) & 0xffff];
						v->silence_counter = data;
						*(buf++) = 0;
					}
					else
					{
						*(buf++) = vol * (data - 0x80);
					}
				}
			}

			v->position = pos;
		}
		else
			memset(buf, 0, length * sizeof(INT16));
	}
}


int namco_63701x_sh_start(const struct MachineSound *msound)
{
	int vol[2];
	char buf[2][40];
	const char *name[2];

	intf = msound->sound_interface;
	rom = memory_region(intf->region);

	name[0] = buf[0];
	sprintf(buf[0],"%s ChA",sound_name(msound));
	name[1] = buf[1];
	sprintf(buf[1],"%s ChB",sound_name(msound));
	vol[0] = intf->mixing_level;
	vol[1] = intf->mixing_level;
	stream = stream_init_multi(2, name, vol, intf->baseclock/1000, 0, namco_63701x_update);

	return 0;
}


void namco_63701x_sh_stop(void)
{
}



void namco_63701x_write(int offset, int data)
{
	int ch = offset / 2;

	if (offset & 1)
		voices[ch].select = data;
	else
	{
		/*
		  should we stop the playing sample if voice_select[ch] == 0 ?
		  originally we were, but this makes us lose a sample in genpeitd,
		  after the continue counter reaches 0. Either we shouldn't stop
		  the sample, or genpeitd is returning to the title screen too soon.
		 */
		if (voices[ch].select & 0x1f)
		{
			int rom_offs;

			/* update the streams */
			stream_update(stream,0);

			voices[ch].playing = 1;
			voices[ch].base_addr = 0x10000 * ((voices[ch].select & 0xe0) >> 5);
			rom_offs = voices[ch].base_addr + 2 * ((voices[ch].select & 0x1f) - 1);
			voices[ch].position = (rom[rom_offs] << 8) + rom[rom_offs+1];
			/* bits 6-7 = volume */
			voices[ch].volume = data >> 6;
			/* bits 0-5 = counter to indicate new sample start? we don't use them */

			voices[ch].silence_counter = 0;
		}
	}
}
