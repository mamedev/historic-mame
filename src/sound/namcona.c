/*
	Namco NA1/2 Sound Hardware

	PCM samples and sound sequencing metadata are written by the main CPU to
	shared RAM.

	The sound CPU's type is unknown, though it appears to be little endian.
	It has an internal BIOS.

	RAM[0x820]:			song select
	RAM[0x822]:			song control (fade?)
	RAM[0x824..0x89e]:
		Even addresses are used to select the sound effect.
		0x40 is written to odd addresses as a signal to play the requested sound.

	Metadata vectors:
	addr  sample
	0000: 0012   // table of addresses for each VOX sequence
	0002: 07e4   // table of wave records (5 words each record)
	0004: 0794   // unknown table (5 words each record)
	0006: 0000   // unknown (always zero?)
	0008: 0102   // unknown address table
	000a: 0384   // unknown address table
	000c: 0690   // unknown chunk
	000e: 07bc   // unknown table (5 words each record)
	0010: 7700   // unknown
	0012: addresses for each song sequence start here

	Known issues:
	- music is (barely) recognizable
	- many opcodes are ignored or implemented imperfectly
	- the metadata contains several unused tables
	- some voices get "stuck on" especially on startup,
	  when bogus song commands are processed during memory tests
*/

#include <math.h>
#include "driver.h"
#include "namcona1.h" /* for namcona1_gametype; used for game-specific hacks */

#define kTwelfthRootTwo 1.059463094
#define FIXED_POINT_SHIFT (10) /* for mixing */
#define MAX_VOICE 16
#define MAX_SEQUENCE  0x40 /* Both "songs" and "sound effects" are general-purpose sequences.
                            * Only a few sequences are ever simultaneously active.
                            */
#define MAX_SEQUENCE_RECURSION 4 /* ? */
#define TEMPO 10 /* this is an ad-hoc value.  songs surely encode their own tempos */

static int mSampleRate;
static int mStream;
static INT16 *mpMixerBuffer;
static INT32 *mpPitchTable;
static data16_t *mpROM;
static data16_t *mpMetaData;

struct voice
{
	INT32 bActive;

	INT32 flags;
	INT32 start; /* fixed point */
	INT32 end;   /* fixed point */
	INT32 loop;  /* fixed point */
	INT32 baseFreq;

	INT32 delta; /* fixed point */
	INT32 pos;
} mVoice[MAX_VOICE];

struct sequence
{
	int bActive;
	data8_t reg1;    /* unknown */
	data8_t reg2;    /* unknown */
	data8_t reg3;    /* unknown */
	data8_t reg23_0; /* unknown */
	data8_t reg23_1; /* bank select for PCM data */
	int addr;
	int pause;
	int channel[8];
	int stackData[MAX_SEQUENCE_RECURSION]; /* used for sub-sequences */
	int stackSize;
	int count; /* used for "repeat" opcode */
} mSequence[MAX_SEQUENCE];

static data8_t
ReadMetaDataByte( int addr )
{
	data16_t data = mpMetaData[addr/2];
	return (addr&1)?(data&0xff):(data>>8);
} /* ReadMetaDataByte */

static data16_t
ReadMetaDataWord( int addr )
{
	return ReadMetaDataByte(addr)+ReadMetaDataByte(addr+1)*256;
} /* ReadMetaDataWord */

static signed char
ReadPCMSample( int addr )
{
	data16_t data16 = mpROM[addr/2];
	int dat = (addr&1)?(data16&0xff):(data16>>8);
	if( dat&0x80 )
	{
		dat = -(dat&0x7f);
	}
	return dat;
} /* ReadPCMSample */

