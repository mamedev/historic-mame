/***************************************************************************

  machine/psx.c

  Thanks to Olivier Galibert for IDCT information

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "includes/psx.h"

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
		if( cpu_getactivecpu() != -1 )
		{
			logerror( "%08x: %s", activecpu_get_pc(), buf );
		}
		else
		{
			logerror( "(timer) : %s", buf );
		}
	}
}

data32_t *g_p_n_psxram;
size_t g_n_psxramsize;

INLINE UINT8 psxreadbyte( UINT32 n_address )
{
	return *( (UINT8 *)g_p_n_psxram + BYTE_XOR_LE( n_address ) );
}

INLINE void psxwriteword( data32_t n_address, data16_t n_data )
{
	*( (UINT16 *)( (UINT8 *)g_p_n_psxram + WORD_XOR_LE( n_address ) ) ) = n_data;
}

INLINE UINT16 psxreadword( UINT32 n_address )
{
	return *( (UINT16 *)( (UINT8 *)g_p_n_psxram + WORD_XOR_LE( n_address ) ) );
}

READ32_HANDLER( psx_com_delay_r )
{
	verboselog( 1, "psx_com_delay_r()\n" );
	return 0;
}

/* IRQ */

static UINT32 m_n_irqdata;
static UINT32 m_n_irqmask;

static void psx_irq_update( void )
{
	if( ( m_n_irqdata & m_n_irqmask ) != 0 )
	{
		verboselog( 2, "psx irq assert\n" );
		cpu_set_irq_line( 0, 0, ASSERT_LINE );
	}
	else
	{
		verboselog( 2, "psx irq clear\n" );
		cpu_set_irq_line( 0, 0, CLEAR_LINE );
	}
}

