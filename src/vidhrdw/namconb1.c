/* vidhrdw/namconb1.c */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namconb1.h"


/* tilemap_palette_bank is used to cache tilemap color, so that we can
 * mark whole tilemaps dirty only when necessary.
 */
static int tilemap_palette_bank[6];

static struct tilemap *background[6];

/* nth_word is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE data16_t nth_word( const data32_t *source, int which )
{
	source += which/2;
	if( which&1 )
	{
		return (*source)&0xffff;
	}
	else
	{
		return (*source)>>16;
	}
}

INLINE void tilemap_get_info(int tile_index,int tilemap_color,const data32_t *tilemap_videoram)
{
	data16_t tile = nth_word( tilemap_videoram, tile_index );
	SET_TILE_INFO(
			1,
			tile,
			tilemap_color,
			0)
	tile_info.mask_data = namconb1_maskrom+tile_info.tile_number*8;
}

static void tilemap_get_info0(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[0],&videoram32[0x0000]); }
static void tilemap_get_info1(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[1],&videoram32[0x0800]); }
static void tilemap_get_info2(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[2],&videoram32[0x1000]); }
static void tilemap_get_info3(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[3],&videoram32[0x1800]); }
static void tilemap_get_info4(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[4],&videoram32[NAMCONB1_FG1BASE/2]); }
static void tilemap_get_info5(int tile_index) { tilemap_get_info(tile_index,tilemap_palette_bank[5],&videoram32[NAMCONB1_FG2BASE/2]); }



WRITE32_HANDLER( namconb1_videoram_w )
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

static void draw_sprite(
		struct mame_bitmap *bitmap,
		const data32_t *pSource,
		int pri_table[8] )
{
	data16_t pict;
	INT16 x0,y0;
	data16_t xsize,ysize;
	data16_t color;
	data32_t format;
	data32_t displace;

	int tile_index;
	int num_cols,num_rows;
	int dx,dy;

	int row,col;
	int sx,sy,tile;
	int flipx,flipy;
	UINT32 zoomx, zoomy;
	int tile_screen_width;
	int tile_screen_height;
	int pri;

	color = pSource[3]>>16;
	pri = pri_table[(color>>4)&0x7];
	pict			= pSource[0]>>16;
	x0				= (pSource[1]>>16) - nth_word( namconb1_spritepos32,1 ) - 0x26;
	y0				= (pSource[1]&0xffff) - nth_word( namconb1_spritepos32,0 ) - 0x19;
	xsize			= pSource[2]>>16;		/* sprite size in pixels */
	ysize			= pSource[2]&0xffff;	/* sprite size in pixels */
	format			= namconb1_spriteformat32[pict*2];
	displace		= namconb1_spriteformat32[pict*2+1];
	tile_index		= format>>16;
	num_cols		= (format>>4)&0xf;
	num_rows		= (format)&0xf;
	dx				= displace>>16;
	dy				= displace&0xffff;

	if( num_cols == 0 ) num_cols = 0x10;
	flipx = (xsize&0x8000)?1:0;
	xsize &= 0x1ff;
	if( xsize == 0 ) return;
	zoomx = (xsize<<16)/(num_cols*16);
	tile_screen_width = (zoomx*16+0x8000)>>16;
	dx = (dx*zoomx+0x8000)>>16;
	if( flipx )
	{
		x0 += dx;
		x0 -= tile_screen_width;
		tile_screen_width = -tile_screen_width;
	}
	else
	{
		x0 -= dx;
	}

	if( num_rows == 0 ) num_rows = 0x10;
	flipy = (ysize&0x8000)?1:0;
	ysize&=0x1ff;
	if( ysize == 0 ) return;
	zoomy = (ysize<<16)/(num_rows*16);
	tile_screen_height = (zoomy*16+0x8000)>>16;
	dy = (dy*zoomy+0x8000)>>16;
	if( flipy )
	{
		y0 += dy;
		y0 -= tile_screen_height;
		tile_screen_height = -tile_screen_height;
	}
	else
	{
		y0 -= dy;
	}

	/* wrap-around */
	x0 &= 0x1ff; if( x0>=288 ) x0 -= 512;
	y0 &= 0x1ff; if( y0>=224 ) y0 -= 512;

	for( row=0; row<num_rows; row++ )
	{
		sy = row*tile_screen_height + y0;
		for( col=0; col<num_cols; col++ )
		{
			tile = nth_word( namconb1_spritetile32, tile_index++ );
			if( (tile&0x8000)==0 )
			{
				sx = col*tile_screen_width + x0;

				pdrawgfxzoom(bitmap,Machine->gfx[0],
					nth_word( namconb1_spritebank32, tile>>11 )*0x800 + (tile&0x7ff),
					color&0xf,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy,
					pri );
			}
		}
	}
}

static void draw_sprites( struct mame_bitmap *bitmap, int pri_table[8] )
{
	int i;
	data16_t which;
	int count;

	if( keyboard_pressed( KEYCODE_H ) ) return;

	count = 0;
	for( i=0; i<256; i++ )
	{
		which = nth_word( namconb1_spritelist32, i );
		count++;
		if( which&0x100 ) break;
	}

	for( i=0; i<count; i++ )
	{
		which = nth_word( namconb1_spritelist32, count-i-1 );
		draw_sprite( bitmap, &spriteram32[(which&0xff)*4], pri_table );
	}
}

