/***************************************************************************

	PSX SPU

	CXD2922BQ/CXD2925Q

	preliminary version by smf.

***************************************************************************/

#include "driver.h"
#include "state.h"

struct PSXSPUinterface *intf;

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

#define MAX_CHANNEL ( 24 )
#define SPU_RAM_SIZE ( 512 * 1024 )
#define SAMPLES_PER_BLOCK ( 28 )
#define PITCH_SHIFT ( 12 )

static data32_t *g_p_n_psxram;
static data16_t m_n_mainvolumeleft;
static data16_t m_n_mainvolumeright;
static data16_t m_n_reverberationdepthleft;
static data16_t m_n_reverberationdepthright;
static data32_t m_n_voiceon;
static data32_t m_n_voiceoff;
static data32_t m_n_modulationmode;
static data32_t m_n_noisemode;
static data32_t m_n_reverbmode;
static data32_t m_n_channelonoff;
static data16_t m_n_reverbworkareastart;
static data16_t m_n_irqaddress;
static data32_t m_n_spuoffset;
static data16_t m_n_spudata;
static data16_t m_n_spucontrol;
static data32_t m_n_spustatus;
static data16_t m_n_cdvolumeleft;
static data16_t m_n_cdvolumeright;
static data16_t m_n_externalvolumeleft;
static data16_t m_n_externalvolumeright;
static data16_t m_p_n_volumeleft[ MAX_CHANNEL ];
static data16_t m_p_n_volumeright[ MAX_CHANNEL ];
static data16_t m_p_n_pitch[ MAX_CHANNEL ];
static data16_t m_p_n_address[ MAX_CHANNEL ];
static data16_t m_p_n_attackdecaysustain[ MAX_CHANNEL ];
static data16_t m_p_n_sustainrelease[ MAX_CHANNEL ];
static data16_t m_p_n_adsrvolume[ MAX_CHANNEL ];
static data16_t m_p_n_repeataddress[ MAX_CHANNEL ];
static data32_t m_p_n_effect[ 16 ];
static data16_t *m_p_n_spuram;
static data32_t m_p_n_blockaddress[ MAX_CHANNEL ];
static data32_t m_p_n_blockoffset[ MAX_CHANNEL ];
static data32_t m_p_n_blockstatus[ MAX_CHANNEL ];
static INT16 m_p_n_blockbuffer[ MAX_CHANNEL * SAMPLES_PER_BLOCK ];
static INT16 m_p_n_s1[ MAX_CHANNEL ];
static INT16 m_p_n_s2[ MAX_CHANNEL ];
static data32_t m_n_loop[ MAX_CHANNEL ];

#define SPU_REG( a ) ( ( a - 0xc00 ) / 4 )
#define SPU_CHANNEL_REG( a ) ( a / 4 )

INLINE int volume( data16_t n_volume )
{
	if( ( n_volume & 0x8000 ) != 0 )
	{
		n_volume = ( n_volume & 0x7f ) * 0x80;
	}
	else if( ( n_volume & 0x4000 ) != 0 )
	{
		n_volume = -( n_volume & 0x3fff );
	}
	return n_volume;
}

INLINE int limit( int v )
{
	if( v < -32768 )
	{
		return -32768;
	}
	else if( v > 32767 )
	{
		return 32767;
	}
	return v;
}

