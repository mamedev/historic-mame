/***************************************************************************

							-= Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)

					rewrite by Manbow-J(manbowj@hamal.freemail.ne.jp)

X1-010 (Seta Custom Sound Chip):

	The X1-010 is 16 Voices sound generator, each channel gets it's
	waveform from RAM (128 bytes per waveform, 8 bit unsigned data)
	or sampling PCM(8bit unsigned data).

Registers:
	8 registers per channel (mapped to the lower bytes of 16 words on the 68K)

	Reg:	Bits:		Meaning:

	0		7654 3---
			---- -2--	PCM/Waveform repeat flag (0:Ones 1:Repeat) (*1)
			---- --1-	Sound out select (0:PCM 1:Waveform)
			---- ---0	Key on / off

	1		7654 ----	PCM Volume 1 (L?)
			---- 3210	PCM Volume 2 (R?)
						Waveform No.

	2					PCM Frequency
						Waveform Pitch Lo

	3					Waveform Pitch Hi

	4					PCM Sample Start / 0x1000 			[Start/End in bytes]
						Waveform Envelope Time

	5					PCM Sample End 0x100 - (Sample End / 0x1000)	[PCM ROM is Max 1MB?]
						Waveform Envelope No.
	6					Reserved
	7					Reserved

	offset 0x0000 - 0x0fff	Wave form data
	offset 0x1000 - 0x1fff	Envelope data

	*1 : when 0 is specified, hardware interruput is caused(allways return soon)

Hardcoded Values:

	PCM ROM region:		REGION_SOUND1

***************************************************************************/

#include "driver.h"
#include "seta.h"



#define LOG_SOUND 0

#define SETA_NUM_CHANNELS 16


#if	__X1_010_V2
#define	LOG_REGISTER_WRITE	0
#define	LOG_REGISTER_READ	0

#define FREQ_BASE_BITS		  8					// Frequency fixed decimal shift bits
#define ENV_BASE_BITS		 16					// wave form envelope fixed decimal shift bits
#define	VOL_BASE	(32*256/30)					// Volume base

/* this structure defines the parameters for a channel */
typedef struct {
	unsigned char	status;
	unsigned char	volume;						//        volume / wave form no.
	unsigned char	frequency;					//     frequency / pitch lo
	unsigned char	pitch_hi;					//      reserved / pitch hi
	unsigned char	start;						// start address / envelope time
	unsigned char	end;						//   end address / envelope no.
	unsigned char	reserve[2];
} X1_010_CHANNEL;

/* Variables only used here */
static int	rate;								// Output sampling rate (Hz)
static int	stream;								// Stream handle
static int	address;							// address eor data
static int	sound_enable = 0;					// sound output enable/disable
static UINT8	x1_010_reg[0x2000];				// X1-010 Register & wave form area
static UINT8	HI_WORD_BUF[0x2000];			// X1-010 16bit access ram check avoidance work
static UINT32	smp_offset[SETA_NUM_CHANNELS];
static UINT32	env_offset[SETA_NUM_CHANNELS];
#else
static int firstchannel;
static int seta_reg[SETA_NUM_CHANNELS][8];
#endif	// __X1_010_V2

static UINT32 base_clock;

/* mixer tables and internal buffers */
//static short	*mixer_buffer = NULL;

int	seta_samples_bank;