static void nab1_install_palette( void )
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

static void handle_mcu( void )
{
	static data16_t credits;
	static int old_coin_state;
	int new_coin_state = readinputport(3)&0x3; /* coin1,2 */

	if( new_coin_state && !old_coin_state )
	{
		credits++;
	}
	old_coin_state = new_coin_state;

	/* Stuff assorted inputs into shared RAM. */
	namconb1_workram32[0x6001/4] &= 0xff00ffff;
	namconb1_workram32[0x6001/4] |= readinputport(0)<<16; /* test mode */

	namconb1_workram32[0x6002/4] &= 0xffff00ff;
	namconb1_workram32[0x6002/4] |= readinputport(1)<<8; /* player#1 */

	namconb1_workram32[0x6004/4] &= 0x00ffffff;
	namconb1_workram32[0x6004/4] |= readinputport(2)<<24; /* player#2 */

	namconb1_workram32[0x601e/4] &= 0xffff0000;
	namconb1_workram32[0x601e/4] |= credits;
}

void namconb1_vh_screenrefresh( struct mame_bitmap *bitmap,int full_refresh )
{
	int beamx,beamy;

	/* We handle sprite-tilemap priority with some trickery.  There are 6 tilemaps,
	 * but only 5 priority levels supported by pdrawgfx.  In practice this doesn't
	 * present any problem, since games rarely (never?) assign each layer a unique
	 * priority value.
	 */
	const int primask[5] =
	{
		0|0xff00|0xf0f0|0xcccc|0xaaaa,	//covered by 1,2,3,4
		0|0xff00|0xf0f0|0xcccc,			//covered by 1,2,3
		0|0xff00|0xf0f0,				//covered by 1,2
		0|0xff00,						//covered by 1
		0
	};
	const int xadjust[4] = { 0,2,3,4 };
	int pri,bPriUsed;
	int pri_table[8]; /* dynamically assigned remap for sprite priority */
	int i,j;
	int scrollx,scrolly,flip;

	handle_mcu();

	nab1_install_palette();

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap( bitmap, 0, 0 ); /* what should background color be? */

	for( i=0; i<6; i++ )
	{
		int tilemap_color = nth_word( &namconb1_scrollram32[0x30/4], i )&7;
		if( tilemap_palette_bank[i]!= tilemap_color )
		{
			tilemap_mark_all_tiles_dirty( background[i] );
			tilemap_palette_bank[i] = tilemap_color;
		}
		switch( i )
		{
		case 4:
		case 5:
			break;
		default:
			flip = 0;
			scrollx = namconb1_scrollram32[i*2+0]+48-xadjust[i];
			if( scrollx&0x8000 )
			{
				flip |= TILEMAP_FLIPX;
				scrollx -=288-64;
			}
			scrolly = namconb1_scrollram32[i*2+1]+24;
			if( scrolly&0x8000 )
			{
				flip |= TILEMAP_FLIPY;
				scrolly +=224;
			}
			tilemap_set_flip(background[i],flip);
			tilemap_set_scrollx( background[i], 0, scrollx );
			tilemap_set_scrolly( background[i], 0, scrolly );
			break;
		}
	}

	pri = 0;
	for( i=0; i<8; i++ )
	{
		bPriUsed = 0;
		for( j=0; j<6; j++ )
		{
			if( nth_word( &namconb1_scrollram32[0x20/4],j ) == i )
			{
				tilemap_draw( bitmap, background[j], 0, 1<<pri );
				bPriUsed = 1;
			}
		}
		if( bPriUsed && pri<4 ) pri++;
		pri_table[i] = primask[pri];
	}

	draw_sprites( bitmap,pri_table );

	if( namconb1_type == key_gunbulet )
	{
		beamx = ((readinputport(4)) * 320)/256;
		beamy = readinputport(5);
		draw_crosshair( bitmap, beamx, beamy, &Machine->visible_area );

		beamx = ((readinputport(6)) * 320)/256;
		beamy = readinputport(7);
		draw_crosshair( bitmap, beamx, beamy, &Machine->visible_area );
	}
}

int namconb1_vh_start( void )
{
	int i;
	static void (*get_info[6])(int tile_index) =
	{ tilemap_get_info0, tilemap_get_info1, tilemap_get_info2, tilemap_get_info3, tilemap_get_info4, tilemap_get_info5 };

	namconb1_maskrom = memory_region( REGION_GFX3 );

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

		tilemap_palette_bank[i] = -1;
	}

	/* Rotate the mask ROM if needed */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		/* borrowed from Namco SystemII */
		int loopX,loopY,tilenum;
		unsigned char tilecache[8],*tiledata;

		for(tilenum=0;tilenum<0x10000;tilenum++)
		{
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Swap X/Y data */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopX]|=(tilecache[loopY]&(0x01<<loopX))?(1<<loopY):0x00;
				}
			}
		}

		/* preprocess bitmask */
		for(tilenum=0;tilenum<0x10000;tilenum++){
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Flip in Y - write back in reverse */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=tilecache[7-loopY];
		}

		for(tilenum=0;tilenum<0x10000;tilenum++){
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Flip in X - do bit reversal */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopY]|=(tilecache[loopY]&(1<<loopX))?(0x80>>loopX):0x00;
				}
			}
		}
	}

	return 0; /* no error */
}

void namconb1_vh_stop( void )
{
}
