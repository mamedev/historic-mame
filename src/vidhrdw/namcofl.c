/* vidhrdw/namcofl.c */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namconb1.h"
#include "namcoic.h"
#include "namcos2.h"

data32_t *namcofl_spritebank32;
//data32_t *namcofl_tilebank32;
data32_t *namcofl_mcuram;

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE data16_t
nth_word32( const data32_t *source, int which )
{
	source += which/2;
	which ^= 1;	/* i960 is little-endian */
	if( which&1 )
	{
		return (*source)&0xffff;
	}
	else
	{
		return (*source)>>16;
	}
}

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
INLINE data8_t
nth_byte32( const data32_t *pSource, int which )
{
		data32_t data = pSource[which/4];

		which ^= 3;	/* i960 is little-endian */
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

static void
namcofl_install_palette( void )
{
	int pen, page, dword_offset, byte_offset;
	data32_t r,g,b;
	data32_t *pSource;

	/* this is unnecessarily expensive.  Better would be to mark palette entries dirty as
	 * they are modified, and only process those that have changed.
	 */
	pen = 0;
	for( page=0; page<4; page++ )
	{
		pSource = &paletteram32[page*0x2000/4];
		for( dword_offset=0; dword_offset<0x800/4; dword_offset++ )
		{
			r = pSource[dword_offset+0x0000/4];
			g = pSource[dword_offset+0x0800/4];
			b = pSource[dword_offset+0x1000/4];

			for( byte_offset=0; byte_offset<4; byte_offset++ )
			{
				palette_set_color( pen++, r&0xff, g&0xff, b&0xff);
				r>>=8; g>>=8; b>>=8;
			}
		}
	}
}

/**
 * MCU simulation.  It manages coinage and input ports.
 */
static void
handle_mcu( void )
{
#if 0	// phil's mcu simulation from NB-1/2
	static int toggle;
	static data16_t credits;
	static int old_coin_state;
	static int old_p1;
	static int old_p2;
	static int old_p3;
	static int old_p4;
	int new_coin_state = readinputport(0)&0x3; /* coin1,2 */
	unsigned dsw = readinputport(1)<<16;
	unsigned p1 = readinputport(2);
	unsigned p2 = readinputport(3);
	unsigned p3;
	unsigned p4;
	toggle = !toggle;
	if( toggle ) dsw &= ~(0x80<<16);
	p3 = 0;
	p4 = 0;

	p1 = (p1&(~old_p1))|(p1<<8);
	p2 = (p2&(~old_p2))|(p2<<8);
	p3 = (p3&(~old_p3))|(p3<<8);
	p4 = (p4&(~old_p4))|(p4<<8);

	old_p1 = p1;
	old_p2 = p2;
	old_p3 = p3;
	old_p4 = p4;

	namcofl_mcuram[0xa000/4] = dsw|p1;
	namcofl_mcuram[0xa004/4] = (p2<<16)|p3;
	namcofl_mcuram[0xa008/4] = p4<<16;

	if( new_coin_state && !old_coin_state )
	{
		credits++;
	}
	old_coin_state = new_coin_state;
	namcofl_mcuram[0xa01e/4] &= 0xffff0000;
	namcofl_mcuram[0xa01e/4] |= credits;
#else	// ElSemi's MCU simulation
	data8_t *IORAM = (data8_t *)namcofl_mcuram;
	static int toggle;
	static int old_p1;
	static int old_p2;
	static int old_p3;
	static int old_p4;
	unsigned p1 = readinputport(0);
	unsigned p2 = readinputport(2);
	unsigned p3 = readinputport(1);
	unsigned p4 = readinputport(3);

	// debounce the inputs for the test menu
	p1 = (p1&(~old_p1))|(p1<<8);
	p2 = (p2&(~old_p2))|(p2<<8);
	p3 = (p3&(~old_p3))|(p3<<8);
	p4 = (p4&(~old_p4))|(p4<<8);

	old_p1 = p1;
	old_p2 = p2;
	old_p3 = p3;
	old_p4 = p4;

	IORAM[0xA000] = p1;
	IORAM[0xA003] = p2;
	IORAM[0xA005] = p3;
	IORAM[0xA0b8] = p4;

	IORAM[0xA014] = 0;	// analog
	IORAM[0xA016] = 0;	// analog
	IORAM[0xA018] = 0;	// analog

	IORAM[0xA009] = 0;

	toggle ^= 1;
	if (toggle)
	{	// final lap
		IORAM[0xA000]|=0x80;
	}
	else
	{	// speed racer
		IORAM[0xA000]&=0x7f;
	}
#endif
} /* handle_mcu */

INLINE void
TilemapCB(data16_t code, int *tile, int *mask )
{
	*tile = code;
	*mask = code;
}


VIDEO_UPDATE( namcofl )
{
	int pri;

	handle_mcu();
	namcofl_install_palette();

	fillbitmap( bitmap, get_black_pen(), cliprect );

	for( pri=0; pri<16; pri++ )
	{
		namco_roz_draw( bitmap,cliprect,pri );
		namco_tilemap_draw( bitmap, cliprect, pri );
		namco_obj_draw( bitmap, cliprect, pri );
	}

} /* namcofl_vh_screenrefresh */

static int FLobjcode2tile( int code )
{
	return code;
}

VIDEO_START( namcofl )
{
	if( namco_tilemap_init( NAMCONB1_TILEGFX, memory_region(NAMCONB1_TILEMASKREGION), TilemapCB ) == 0 )
	{
		namco_obj_init(NAMCONB1_SPRITEGFX,0x0,FLobjcode2tile);
		if( namco_roz_init(NAMCONB1_ROTGFX,NAMCONB1_ROTMASKREGION)==0 )
		{
			return 0;
		}
	}
	return -1;
} /* namcofl_vh_start */