static void
RenderSamples( INT16 **buffer, INT16 *pSource, int length )
{
	int i;
	INT16 * pDest1 = buffer[0];
	INT16 * pDest2 = buffer[1];
	for (i = 0; i < length; i++)
	{
		INT32 data = 100* (*pSource++);
		if( data > 0x7fff )
		{
			data =  0x7fff; /* clip */
		}
		else if( data < -0x8000 )
		{
			data = -0x8000; /* clip */
		}
		*pDest1++ = (INT16)data; /* stereo left */
		*pDest2++ = (INT16)data; /* stereo right (for now, just output same values to both left and right) */
	}
} /* RenderSamples */

static void
PushSequenceAddr( struct sequence *pSequence, int addr )
{
	if( pSequence->stackSize<MAX_SEQUENCE_RECURSION )
	{
		pSequence->stackData[pSequence->stackSize++] = addr;
	}
	else
	{
//		printf( "sound/namcona.c stack overflow!\n" );
		pSequence->bActive = 0;
	}
} /* PushSequenceAddr */

static void
PopSequenceAddr( struct sequence *pSequence )
{
	if( pSequence->stackSize )
	{
		pSequence->addr = pSequence->stackData[--pSequence->stackSize];
	}
	else
	{
		pSequence->bActive = 0;
	}
} /* PopSequenceAddr */

static void
HandleSubroutine( struct sequence *pSequence )
{
	int addr = ReadMetaDataWord( pSequence->addr );
	PushSequenceAddr( pSequence, pSequence->addr+2 );
	pSequence->addr = addr;
} /* HandleSubroutine */

static void
HandleRepeat( struct sequence *pSequence )
{
	int count = ReadMetaDataByte( pSequence->addr++ );
	int addr = ReadMetaDataWord( pSequence->addr );
	if( pSequence->count < count )
	{
		pSequence->count++;
		pSequence->addr = addr;
	}
	else
	{
		pSequence->addr += 2;
	}
} /* HandleRepeat */

static void
MapArgs( struct sequence *pSequence,
		 int bCommon,
		 void (*callback)( struct sequence *, int chan, int data ) )
{
	int set = ReadMetaDataByte(pSequence->addr++);
	int data = 0;
	int i;
	if( bCommon )
	{
		data = ReadMetaDataByte( pSequence->addr++ );
	}
	for( i=0; i<8; i++ )
	{
		if( set&(1<<i) )
		{
			if( !bCommon )
			{
				data = ReadMetaDataByte( pSequence->addr++ );
			}
			callback( pSequence, i, data );
		}
	}
} /* MapArgs */

static void
AssignChannel( struct sequence *pSequence, int chan, int data )
{
	if( data<0x10 )
	{
		pSequence->channel[chan] = data;
	}
	else
	{
//		printf( "NAMCONA invalid channel mappling! %02x\n", data );
		pSequence->bActive = 0;
	}
} /* AssignChannel */

static void
IgnoreUnknownOp( struct sequence *pSequence, int chan, int data )
{
} /* IgnoreUnknownOp */

static void
SelectWave( struct sequence *pSequence, int chan, int data )
{
	int bank = 0x20000 + pSequence->reg23_1*0x20000;
	struct voice *pVoice = &mVoice[pSequence->channel[chan]];

	int addr  = ReadMetaDataWord(2)+10*data;

//	int flags = ReadMetaDataWord(addr+0*2);
	/* 0x1000 indicates a looped sample.
	 * For now, always use 0 for flags.
	 */
	pVoice->flags    = 0;

	pVoice->start    = ReadMetaDataWord(addr+1*2)*2+bank;
	pVoice->end      = ReadMetaDataWord(addr+2*2)*2+bank;
	pVoice->loop     = ReadMetaDataWord(addr+3*2);
	pVoice->baseFreq = ReadMetaDataWord(addr+4*2); /* unsure what this is; not currently used */

	pVoice->start <<= FIXED_POINT_SHIFT;
	pVoice->end   <<= FIXED_POINT_SHIFT;
	pVoice->loop  <<= FIXED_POINT_SHIFT;
} /* SelectWave */

