/***************************************************************************

							-= Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


X1-010 (Seta Custom Sound Chip):

	16 Voices, 8 Bit PCM

Registers:

	8 registers per channel (mapped to the lower bytes of 16 words on the 68K)

	Reg:	Bits:		Meaning:

	0		7654 3---
			---- -2--	?
			---- --1-	?
			---- ---0	Key On / Off

	1		7654 ----	Volume 1 (L?)
			---- 3210	Volume 2 (R?)

	2					Frequency
	3					""

	4					Sample Start / 0x1000 			[Start/End in bytes]
	5					0x100 - (Sample End / 0x1000)	[PCM ROM is Max 1MB?]

	6					?
	7					?


Hardcoded Values:

	PCM ROM region:		REGION_SOUND1

***************************************************************************/

#include "driver.h"
#include "seta.h"

#define LOG_SOUND 0

#define SETA_NUM_CHANNELS 16


/* Variables only used here */

static int firstchannel;
static int seta_reg[SETA_NUM_CHANNELS][8];

/* Variables used elsewhere */

int seta_samples_bank;

struct CustomSound_interface seta_sound_interface =
{
	seta_sh_start,
	0,
	0,
};



int seta_sh_start(const struct MachineSound *msound)
{
	int i;
	int mix_lev[SETA_NUM_CHANNELS];

	for (i = 0; i < SETA_NUM_CHANNELS; i++)	mix_lev[i] = (100 * 2) / SETA_NUM_CHANNELS + 1;
	firstchannel = mixer_allocate_channels(SETA_NUM_CHANNELS,mix_lev);

	for (i = 0; i < SETA_NUM_CHANNELS; i++)
	{
		char buf[40];
		sprintf(buf,"X1-010 Channel #%d",i);
		mixer_set_name(firstchannel + i,buf);
	}

	return 0;
}

void seta_sound_enable_w(int data)
{
	mixer_sound_enable_global_w(data);
}



/* Use these for 8 bit CPUs */


READ_HANDLER( seta_sound_r )
{
	int channel	=	offset / 8;
	int reg		=	offset % 8;

	if (channel < SETA_NUM_CHANNELS)
	{
		switch (reg)
		{
			case 0:
				return	(seta_reg[channel][0] & ~1) |
						(mixer_is_sample_playing(firstchannel + channel) ? 1 : 0 );
			default:
#if LOG_SOUND
logerror("PC: %06X - X1-010 channel %X, register %X read!\n",cpu_get_pc(),channel,reg);
#endif
				return seta_reg[channel][reg];
		}
	}

	return 0;
}




WRITE_HANDLER( seta_sound_w )
{
	int channel, reg;
	int frequency;

	channel	=	offset / 8;
	reg		=	offset % 8;

	if (channel >= SETA_NUM_CHANNELS)	return;

	seta_reg[channel][reg] = data & 0xff;

	if (Machine->sample_rate == 0)		return;

	switch (reg)
	{

		case 0:

#if LOG_SOUND
logerror("X1-010 REGS: ch %X] %02X %02X %02X %02X - %02X %02X %02X %02X\n",
		channel,	seta_reg[channel][0],seta_reg[channel][1],
					seta_reg[channel][2],seta_reg[channel][3],
					seta_reg[channel][4],seta_reg[channel][5],
					seta_reg[channel][6],seta_reg[channel][7]	);
#endif


			if (data & 1)	// key on
			{
				int volumeL, volumeR;
				int volume	=	seta_reg[channel][1];

				int start	=	seta_reg[channel][4]           * 0x1000;
				int end		=	(0x100 - seta_reg[channel][5]) * 0x1000; // from the end of the rom

				int len		=	end - start;
				int maxlen	=	memory_region_length(REGION_SOUND1);

				if (!( (start < end) && (end <= maxlen) ))
				{
					logerror("PC: %06X - X1-010 OUT OF RANGE SAMPLE: %06X - %06X, channel %X\n",cpu_get_pc(),start,end,channel);
					return;
				}

				/*
				   Twineagl continuosly writes 1 to reg 0 of the channel, so
				   the sample is restarted every time and never plays to the
				   end. It looks like the previous sample must be explicitly
				   stopped before a new one can be played
				*/
				if ( seta_sound_r(offset) & 1 )	return;	// play to the end

				/* These samples are probaly looped and use the 3rd & 4th register's value */
				if (data & 2)	return;

#if LOG_SOUND
/* Print some more debug info */
logerror("PC: %06X - Play 8 bit sample %06X - %06X, channel %X\n",cpu_get_pc(),start, end, channel);
#endif

				/* Left and right speaker's volume can be set indipendently.
				   Some games (the mono ones, I guess) only set one of the two
				   to a non-zero value.
				   So we use the highest of the two volumes for now */

				volumeL = (volume >> 4) & 0xf;
				volumeR = (volume >> 0) & 0xf;
				volume = (volumeL > volumeR) ? volumeL : volumeR;
				mixer_set_volume(firstchannel + channel, (volume * 100) / 0xf );

				/* *Preliminary* pitch selection */

				if	( ( seta_reg[channel][3] == 0) ||
					  ( seta_reg[channel][3] == 0xff) )	// sokonuke
				{
					int f = seta_reg[channel][2] /*% 17*/;
					frequency = f * 1000;
				}
				else
				{
					int f = (seta_reg[channel][2] + seta_reg[channel][3]*256) % 0x4000;
					frequency =	0x4000 - f;
				}

				/* Meta Fox does not write the frequency register. Ever */
				if (frequency == 0)	frequency = 4000;

				mixer_play_sample(
					firstchannel + channel,							// channel
					(INT8 *)(memory_region(REGION_SOUND1) + start),	// start
					len,											// len
					frequency*2,									// frequency
					0);												// loop
			}
			else
				mixer_stop_sample(channel + firstchannel);

			break;

	}
}




/* Use these for 16 bit CPUs */

READ16_HANDLER( seta_sound_word_r )
{
	return seta_sound_r(offset) & 0xff;
}

WRITE16_HANDLER( seta_sound_word_w )
{
	if (ACCESSING_LSB)
		seta_sound_w(offset, data & 0xff);
}