WRITE32_HANDLER( psx_irq_w )
{
	switch( offset )
	{
	case 0x00:
		verboselog( 2, "psx irq data ( %08x, %08x ) %08x -> %08x\n", data, mem_mask, m_n_irqdata, ( m_n_irqdata & mem_mask ) | ( m_n_irqdata & m_n_irqmask & data ) );
		m_n_irqdata = ( m_n_irqdata & mem_mask ) | ( m_n_irqdata & m_n_irqmask & data );
		psx_irq_update();
		break;
	case 0x01:
		verboselog( 2, "psx irq mask ( %08x, %08x ) %08x -> %08x\n", data, mem_mask, m_n_irqmask, ( m_n_irqmask & mem_mask ) | data );
		m_n_irqmask = ( m_n_irqmask & mem_mask ) | data;
		if( ( m_n_irqmask & ~( 0x1 | 0x08 | 0x10 | 0x20 | 0x40 | 0x100 | 0x200 | 0x400 ) ) != 0 )
		{
			verboselog( 0, "psx_irq_w( %08x, %08x, %08x ) unknown irq\n", offset, data, mem_mask );
		}
		psx_irq_update();
		break;
	default:
		verboselog( 0, "psx_irq_w( %08x, %08x, %08x ) unknown register\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psx_irq_r )
{
	switch( offset )
	{
	case 0x00:
		verboselog( 1, "psx_irq_r irq data %08x\n", m_n_irqdata );
		return m_n_irqdata;
	case 0x01:
		verboselog( 1, "psx_irq_r irq mask %08x\n", m_n_irqmask );
		return m_n_irqmask;
	default:
		verboselog( 0, "psx_irq_r unknown register %d\n", offset );
		break;
	}
	return 0;
}

void psx_irq_set( UINT32 data )
{
	m_n_irqdata |= data;
	psx_irq_update();
}

/* DMA */

static UINT32 m_p_n_dmabase[ 7 ];
static UINT32 m_p_n_dmablockcontrol[ 7 ];
static UINT32 m_p_n_dmachannelcontrol[ 7 ];
static void *m_p_timer_dma[ 7 ];
static psx_dma_read_handler m_p_fn_dma_read[ 7 ];
static psx_dma_write_handler m_p_fn_dma_write[ 7 ];
static UINT32 m_p_n_dma_lastscanline[ 7 ];
static UINT32 m_n_dpcp;
static UINT32 m_n_dicr;

static void dma_timer( int n_channel, UINT32 n_scanline )
{
	if( n_scanline != 0xffffffff )
	{
		timer_adjust( m_p_timer_dma[ n_channel ], cpu_getscanlinetime( n_scanline ), n_channel, 0 );
	}
	else
	{
		timer_adjust( m_p_timer_dma[ n_channel ], TIME_NEVER, 0, 0 );
	}
	m_p_n_dma_lastscanline[ n_channel ] = n_scanline;
}

void dma_finished( int n_channel )
{
	m_p_n_dmachannelcontrol[ n_channel ] &= ~( ( 1L << 0x18 ) | ( 1L << 0x1c ) );

	if( ( m_n_dicr & ( 1 << ( 16 + n_channel ) ) ) != 0 )
	{
		m_n_dicr |= 0x80000000 | ( 1 << ( 24 + n_channel ) );
		psx_irq_set( 0x0008 );
		verboselog( 2, "dma_finished( %d ) interrupt triggered\n", n_channel );
	}
	else
	{
		verboselog( 2, "dma_finished( %d ) interrupt not enabled\n", n_channel );
	}
	dma_timer( n_channel, 0xffffffff );
}

void psx_dma_install_read_handler( int n_channel, psx_dma_read_handler p_fn_dma_read )
{
	m_p_fn_dma_read[ n_channel ] = p_fn_dma_read;
}

void psx_dma_install_write_handler( int n_channel, psx_dma_read_handler p_fn_dma_write )
{
	m_p_fn_dma_write[ n_channel ] = p_fn_dma_write;
}

WRITE32_HANDLER( psx_dma_w )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			verboselog( 2, "dmabase( %d ) = %08x\n", n_channel, data );
			m_p_n_dmabase[ n_channel ] = data;
			break;
		case 1:
			verboselog( 2, "dmablockcontrol( %d ) = %08x\n", n_channel, data );
			m_p_n_dmablockcontrol[ n_channel ] = data;
			break;
		case 2:
			verboselog( 2, "dmachannelcontrol( %d ) = %08x\n", n_channel, data );
			m_p_n_dmachannelcontrol[ n_channel ] = data;
			if( ( m_p_n_dmachannelcontrol[ n_channel ] & ( 1L << 0x18 ) ) != 0 && ( m_n_dpcp & ( 1 << ( 3 + ( n_channel * 4 ) ) ) ) != 0 )
			{
				INT32 n_size;
				UINT32 n_address;
				UINT32 n_nextaddress;
				UINT32 n_adrmask;

				n_adrmask = g_n_psxramsize - 1;

				n_address = ( m_p_n_dmabase[ n_channel ] & n_adrmask );
				n_size = m_p_n_dmablockcontrol[ n_channel ];
				if( ( m_p_n_dmachannelcontrol[ n_channel ] & 0x200 ) != 0 )
				{
					UINT32 n_ba;
					n_ba = m_p_n_dmablockcontrol[ n_channel ] >> 16;
					if( n_ba == 0 )
					{
						n_ba = 0x10000;
					}
					n_size = ( n_size & 0xffff ) * n_ba;
				}

				if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000000 && 
					m_p_fn_dma_read[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d read block %08x %08x\n", n_channel, n_address, n_size );
					m_p_fn_dma_read[ n_channel ]( n_address, n_size );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000200 &&
					m_p_fn_dma_read[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d read block %08x %08x\n", n_channel, n_address, n_size );
					m_p_fn_dma_read[ n_channel ]( n_address, n_size );
					if( n_channel == 1 )
					{
						dma_timer( n_channel, cpu_getscanline() + 16 );
					}
					else
					{
						dma_finished( n_channel );
					}
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000201 &&
					m_p_fn_dma_write[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d write block %08x %08x\n", n_channel, n_address, n_size );
					m_p_fn_dma_write[ n_channel ]( n_address, n_size );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000401 &&
					n_channel == 2 &&
					m_p_fn_dma_write[ n_channel ] != NULL )
				{
					UINT32 n_total = 0;

					verboselog( 1, "dma %d write linked list %08x\n",
						n_channel, m_p_n_dmabase[ n_channel ] );
					do
					{
						n_address &= n_adrmask;
						n_nextaddress = g_p_n_psxram[ n_address / 4 ];
						n_size = n_nextaddress >> 24;
						m_p_fn_dma_write[ n_channel ]( n_address + 4, n_size );
						n_address = ( n_nextaddress & 0xffffff );

						n_total += n_size;
						n_total++;
						if( n_total > ( 8 * 1024 * 1024 ) / 4 )
						{
							/* todo: find a better way of detecting this */
							verboselog( 1, "dma looped\n" );
							break;
						}
					} while( n_address != 0xffffff );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x11000002 &&
					n_channel == 6 )
				{
					verboselog( 1, "dma 6 reverse clear %08x %08x\n",
						m_p_n_dmabase[ n_channel ], m_p_n_dmablockcontrol[ n_channel ] );
					if( n_size > 0 )
					{
						n_size--;
						while( n_size > 0 )
						{
							n_nextaddress = ( n_address - 4 ) & 0xffffff;
							g_p_n_psxram[ n_address / 4 ] = n_nextaddress;
							n_address = n_nextaddress;
							n_size--;
						}
						g_p_n_psxram[ n_address / 4 ] = 0xffffff;
					}
					dma_finished( n_channel );
				}
				else
				{
					verboselog( 0, "dma %d unknown mode %08x\n", n_channel, m_p_n_dmachannelcontrol[ n_channel ] );
				}
			}
			else if( m_p_n_dmachannelcontrol[ n_channel ] != 0 )
			{
				verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) channel not enabled\n", offset, m_p_n_dmachannelcontrol[ n_channel ], mem_mask );
			}
			break;
		default:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) Unknown dma channel register\n", offset, data, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) dpcp\n", offset, data, mem_mask );
			m_n_dpcp = ( m_n_dpcp & mem_mask ) | data;
			break;
		case 0x1:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) dicr\n", offset, data, mem_mask );
			m_n_dicr = ( m_n_dicr & mem_mask ) | ( data & 0xffffff );
			break;
		default:
			verboselog( 0, "psx_dma_w( %04x, %08x, %08x ) Unknown dma control register\n", offset, data, mem_mask );
			break;
		}
	}
}

READ32_HANDLER( psx_dma_r )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			verboselog( 1, "psx_dma_r dmabase[ %d ] ( %08x )\n", n_channel, m_p_n_dmabase[ n_channel ] );
			return m_p_n_dmabase[ n_channel ];
		case 1:
			verboselog( 1, "psx_dma_r dmablockcontrol[ %d ] ( %08x )\n", n_channel, m_p_n_dmablockcontrol[ n_channel ] );
			return m_p_n_dmablockcontrol[ n_channel ];
		case 2:
			verboselog( 1, "psx_dma_r dmachannelcontrol[ %d ] ( %08x )\n", n_channel, m_p_n_dmachannelcontrol[ n_channel ] );
			return m_p_n_dmachannelcontrol[ n_channel ];
		default:
			verboselog( 0, "psx_dma_r( %08x, %08x ) Unknown dma channel register\n", offset, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			verboselog( 1, "psx_dma_r dpcp ( %08x )\n", m_n_dpcp );
			return m_n_dpcp;
		case 0x1:
			verboselog( 1, "psx_dma_r dicr ( %08x )\n", m_n_dicr );
			return m_n_dicr;
		default:
			verboselog( 0, "psx_dma_r( %08x, %08x ) Unknown dma control register\n", offset, mem_mask );
			break;
		}
	}
	return 0;
}

/* Root Counters */

