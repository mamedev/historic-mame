/*
 * ATMEL AT28C16
 *
 * 16K ( 2K x 8 ) Parallel EEPROM
 *
 * Todo:
 *  Emulate write timing.
 *  
 */

#include "state.h"
#include "machine/at28c16.h"

#define MAX_AT28C16_CHIPS ( 4 )

#define DATA_SIZE ( 0x800 )
#define ID_SIZE ( 0x020 )
#define ID_OFFSET ( DATA_SIZE - ID_SIZE )

struct at28c16_chip
{
	data8_t p_n_data[ DATA_SIZE ];
	data8_t p_n_id[ ID_SIZE ];
	data8_t b_a9_12v;
};

static struct at28c16_chip at28c16[ MAX_AT28C16_CHIPS ];

void at28c16_a9_12v( int chip, int a9_12v )
{
	at28c16[ chip ].b_a9_12v = a9_12v;
}

/* nvram handlers */

static void at28c16_init( int chip )
{
	at28c16[ chip ].b_a9_12v = 0;

	state_save_register_UINT8( "at28c16", chip, "p_n_data", at28c16[ chip ].p_n_data, DATA_SIZE );
	state_save_register_UINT8( "at28c16", chip, "p_n_id", at28c16[ chip ].p_n_id, ID_SIZE );
	state_save_register_UINT8( "at28c16", chip, "b_a9_12v", &at28c16[ chip ].b_a9_12v, 1 );
}

static void nvram_handler_at28c16( int chip, mame_file *file, int read_or_write )
{
	if( chip >= MAX_AT28C16_CHIPS )
	{
		logerror( "at28c16_nvram_handler: invalid chip %d\n", chip );
		return;
	}
	if( read_or_write )
	{
		mame_fwrite( file, at28c16[ chip ].p_n_data, DATA_SIZE );
		mame_fwrite( file, at28c16[ chip ].p_n_id, ID_SIZE );
	}
	else
	{
		at28c16_init( chip );
		if( file )
		{
			mame_fread( file, at28c16[ chip ].p_n_data, DATA_SIZE );
			mame_fread( file, at28c16[ chip ].p_n_id, ID_SIZE );
		}
		else
		{
			memset( at28c16[ chip ].p_n_data, 0, DATA_SIZE );
			memset( at28c16[ chip ].p_n_id, 0, ID_SIZE );
		}
	}
}

void nvram_handler_at28c16_0( mame_file *file, int read_or_write ) { nvram_handler_at28c16( 0, file, read_or_write ); }
void nvram_handler_at28c16_1( mame_file *file, int read_or_write ) { nvram_handler_at28c16( 1, file, read_or_write ); }
void nvram_handler_at28c16_2( mame_file *file, int read_or_write ) { nvram_handler_at28c16( 2, file, read_or_write ); }
void nvram_handler_at28c16_3( mame_file *file, int read_or_write ) { nvram_handler_at28c16( 3, file, read_or_write ); }

/* read / write */

static data8_t at28c16_8_r( data32_t chip, offs_t offset )
{
	if( offset >= ID_OFFSET && at28c16[ chip ].b_a9_12v )
	{
		logerror( "at28c16_8_r( %04x ) id\n", offset );
		return at28c16[ chip ].p_n_id[ offset - ID_OFFSET ];
	}
	else
	{
//		logerror( "at28c16_8_r( %04x ) data\n", offset );
		return at28c16[ chip ].p_n_data[ offset ];
	}
}

static void at28c16_8_w( data32_t chip, offs_t offset, data8_t data )
{
	if( offset >= ID_OFFSET && at28c16[ chip ].b_a9_12v )
	{
		logerror( "at28c16_8_w( %04x, %02x ) id\n", offset, data );
		at28c16[ chip ].p_n_id[ offset - ID_OFFSET ] = data;
	}
	else
	{
//		logerror( "at28c16_8_w( %04x, %02x ) data\n", offset, data );
		at28c16[ chip ].p_n_data[ offset ] = data;
	}
}