static void
PlayNote( struct sequence *pSequence, int chan, int data )
{
	struct voice *pVoice = &mVoice[pSequence->channel[chan]];
	if( data==0xff )
	{
		pVoice->bActive = 0;
	}
	else
	{
		pVoice->delta = mpPitchTable[data];
		pVoice->bActive = 1;
		pVoice->pos = pVoice->start;
	}
} /* PlayNote */

static void
UpdateSequence( struct sequence *pSequence )
{
	while( pSequence->bActive )
	{
		if( pSequence->pause )
		{
			pSequence->pause--;
			return;
		}
		else
		{
			int code = ReadMetaDataByte(pSequence->addr++);
			if( code&0x80 )
			{
				pSequence->pause = TEMPO*((code&0x3f)+1);
			}
			else
			{
				int bCommon = (code&0x40);
				switch( code&0x3f )
				{
				case 0x01: /* ? */
					pSequence->reg1 = ReadMetaDataByte(pSequence->addr++);
					break;

				case 0x02: /* ? */
					pSequence->reg2 = ReadMetaDataByte(pSequence->addr++);
					break;

				case 0x03: /* ? */
					pSequence->reg3 = ReadMetaDataByte(pSequence->addr++);
					break;

				case 0x04:
					HandleSubroutine( pSequence );
					break;

				case 0x05: /* end-of-sequence */
					PopSequenceAddr( pSequence );
					break;

				case 0x06: /* operand is note index */
					MapArgs( pSequence, bCommon, PlayNote );
					pSequence->pause = TEMPO;
					break;

				case 0x07:
					MapArgs( pSequence, bCommon, SelectWave );
					break;

				case 0x08: /* attack? */
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x09:
					pSequence->addr = ReadMetaDataWord(pSequence->addr);
					break;

				case 0x0a:
					HandleRepeat( pSequence ); /* probably wrong! */
					break;

				case 0x0b: /* ? */
				case 0x0c: /* ? */
				case 0x0d: /* ? */
				case 0x0e: /* ? */
				case 0x0f: /* ? */
				case 0x10: /* ? */
				case 0x11: /* ? */
				case 0x12: /* ? */
				case 0x13: /* always used? normally 0x00 */
				case 0x14: /* ? */
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x16: /* stereo/volume-related?
				            * 0x40 left stereo output enable?
							* 0x80 right stereo output enable?
							*/
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x17:
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x19: // gosub?
					pSequence->addr = ReadMetaDataWord(pSequence->addr);
					break;

				case 0x1b: // ?
					pSequence->addr += 2;
					break;

				case 0x1c: // ?
					pSequence->addr += 2;
					break;

				case 0x1e:
					pSequence->addr += 2;
					break;

				case 0x20:
					MapArgs( pSequence, bCommon, AssignChannel );
					break;

				case 0x22:
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x23:
					pSequence->reg23_0 = ReadMetaDataByte(pSequence->addr++); /* unknown */
					pSequence->reg23_1 = ReadMetaDataByte(pSequence->addr++); /* PCM bank select */
					break;

				case 0x24:
					MapArgs( pSequence, bCommon, IgnoreUnknownOp );
					break;

				default:
//					printf( "unknown code = 0x%02x\n",code );
					pSequence->bActive = 0;
					break;
				}
			}
		}
	}
} /* UpdateSequence */

static void
UpdateSequences( void )
{
	int i;
	for( i=0; i<MAX_SEQUENCE; i++ )
	{
		UpdateSequence( &mSequence[i] );
	}
} /* UpdateSequences */

static void
PlaySample( int code, int which )
{
	struct sequence *pSequence = &mSequence[which];
	int offs = ReadMetaDataWord(0)+code*2;
//	printf( "snd = 0x%02x\n", code );
	pSequence->addr = ReadMetaDataWord(offs);
	pSequence->bActive = 1;
} /* PlaySample */