static void *m_p_timer_root[ 7 ];
static data16_t m_p_n_root_count[ 3 ];
static data16_t m_p_n_root_mode[ 3 ];
static data16_t m_p_n_root_target[ 3 ];

static void root_finished( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & 0x50 ) != 0 )
	{
		psx_irq_set( 0x10 << n_counter );
	}
}

static void root_timer( int n_counter )
{
	int n_duration;

	n_duration = m_p_n_root_target[ n_counter ] - m_p_n_root_count[ n_counter ];
	if( n_duration < 1 )
	{
		n_duration += 0x10000;
	}

	if( n_counter == 0 )
	{
		n_duration *= 1200;
	}
	else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x08 ) ) != 0 )
	{
		n_duration *= 4800;
	}
	else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x09 ) ) != 0 )
	{
		n_duration *= 480;
	}

	timer_adjust( m_p_timer_root[ n_counter ], TIME_IN_SEC( (double)n_duration / 33868800 ), n_counter, 0 );
}

WRITE32_HANDLER( psx_counter_w )
{
	int n_counter;

	verboselog( 1, "psx_counter_w ( %08x, %08x, %08x )\n", offset, data, mem_mask );

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		m_p_n_root_count[ n_counter ] = data;
		break;
	case 1:
		m_p_n_root_mode[ n_counter ] = data;
		break;
	case 2:
		m_p_n_root_target[ n_counter ] = data;
		break;
	}

	root_timer( n_counter );
}

