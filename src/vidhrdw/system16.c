/***************************************************************************

Sega System 16 Video Hardware

Each scrolling layer (foreground, background) is an arrangement
of 4 pages selected from 16 available pages, laid out as follows:

	Page0  Page1
	Page2  Page3

Each page is an arrangement of 8x8 tiles, 64 tiles wide, and 32 tiles high.

***************************************************************************/

#include "driver.h"
#include "sprite.h" /* WIP */

#define MAX_SPRITES 64

extern unsigned char *sys16_textram;
extern unsigned char *sys16_spriteram;
extern unsigned char *sys16_tileram; /* contains tilemaps for 16 pages */

/* video driver constants (potentially different for each game) */
int sys16_spritesystem;
int sys16_sprxoffset;
int *sys16_obj_bank;
extern void (* sys16_update_proc)( void );

/* video registers */
int sys16_tile_bank1;
int sys16_tile_bank0;
int sys16_refreshenable;
int sys16_bg_scrollx, sys16_bg_scrolly;
int sys16_bg_page[4];
int sys16_fg_scrollx, sys16_fg_scrolly;
int sys16_fg_page[4];

static struct tilemap *background, *foreground, *text_layer;
static int old_bg_page[4],old_fg_page[4], old_tile_bank1, old_tile_bank0;

/***************************************************************************/

static void update_page( void ){
	int dirty,i, all_dirty = 0;

	if( old_tile_bank1 != sys16_tile_bank1 ){
		all_dirty = 1;
		old_tile_bank1 = sys16_tile_bank1;
	}
	if( old_tile_bank0 != sys16_tile_bank0 ){
		all_dirty = 1;
		old_tile_bank0 = sys16_tile_bank0;
	}

	dirty = all_dirty;
	for( i=0; i<4; i++ ){
		if( old_bg_page[i]!=sys16_bg_page[i] ){
			dirty = 1;
			old_bg_page[i] = sys16_bg_page[i];
		}
	}
	if( dirty ) tilemap_mark_all_tiles_dirty( background );

	dirty = all_dirty;
	for( i=0; i<4; i++ ){
		if( old_fg_page[i]!=sys16_fg_page[i] ){
			dirty = 1;
			old_fg_page[i] = sys16_fg_page[i];
		}
	}
	if( dirty ) tilemap_mark_all_tiles_dirty( foreground );
}

static void get_bg_tile_info( int col, int row ){
	const UINT16 *source = (const UINT16 *)sys16_tileram;

	if( row<32 ){
		if( col<64 ){
			source += 64*32*sys16_bg_page[0];
		}
		else {
			source += 64*32*sys16_bg_page[1];
		}
	}
	else {
		if( col<64 ){
			source += 64*32*sys16_bg_page[2];
		}
		else {
			source += 64*32*sys16_bg_page[3];
		}
	}
	row = row%32;
	col = col%64;

	{
		int data = source[row*64+col];
		int tile_number = (data&0xfff) +
				0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
		SET_TILE_INFO( 0, tile_number, (data>>6)&0x7f );
	}
}

static void get_fg_tile_info( int col, int row ){
	const UINT16 *source = (const UINT16 *)sys16_tileram;

	if( row<32 ){
		if( col<64 ){
			source += 64*32*sys16_fg_page[0];
		}
		else {
			source += 64*32*sys16_fg_page[1];
		}
	}
	else {
		if( col<64 ){
			source += 64*32*sys16_fg_page[2];
		}
		else {
			source += 64*32*sys16_fg_page[3];
		}
	}
	row = row%32;
	col = col%64;

	{
		int data = source[row*64+col];
		int tile_number = (data&0xfff) +
				0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

		SET_TILE_INFO( 0, tile_number, (data>>6)&0x7f );
		tile_info.priority = (data&0x8000)?1:0;
	}
}

