#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *lkage_scroll, *lkage_vreg;
static unsigned char bg_tile_bank, fg_tile_bank;

/*
	lkage_scroll[0x00]: unknown (always 0xf7)
	lkage_scroll[0x01]: unknown (always 0x00)
	lkage_scroll[0x02]: foreground layer horizontal scroll
	lkage_scroll[0x03]: foreground layer vertical scroll
	lkage_scroll[0x04]: background layer horizontal scroll
	lkage_scroll[0x05]: background layer vertical scroll

	lkage_vreg[0x00]:
		0x04: tile bank select

	lkage_vreg[0x01]:
		0xf0: background, foreground palette select
		0x08: tile bank select
		0x07: text layer palette select
*/

struct tilemap *bg_tilemap, *fg_tilemap, *text_tilemap;

void lkage_videoram_w( int offset, int data ){
	if( videoram[offset]!=data ){
		int row = (offset/32)&0x1f;
		int col = offset%32;

		videoram[offset] = data;

		switch( offset/0x400 ){
			case 0:
			tilemap_mark_tile_dirty( text_tilemap, col, row );
			break;

			case 1:
			tilemap_mark_tile_dirty( fg_tilemap, col, row );
			break;

			case 2:
			tilemap_mark_tile_dirty( bg_tilemap, col, row );
			break;
		}
	}
}

static void get_bg_tile_info( int col, int row ){
	SET_TILE_INFO(
		bg_tile_bank?2:1,
		videoram[col+row*32 + 0x800],
		0
	);
}

static void get_fg_tile_info( int col, int row ){
	SET_TILE_INFO(
		fg_tile_bank?1:0,
		videoram[col+row*32 + 0x400],
		1
	);
}

static void get_text_tile_info( int col, int row ){
	SET_TILE_INFO(
		0,
		videoram[col+row*32],
		2
	);
}

int lkage_vh_start(void){
	bg_tile_bank = fg_tile_bank = 0;

	bg_tilemap = tilemap_create(
		get_bg_tile_info,
		TILEMAP_OPAQUE,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);

	fg_tilemap = tilemap_create(
		get_fg_tile_info,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);

	text_tilemap = tilemap_create(
		get_text_tile_info,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);

	if( bg_tilemap && fg_tilemap && text_tilemap ){
		fg_tilemap->transparent_pen = 0x0;
		text_tilemap->transparent_pen = 0x0;
		return 0;
	}
	return 1;
}

void lkage_vh_stop(void){
}

static void draw_sprites( struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned char *finish = spriteram;
	const unsigned char *source = spriteram+0x60-4;
	const struct GfxElement *gfx = Machine->gfx[3];

	while( source>=finish ){
		int attributes = source[2];
		/*
			bit 0: horizontal flip
			bit 1: vertical flip
			bit 2: bank select
			bit 3: sprite size
			bit 4..6: color
			bit 7: priority
		*/

		if( (attributes>>7) == priority ){
			int sy = 240-source[1];
			int sx = source[0] - 16;
			int color = (attributes>>4)&7;

			int sprite_number = source[3];
			if( attributes&0x04 )
				sprite_number += 128;
			else
				sprite_number += 256;

			if( sprite_number!=256 ){ /* enable */
				if( attributes&0x02 ){ /* vertical flip */
					if( attributes&0x08 ){ /* tall sprite */
						sy -= 16;
						drawgfx( bitmap,gfx,
							sprite_number^1,
							color,
							attributes&1,1, /* flip */
							sx,sy+16,
							clip,
							TRANSPARENCY_PEN,0 );
					}
					drawgfx( bitmap,gfx,
						sprite_number,
						color,
						attributes&1,1, /* flip */
						sx,sy,
						clip,
						TRANSPARENCY_PEN,0 );
				}
				else {
					if( attributes&0x08 ){ /* tall sprite */
						drawgfx( bitmap,gfx,
							sprite_number^1,
							color,
							attributes&1,0, /* flip */
							sx,sy-16,
							clip,
							TRANSPARENCY_PEN,0 );
					}
					drawgfx( bitmap,gfx,
						sprite_number,
						color,
						attributes&1,0, /* flip */
						sx,sy,
						clip,
						TRANSPARENCY_PEN,0 );
				}
			}
		}
		source-=4;
	}
}

void lkage_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	if( bg_tile_bank != (lkage_vreg[0x01]&0x08) ){
		bg_tile_bank = lkage_vreg[0x01]&0x08;
		tilemap_mark_all_tiles_dirty( bg_tilemap );
	}

	if( fg_tile_bank != (lkage_vreg[0x00]&0x04) ){
		fg_tile_bank = lkage_vreg[0x00]&0x04;
		tilemap_mark_all_tiles_dirty( fg_tilemap );
	}

	/*
		Each tile in a layer uses the same 16 color palette.  Rather than redraw
		an entire tilemap when its associated palette register changes, we make
		use of a virtual palette to achieve the same effect.  This is important
		especially when you grab the scroll in the forest, since the palette
		register is toggled rapidly for about 6 seconds as a special effect.
	*/
	{
		unsigned char *source = paletteram;
		int i = 0, k;
		for( k=0; k<128; k++ ){
			int greenblue = *source++;
			int red = *source++;
			palette_change_color( i++, (red&0xf)<<4, greenblue&0xf0, (greenblue&0xf)<<4 );
		}

		source = &paletteram[(0x30 + (lkage_vreg[1]>>4))*32]; /* background color */
		for( k=0; k<16; k++ ){
			int greenblue = *source++;
			int red = *source++;
			palette_change_color( i++, (red&0xf)<<4, greenblue&0xf0, (greenblue&0xf)<<4 );
		}

		source = &paletteram[(0x20 + (lkage_vreg[1]>>4))*32]; /* foreground color */
		for( k=0; k<16; k++ ){
			int greenblue = *source++;
			int red = *source++;
			palette_change_color( i++, (red&0xf)<<4, greenblue&0xf0, (greenblue&0xf)<<4 );
		}

		source = &paletteram[(0x10 + (lkage_vreg[1]&7))*32]; /* text color - wrong! */
		for( k=0; k<16; k++ ){
			int greenblue = *source++;
			int red = *source++;
			palette_change_color( i++, (red&0xf)<<4, greenblue&0xf0, (greenblue&0xf)<<4 );
		}
	}

	tilemap_set_scrollx( fg_tilemap,0, lkage_scroll[2]+8 );
	tilemap_set_scrolly( fg_tilemap,0, lkage_scroll[3] );
	tilemap_set_scrollx( bg_tilemap,0, lkage_scroll[4]+8 );
	tilemap_set_scrolly( bg_tilemap,0, lkage_scroll[5] );

	tilemap_update( ALL_TILEMAPS );
	palette_recalc();
	tilemap_render( ALL_TILEMAPS );

	/*
		a register exists which is normally 0xf3,
		but goes to 0x13 inbetween stages.  this is
		probably used to selectively enable backgrounds.
	*/
	if( lkage_vreg[2]==0xf3 ){
		tilemap_draw( bitmap,bg_tilemap,0 );
		draw_sprites( bitmap, 1 );
		tilemap_draw( bitmap,fg_tilemap,0 );
		draw_sprites( bitmap, 0 );
		tilemap_draw( bitmap,text_tilemap,0 );
	}
	else {
		tilemap_draw( bitmap,text_tilemap,TILEMAP_IGNORE_TRANSPARENCY );
	}
}