static void PSXSPU_update(int num, INT16 **buffer, int length)
{
	int v;
	int voll;
	int volr;
	int n_channel;
	int n_sample;
	int n_word;
	int n_shift;
	int n_predict;
	int n_flags;
	int n_nibble;
	int n_packed;
	int n_unpacked;

	static const int f[ 5 ][ 2 ] =
	{
		{   0,   0  },
		{  60,   0  },
		{ 115, -52 },
		{  98, -55 },
		{ 122, -60 }
	};

	memset( buffer[ 0 ], 0, length * 2 );
	memset( buffer[ 1 ], 0, length * 2 );

	for( n_channel = 0; n_channel < MAX_CHANNEL; n_channel++ )
	{
		voll = volume( m_p_n_volumeleft[ n_channel ] );
		volr = volume( m_p_n_volumeright[ n_channel ] );

		for( n_sample = 0; n_sample < length; n_sample++ )
		{
			if( m_p_n_blockoffset[ n_channel ] >= ( SAMPLES_PER_BLOCK << PITCH_SHIFT ) )
			{
				if( !m_p_n_blockstatus[ n_channel ] )
				{
					break;
				}
				if( ( m_n_spucontrol & 0x40 ) != 0 &&
					( m_n_irqaddress * 4 ) >= m_p_n_blockaddress[ n_channel ] &&
					( m_n_irqaddress * 4 ) <= m_p_n_blockaddress[ n_channel ] + 7 )
				{
					intf->irq_set( 0x0200 );
				}

				n_shift =   ( m_p_n_spuram[ m_p_n_blockaddress[ n_channel ] ] >> 0 ) & 0x0f;
				n_predict = ( m_p_n_spuram[ m_p_n_blockaddress[ n_channel ] ] >> 4 ) & 0x0f;
				n_flags =   ( m_p_n_spuram[ m_p_n_blockaddress[ n_channel ] ] >> 8 ) & 0xff;
				if( ( n_flags & 4 ) != 0 )
				{
					m_n_loop[ n_channel ] = m_p_n_blockaddress[ n_channel ];
				}
				m_p_n_blockaddress[ n_channel ]++;
				m_p_n_blockaddress[ n_channel ] %= ( SPU_RAM_SIZE / 2 );

				for( n_word = 0; n_word < 7; n_word++ )
				{
					n_packed = m_p_n_spuram[ m_p_n_blockaddress[ n_channel ] ];
					m_p_n_blockaddress[ n_channel ]++;
					m_p_n_blockaddress[ n_channel ] %= ( SPU_RAM_SIZE / 2 );

					for( n_nibble = 0; n_nibble < 4; n_nibble++ )
					{
						n_unpacked = ( ( n_packed & 0xf ) << 12 );
						if( ( n_unpacked & 0x8000 ) != 0 )
						{
							n_unpacked |= 0xffff0000;
						}
						n_unpacked = ( n_unpacked >> n_shift ) +
							( ( m_p_n_s1[ n_channel ] * f[ n_predict ][ 0 ] ) >> 6 ) +
							( ( m_p_n_s2[ n_channel ] * f[ n_predict ][ 1 ] ) >> 6 );
						m_p_n_s2[ n_channel ] = m_p_n_s1[ n_channel ];
						m_p_n_s1[ n_channel ] = n_unpacked;

						m_p_n_blockbuffer[ ( n_channel * SAMPLES_PER_BLOCK ) + ( n_word * 4 ) + n_nibble ] = n_unpacked;
						n_packed >>= 4;
					}
				}
				if( ( n_flags & 1 ) != 0 )
				{
					if( n_flags != 3 )
					{
						m_p_n_blockstatus[ n_channel ] = 0;
					}
					else
					{
						m_p_n_blockaddress[ n_channel ] = m_n_loop[ n_channel ];
					}
				}
				m_p_n_blockoffset[ n_channel ] %= ( SAMPLES_PER_BLOCK << PITCH_SHIFT );
			}
			v = m_p_n_blockbuffer[ ( n_channel * SAMPLES_PER_BLOCK ) + ( m_p_n_blockoffset[ n_channel ] >> PITCH_SHIFT ) ];
			m_p_n_blockoffset[ n_channel ] += m_p_n_pitch[ n_channel ];
			buffer[ 0 ][ n_sample ] = limit( buffer[ 0 ][ n_sample ] + ( ( v * voll ) / 0x4000 ) );
			buffer[ 1 ][ n_sample ] = limit( buffer[ 1 ][ n_sample ] + ( ( v * volr ) / 0x4000 ) );
		}
	}
}

