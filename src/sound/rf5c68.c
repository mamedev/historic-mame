/*********************************************************/
/*    ricoh RF5C68(or clone) PCM controller              */
/*********************************************************/

#include "driver.h"
#include "rf5c68.h"
#include <math.h>

#define PCM_NORMALIZE

/******************************************/
#define  RF5C68_PCM_MAX    (8)


typedef struct rf5c68pcm
{
	int clock;
	unsigned char env[RF5C68_PCM_MAX];
	unsigned char pan[RF5C68_PCM_MAX];
	unsigned int  addr[RF5C68_PCM_MAX];
	unsigned int  start[RF5C68_PCM_MAX];
	unsigned int  step[RF5C68_PCM_MAX];
	unsigned int  loop[RF5C68_PCM_MAX];
	int pcmx[2][RF5C68_PCM_MAX];
	int flag[RF5C68_PCM_MAX];

	int pcmd[RF5C68_PCM_MAX];
	int pcma[RF5C68_PCM_MAX];

	int  reg_port;
	int emulation_rate;

	int buffer_len;
	sound_stream * stream;

	unsigned char pcmbuf[0x10000];

	const struct RF5C68interface *intf;

	unsigned char wreg[0x10]; /* write data */
} RF5C68PCM;

enum
{
	RF_L_PAN = 0, RF_R_PAN = 1, RF_LR_PAN = 2
};



#define   BASE_SHIFT    (11+4)

#define    RF_ON     (1<<0)
#define    RF_START  (1<<1)


static void RF5C68Update( void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length );

/************************************************/
/*    RF5C68 start                              */
/************************************************/
static void *rf5c68_start(int sndindex, int clock, const void *config)
{
	int i;
	int rate = Machine->sample_rate;
	const struct RF5C68interface *inintf = config;
	struct rf5c68pcm *chip;
	
	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	if (Machine->sample_rate == 0) return 0;

	memset(chip->pcmbuf, 0xff, 0x10000);

	chip->intf = inintf;
	chip->buffer_len = rate / Machine->drv->frames_per_second;
	chip->emulation_rate = chip->buffer_len * Machine->drv->frames_per_second;

	chip->clock = clock;
	for( i = 0; i < RF5C68_PCM_MAX; i++ )
	{
		chip->env[i] = 0;
		chip->pan[i] = 0;
		chip->start[i] = 0;
		chip->step[i] = 0;
		chip->flag[i] = 0;
	}
	for( i = 0; i < 0x10; i++ )  chip->wreg[i] = 0;
	chip->reg_port = 0;

	chip->stream = stream_create( 0, RF_LR_PAN, rate, chip, RF5C68Update );

	return chip;
}

/************************************************/
/*    RF5C68 update                             */
/************************************************/

INLINE int ILimit(int v, int max, int min) { return v > max ? max : (v < min ? min : v); }

static void RF5C68Update( void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length )
{
	struct rf5c68pcm *chip = param;
	int i, j, tmp;
	unsigned int addr, old_addr;
	signed int ld, rd;
	stream_sample_t  *datap[2];

	datap[RF_L_PAN] = buffer[0];
	datap[RF_R_PAN] = buffer[1];

	memset( datap[RF_L_PAN], 0x00, length * sizeof(*datap[0]) );
	memset( datap[RF_R_PAN], 0x00, length * sizeof(*datap[1]) );

	for( i = 0; i < RF5C68_PCM_MAX; i++ )
	{
		if( (chip->flag[i]&(RF_START|RF_ON)) == (RF_START|RF_ON) )
		{
			/**** PCM setup ****/
			addr = (chip->addr[i]>>BASE_SHIFT)&0xffff;
			ld = (chip->pan[i]&0x0f);
			rd = ((chip->pan[i]>>4)&0x0f);
			/*       cen = (ld + rd) / 4; */
			/*       ld = (chip->env[i]&0xff) * (ld + cen); */
			/*       rd = (chip->env[i]&0xff) * (rd + cen); */
			ld = (chip->env[i]&0xff) * ld;
			rd = (chip->env[i]&0xff) * rd;

			for( j = 0; j < length; j++ )
			{
				old_addr = addr;
				addr = (chip->addr[i]>>BASE_SHIFT)&0xffff;
				for(; old_addr <= addr; old_addr++ )
				{
					/**** PCM end check ****/
					if( (((unsigned int)chip->pcmbuf[old_addr])&0x00ff) == (unsigned int)0x00ff )
					{
						chip->addr[i] = chip->loop[i] + ((addr - old_addr)<<BASE_SHIFT);
						addr = (chip->addr[i]>>BASE_SHIFT)&0xffff;
						/**** PCM loop check ****/
						if( (((unsigned int)chip->pcmbuf[addr])&0x00ff) == (unsigned int)0x00ff )	// this causes looping problems
						{
							chip->flag[i] = 0;
							break;
						}
						else
						{
							old_addr = addr;
						}
					}
#ifdef PCM_NORMALIZE
					tmp = chip->pcmd[i];
					chip->pcmd[i] = (chip->pcmbuf[old_addr]&0x7f) * (-1 + (2 * ((chip->pcmbuf[old_addr]>>7)&0x01)));
					chip->pcma[i] = (tmp - chip->pcmd[i]) / 2;
					chip->pcmd[i] += chip->pcma[i];
#endif
				}
				chip->addr[i] += chip->step[i];
				if( !chip->flag[i] )  break; /* skip PCM */
#ifndef PCM_NORMALIZE
				if( chip->pcmbuf[addr]&0x80 )
				{
					chip->pcmx[0][i] = ((signed int)(chip->pcmbuf[addr]&0x7f))*ld;
					chip->pcmx[1][i] = ((signed int)(chip->pcmbuf[addr]&0x7f))*rd;
				}
				else
				{
					chip->pcmx[0][i] = ((signed int)(-(chip->pcmbuf[addr]&0x7f)))*ld;
					chip->pcmx[1][i] = ((signed int)(-(chip->pcmbuf[addr]&0x7f)))*rd;
				}
#else
				chip->pcmx[0][i] = chip->pcmd[i] * ld;
				chip->pcmx[1][i] = chip->pcmd[i] * rd;
#endif
				*(datap[RF_L_PAN] + j) = ILimit( ((int)*(datap[RF_L_PAN] + j) + ((chip->pcmx[0][i])>>4)), 32767, -32768 );
				*(datap[RF_R_PAN] + j) = ILimit( ((int)*(datap[RF_R_PAN] + j) + ((chip->pcmx[1][i])>>4)), 32767, -32768 );
			}
		}
	}
}