#if	__X1_010_V2
/*--------------------------------------------------------------
 generate sound to the mix buffer
--------------------------------------------------------------*/
void seta_sh_update( int param, INT16 *buffer, int length )
{
	X1_010_CHANNEL	*reg;
	short	*mix;
	int		ch, i, vol, freq;
	register INT8	*start, *end, data;
	register UINT8	*env;
	register UINT32	smp_offs, smp_step, env_offs, env_step, delta;

	// mixer buffer zero clear
	memset( buffer, 0, length*sizeof(short) );

//	if( sound_enable == 0 ) return;

	for( ch = 0; ch < SETA_NUM_CHANNELS; ch++ ) {
		reg = (X1_010_CHANNEL *)&(x1_010_reg[ch*sizeof(X1_010_CHANNEL)]);
		if( (reg->status&1) != 0 ) {							// Key On
			mix = buffer;
			if( (reg->status&2) == 0 ) {						// PCM sampling
				start    = (INT8 *)(reg->start      *0x1000+memory_region(REGION_SOUND1));
				end      = (INT8 *)((0x100-reg->end)*0x1000+memory_region(REGION_SOUND1));
				vol      = (((reg->volume>>4)&0xf)+(reg->volume&0xf))*VOL_BASE;
				smp_offs = smp_offset[ch];
				freq     = reg->frequency&0x1f;
				// Meta Fox does not write the frequency register. Ever
				if( freq == 0 ) freq = 4;
				smp_step = (UINT32)((float)base_clock/8192.0
							*freq*(1<<FREQ_BASE_BITS)/(float)rate);
#if LOG_SOUND
				if( smp_offs == 0 ) {
					logerror( "Play sample %06X - %06X, channel %X volume %d freq %X step %X offset %X\n",
						start, end, ch, vol, freq, smp_step, smp_offs );
				}
#endif
				for( i = 0; i < length; i++ ) {
					delta = smp_offs>>FREQ_BASE_BITS;
					// sample ended?
					if( start+delta >= end ) {
						reg->status &= 0xfe;					// Key off
						break;
					}
					data = *(start+delta);
					*mix++ += (data*vol/256);
					smp_offs += smp_step;
				}
				smp_offset[ch] = smp_offs;
			} else {											// Wave form
				start    = (INT8 *)&(x1_010_reg[reg->volume*128+0x1000]);
				smp_offs = smp_offset[ch];
				freq     = (reg->pitch_hi<<8)+reg->frequency;
				smp_step = (UINT32)((float)base_clock/128.0/1024.0/4.0*freq*(1<<FREQ_BASE_BITS)/(float)rate);

				env      = (UINT8 *)&(x1_010_reg[reg->end*128]);
				env_offs = env_offset[ch];
				env_step = (UINT32)((float)base_clock/128.0/1024.0/4.0*reg->start*(1<<ENV_BASE_BITS)/(float)rate);
#if LOG_SOUND
/* Print some more debug info */
				if( smp_offs == 0 ) {
					logerror( "Play waveform %X, channel %X volume %X freq %4X step %X offset %X\n",
						reg->volume, ch, reg->end, freq, smp_step, smp_offs );
				}
#endif
				for( i = 0; i < length; i++ ) {
					delta = env_offs>>ENV_BASE_BITS;
	 				// Envelope one shot mode
					if( (reg->status&4) != 0 && delta >= 0x80 ) {
						reg->status &= 0xfe;					// Key off
						break;
					}
					vol = *(env+(delta&0x7f));
					vol = ((vol>>4)&0xf)+(vol&0xf);
					data  = *(start+((smp_offs>>FREQ_BASE_BITS)&0x7f));
					*mix++ += (data*VOL_BASE*vol/256);
					smp_offs += smp_step;
					env_offs += env_step;
				}
				smp_offset[ch] = smp_offs;
				env_offset[ch] = env_offs;
			}
		}
	}
}
#endif	// __X1_010_V2



#if	__X1_010_V2
int seta_sh_start( const struct MachineSound *msound, UINT32 clock, int adr )
{
	int i;
	const char *snd_name = "X1-010";

	base_clock	= clock;
	rate		= Machine->sample_rate;
	address		= adr;

	for( i = 0; i < SETA_NUM_CHANNELS; i++ ) {
		smp_offset[i] = 0;
		env_offset[i] = 0;
	}
#if LOG_SOUND
	/* Print some more debug info */
	logerror("masterclock = %d rate = %d\n", master_clock, rate );
#endif
	/* get stream channels */
	stream = stream_init( snd_name, 100, rate, 0, seta_sh_update );

	return 0;
}
#else
int seta_sh_start(const struct MachineSound *msound, UINT32 clock)
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

	base_clock = clock;

	return 0;
}
#endif	// __X1_010_V2



void seta_sound_enable_w(int data)
{
#if	__X1_010_V2
	sound_enable = data;
#else
	mixer_sound_enable_global_w(data);
#endif	// __X1_010_V2
}