static void spu_read( UINT32 n_address, INT32 n_size )
{
	verboselog( 1, "spu_read( %08x, %08x )\n", n_address, n_size );

	while( n_size > 0 )
	{
		g_p_n_psxram[ n_address / 4 ] = 
			( m_p_n_spuram[ m_n_spuoffset + 0 ] << 0 ) |
			( m_p_n_spuram[ m_n_spuoffset + 1 ] << 16 );
		verboselog( 2, "%08x > %04x\n", m_n_spuoffset + 0, m_p_n_spuram[ m_n_spuoffset + 0 ] );
		verboselog( 2, "%08x > %04x\n", m_n_spuoffset + 1, m_p_n_spuram[ m_n_spuoffset + 1 ] );
		m_n_spuoffset += 2;
		m_n_spuoffset %= ( SPU_RAM_SIZE / 2 );
		n_address += 4;
		n_size--;
	}
}

static void spu_write( UINT32 n_address, INT32 n_size )
{
	verboselog( 1, "spu_write( %08x, %08x )\n", n_address, n_size );

	while( n_size > 0 )
	{
		m_p_n_spuram[ m_n_spuoffset + 0 ] = ( g_p_n_psxram[ n_address / 4 ] >> 0 );
		m_p_n_spuram[ m_n_spuoffset + 1 ] = ( g_p_n_psxram[ n_address / 4 ] >> 16 );
		verboselog( 2, "%08x < %04x\n", m_n_spuoffset + 0, m_p_n_spuram[ m_n_spuoffset + 0 ] );
		verboselog( 2, "%08x < %04x\n", m_n_spuoffset + 1, m_p_n_spuram[ m_n_spuoffset + 1 ] );
		m_n_spuoffset += 2;
		m_n_spuoffset %= ( SPU_RAM_SIZE / 2 );
		n_address += 4;
		n_size--;
	}
}

