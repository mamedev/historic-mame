/* vidhrdw/namcofl.c */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namconb1.h"
#include "namcoic.h"
#include "namcos2.h"

data32_t *namcofl_spritebank32;
data32_t *namcofl_tilebank32;
data32_t *namcofl_scrollram32;
data32_t *namcofl_mcuram;

/* tilemap_palette_bank is used to cache tilemap color, so that we can
 * mark whole tilemaps dirty only when necessary.
 */
static int tilemap_palette_bank[6];
static UINT8 *mpMaskData;
static struct tilemap *background[6];
//static data32_t tilemap_tile_bank[4];

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

WRITE32_HANDLER( namcofl_videoram_w )
{
	int layer;
	data32_t old_data;

	old_data = videoram32[offset];
	COMBINE_DATA( &videoram32[offset] );
	if( videoram32[offset]!=old_data )
	{
		offset*=2; /* convert dword offset to word offset */
		layer = offset/(64*64);
		if( layer<4 )
		{
			offset &= 0xfff;
			tilemap_mark_tile_dirty( background[layer], offset );
			tilemap_mark_tile_dirty( background[layer], offset+1 );
		}
		else
		{
			if( offset >= NAMCONB1_FG1BASE &&
				offset<NAMCONB1_FG1BASE+NAMCONB1_COLS*NAMCONB1_ROWS )
			{
				offset -= NAMCONB1_FG1BASE;
				tilemap_mark_tile_dirty( background[4], offset );
				tilemap_mark_tile_dirty( background[4], offset+1 );
			}
			else if( offset >= NAMCONB1_FG2BASE &&
				offset<NAMCONB1_FG2BASE+NAMCONB1_COLS*NAMCONB1_ROWS )
			{
				offset -= NAMCONB1_FG2BASE;
				tilemap_mark_tile_dirty( background[5], offset );
				tilemap_mark_tile_dirty( background[5], offset+1 );
			}
		}
	}
}

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
				palette_set_color( pen++, r>>24, g>>24, b>>24 );
				r<<=8; g<<=8; b<<=8;
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
tilemapFL_get_info(int tile_index,int which,const data32_t *tilemap_videoram)
{
	int code = nth_word32( tilemap_videoram, tile_index );
	int mangle;

	code &= 0x1fff;
	mangle = code;
	SET_TILE_INFO( NAMCONB1_TILEGFX,mangle,tilemap_palette_bank[which],0)
	tile_info.mask_data = 8*code + mpMaskData;
} /* tilemapFL_get_info */

static void tilemapFL_get_info0(int tile_index) { tilemapFL_get_info(tile_index,0,&videoram32[0x0000/4]); }
static void tilemapFL_get_info1(int tile_index) { tilemapFL_get_info(tile_index,1,&videoram32[0x2000/4]); }
static void tilemapFL_get_info2(int tile_index) { tilemapFL_get_info(tile_index,2,&videoram32[0x4000/4]); }
static void tilemapFL_get_info3(int tile_index) { tilemapFL_get_info(tile_index,3,&videoram32[0x6000/4]); }
static void tilemapFL_get_info4(int tile_index) { tilemapFL_get_info(tile_index,4,&videoram32[NAMCONB1_FG1BASE/2]); }
static void tilemapFL_get_info5(int tile_index) { tilemapFL_get_info(tile_index,5,&videoram32[NAMCONB1_FG2BASE/2]); }

VIDEO_UPDATE( namcofl )
{
	const int xadjust[4] = { 0,2,3,4 };
	int i,pri;

	handle_mcu();
	namcofl_install_palette();

	fillbitmap( bitmap, get_black_pen(), cliprect );

	for( i=0; i<6; i++ )
	{
		int tilemap_color = nth_word32( &namcofl_scrollram32[0x30/4], i )&7;
		if( tilemap_palette_bank[i]!= tilemap_color )
		{
			tilemap_palette_bank[i] = tilemap_color;
			tilemap_mark_all_tiles_dirty( background[i] );
		}
		if( i<4 )
		{
			tilemap_set_scrollx( background[i],0,namcofl_scrollram32[i*2]+48-xadjust[i] );
			tilemap_set_scrolly( background[i],0,namcofl_scrollram32[i*2+1]+24 );
		}
	}

	for( pri=0; pri<8; pri++ )
	{
		namco_roz_draw( bitmap,cliprect,pri );
		for( i=0; i<6; i++ )
		{
			if( nth_word32( &namcofl_scrollram32[0x20/4],i ) == pri )
			{
				if( namcos2_gametype == NAMCONB1_NEBULRAY && i==3 )
				{
					/* HACK; don't draw this tilemap - it contains garbage */
				}
				else
				{
					tilemap_draw( bitmap,cliprect,background[i],0,0/*1<<pri*/ );
				}
			}
		}
		namco_obj_draw( bitmap, cliprect, pri );
	}
} /* namcofl_vh_screenrefresh */

static int
FLobjcode2tile( int code )
{
	int bank = 0; //nth_byte32( namcofl_spritebank32, (code>>11)&0xf );
	code &= 0x7ff;
	if( bank&0x01 ) code |= 0x01*0x800;
	if( bank&0x02 ) code |= 0x02*0x800;
	if( bank&0x04 ) code |= 0x04*0x800;
	if( bank&0x08 ) code |= 0x08*0x800;
	if( bank&0x10 ) code |= 0x10*0x800;
	if( bank&0x40 ) code |= 0x20*0x800;
	return code;
}

VIDEO_START( namcofl )
{
	int i;
	static void (*get_info[6])(int tile_index) =
		{ tilemapFL_get_info0, tilemapFL_get_info1, tilemapFL_get_info2,
		  tilemapFL_get_info3, tilemapFL_get_info4, tilemapFL_get_info5 };

	namco_obj_init(NAMCONB1_SPRITEGFX,0x0,FLobjcode2tile);

	if( namco_roz_init(NAMCONB1_ROTGFX,NAMCONB1_ROTMASKREGION)!=0 ) return 1;

	mpMaskData = (UINT8 *)memory_region( NAMCONB1_TILEMASKREGION );
	for( i=0; i<6; i++ )
	{
		if( i<4 )
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,64,64 );
		}
		else
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,NAMCONB1_COLS,NAMCONB1_ROWS );
		}

		if( background[i]==NULL ) return 1; /* error */
	}

	return 0;
} /* namcofl_vh_start */