void sys16_tileram_w( int offset, int data ){
	int oldword = READ_WORD(&sys16_tileram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	if( oldword != newword ){
		int row,col,page;
		WRITE_WORD(&sys16_tileram[offset],newword);
		offset = offset/2;
		col = offset%64;
		row = (offset/64)%32;
		page = offset/(64*32);
		if( sys16_bg_page[0]==page ) tilemap_mark_tile_dirty( background, col, row );
		if( sys16_bg_page[1]==page ) tilemap_mark_tile_dirty( background, col+64, row );
		if( sys16_bg_page[2]==page ) tilemap_mark_tile_dirty( background, col, row+32 );
		if( sys16_bg_page[3]==page ) tilemap_mark_tile_dirty( background, col+64, row+32 );

		if( sys16_fg_page[0]==page ) tilemap_mark_tile_dirty( foreground, col, row );
		if( sys16_fg_page[1]==page ) tilemap_mark_tile_dirty( foreground, col+64, row );
		if( sys16_fg_page[2]==page ) tilemap_mark_tile_dirty( foreground, col, row+32 );
		if( sys16_fg_page[3]==page ) tilemap_mark_tile_dirty( foreground, col+64, row+32 );
	}
}

int sys16_tileram_r( int offset ){
	return READ_WORD (&sys16_tileram[offset]);
}

/***************************************************************************/

static void get_text_tile_info( int col, int row ){
	const UINT16 *source = (UINT16 *)sys16_textram;
	int tile_number = source[row*64+col + (64-40)];
	SET_TILE_INFO( 0, tile_number&0x1ff, (tile_number>>9)%8 );
}

void sys16_textram_w( int offset, int data ){
	int oldword = READ_WORD(&sys16_textram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	if( oldword != newword ){
		int row,col;
		WRITE_WORD(&sys16_textram[offset],newword);
		offset = (offset/2);
		col = (offset%64)-(64-40);
		row = offset/64;
		if( col>=0 && col<40 && row<28 ){
			tilemap_mark_tile_dirty( text_layer, col, row );
		}
	}
}

int sys16_textram_r( int offset ){
	return READ_WORD (&sys16_textram[offset]);
}

/***************************************************************************/

void sys16_vh_stop( void ){
}

int sys16_vh_start( void ){
	background = tilemap_create(
		get_bg_tile_info,
		TILEMAP_OPAQUE,
		8,8,
		64*2,32*2 );

	foreground = tilemap_create(
		get_fg_tile_info,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	text_layer = tilemap_create(
		get_text_tile_info,
		TILEMAP_TRANSPARENT,
		8,8,
		40,28 );


	if( background && foreground && text_layer )
	{
		int i;


		/* initialize all entries to black - needed for Golden Axe*/
		for( i=0; i<2048; i++ ){
			palette_change_color( i, 0,0,0 );
		}

		foreground->transparent_pen = 0;
		text_layer->transparent_pen = 0;

		tilemap_set_scroll_rows(background,1);
		tilemap_set_scroll_cols(background,1);
		tilemap_set_scroll_rows(foreground,1);
		tilemap_set_scroll_cols(foreground,1);

		sys16_tile_bank0 = 0;
		sys16_tile_bank1 = 1;

		sys16_fg_scrollx = 0;
		sys16_fg_scrolly = 0;

		sys16_bg_scrollx = 0;
		sys16_bg_scrolly = 0;

		sys16_refreshenable = 1;

		/* common defaults */
		sys16_update_proc = 0;
		sys16_spritesystem = 1;
		sys16_sprxoffset = -0xb8;

		return 0;
	}
	return 1;
}

/***************************************************************************/

void sys16_draw_sprites( struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned short *base_pal = Machine->gfx[0]->colortable + 1024;
	const unsigned char *base_gfx = Machine->memory_region[2];

	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+MAX_SPRITES*8;


	switch( sys16_spritesystem  ){
		case 1: /* standard sprite hardware */
		do{
			unsigned short attributes = source[4];

			if( ((attributes>>6)&0x3) == priority ){
				const unsigned char *gfx = base_gfx + source[3]*4 + (sys16_obj_bank[(attributes>>8)&0xf] << 17);

				int sy = source[0];
				int end_line = sy>>8;
				sy &= 0xff;

				if( end_line == 0xff ) break;

				{
					int zoom = source[5]&0x3ff;
					int flip;

					sprite_info.pal_data = base_pal + ((attributes&0x3f)<<4);

					sprite_info.dest_x = source[1] + sys16_sprxoffset;;
					sprite_info.dest_y = sy;
					sprite_info.dest_h = end_line-sy;
					sprite_info.source_h = sprite_info.dest_h*(0x400+zoom)/0x400;
					sprite_info.source_w = source[2];

					sprite_info.flags = 0;
					if( sprite_info.source_w&0x100 ) sprite_info.flags |= SPRITE_FLIPX;
					if( sprite_info.source_w&0x080 ) sprite_info.flags |= SPRITE_FLIPY;

					sprite_info.source_w = (sprite_info.source_w&0x7f)*4;

					if( sprite_info.flags&SPRITE_FLIPY ){
						sprite_info.source_w = 512-sprite_info.source_w;

						if( sprite_info.flags&SPRITE_FLIPX ){
							gfx += 4 - sprite_info.source_w*(sprite_info.source_h+1);
						}
						else {
							gfx -= sprite_info.source_w*sprite_info.source_h;
						}
					}
					else {
						if( sprite_info.flags&SPRITE_FLIPX ){
							gfx += 4;
						}
						else {
							gfx += sprite_info.source_w;
						}
					}

					sprite_info.source_row_offset = sprite_info.source_w;
					sprite_info.dest_w = sprite_info.source_w*0x400/(0x400 + zoom);
					sprite_info.source_baseaddr = gfx;

					draw_sprite( bitmap );
				}
			}

			source += 8;
		} while( source<finish );
		break;

		case 0: /* passing shot */
		do{
			unsigned short attributes = source[5];
			if( ((attributes>>14)&0x3) == priority ){
				int sy = source[1];
				if( sy != 0xffff ){
					int bank = (attributes>>4)&0xf;
					int number = source[2];
					int horizontal_flip = number & 0x8000;
					int zoom = source[4]&0x3ff;
					int sx = source[0] + sys16_sprxoffset;
					int width = source[3];
					int vertical_flip = width&0x80;
					int end_line = sy>>8;
					sy = sy&0xff;
					sy+=2;
					end_line+=2;

					sprite_info.dest_h = end_line - sy;
					sprite_info.source_h = sprite_info.dest_h*(0x400+zoom)/0x400;

					if( vertical_flip ){
						width &= 0x7f;
						width = 0x80-width;
						sprite_info.source_w = width*4;
					}
					else{
						width &= 0x7f;
						sprite_info.source_w = width*4;
					}

					if( horizontal_flip ){
						bank = (bank-1) & 0xf;
						if( vertical_flip ){
							sx += 5;
						}
						else {
							number += 1-width;
						}
					}

					sprite_info.source_baseaddr = base_gfx + number*4 + (sys16_obj_bank[bank] << 17);
					if( vertical_flip ) sprite_info.source_baseaddr -= sprite_info.source_h*sprite_info.source_w;

					sprite_info.dest_x = sx;
					sprite_info.dest_y = sy;
					sprite_info.dest_w = sprite_info.source_w*0x400/(0x400+zoom);
					sprite_info.flags = (horizontal_flip?SPRITE_FLIPX:0)|
						(vertical_flip?SPRITE_FLIPY:0);
					sprite_info.pal_data = base_pal + ((attributes>>4)&0x3f0);
					sprite_info.source_row_offset = sprite_info.source_w;

					draw_sprite( bitmap );
				}
			}

			source += 8;
		}while( source<finish );
		break;
	}
}

/***************************************************************************/

static void mark_sprite_colors( void ){
	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+MAX_SPRITES*8;

	char used[64];
	memset( used, 0, 64 );

	switch( sys16_spritesystem ){
		case 1: /* standard sprite hardware */
		do{
			if( (source[0]>>8) == 0xff ) break;
			used[source[4]&0x3f] = 1;
			source+=8;
		}while( source<finish );
		break;

		case 0: /* passing shot */
		do{
			if( source[1]!=0xffff ) used[(source[5]>>8)&0x3f] = 1;
			source+=8;
		}while( source<finish );
		break;
	}

	{
		unsigned char *pal = &palette_used_colors[1024];
		int i;
		for (i = 0; i < 64; i++){
			if ( used[i] ){
				pal[0] = PALETTE_COLOR_UNUSED;
				memset( &pal[1],PALETTE_COLOR_USED,14 );
				pal[15] = PALETTE_COLOR_UNUSED;
			}
			else {
				memset( pal, PALETTE_COLOR_UNUSED, 16 );
			}
			pal += 16;
		}
	}
}

void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	if( sys16_update_proc ) sys16_update_proc();

	if( sys16_refreshenable ){
		update_page();

		tilemap_set_scrollx( background, 0, 320+sys16_bg_scrollx );
		tilemap_set_scrolly( background, 0, 256-sys16_bg_scrolly );

		tilemap_set_scrollx( foreground, 0, 320+sys16_fg_scrollx );
		tilemap_set_scrolly( foreground, 0, 256-sys16_fg_scrolly );

		tilemap_update(  ALL_TILEMAPS  );
		palette_init_used_colors();
		mark_sprite_colors();

		if( palette_recalc() ) tilemap_mark_all_pixels_dirty( ALL_TILEMAPS );
		tilemap_render(  ALL_TILEMAPS  );

		tilemap_draw( bitmap, background, 0 );
		sys16_draw_sprites(bitmap,1);
		tilemap_draw( bitmap, foreground, 0 );
		sys16_draw_sprites(bitmap,2);
		tilemap_draw( bitmap, foreground, 1 );
		sys16_draw_sprites(bitmap,3);
		tilemap_draw( bitmap, text_layer, 0 );
	}
}