/* Use these for 8 bit CPUs */


READ_HANDLER( seta_sound_r )
{
#if	__X1_010_V2
	offset ^= address;
	return x1_010_reg[offset];
#else
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
logerror("PC: %06X - X1-010 channel %X, register %X read!\n",activecpu_get_pc(),channel,reg);
#endif
				return seta_reg[channel][reg];
		}
	}

	return 0;
#endif	// __X1_010_V2
}




WRITE_HANDLER( seta_sound_w )
{
	int channel, reg;
#if	__X1_010_V2
	offset ^= address;

	channel	= offset/sizeof(X1_010_CHANNEL);
	reg		= offset%sizeof(X1_010_CHANNEL);

	if( channel < SETA_NUM_CHANNELS && reg == 0
	 && (x1_010_reg[offset]&1) == 0 && (data&1) != 0 ) {
	 	smp_offset[channel] = 0;
	 	env_offset[channel] = 0;
	}
#if	LOG_REGISTER_WRITE
	logerror("PC: %06X : offset %6X : data %2X\n", activecpu_get_pc(), offset, data );
#endif
	x1_010_reg[offset] = data;
#else
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

			/*
			   Twineagl continuosly writes 1 to reg 0 of the channel, so
			   the sample is restarted every time and never plays to the
			   end. It looks like the previous sample must be explicitly
			   stopped before a new one can be played
			*/
			if ( (data & 1) && !(seta_sound_r(0) & 1) )	// key on (0->1 only)
			{
				if (data & 2)
				{
				}
				else
				{
					int volumeL, volumeR;
					UINT32 frequency;

					int volume	=	seta_reg[channel][1];

					int start	=	seta_reg[channel][4]           * 0x1000;
					int end		=	(0x100 - seta_reg[channel][5]) * 0x1000; // from the end of the rom

					int len		=	end - start;
					int maxlen	=	memory_region_length(REGION_SOUND1);

					if (!( (start < end) && (end <= maxlen) ))
					{
						logerror("PC: %06X - X1-010 OUT OF RANGE SAMPLE: %06X - %06X, channel %X\n",activecpu_get_pc(),start,end,channel);
						return;
					}

#if LOG_SOUND
/* Print some more debug info */
logerror("PC: %06X - Play 8 bit sample %06X - %06X, channel %X\n",activecpu_get_pc(),start, end, channel);
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

					frequency = seta_reg[channel][2];

					/* Meta Fox does not write the frequency register. Ever */
					if (frequency == 0)	frequency = 4;

					frequency	*=	(((float)base_clock)/16000000) * 2000;

					mixer_play_sample(
						firstchannel + channel,							// channel
						(INT8 *)(memory_region(REGION_SOUND1) + start),	// start
						len,											// len
						frequency,										// frequency
						0);												// loop
				}
			}
			else
				mixer_stop_sample(channel + firstchannel);

			break;

	}
#endif	// __X1_010_V2
}




/* Use these for 16 bit CPUs */

READ16_HANDLER( seta_sound_word_r )
{
#if	__X1_010_V2
	UINT16	ret;

	ret = HI_WORD_BUF[offset]<<8;
	ret += (seta_sound_r( offset )&0xff);
#if LOG_REGISTER_READ
	logerror( "Read X1-010 PC:%06X Offset:%04X Data:%04X\n", activecpu_get_pc(), offset, ret );
#endif
	return ret;
#else
	return seta_sound_r(offset) & 0xff;
#endif	// __X1_010_V2
}

WRITE16_HANDLER( seta_sound_word_w )
{
#if	__X1_010_V2
	HI_WORD_BUF[offset] = (data>>8)&0xff;
	seta_sound_w( offset, data&0xff );
#if	LOG_REGISTER_WRITE
	logerror( "Write X1-010 PC:%06X Offset:%04X Data:%04X\n", activecpu_get_pc(), offset, data );
#endif
#else
	if (ACCESSING_LSB)
		seta_sound_w(offset, data & 0xff);
#endif	// __X1_010_V2
}