/************************************************/
/*    RF5C68 write register                     */
/************************************************/
WRITE8_HANDLER( RF5C68_reg_w )
{
	struct rf5c68pcm *chip = sndti_token(SOUND_RF5C68, 0);
	int  i;
	int  val;

	chip->wreg[offset] = data;			/* stock write data */
	/**** set PCM registers ****/

	switch( offset )
	{
		case 0x00:
			chip->env[chip->reg_port] = data;		/* set env. */
			break;
		case 0x01:
			chip->pan[chip->reg_port] = data;		/* set pan */
			break;
		case 0x02:
		case 0x03:
			/**** address step ****/
			val = (((((int)chip->wreg[0x03])<<8)&0xff00) | (((int)chip->wreg[0x02])&0x00ff));
			chip->step[chip->reg_port] = (int)(
			(
			( 28456.0 / (float)chip->emulation_rate ) *
			( val / (float)(0x0800) ) *
			(chip->clock / 8000000.0) *
			(1<<BASE_SHIFT)
			)
			);
			break;
		case 0x04:
		case 0x05:
			/**** loop address ****/
			chip->loop[chip->reg_port] = (((((unsigned int)chip->wreg[0x05])<<8)&0xff00)|(((unsigned int)chip->wreg[0x04])&0x00ff))<<(BASE_SHIFT);
			break;
		case 0x06:
			/**** start address ****/
			chip->start[chip->reg_port] = (((unsigned int)chip->wreg[0x06])&0x00ff)<<(BASE_SHIFT + 8);
			chip->addr[chip->reg_port] = chip->start[chip->reg_port];
			break;
		case 0x07:
			chip->reg_port = chip->wreg[0x07]&0x07;
			if( data&0x80 )
			{
				chip->pcmx[0][chip->reg_port] = 0;
				chip->pcmx[1][chip->reg_port] = 0;
				chip->flag[chip->reg_port] |= RF_START;
			}
			break;

		case 0x08:
			/**** pcm on/off ****/
			for( i = 0; i < RF5C68_PCM_MAX; i++ )
			{
				if( !(data&(1<<i)) )
				{
					chip->flag[i] |= RF_ON;	/* PCM on */
				}
				else
				{
					chip->flag[i] &= ~(RF_ON); /* PCM off */
				}
			}
			break;
	}
}

/************************************************/
/*    RF5C68 read memory                        */
/************************************************/
READ8_HANDLER( RF5C68_r )
{
	struct rf5c68pcm *chip = sndti_token(SOUND_RF5C68, 0);
	unsigned int  bank;
	bank = chip->wreg[0x7] & 0x40 ? chip->start[chip->reg_port] >> BASE_SHIFT : (chip->wreg[0x7] & 0xf) << 12;
	return chip->pcmbuf[(bank + offset) & 0xffff];
}
/************************************************/
/*    RF5C68 write memory                       */
/************************************************/
WRITE8_HANDLER( RF5C68_w )
{
	struct rf5c68pcm *chip = sndti_token(SOUND_RF5C68, 0);
	unsigned int  bank;
	bank = chip->wreg[0x7] & 0x40 ? chip->start[chip->reg_port] >> BASE_SHIFT : (chip->wreg[0x7] & 0xf) << 12;
	chip->pcmbuf[(bank + offset) & 0xffff] = data;
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void rf5c68_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void rf5c68_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = rf5c68_set_info;		break;
		case SNDINFO_PTR_START:							info->start = rf5c68_start;				break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "RF5C68";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Ricoh PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

/**************** end of file ****************/
