/* from Andrew Scott (ascott@utkux.utcc.utk.edu) */
#include "driver.h"
#include "vidhrdw/generic.h"

extern WRITE8_HANDLER( rockola_flipscreen_w );

#define TONE_VOLUME		25
#define SAMPLE_VOLUME	25
#define CHANNELS		3

static int tonechannels, samplechannels;

static int SoundMute[CHANNELS];
static int SoundOffset[CHANNELS];
static int SoundBase[CHANNELS];
static int SoundMask[CHANNELS];
static int SoundVolume;
static int Sound0StopOnRollover;
static UINT8 LastPort1;

static UINT8 waveform[32] =
{
	0x88, 0x88, 0x88, 0x88, 0xaa, 0xaa, 0xaa, 0xaa,
	0xcc, 0xcc, 0xcc, 0xcc, 0xee, 0xee, 0xee, 0xee,
	0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
	0x44, 0x44, 0x44, 0x44, 0x66, 0x66, 0x66, 0x66
};

const char *vanguard_sample_names[] =
{
	"*vanguard",
	"explsion.wav",
	"fire.wav",
	0
};

int rockola_sh_start(const struct MachineSound *msound)
{
	int vol[CHANNELS];
	int i, offs = 0;

	// allocate tone channels

	for (i = 0; i < CHANNELS; i++)
		vol[i] = TONE_VOLUME;

	tonechannels = mixer_allocate_channels(CHANNELS, vol);

	// allocate sample channels

	for (i = 0; i < CHANNELS; i++)
		vol[i] = SAMPLE_VOLUME;

	samplechannels = mixer_allocate_channels(CHANNELS, vol);

	// clear tone channels

	for (i = 0; i < CHANNELS; i++)
	{
		SoundMute[i] = 1;
		SoundOffset[i] = 0;
		SoundBase[i] = offs;

		offs += 0x800;

		mixer_set_volume(tonechannels + i, 0);
		mixer_play_sample(tonechannels + i, (signed char *)waveform, 32, 1000, 1);
	}

	return 0;
}

void rockola_sh_update(void)
{
	static int count;
	int i;

	// only update every second call (30 Hz update)

	count++;
	if (count & 1) return;

	// play musical tones according to tunes stored in ROM

	for (i = 0; i < CHANNELS; i++)
	{
		if (!SoundMute[i])
		{
			if (memory_region(REGION_SOUND1)[SoundBase[i] + SoundOffset[i]] != 0xff)
			{
				mixer_set_sample_frequency(tonechannels + i, (32000 / (256 - memory_region(REGION_SOUND1)[SoundBase[i] + SoundOffset[i]])) * 16);
				mixer_set_volume(tonechannels + i, SoundVolume);
			}
			else
			{
				mixer_set_volume(tonechannels + i, 0);
			}
			SoundOffset[i] = (SoundOffset[i] + 1) & SoundMask[i];
		}
		else
		{
			mixer_set_volume(tonechannels + i, 0);
		}
	}

	if (SoundOffset[0] == 0 && Sound0StopOnRollover)
	{
		SoundMute[0] = 1;
	}
}

WRITE8_HANDLER( sasuke_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
			bit	description

		*/
		break;
	case 1:
		/*
			bit	description

		*/
		break;
	}
}

WRITE8_HANDLER( satansat_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
			bit	description

		*/

		/* bit 0 = analog sound trigger */

		/* bit 1 = to 76477 */

		/* bit 2 = analog sound trigger */
		if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
		{
			if (data & 0x04 && !(LastPort1 & 0x04))
			{
				mixer_play_sample(samplechannels+0,Machine->samples->sample[0]->data,
								Machine->samples->sample[0]->length,
								Machine->samples->sample[0]->smpfreq,
								0);
			}
		}

		if (data & 0x08)
		{
			SoundMute[0]=1;
			SoundOffset[0] = 0;
		}

		/* bit 4-6 sound0 volume control (TODO) */
		/* bit 7 sound1 volume control (TODO) */
		SoundVolume = 100;

		LastPort1 = data;
		break;
	case 1:
		/*
			bit	description

		*/

		/* select tune in ROM based on sound command byte */
		SoundBase[0] = 0x0000 + ((data & 0x0e) << 7);
		SoundMask[0] = 0xff;
		Sound0StopOnRollover = 1;
		SoundBase[1] = 0x0800 + ((data & 0x60) << 4);
		SoundMask[1] = 0x1ff;

		if (data & 0x01)
			SoundMute[0]=0;

		if (data & 0x10)
			SoundMute[1]=0;
		else
		{
			SoundMute[1]=1;
			SoundOffset[1] = 0;
		}

		/* bit 7 = ? */
		break;
	}
}