static void
PlaySong( int code )
{
	struct sequence *pSequence = &mSequence[0];
//	printf( "sng = 0x%02x\n", code );

	//return;

	if( pSequence )
	{
		int offs = 0x12+code*2;
		memset( pSequence, 0x00, sizeof(struct sequence) );
		pSequence->addr = ReadMetaDataWord(offs);
		pSequence->bActive = 1;
	}
} /* PlaySong */

void
namcona_mcu_init( void )
{
	int i;
	for( i=0; i<MAX_VOICE; i++ )
	{
		mVoice[i].bActive = 0;
	}
	for( i=0; i<MAX_SEQUENCE; i++ )
	{
		mSequence[i].bActive = 0;
	}
}

static void
UpdateSound( int ch, INT16 **buffer, int length )
{
	int i;
	data16_t data;
	data = mpROM[0x820/2];
	if( data&0x40 )
	{
		mpROM[0x820/2] = data^0x40;
		PlaySong( data>>8 );
	}
	for( i=0x824; i<=0x89e; i+=2 )
	{
		data = mpROM[i/2];
		if( data&0x40 )
		{
			mpROM[i/2] = data^0x40;
			PlaySample( data>>8, (i-0x820)/2 );
		}
	}
	UpdateSequences();
	if( length>mSampleRate ) length = mSampleRate;
	memset(mpMixerBuffer, 0, length * sizeof(INT16));
	for( i=0;i<MAX_VOICE;i++ )
	{
		struct voice *pVoice = &mVoice[i];
		if( pVoice->bActive && pVoice->delta )
		{
			INT32 delta  = pVoice->delta;
			INT32 end    = pVoice->end;
			INT32 pos    = pVoice->pos;
			INT16 *pDest = mpMixerBuffer;
			int j;
			for( j=0; j<length; j++ )
			{
				if( pos >= end )
				{
					if( pVoice->flags&0x1000 )
					{
						pos = pVoice->loop;
					}
					else
					{
						pVoice->bActive=0;
						break;
					}
				}
				*pDest++ += ReadPCMSample(pos>>FIXED_POINT_SHIFT);
				pos += delta;
			}
			pVoice->pos = pos;
		}
	}
	RenderSamples( buffer, mpMixerBuffer, length );
} /* UpdateSound */

int
NAMCONA_sh_start( const struct MachineSound *msound )
{
	const struct NAMCONAinterface *intf = msound->sound_interface;
	int vol[2];
	const char *name[2] = { "NAMCONA Left","NAMCONA Right" };
	vol[0] = intf->mixing_level;
	vol[1] = intf->mixing_level;
	mSampleRate = intf->frequency;
	mStream = stream_init_multi(2,name,vol,mSampleRate,0,UpdateSound);
	mpROM = (data16_t *)memory_region(REGION_CPU1);
	if( namcona1_gametype==NAMCO_KNCKHEAD )
	{
		mpMetaData = mpROM+0x10000/2;
	}
	else
	{
		mpMetaData = mpROM+0x70000/2;
	}
	memset( mVoice, 0x00, sizeof(mVoice) );
	mpMixerBuffer = auto_malloc( sizeof(INT16)*mSampleRate );
	if( mpMixerBuffer )
	{
		mpPitchTable = auto_malloc( sizeof(INT32)*0xff );
		if( mpPitchTable )
		{
			int i;
			for( i=0; i<0xff; i++ )
			{
				int data = i;
				double freq = freq = (1<<FIXED_POINT_SHIFT)/4;
				while( data>0x3a )
				{
					data--;
					freq *= kTwelfthRootTwo;
				}
				while( data<0x3a )
				{
					data++;
					freq /= kTwelfthRootTwo;
				}
				mpPitchTable[i] = (INT32)freq;
			}
			return 0;
		}
	}
	return 1;
} /* NAMCONA_sh_start */

void
NAMCONA_sh_stop( void )
{
} /* NAMCONA_sh_stop */