/* 32bit memory handlers */

static data32_t at28c16_32_r( data32_t chip, offs_t offset, data32_t mem_mask )
{
	data32_t data = 0;
	if( ACCESSING_LSB32 )
	{
		data |= at28c16_8_r( chip, ( offset * 4 ) + 0 ) << 0;
	}
	if( ( mem_mask & 0x0000ff00 ) == 0 )
	{
		data |= at28c16_8_r( chip, ( offset * 4 ) + 1 ) << 8;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		data |= at28c16_8_r( chip, ( offset * 4 ) + 2 ) << 16;
	}
	if( ACCESSING_MSB32 )
	{
		data |= at28c16_8_r( chip, ( offset * 4 ) + 3 ) << 24;
	}
	return data;
}

static void at28c16_32_w( data32_t chip, offs_t offset, data32_t data, data32_t mem_mask )
{
	if( ACCESSING_LSB32 )
	{
		at28c16_8_w( chip, ( offset * 4 ) + 0, data >> 0 );
	}
	if( ( mem_mask & 0x0000ff00 ) == 0 )
	{
		at28c16_8_w( chip, ( offset * 4 ) + 1, data >> 8 );
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		at28c16_8_w( chip, ( offset * 4 ) + 2, data >> 16 );
	}
	if( ACCESSING_MSB32 )
	{
		at28c16_8_w( chip, ( offset * 4 ) + 3, data >> 24 );
	}
}

READ32_HANDLER( at28c16_0_32_r ) { return at28c16_32_r( 0, offset, mem_mask ); }
READ32_HANDLER( at28c16_1_32_r ) { return at28c16_32_r( 1, offset, mem_mask ); }
READ32_HANDLER( at28c16_2_32_r ) { return at28c16_32_r( 2, offset, mem_mask ); }
READ32_HANDLER( at28c16_3_32_r ) { return at28c16_32_r( 3, offset, mem_mask ); }
WRITE32_HANDLER( at28c16_0_32_w ) { at28c16_32_w( 0, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_1_32_w ) { at28c16_32_w( 1, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_2_32_w ) { at28c16_32_w( 2, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_3_32_w ) { at28c16_32_w( 3, offset, data, mem_mask ); }

static data32_t at28c16_32_lsb_r( data32_t chip, offs_t offset, data32_t mem_mask )
{
	data32_t data = 0;
	if( ACCESSING_LSB32 )
	{
		data |= at28c16_8_r( chip, ( offset * 2 ) + 0 ) << 0;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		data |= at28c16_8_r( chip, ( offset * 2 ) + 1 ) << 16;
	}
	return data;
}

static void at28c16_32_lsb_w( data32_t chip, offs_t offset, data32_t data, data32_t mem_mask )
{
	if( ACCESSING_LSB32 )
	{
		at28c16_8_w( chip, ( offset * 2 ) + 0, data >> 0 );
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		at28c16_8_w( chip, ( offset * 2 ) + 1, data >> 16 );
	}
}

READ32_HANDLER( at28c16_0_32_lsb_r ) { return at28c16_32_lsb_r( 0, offset, mem_mask ); }
READ32_HANDLER( at28c16_1_32_lsb_r ) { return at28c16_32_lsb_r( 1, offset, mem_mask ); }
READ32_HANDLER( at28c16_2_32_lsb_r ) { return at28c16_32_lsb_r( 2, offset, mem_mask ); }
READ32_HANDLER( at28c16_3_32_lsb_r ) { return at28c16_32_lsb_r( 3, offset, mem_mask ); }
WRITE32_HANDLER( at28c16_0_32_lsb_w ) { at28c16_32_lsb_w( 0, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_1_32_lsb_w ) { at28c16_32_lsb_w( 1, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_2_32_lsb_w ) { at28c16_32_lsb_w( 2, offset, data, mem_mask ); }
WRITE32_HANDLER( at28c16_3_32_lsb_w ) { at28c16_32_lsb_w( 3, offset, data, mem_mask ); }
