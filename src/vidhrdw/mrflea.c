/******************************************************************

Mr. F. Lea
(C) 1983 PACIFIC NOVELTY MFG. INC.

******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int mrflea_gfx_bank;

WRITE_HANDLER( mrflea_gfx_bank_w ){
	mrflea_gfx_bank = data;
	if( data & ~0x14 )
		logerror( "unknown gfx bank: 0x%02x\n", data );
}

static void draw_sprites( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[0];
	const UINT8 *source = spriteram;
	const UINT8 *finish = source+0x100;
	
	struct rectangle clip = Machine->visible_area;
	clip.max_x -= 24;
	clip.min_x += 16;
	
	while( source<finish ){
		int xpos = source[1];
		int ypos = source[0]-16;
		int tile_number = source[3];
		if( tile_number ) tile_number += 0x100; else tile_number = source[2];

		/*	the location of the tile_number byte (either 0x02 or 0x03)
		**	determines the bank. */
		
		drawgfx( bitmap, gfx,
			tile_number,
			0, /* color */
			0,0, /* no flip */
			xpos,ypos,
			&clip,TRANSPARENCY_PEN,0 );
		drawgfx( bitmap, gfx,
			tile_number,
			0, /* color */
			0,0, /* no flip */
			xpos,256+ypos,
			&clip,TRANSPARENCY_PEN,0 );
		source+=4;
	}
}

WRITE_HANDLER( mrflea_videoram_w ){
	int bank = offset/0x400;
	offset &= 0x3ff;
	videoram[offset] = data;
	videoram[offset+0x400] = bank;
	/*	the address range that tile data is written to sets one bit of
	**	the bank select.  The remaining bits are from a video register.
	*/
}

static void draw_background( struct osd_bitmap *bitmap ){
	const UINT8 *source = videoram;
	const struct GfxElement *gfx = Machine->gfx[1];
	int sx,sy;
	int base = 0;
	if( mrflea_gfx_bank&0x04 ) base |= 0x400;
	if( mrflea_gfx_bank&0x10 ) base |= 0x200;
	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			int tile_number = base+source[0]+source[0x400]*0x100;
			source++;
			drawgfx( bitmap, gfx,
				tile_number,
				0, /* color */
				0,0, /* no flip */
				sx,sy,
				0, /* no clip */
				TRANSPARENCY_NONE,0 );
		}
	}
}

int mrflea_vh_start( void ){
	return 0;
}

void mrflea_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	palette_recalc();
	draw_background( bitmap );
	draw_sprites( bitmap );
}