WRITE8_HANDLER( vanguard_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
			bit	description

			0	MUSIC A10
			1	MUSIC A9
			2	MUSIC A8
			3	LS05 PORT 1
			4	LS04 PORT 2
			5	SHOT A
			6	SHOT B
			7	BOMB
		*/

		/* select musical tune in ROM based on sound command byte */
		SoundBase[0] = ((data & 0x07) << 8);
		SoundMask[0] = 0xff;
		Sound0StopOnRollover = 0;

		/* play noise samples requested by sound command byte */
		if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
		{
			/* SHOT A */
			if (data & 0x20 && !(LastPort1 & 0x20))
				mixer_play_sample(samplechannels+2,Machine->samples->sample[1]->data,
								Machine->samples->sample[1]->length,
								Machine->samples->sample[1]->smpfreq,
								0);
			else if (!(data & 0x20) && LastPort1 & 0x20)
				mixer_stop_sample(samplechannels+2);

			/* BOMB */
			if (data & 0x80 && !(LastPort1 & 0x80))
			{
				mixer_play_sample(samplechannels+1,Machine->samples->sample[0]->data,
								Machine->samples->sample[0]->length,
								Machine->samples->sample[0]->smpfreq,
								0);
			}
		}

		if (data & 0x08)
		{
			SoundMute[0]=1;
			SoundOffset[0] = 0;
		}

		if (data & 0x10)
		{
			SoundMute[0]=0;
		}

		/* SHOT B */
		SN76477_enable_w(1, (data & 0x40) ? 0 : 1);

		LastPort1 = data;
		break;
	case 1:
		/*
			bit	description

			0	MUSIC A10
			1	MUSIC A9
			2	MUSIC A8
			3	LS04 PORT 3
			4	EXTP A (goes to speech board)
			5	EXTP B (goes to speech board)
			6	
			7	
		*/

		/* select tune in ROM based on sound command byte */
		SoundBase[1] = 0x0800 + ((data & 0x07) << 8);
		SoundMask[1] = 0xff;

		if (data & 0x08)
			SoundMute[1]=0;
		else
		{
			SoundMute[1]=1;
			SoundOffset[1] = 0;
		}
		break;
	case 2:
		/*
			bit	description

			0	AS 1
			1	AS 2
			2	AS 3
			3	AS 4
			4	AS 5
			5	AS 6
			6	AS 7
			7	AS 8
		*/

		SoundVolume = 100; // TODO: real volume calculation
		break;
	}
}

WRITE8_HANDLER( fantasy_sound_w )
{
	switch (offset)
	{
	case 0:
		/*
			bit	description

			0	MUSIC A10
			1	MUSIC A9
			2	MUSIC A8
			3	LS04 PART 1
			4	LS04 PART 2
			5
			6
			7	BOMB
		*/

		/* select musical tune in ROM based on sound command byte */
		SoundBase[0] = 0x0000 + ((data & 0x07) << 8);
		SoundMask[0] = 0xff;
		Sound0StopOnRollover = 0;

		if (data & 0x08)
			SoundMute[0]=0;
		else
		{
			SoundOffset[0] = SoundBase[0];
			SoundMute[0]=1;
		}

		if (data & 0x10)
			SoundMute[2]=0;
		else
		{
			SoundOffset[2] = 0;
			SoundMute[2]=1;
		}

		/* BOMB */
		SN76477_enable_w(0, (data & 0x80) ? 0 : 1);
		/*

			In the real hardware the SN76477 enable line is grounded
			and the sound output is switched on/off by a 4066 IC. 

		*/

		LastPort1 = data;
		break;
	case 1:
		/*
			bit	description

			0	MUSIC A10
			1	MUSIC A9
			2	MUSIC A8
			3	LS04 PART 3
			4	EXT PA (goes to speech board)
			5	EXT PB (goes to speech board)
			6	
			7	
		*/

		/* select tune in ROM based on sound command byte */
		SoundBase[1] = 0x0800 + ((data & 0x07) << 8);
		SoundMask[1] = 0xff;

		if (data & 0x08)
			SoundMute[1]=0;
		else
		{
			SoundMute[1]=1;
			SoundOffset[1] = 0;
		}
		break;
	case 2:
		/*
			bit	description

			0	AS 1
			1	AS 2
			2	AS 3
			3	AS 4
			4	AS 5
			5	AS 6
			6	AS 7
			7	AS 8
		*/

		SoundVolume = 100; // TODO: real volume calculation
		break;
	case 3:
		/*
			bit	description

			0	BC 1
			1	BC 2
			2	BC 3
			3	MUSIC A10
			4	MUSIC A9
			5	MUSIC A8
			6	
			7	INV
		*/

		/* select tune in ROM based on sound command byte */
		SoundBase[2] = 0x1000 + ((data & 0x70) << 4);
	//	SoundBase[2] = 0x1000 + ((data & 0x10) << 5) + ((data & 0x20) << 5) + ((data & 0x40) << 2);
		SoundMask[2] = 0xff;

		rockola_flipscreen_w(0, data);
		break;
	}
}