READ32_HANDLER( psx_counter_r )
{
	int n_counter;
	data32_t data;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		data = activecpu_gettotalcycles() / 2;
		if( n_counter == 0 )
		{
			data /= 1200;
		}
		else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x08 ) ) != 0 )
		{
			data /= 4800;
		}
		else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x09 ) ) != 0 )
		{
			data /= 480;
		}
		data &= 0xffff;
		m_p_n_root_count[ n_counter ] = data;
		break;
	case 1:
		data = m_p_n_root_mode[ n_counter ];
		break;
	case 2:
		data = m_p_n_root_target[ n_counter ];
		break;
	default:
		data = 0;
		break;
	}
	verboselog( 1, "psx_counter_r ( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

/* SIO */

#define SIO_BUF_SIZE ( 8 )

static data32_t m_p_n_sio_status[ 2 ];
static data32_t m_p_n_sio_mode[ 2 ];
static data32_t m_p_n_sio_control[ 2 ];
static data32_t m_p_n_sio_baud[ 2 ];
static data32_t m_p_n_sio_tx[ 2 ];
static data32_t m_p_n_sio_rx[ 2 ];
static data32_t m_p_n_sio_tx_prev[ 2 ];
static data32_t m_p_n_sio_rx_prev[ 2 ];
static data32_t m_p_n_sio_tx_data[ 2 ];
static data32_t m_p_n_sio_rx_data[ 2 ];
static data32_t m_p_n_sio_tx_shift[ 2 ];
static data32_t m_p_n_sio_rx_shift[ 2 ];
static data32_t m_p_n_sio_tx_bits[ 2 ];
static data32_t m_p_n_sio_rx_bits[ 2 ];

static void *m_p_timer_sio[ 2 ];
static psx_sio_handler m_p_f_sio_handler[ 2 ];

#define SIO_STATUS_TX_RDY ( 1 << 0 )
#define SIO_STATUS_RX_RDY ( 1 << 1 )
#define SIO_STATUS_TX_EMPTY ( 1 << 2 )
#define SIO_STATUS_OVERRUN ( 1 << 4 )
#define SIO_STATUS_DSR ( 1 << 7 )
#define SIO_STATUS_IRQ ( 1 << 9 )

#define SIO_CONTROL_IACK ( 1 << 4 )
#define SIO_CONTROL_RESET ( 1 << 6 )
#define SIO_CONTROL_TX_IENA ( 1 << 10 )
#define SIO_CONTROL_RX_IENA ( 1 << 11 )
#define SIO_CONTROL_DSR_IENA ( 1 << 12 )
#define SIO_CONTROL_DTR ( 1 << 13 )

#define BITS_PER_TICK ( 8 )

static void sio_interrupt( int n_port )
{
	verboselog( 1, "sio_interrupt( %d )\n", n_port );
	m_p_n_sio_status[ n_port ] |= SIO_STATUS_IRQ;
	if( n_port == 0 )
	{
		psx_irq_set( 0x80 );
	}
	else
	{
		psx_irq_set( 0x100 );
	}
}

static void sio_timer( int n_port )
{
	int n_prescaler;
	double n_time;

	switch( m_p_n_sio_mode[ n_port ] & 3 )
	{
	case 1:
		n_prescaler = 1;
		break;
	case 2:
		n_prescaler = 16;
		break;
	case 3:
		n_prescaler = 64;
		break;
	default:
		n_prescaler = 0;
		break;
	}

	n_prescaler *= BITS_PER_TICK;
	if( m_p_n_sio_baud[ n_port ] != 0 && n_prescaler != 0 )
	{
		n_time = TIME_IN_SEC( (double)( n_prescaler * m_p_n_sio_baud[ n_port ] ) / 33868800 );
		verboselog( 2, "sio_timer( %f ) %d %d\n", n_time, n_prescaler, m_p_n_sio_baud[ n_port ] );
	}
	else
	{
		n_time = TIME_NEVER;
		verboselog( 2, "sio_timer( NEVER )\n" );
	}
	timer_adjust( m_p_timer_sio[ n_port ], n_time, n_port, 0 );
}

static void sio_clock( int n_port )
{
	int n_bit;

	verboselog( 2, "sio tick\n" );

	for( n_bit = 0; n_bit < BITS_PER_TICK; n_bit++ )
	{
		if( m_p_n_sio_tx_bits[ n_port ] == 0 &&
			( m_p_n_sio_status[ n_port ] & SIO_STATUS_TX_EMPTY ) == 0 )
		{
			m_p_n_sio_tx_bits[ n_port ] = 8;
			m_p_n_sio_tx_shift[ n_port ] = m_p_n_sio_tx_data[ n_port ];
			if( n_port == 0 )
			{
				m_p_n_sio_rx_bits[ n_port ] = 8;
				m_p_n_sio_rx_shift[ n_port ] = 0;
			}
			m_p_n_sio_status[ n_port ] |= SIO_STATUS_TX_EMPTY;
			m_p_n_sio_status[ n_port ] |= SIO_STATUS_TX_RDY;
		}

		if( n_port == 0 )
		{
			m_p_n_sio_rx[ n_port ] |= PSX_SIO_IN_DATA;
		}

		if( m_p_n_sio_tx_bits[ n_port ] != 0 )
		{
			m_p_n_sio_tx[ n_port ] = ( m_p_n_sio_tx[ n_port ] & ~PSX_SIO_OUT_DATA ) | ( ( m_p_n_sio_tx_shift[ n_port ] & 1 ) * PSX_SIO_OUT_DATA );
			m_p_n_sio_tx_shift[ n_port ] >>= 1;
			m_p_n_sio_tx_bits[ n_port ]--;

			if( m_p_f_sio_handler[ n_port ] != NULL )
			{
				if( n_port == 0 )
				{
					m_p_f_sio_handler[ n_port ]( m_p_n_sio_tx[ n_port ] | PSX_SIO_OUT_CLOCK );
				}
				m_p_f_sio_handler[ n_port ]( m_p_n_sio_tx[ n_port ] );
			}

			if( m_p_n_sio_tx_bits[ n_port ] == 0 &&
				( m_p_n_sio_control[ n_port ] & SIO_CONTROL_TX_IENA ) != 0 )
			{
				sio_interrupt( n_port );
			}
		}

		if( m_p_n_sio_rx_bits[ n_port ] != 0 )
		{
			m_p_n_sio_rx_shift[ n_port ] = ( m_p_n_sio_rx_shift[ n_port ] >> 1 ) | ( ( ( m_p_n_sio_rx[ n_port ] & PSX_SIO_IN_DATA ) / PSX_SIO_IN_DATA ) << 7 );
			m_p_n_sio_rx_bits[ n_port ]--;

			if( m_p_n_sio_rx_bits[ n_port ] == 0 )
			{
				if( ( m_p_n_sio_status[ n_port ] & SIO_STATUS_RX_RDY ) != 0 )
				{
					m_p_n_sio_status[ n_port ] |= SIO_STATUS_OVERRUN;
				}
				else
				{
					m_p_n_sio_rx_data[ n_port ] = m_p_n_sio_rx_shift[ n_port ];
					m_p_n_sio_status[ n_port ] |= SIO_STATUS_RX_RDY;
				}
				if( ( m_p_n_sio_control[ n_port ] & SIO_CONTROL_RX_IENA ) != 0 )
				{
					sio_interrupt( n_port );
				}
			}
		}
	}

	sio_timer( n_port );
}

void psx_sio_input( int n_port, int n_mask, int n_data )
{
	verboselog( 1, "psx_sio_input( %d, %02x, %02x )\n", n_port, n_mask, n_data );
	m_p_n_sio_rx[ n_port ] = ( m_p_n_sio_rx[ n_port ] & ~n_mask ) | ( n_data & n_mask );

	if( ( m_p_n_sio_rx[ n_port ] & PSX_SIO_IN_DSR ) != 0 )
	{
		m_p_n_sio_status[ n_port ] |= SIO_STATUS_DSR;
		if( ( m_p_n_sio_rx_prev[ n_port ] & PSX_SIO_IN_DSR ) == 0 &&
			( m_p_n_sio_control[ n_port ] & SIO_CONTROL_DSR_IENA ) != 0 )
		{
			sio_interrupt( n_port );
		}
	}
	else
	{
		m_p_n_sio_status[ n_port ] &= ~SIO_STATUS_DSR;
	}
	m_p_n_sio_rx_prev[ n_port ] = m_p_n_sio_rx[ n_port ];
}

WRITE32_HANDLER( psx_sio_w )
{
	int n_port;

	n_port = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		verboselog( 1, "psx_sio_w %d data %02x (%08x)\n", n_port, data, mem_mask );
		m_p_n_sio_tx_data[ n_port ] = data;
		m_p_n_sio_status[ n_port ] &= ~( SIO_STATUS_TX_RDY );
		m_p_n_sio_status[ n_port ] &= ~( SIO_STATUS_TX_EMPTY );
		break;
	case 1:
		verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	case 2:
		if( ACCESSING_LSW32 )
		{
			m_p_n_sio_mode[ n_port ] = data & 0xffff;
			verboselog( 1, "psx_sio_w %d mode %04x\n", n_port, data & 0xffff );
			sio_timer( n_port );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_w %d control %04x\n", n_port, data >> 16 );
			m_p_n_sio_control[ n_port ] = data >> 16;

			if( ( m_p_n_sio_control[ n_port ] & SIO_CONTROL_RESET ) != 0 )
			{
				verboselog( 1, "psx_sio_w reset\n" );
				m_p_n_sio_rx_bits[ n_port ] = 0;
				m_p_n_sio_tx_bits[ n_port ] = 0;
				m_p_n_sio_status[ n_port ] = SIO_STATUS_TX_EMPTY | SIO_STATUS_TX_RDY;
			}
			if( ( m_p_n_sio_control[ n_port ] & SIO_CONTROL_IACK ) != 0 )
			{
				verboselog( 1, "psx_sio_w iack\n" );
				m_p_n_sio_status[ n_port ] &= ~( SIO_STATUS_IRQ );
				m_p_n_sio_control[ n_port ] &= ~( SIO_CONTROL_IACK );
			}
			if( ( m_p_n_sio_control[ n_port ] & SIO_CONTROL_DTR ) != 0 )
			{
				m_p_n_sio_tx[ n_port ] |= PSX_SIO_OUT_DTR;
			}
			else
			{
				m_p_n_sio_tx[ n_port ] &= ~PSX_SIO_OUT_DTR;
			}

			if( ( ( m_p_n_sio_tx[ n_port ] ^ m_p_n_sio_tx_prev[ n_port ] ) & PSX_SIO_OUT_DTR ) != 0 )
			{
				if( m_p_f_sio_handler[ n_port ] != NULL )
				{
					m_p_f_sio_handler[ n_port ]( m_p_n_sio_tx[ n_port ] );
				}
			}
			m_p_n_sio_tx_prev[ n_port ] = m_p_n_sio_tx[ n_port ];
	
		}
		break;
	case 3:
		if( ACCESSING_LSW32 )
		{
			verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		}
		if( ACCESSING_MSW32 )
		{
			m_p_n_sio_baud[ n_port ] = data >> 16;
			verboselog( 1, "psx_sio_w %d baud %04x\n", n_port, data >> 16 );
			sio_timer( n_port );
		}
		break;
	default:
		verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psx_sio_r )
{
	data32_t data;
	int n_port;

	n_port = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		data = m_p_n_sio_rx_data[ n_port ];
		m_p_n_sio_status[ n_port ] &= ~( SIO_STATUS_RX_RDY );
		m_p_n_sio_rx_data[ n_port ] = 0xff;
		verboselog( 1, "psx_sio_r %d data %02x (%08x)\n", n_port, data, mem_mask );
		break;
	case 1:
		data = m_p_n_sio_status[ n_port ];
		if( ACCESSING_LSW32 )
		{
			verboselog( 1, "psx_sio_r %d status %04x\n", n_port, data & 0xffff );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		}
		break;
	case 2:
		data = ( m_p_n_sio_control[ n_port ] << 16 ) | m_p_n_sio_mode[ n_port ];
		if( ACCESSING_LSW32 )
		{
			verboselog( 1, "psx_sio_r %d mode %04x\n", n_port, data & 0xffff );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_r %d control %04x\n", n_port, data >> 16 );
		}
		break;
	case 3:
		data = m_p_n_sio_baud[ n_port ] << 16;
		if( ACCESSING_LSW32 )
		{
			verboselog( 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_r %d baud %04x\n", n_port, data >> 16 );
		}
		break;
	default:
		data = 0;
		verboselog( 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		break;
	}
	return data;
}

void psx_sio_install_handler( int n_port, psx_sio_handler p_f_sio_handler )
{
	m_p_f_sio_handler[ n_port ] = p_f_sio_handler;
}

/* MDEC */

#define	DCTSIZE ( 8 )
#define	DCTSIZE2 ( DCTSIZE * DCTSIZE )

static INT32 m_p_n_mdec_quantize_y[ DCTSIZE2 ];
static INT32 m_p_n_mdec_quantize_uv[ DCTSIZE2 ];
static INT32 m_p_n_mdec_cos[ DCTSIZE2 ];
static INT32 m_p_n_mdec_cos_precalc[ DCTSIZE2 * DCTSIZE2 ];

static UINT32 m_n_mdec0_command;
static UINT32 m_n_mdec0_address;
static UINT32 m_n_mdec0_size;
static UINT32 m_n_mdec1_command;
static UINT32 m_n_mdec1_status;

static UINT16 m_p_n_mdec_clamp8[ 256 * 3 ];
static UINT16 m_p_n_mdec_r5[ 256 * 3 ];
static UINT16 m_p_n_mdec_g5[ 256 * 3 ];
static UINT16 m_p_n_mdec_b5[ 256 * 3 ];

static UINT32 m_p_n_mdec_zigzag[ DCTSIZE2 ] =
{
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

static INT32 m_p_n_mdec_unpacked[ DCTSIZE2 * 6 * 2 ];

#define MDEC_COS_PRECALC_BITS ( 21 )

static void mdec_cos_precalc( void )
{
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_u;
	UINT32 n_v;
	INT32 *p_n_precalc;

	p_n_precalc = m_p_n_mdec_cos_precalc;

	for( n_y = 0; n_y < 8; n_y++ )
	{
		for( n_x = 0; n_x < 8; n_x++ )
		{
			for( n_v = 0; n_v < 8; n_v++ )
			{
				for( n_u = 0; n_u < 8; n_u++ )
				{
					*( p_n_precalc++ ) =
						( ( m_p_n_mdec_cos[ ( n_u * 8 ) + n_x ] *
						m_p_n_mdec_cos[ ( n_v * 8 ) + n_y ] ) >> ( 30 - MDEC_COS_PRECALC_BITS ) );
				}
			}
		}
	}
}

static void mdec_idct( INT32 *p_n_src, INT32 *p_n_dst )
{
	UINT32 n_yx;
	UINT32 n_vu;
	INT32 p_n_z[ 8 ];
	INT32 *p_n_data;
	INT32 *p_n_precalc;

	p_n_precalc = m_p_n_mdec_cos_precalc;

	for( n_yx = 0; n_yx < DCTSIZE2; n_yx++ )
	{
		p_n_data = p_n_src;

		memset( p_n_z, 0, sizeof( p_n_z ) );

		for( n_vu = 0; n_vu < DCTSIZE2 / 8; n_vu++ )
		{
			p_n_z[ 0 ] += p_n_data[ 0 ] * p_n_precalc[ 0 ];
			p_n_z[ 1 ] += p_n_data[ 1 ] * p_n_precalc[ 1 ];
			p_n_z[ 2 ] += p_n_data[ 2 ] * p_n_precalc[ 2 ];
			p_n_z[ 3 ] += p_n_data[ 3 ] * p_n_precalc[ 3 ];
			p_n_z[ 4 ] += p_n_data[ 4 ] * p_n_precalc[ 4 ];
			p_n_z[ 5 ] += p_n_data[ 5 ] * p_n_precalc[ 5 ];
			p_n_z[ 6 ] += p_n_data[ 6 ] * p_n_precalc[ 6 ];
			p_n_z[ 7 ] += p_n_data[ 7 ] * p_n_precalc[ 7 ];
			p_n_data += 8;
			p_n_precalc += 8;
		}

		*( p_n_dst++ ) = ( p_n_z[ 0 ] + p_n_z[ 1 ] + p_n_z[ 2 ] + p_n_z[ 3 ] +
			p_n_z[ 4 ] + p_n_z[ 5 ] + p_n_z[ 6 ] + p_n_z[ 7 ] ) >> ( MDEC_COS_PRECALC_BITS + 2 );
	}
}

INLINE UINT16 mdec_unpack_run( UINT16 n_packed )
{
	return n_packed >> 10;
}

INLINE INT32 mdec_unpack_val( UINT16 n_packed )
{
	return ( ( (INT32)n_packed ) << 22 ) >> 22;
}

static UINT32 mdec_unpack( UINT32 n_address )
{
	UINT8 n_z;
	INT32 n_qscale;
	UINT16 n_packed;
	UINT32 n_block;
	INT32 *p_n_block;
	INT32 p_n_unpacked[ 64 ];
	INT32 *p_n_q;

	p_n_q = m_p_n_mdec_quantize_uv;
	p_n_block = m_p_n_mdec_unpacked;

	for( n_block = 0; n_block < 6; n_block++ )
	{
		memset( p_n_unpacked, 0, sizeof( p_n_unpacked ) );

		if( n_block == 2 )
		{
			p_n_q = m_p_n_mdec_quantize_y;
		}
		n_packed = psxreadword( n_address );
		n_address += 2;

		n_qscale = mdec_unpack_run( n_packed );

		p_n_unpacked[ 0 ] = mdec_unpack_val( n_packed ) * p_n_q[ 0 ];

		n_z = 0;
		for( ;; )
		{
			n_packed = psxreadword( n_address );
			n_address += 2;

			if( n_packed == 0xfe00 )
			{
				break;
			}
			n_z += mdec_unpack_run( n_packed ) + 1;
			if( n_z > 63 )
			{
				break;
			}
			p_n_unpacked[ m_p_n_mdec_zigzag[ n_z ] ] = ( mdec_unpack_val( n_packed ) * p_n_q[ n_z ] * n_qscale ) / 8;
		}
		mdec_idct( p_n_unpacked, p_n_block );
		p_n_block += DCTSIZE2;
	}
	return n_address;
}

INLINE INT32 mdec_cr_to_r( INT32 n_cr )
{
	return ( 1435 * n_cr ) >> 10;
}

INLINE INT32 mdec_cr_to_g( INT32 n_cr )
{
	return ( -731 * n_cr ) >> 10;
}

INLINE INT32 mdec_cb_to_g( INT32 n_cb )
{
	return ( -351 * n_cb ) >> 10;
}

INLINE INT32 mdec_cb_to_b( INT32 n_cb )
{
	return ( 1814 * n_cb ) >> 10;
}

INLINE UINT16 mdec_clamp_r5( INT32 n_r )
{
	return m_p_n_mdec_r5[ n_r + 128 + 256 ];
}

INLINE UINT16 mdec_clamp_g5( INT32 n_g )
{
	return m_p_n_mdec_g5[ n_g + 128 + 256 ];
}

INLINE UINT16 mdec_clamp_b5( INT32 n_b )
{
	return m_p_n_mdec_b5[ n_b + 128 + 256 ];
}

INLINE void mdec_makergb15( UINT32 n_address, INT32 n_r, INT32 n_g, INT32 n_b, INT32 *p_n_y, UINT32 n_stp )
{
	g_p_n_psxram[ n_address / 4 ] = n_stp |
		mdec_clamp_r5( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_r ) |
		mdec_clamp_g5( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_g ) |
		mdec_clamp_b5( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_b ) |
		( mdec_clamp_r5( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_r ) |
		mdec_clamp_g5( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_g ) |
		mdec_clamp_b5( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_b ) ) << 16;
}

static void mdec_yuv2_to_rgb15( UINT32 n_address )
{
	INT32 n_r;
	INT32 n_g;
	INT32 n_b;
	INT32 n_cb;
	INT32 n_cr;
	INT32 *p_n_cb;
	INT32 *p_n_cr;
	INT32 *p_n_y;
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_z;
	UINT32 n_stp;

	if( ( m_n_mdec0_command & ( 1L << 25 ) ) != 0 )
	{
		n_stp = 0x80008000;
	}
	else
	{
		n_stp = 0x00000000;
	}

	p_n_cb = &m_p_n_mdec_unpacked[ 0 ];
	p_n_cr = &m_p_n_mdec_unpacked[ DCTSIZE2 ];
	p_n_y = &m_p_n_mdec_unpacked[ DCTSIZE2 * 2 ];

	for( n_z = 0; n_z < 2; n_z++ )
	{
		for( n_y = 0; n_y < 4; n_y++ )
		{
			for( n_x = 0; n_x < 4; n_x++ )
			{
				n_cr = *( p_n_cr );
				n_cb = *( p_n_cb );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				mdec_makergb15( ( n_address +  0 ), n_r, n_g, n_b, p_n_y, n_stp );
				mdec_makergb15( ( n_address + 32 ), n_r, n_g, n_b, p_n_y + 8, n_stp );

				n_cr = *( p_n_cr + 4 );
				n_cb = *( p_n_cb + 4 );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				mdec_makergb15( ( n_address + 16 ), n_r, n_g, n_b, p_n_y + DCTSIZE2, n_stp );
				mdec_makergb15( ( n_address + 48 ), n_r, n_g, n_b, p_n_y + DCTSIZE2 + 8, n_stp );

				p_n_cr++;
				p_n_cb++;
				p_n_y += 2;
				n_address += 4;
			}
			p_n_cr += 4;
			p_n_cb += 4;
			p_n_y += 8;
			n_address += 48;
		}
		p_n_y += DCTSIZE2;
	}
}

INLINE UINT16 mdec_clamp8( INT32 n_r )
{
	return m_p_n_mdec_clamp8[ n_r + 128 + 256 ];
}

INLINE void mdec_makergb24( UINT32 n_address, INT32 n_r, INT32 n_g, INT32 n_b, INT32 *p_n_y, UINT32 n_stp )
{
	psxwriteword( n_address + 0, ( mdec_clamp8( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_g ) << 8 ) | mdec_clamp8( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_r ) );
	psxwriteword( n_address + 2, ( mdec_clamp8( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_r ) << 8 ) | mdec_clamp8( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_b ) );
	psxwriteword( n_address + 4, ( mdec_clamp8( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_b ) << 8 ) | mdec_clamp8( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_g ) );
}

static void mdec_yuv2_to_rgb24( UINT32 n_address )
{
	INT32 n_r;
	INT32 n_g;
	INT32 n_b;
	INT32 n_cb;
	INT32 n_cr;
	INT32 *p_n_cb;
	INT32 *p_n_cr;
	INT32 *p_n_y;
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_z;
	UINT32 n_stp;

	if( ( m_n_mdec0_command & ( 1L << 25 ) ) != 0 )
	{
		n_stp = 0x80008000;
	}
	else
	{
		n_stp = 0x00000000;
	}

	p_n_cb = &m_p_n_mdec_unpacked[ 0 ];
	p_n_cr = &m_p_n_mdec_unpacked[ DCTSIZE2 ];
	p_n_y = &m_p_n_mdec_unpacked[ DCTSIZE2 * 2 ];

	for( n_z = 0; n_z < 2; n_z++ )
	{
		for( n_y = 0; n_y < 4; n_y++ )
		{
			for( n_x = 0; n_x < 4; n_x++ )
			{
				n_cr = *( p_n_cr );
				n_cb = *( p_n_cb );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				mdec_makergb24( ( n_address +  0 ), n_r, n_g, n_b, p_n_y, n_stp );
				mdec_makergb24( ( n_address + 48 ), n_r, n_g, n_b, p_n_y + 8, n_stp );

				n_cr = *( p_n_cr + 4 );
				n_cb = *( p_n_cb + 4 );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				mdec_makergb24( ( n_address + 24 ), n_r, n_g, n_b, p_n_y + DCTSIZE2, n_stp );
				mdec_makergb24( ( n_address + 72 ), n_r, n_g, n_b, p_n_y + DCTSIZE2 + 8, n_stp );

				p_n_cr++;
				p_n_cb++;
				p_n_y += 2;
				n_address += 6;
			}
			p_n_cr += 4;
			p_n_cb += 4;
			p_n_y += 8;
			n_address += 72;
		}
		p_n_y += DCTSIZE2;
	}
}

static void mdec0_write( UINT32 n_address, INT32 n_size )
{
	int n_index;

	switch( m_n_mdec0_command >> 28 )
	{
	case 0x3:
		verboselog( 1, "mdec decode %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		m_n_mdec0_address = n_address;
		m_n_mdec0_size = n_size;
		m_n_mdec1_status |= ( 1L << 29 );
		break;
	case 0x4:
		verboselog( 1, "mdec quantize table %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		for( n_index = 0; n_index < DCTSIZE2; n_index++ )
		{
			m_p_n_mdec_quantize_y[ n_index ] = psxreadbyte( n_address + n_index );
			m_p_n_mdec_quantize_uv[ n_index ] = psxreadbyte( n_address + n_index + DCTSIZE2 );
		}
		break;
	case 0x6:
		verboselog( 1, "mdec cosine table %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		for( n_index = 0; n_index < DCTSIZE2; n_index++ )
		{
			m_p_n_mdec_cos[ n_index ] = (INT16)psxreadword( n_address + ( n_index * 2 ) );
		}
		mdec_cos_precalc();
		break;
	default:
		verboselog( 0, "mdec unknown command %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		break;
	}
}

static void mdec1_read( UINT32 n_address, INT32 n_size )
{
	if( ( m_n_mdec0_command & ( 1L << 29 ) ) != 0 )
	{
		if( ( m_n_mdec0_command & ( 1L << 27 ) ) != 0 )
		{
			while( n_size > 0 )
			{
				m_n_mdec0_address = mdec_unpack( m_n_mdec0_address );
				mdec_yuv2_to_rgb15( n_address );
				n_address += ( 16 * 16 ) * 2;
				n_size -= ( 16 * 16 ) / 2;
			}
		}
		else
		{
			verboselog( 0, "mdec 24bit not supported\n" );

			while( n_size > 0 )
			{
				m_n_mdec0_address = mdec_unpack( m_n_mdec0_address );
				mdec_yuv2_to_rgb24( n_address );
				n_address += ( 24 * 16 ) * 2;
				n_size -= ( 24 * 16 ) / 2;
			}
		}
	}
	m_n_mdec1_status &= ~( 1L << 29 );
}

WRITE32_HANDLER( psx_mdec_w )
{
	switch( offset )
	{
	case 0:
		verboselog( 2, "mdec 0 command %08x\n", data );
		m_n_mdec0_command = data;
		break;
	case 1:
		verboselog( 2, "mdec 1 command %08x\n", data );
		m_n_mdec1_command = data;
		break;
	}
}

READ32_HANDLER( psx_mdec_r )
{
	switch( offset )
	{
	case 0:
		return 0;
	case 1:
		return m_n_mdec1_status;
	}
	return 0;
}

static void gpu_read( UINT32 n_address, INT32 n_size )
{
	psx_gpu_read( &g_p_n_psxram[ n_address / 4 ], n_size );
}

static void gpu_write( UINT32 n_address, INT32 n_size )
{
	psx_gpu_write( &g_p_n_psxram[ n_address / 4 ], n_size );
}

void psx_machine_init( void )
{
	int n_channel;

	/* irq */
	m_n_irqdata = 0;
	m_n_irqmask = 0;

	/* dma */
	m_n_dpcp = 0;
	m_n_dicr = 0;

	m_n_mdec0_command = 0;
	m_n_mdec0_address = 0;
	m_n_mdec0_size = 0;
	m_n_mdec1_command = 0;
	m_n_mdec1_status = 0;

	for( n_channel = 0; n_channel < 7; n_channel++ )
	{
		m_p_n_dma_lastscanline[ n_channel ] = 0xffffffff;
	}

	psx_gpu_reset();
}

static void psx_postload( void )
{
	int n;

	psx_irq_update();

	for( n = 0; n < 7; n++ )
	{
		dma_timer( n, m_p_n_dma_lastscanline[ n ] );
	}

	for( n = 0; n < 3; n++ )
	{
		root_timer( n );
	}

	for( n = 0; n < 2; n++ )
	{
		sio_timer( n );
	}

	mdec_cos_precalc();
}

void psx_driver_init( void )
{
	int n;

	for( n = 0; n < 7; n++ )
	{
		m_p_timer_dma[ n ] = timer_alloc( dma_finished );
		m_p_fn_dma_read[ n ] = NULL;
		m_p_fn_dma_write[ n ] = NULL;
	}

	for( n = 0; n < 3; n++ )
	{
		m_p_timer_root[ n ] = timer_alloc( root_finished );
	}

	for( n = 0; n < 2; n++ )
	{
		m_p_timer_sio[ n ] = timer_alloc( sio_clock );
	}

	for( n = 0; n < 256; n++ )
	{
		m_p_n_mdec_clamp8[ n ] = 0;
		m_p_n_mdec_clamp8[ n + 256 ] = n;
		m_p_n_mdec_clamp8[ n + 512 ] = 255;

		m_p_n_mdec_r5[ n ] = 0;
		m_p_n_mdec_r5[ n + 256 ] = ( n >> 3 ) << 10;
		m_p_n_mdec_r5[ n + 512 ] = ( 255 >> 3 ) << 10;

		m_p_n_mdec_g5[ n ] = 0;
		m_p_n_mdec_g5[ n + 256 ] = ( n >> 3 ) << 5;
		m_p_n_mdec_g5[ n + 512 ] = ( 255 >> 3 ) << 5;

		m_p_n_mdec_b5[ n ] = 0;
		m_p_n_mdec_b5[ n + 256 ] = ( n >> 3 );
		m_p_n_mdec_b5[ n + 512 ] = ( 255 >> 3 );
	}

	for( n = 0; n < 2; n++ )
	{
		m_p_n_sio_status[ n ] = SIO_STATUS_TX_EMPTY | SIO_STATUS_TX_RDY;
		m_p_n_sio_mode[ n ] = 0;
		m_p_n_sio_control[ n ] = 0;
		m_p_n_sio_baud[ n ] = 0;
		m_p_n_sio_tx[ n ] = 0;
		m_p_n_sio_rx[ n ] = 0;
		m_p_n_sio_tx_prev[ n ] = 0;
		m_p_n_sio_rx_prev[ n ] = 0;
		m_p_n_sio_rx_data[ n ] = 0;
		m_p_n_sio_tx_data[ n ] = 0;
		m_p_n_sio_rx_shift[ n ] = 0;
		m_p_n_sio_tx_shift[ n ] = 0;
		m_p_n_sio_rx_bits[ n ] = 0;
		m_p_n_sio_tx_bits[ n ] = 0;
		m_p_f_sio_handler[ n ] = NULL;
	}

	psx_dma_install_read_handler( 1, mdec1_read );
	psx_dma_install_read_handler( 2, gpu_read );

	psx_dma_install_write_handler( 0, mdec0_write );
	psx_dma_install_write_handler( 2, gpu_write );

	state_save_register_UINT32( "psx", 0, "m_n_irqdata", &m_n_irqdata, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_irqmask", &m_n_irqmask, 1 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmabase", m_p_n_dmabase, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmablockcontrol", m_p_n_dmablockcontrol, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmachannelcontrol", m_p_n_dmachannelcontrol, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dma_lastscanline", m_p_n_dma_lastscanline, 7 );
	state_save_register_UINT32( "psx", 0, "m_n_dpcp", &m_n_dpcp, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_dicr", &m_n_dicr, 1 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_count", m_p_n_root_count, 3 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_mode", m_p_n_root_mode, 3 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_target", m_p_n_root_target, 3 );

	state_save_register_UINT32( "psx", 0, "m_p_n_sio_status", m_p_n_sio_status, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_mode", m_p_n_sio_mode, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_control", m_p_n_sio_control, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_baud", m_p_n_sio_baud, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_tx", m_p_n_sio_tx, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_rx", m_p_n_sio_rx, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_tx_prev", m_p_n_sio_tx_prev, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_rx_prev", m_p_n_sio_rx_prev, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_rx_data", m_p_n_sio_rx_data, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_tx_data", m_p_n_sio_tx_data, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_rx_shift", m_p_n_sio_rx_shift, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_tx_shift", m_p_n_sio_tx_shift, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_rx_bits", m_p_n_sio_rx_bits, 2 );
	state_save_register_UINT32( "psx", 0, "m_p_n_sio_tx_bits", m_p_n_sio_tx_bits, 2 );

	state_save_register_UINT32( "psx", 0, "m_n_mdec0_command", &m_n_mdec0_command, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec0_address", &m_n_mdec0_address, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec0_size", &m_n_mdec0_size, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec1_command", &m_n_mdec1_command, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec1_status", &m_n_mdec1_status, 1 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_quantize_y", m_p_n_mdec_quantize_y, DCTSIZE2 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_quantize_uv", m_p_n_mdec_quantize_uv, DCTSIZE2 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_cos", m_p_n_mdec_cos, DCTSIZE2 );

	state_save_register_func_postload( psx_postload );
}