int PSX_sh_start( const struct MachineSound *msound )
{
	int n_effect;
	int n_channel;
	const char *name[2];
	int vol[2];

	intf = msound->sound_interface;
	g_p_n_psxram = *(intf->p_psxram);

	m_n_mainvolumeleft = 0;
	m_n_mainvolumeright = 0;
	m_n_reverberationdepthleft = 0;
	m_n_reverberationdepthright = 0;
	m_n_voiceon = 0;
	m_n_voiceoff = 0;
	m_n_modulationmode = 0;
	m_n_noisemode = 0;
	m_n_reverbmode = 0;
	m_n_channelonoff = 0;
	m_n_reverbworkareastart = 0;
	m_n_irqaddress = 0;
	m_n_spuoffset = 0;
	m_n_spudata = 0;
	m_n_spucontrol = 0;
	m_n_spustatus = 0;
	m_n_cdvolumeleft = 0;
	m_n_cdvolumeright = 0;
	m_n_externalvolumeleft = 0;
	m_n_externalvolumeright = 0;

	for( n_channel = 0; n_channel < MAX_CHANNEL; n_channel++ )
	{
		m_p_n_volumeleft[ n_channel ] = 0;
		m_p_n_volumeright[ n_channel ] = 0;
		m_p_n_pitch[ n_channel ] = 0;
		m_p_n_address[ n_channel ] = 0;
		m_p_n_attackdecaysustain[ n_channel ] = 0;
		m_p_n_sustainrelease[ n_channel ] = 0;
		m_p_n_adsrvolume[ n_channel ] = 0;
		m_p_n_repeataddress[ n_channel ] = 0;
		m_p_n_blockaddress[ n_channel ] = 0;
		m_p_n_blockoffset[ n_channel ] = 0;
		m_p_n_blockstatus[ n_channel ] = 0;
		{
			char s[ 1024 ];
			sprintf( s, "SPU%d", n_channel );
		}
	}

	for( n_effect = 0; n_effect < 16; n_effect++ )
	{
		m_p_n_effect[ n_effect ] = 0;
	}

	m_p_n_spuram = malloc( SPU_RAM_SIZE );
	if( m_p_n_spuram == NULL )
	{
		return 1;
	}

	state_save_register_UINT16( "psx", 0, "m_n_mainvolumeleft", &m_n_mainvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_mainvolumeright", &m_n_mainvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthleft", &m_n_reverberationdepthleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthright", &m_n_reverberationdepthright, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_voiceon", &m_n_voiceon, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_voiceoff", &m_n_voiceoff, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_modulationmode", &m_n_modulationmode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_noisemode", &m_n_noisemode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_reverbmode", &m_n_reverbmode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_channelonoff", &m_n_channelonoff, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverbworkareastart", &m_n_reverbworkareastart, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_irqaddress", &m_n_irqaddress, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_spuoffset", &m_n_spuoffset, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_spudata", &m_n_spudata, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_spucontrol", &m_n_spucontrol, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_spustatus", &m_n_spustatus, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_cdvolumeleft", &m_n_cdvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_cdvolumeright", &m_n_cdvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_externalvolumeleft", &m_n_externalvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_externalvolumeright", &m_n_externalvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_p_n_volumeleft", m_p_n_volumeleft, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_volumeright", m_p_n_volumeright, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_pitch", m_p_n_pitch, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_address", m_p_n_address, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_attackdecaysustain", m_p_n_attackdecaysustain, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_sustainrelease", m_p_n_sustainrelease, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_adsrvolume", m_p_n_adsrvolume, MAX_CHANNEL );
	state_save_register_UINT16( "psx", 0, "m_p_n_repeataddress", m_p_n_repeataddress, MAX_CHANNEL );
	state_save_register_UINT32( "psx", 0, "m_p_n_effect", m_p_n_effect, 16 );
	state_save_register_UINT16( "psx", 0, "m_p_n_spuram", m_p_n_spuram, SPU_RAM_SIZE / 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_blockaddress", m_p_n_blockaddress, MAX_CHANNEL );
	state_save_register_UINT32( "psx", 0, "m_p_n_blockoffset", m_p_n_blockoffset, MAX_CHANNEL );
	state_save_register_UINT32( "psx", 0, "m_p_n_blockstatus", m_p_n_blockstatus, MAX_CHANNEL );
	state_save_register_INT16( "psx", 0, "m_p_n_blockbuffer", m_p_n_blockbuffer, MAX_CHANNEL * SAMPLES_PER_BLOCK );
	state_save_register_INT16( "psx", 0, "m_p_n_s1", m_p_n_s1, MAX_CHANNEL );
	state_save_register_INT16( "psx", 0, "m_p_n_s2", m_p_n_s2, MAX_CHANNEL );
	state_save_register_UINT32( "psx", 0, "m_n_loop", m_n_loop, MAX_CHANNEL );

	intf->spu_install_read_handler( 4, spu_read );
	intf->spu_install_write_handler( 4, spu_write );

	name[ 0 ] = "PSX SPU L";
	name[ 1 ] = "PSX SPU R";
	vol[ 0 ] = MIXER( intf->mixing_level, MIXER_PAN_LEFT );
	vol[ 1 ] = MIXER( intf->mixing_level, MIXER_PAN_RIGHT );
	stream_init_multi( 2, name, vol, 44100, 0, PSXSPU_update );
	return 0;
}

void PSX_sh_stop( void )
{
	free( m_p_n_spuram );
	m_p_n_spuram = NULL;
}

static data32_t psx_spu_delay = 0;

WRITE32_HANDLER( psx_spu_delay_w )
{
	COMBINE_DATA( &psx_spu_delay );
	verboselog( 1, "psx_spu_delay_w( %08x %08x )\n", data, mem_mask );
}

READ32_HANDLER( psx_spu_delay_r )
{
	verboselog( 1, "psx_spu_delay_r( %08x )\n", mem_mask );
	return psx_spu_delay;
}

READ32_HANDLER( psx_spu_r )
{
	int n_channel;
	n_channel = offset / 4;
	if( n_channel < MAX_CHANNEL )
	{
		switch( offset % 4 )
		{
		case SPU_CHANNEL_REG( 0x8 ):
			if( ACCESSING_LSW32 )
			{
				verboselog( 1, "psx_spu_r() channel %d attack/decay/sustain = %04x\n", n_channel, m_p_n_attackdecaysustain[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				verboselog( 1, "psx_spu_r() channel %d sustain/release = %04x\n", n_channel, m_p_n_sustainrelease[ n_channel ] );
			}
			return ( m_p_n_sustainrelease[ n_channel ] << 16 ) | m_p_n_attackdecaysustain[ n_channel ];
		case SPU_CHANNEL_REG( 0xc ):
			if( ACCESSING_LSW32 )
			{
				verboselog( 1, "psx_spu_r() channel %d adsr volume = %04x\n", n_channel, m_p_n_adsrvolume[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				verboselog( 1, "psx_spu_r() channel %d repeat address = %04x\n", n_channel, m_p_n_repeataddress[ n_channel ] );
			}
			return ( m_p_n_repeataddress[ n_channel ] << 16 ) | m_p_n_adsrvolume[ n_channel ];
		default:
			verboselog( 0, "psx_spu_r( %08x, %08x ) channel %d reg %d\n", offset, mem_mask, n_channel, offset % 4 ); 
			return 0;
		}
	}
	else
	{
		switch( offset )
		{
		case SPU_REG( 0xd88 ):
			verboselog( 1, "psx_spu_r( %08x ) voice on = %08x\n", mem_mask, m_n_voiceon );
			return m_n_voiceon;
		case SPU_REG( 0xd8c ):
			verboselog( 1, "psx_spu_r( %08x ) voice off = %08x\n", mem_mask, m_n_voiceoff );
			return m_n_voiceoff;
		case SPU_REG( 0xd90 ):
			verboselog( 1, "psx_spu_r( %08x ) modulation mode = %08x\n", mem_mask, m_n_modulationmode );
			return m_n_modulationmode;
		case SPU_REG( 0xd94 ):
			verboselog( 1, "psx_spu_r( %08x ) noise mode = %08x\n", mem_mask, m_n_noisemode );
			return m_n_noisemode;
		case SPU_REG( 0xd98 ):
			verboselog( 1, "psx_spu_r( %08x ) reverb mode = %08x\n", mem_mask, m_n_reverbmode );
			return m_n_reverbmode;
		case SPU_REG( 0xda4 ):
			verboselog( 1, "psx_spu_r( %08x ) dma/irq address = %08x\n", mem_mask, ( ( m_n_spuoffset / 4 ) << 16 ) | m_n_irqaddress );
			return ( ( m_n_spuoffset / 4 ) << 16 ) | m_n_irqaddress;
		case SPU_REG( 0xda8 ):
			verboselog( 1, "psx_spu_r( %08x ) spu control/data = %08x\n", mem_mask, ( m_n_spucontrol << 16 ) | m_n_spudata );
			return m_n_spudata | ( m_n_spucontrol << 16 );
		case SPU_REG( 0xdac ):
			verboselog( 1, "psx_spu_r( %08x ) spu status = %08x\n", mem_mask, m_n_spustatus );
			return m_n_spustatus;
		default:
			verboselog( 0, "psx_spu_r( %08x, %08x ) %08x\n", offset, mem_mask, 0xc00 + ( offset * 4 ) );
			return 0;
		}
	}
}

WRITE32_HANDLER( psx_spu_w )
{
	int n_channel;
	n_channel = offset / 4;
	if( n_channel < MAX_CHANNEL )
	{
		switch( offset % 4 )
		{
		case SPU_CHANNEL_REG( 0x0 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_volumeleft[ n_channel ] = data & 0xffff;
				verboselog( 1, "psx_spu_w() channel %d volume left = %04x\n", n_channel, m_p_n_volumeleft[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_volumeright[ n_channel ] = data >> 16;
				verboselog( 1, "psx_spu_w() channel %d volume right = %04x\n", n_channel, m_p_n_volumeright[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0x4 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_pitch[ n_channel ] = data & 0xffff;
				verboselog( 1, "psx_spu_w() channel %d pitch = %04x\n", n_channel, m_p_n_pitch[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_address[ n_channel ] = data >> 16;
				verboselog( 1, "psx_spu_w() channel %d address = %04x\n", n_channel, m_p_n_address[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0x8 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_attackdecaysustain[ n_channel ] = data & 0xffff;
				verboselog( 1, "psx_spu_w() channel %d attack/decay/sustain = %04x\n", n_channel, m_p_n_attackdecaysustain[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_sustainrelease[ n_channel ] = data >> 16;
				verboselog( 1, "psx_spu_w() channel %d sustain/release = %04x\n", n_channel, m_p_n_sustainrelease[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0xc ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_adsrvolume[ n_channel ] = data & 0xffff;
				verboselog( 1, "psx_spu_w() channel %d adsr volume = %04x\n", n_channel, m_p_n_adsrvolume[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_repeataddress[ n_channel ] = data >> 16;
				verboselog( 1, "psx_spu_w() channel %d repeat address = %04x\n", n_channel, m_p_n_repeataddress[ n_channel ] );
			}
			break;
		default:
			verboselog( 0, "psx_spu_w( %08x, %08x, %08x ) channel %d reg %d\n", offset, mem_mask, data, n_channel, offset % 4 ); 
			break;
		}
	}
	else
	{
		switch( offset )
		{
		case SPU_REG( 0xd80 ):
			if( ACCESSING_LSW32 )
			{
				m_n_mainvolumeleft = data & 0xffff;
				verboselog( 1, "psx_spu_w() main volume left = %04x\n", m_n_mainvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_mainvolumeright = data >> 16;
				verboselog( 1, "psx_spu_w() main volume right = %04x\n", m_n_mainvolumeright );
			}
			break;
		case SPU_REG( 0xd84 ):
			if( ACCESSING_LSW32 )
			{
				m_n_reverberationdepthleft = data & 0xffff;
				verboselog( 1, "psx_spu_w() reverberation depth left = %04x\n", m_n_reverberationdepthleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_reverberationdepthright = data >> 16;
				verboselog( 1, "psx_spu_w() reverberation depth right = %04x\n", m_n_reverberationdepthright );
			}
			break;
		case SPU_REG( 0xd88 ):
			m_n_voiceon = 0;
			COMBINE_DATA( &m_n_voiceon );
			verboselog( 1, "psx_spu_w() voice on = %08x\n", m_n_voiceon );
			for( n_channel = 0; n_channel < 32; n_channel++ )
			{
				if( ( m_n_voiceon & ( 1 << n_channel ) ) != 0 )
				{
					m_p_n_blockaddress[ n_channel ] = ( m_p_n_address[ n_channel ] * 4 ) % ( SPU_RAM_SIZE / 2 );
					m_p_n_blockoffset[ n_channel ] = ( SAMPLES_PER_BLOCK << PITCH_SHIFT );
					m_p_n_s1[ n_channel ] = 0;
					m_p_n_s2[ n_channel ] = 0;
					m_p_n_blockstatus[ n_channel ] = 1;
				}
			}
			break;
		case SPU_REG( 0xd8c ):
			m_n_voiceoff = 0;
			COMBINE_DATA( &m_n_voiceoff );
			verboselog( 1, "psx_spu_w() voice off = %08x\n", m_n_voiceoff );
			break;
		case SPU_REG( 0xd90 ):
			COMBINE_DATA( &m_n_modulationmode );
			verboselog( 1, "psx_spu_w() modulation mode = %08x\n", m_n_modulationmode );
			break;
		case SPU_REG( 0xd94 ):
			COMBINE_DATA( &m_n_noisemode );
			verboselog( 1, "psx_spu_w() noise mode = %08x\n", m_n_noisemode );
			break;
		case SPU_REG( 0xd98 ):
			COMBINE_DATA( &m_n_reverbmode );
			verboselog( 1, "psx_spu_w() reverb mode = %08x\n", m_n_reverbmode );
			break;
		case SPU_REG( 0xd9c ):
			COMBINE_DATA( &m_n_channelonoff );
			verboselog( 1, "psx_spu_w() channel on/off = %08x\n", m_n_channelonoff );
			break;
		case SPU_REG( 0xda0 ):
			if( ACCESSING_LSW32 )
			{
				verboselog( 0, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			}
			if( ACCESSING_MSW32 )
			{
				m_n_reverbworkareastart = data >> 16;
				verboselog( 1, "psx_spu_w() reverb work area start = %04x\n", m_n_reverbworkareastart );
			}
			break;
		case SPU_REG( 0xda4 ):
			if( ACCESSING_LSW32 )
			{
				m_n_irqaddress = data & 0xffff;
				verboselog( 1, "psx_spu_w() irq address = %04x\n", m_n_irqaddress );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_spuoffset = ( data >> 16 ) * 4;
				m_n_spuoffset %= ( SPU_RAM_SIZE / 2 );
				verboselog( 1, "psx_spu_w() spu offset = %04x\n", m_n_spuoffset );
			}
			break;
		case SPU_REG( 0xda8 ):
			if( ACCESSING_LSW32 )
			{
				m_n_spudata = data & 0xffff;
				m_p_n_spuram[ m_n_spuoffset++ ] = m_n_spudata;
				m_n_spuoffset %= ( SPU_RAM_SIZE / 2 );
				verboselog( 1, "psx_spu_w() spu data = %04x\n", m_n_spudata );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_spucontrol = data >> 16;
				verboselog( 1, "psx_spu_w() spu control = %04x\n", m_n_spucontrol );
			}
			break;
		case SPU_REG( 0xdac ):
			COMBINE_DATA( &m_n_spustatus );
			m_n_spustatus &= 0xf800ffff;
			verboselog( 1, "psx_spu_w() spu status = %08x\n", m_n_spustatus );
			break;
		case SPU_REG( 0xdb0 ):
			if( ACCESSING_LSW32 )
			{
				m_n_cdvolumeleft = data & 0xffff;
				verboselog( 1, "psx_spu_w() cd volume left = %04x\n", m_n_cdvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_cdvolumeright = data >> 16;
				verboselog( 1, "psx_spu_w() cd volume right = %04x\n", m_n_cdvolumeright );
			}
			break;
		case SPU_REG( 0xdb4 ):
			if( ACCESSING_LSW32 )
			{
				m_n_externalvolumeleft = data & 0xffff;
				verboselog( 1, "psx_spu_w() external volume left = %04x\n", m_n_externalvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_externalvolumeright = data >> 16;
				verboselog( 1, "psx_spu_w() external volume right = %04x\n", m_n_externalvolumeright );
			}
			break;
		case SPU_REG( 0xdc0 ):
		case SPU_REG( 0xdc4 ):
		case SPU_REG( 0xdc8 ):
		case SPU_REG( 0xdcc ):
		case SPU_REG( 0xdd0 ):
		case SPU_REG( 0xdd4 ):
		case SPU_REG( 0xdd8 ):
		case SPU_REG( 0xddc ):
		case SPU_REG( 0xde0 ):
		case SPU_REG( 0xde4 ):
		case SPU_REG( 0xde8 ):
		case SPU_REG( 0xdec ):
		case SPU_REG( 0xdf0 ):
		case SPU_REG( 0xdf4 ):
		case SPU_REG( 0xdf8 ):
		case SPU_REG( 0xdfc ):
			COMBINE_DATA( &m_p_n_effect[ offset & 0x0f ] );
			verboselog( 1, "psx_spu_w() effect %d = %04x\n", offset & 0x0f, m_p_n_effect[ offset & 0x0f ] );
			break;
		default:
			verboselog( 0, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			break;
		}
	}
}
