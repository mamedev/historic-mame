#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


static unsigned char videoreg[7];
static unsigned char sprite_layer, bg_color, text_color = 0x04;


unsigned char *marvins_videoram1;
unsigned char *marvins_videoram2;
unsigned char *marvins_videoram3;

struct tilemap *tilemap1, *tilemap2, *tilemap3;

void marvins_videoram1_w( int offset, int data ){
	tilemap_mark_tile_dirty( tilemap1, offset/32, offset%32 );
	marvins_videoram1[offset] = data;
}

void marvins_videoram2_w( int offset, int data ){
	tilemap_mark_tile_dirty( tilemap2, offset/32, offset%32 );
	marvins_videoram2[offset] = data;
}

void marvins_videoram3_w( int offset, int data ){
	tilemap_mark_tile_dirty( tilemap3, offset/32, offset%32 );
	marvins_videoram3[offset] = data;
}

void marvins_sprite_layer_w( int offset, int data ){ sprite_layer = data; }

void marvins_bg_color_w( int offset, int data ){
	if( bg_color!=data ){
		bg_color = data;
		tilemap_mark_all_tiles_dirty( tilemap1 );
		tilemap_mark_all_tiles_dirty( tilemap2 );
	}
}

void marvins_videoreg0_w( int offset, int data ){ videoreg[0] = data; }
void marvins_videoreg1_w( int offset, int data ){ videoreg[1] = data; }
void marvins_videoreg2_w( int offset, int data ){ videoreg[2] = data; }
void marvins_videoreg3_w( int offset, int data ){ videoreg[3] = data; }
void marvins_videoreg4_w( int offset, int data ){ videoreg[4] = data; }
void marvins_videoreg5_w( int offset, int data ){ videoreg[5] = data; }
void marvins_videoreg6_w( int offset, int data ){ videoreg[6] = data; }

extern void aso_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

static void get_tilemap1_info( int col, int row ){
	SET_TILE_INFO(
		2,
		marvins_videoram1[col*32+row],
		bg_color&0xf
	);
}
static void get_tilemap2_info( int col, int row ){
	SET_TILE_INFO(
		1,
		marvins_videoram2[col*32+row],
		bg_color>>4
	);
}
static void get_tilemap3_info( int col, int row ){
	SET_TILE_INFO(
		0,
		marvins_videoram3[col*32+row],
		text_color&0xf
	);
}

int marvins_vh_start( void ){
	tilemap1 = tilemap_create(
		get_tilemap1_info,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap2 = tilemap_create(
		get_tilemap2_info,
		TILEMAP_OPAQUE,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap3 = tilemap_create(
		get_tilemap3_info,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	if( tilemap1 && tilemap2 && tilemap3 ){
		struct rectangle clip = Machine->drv->visible_area;
		clip.max_x-=16;
		clip.min_x+=16;
		tilemap_set_clip( tilemap1, &clip );
		tilemap_set_clip( tilemap2, &clip );
		tilemap_set_clip( tilemap3, &clip );

		tilemap1->transparent_pen = tilemap3->transparent_pen = 0xf;

		tilemap_set_scrollx( tilemap1, 0, -16 );
		tilemap_set_scrollx( tilemap2, 0, -16 );
		tilemap_set_scrollx( tilemap3, 0, -16 );

		return 0;
	}
	return 1;
}

void marvins_vh_stop( void ){
}

static void draw_status( struct osd_bitmap *bitmap, unsigned char *base, int color, int bank ){
	struct rectangle clip = Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[bank];
	int row;
	for( row=0; row<4; row++ ){
		int sy,sx = (row&1)*8;
		const unsigned char *source = base + (row&1)*32;
		if( row>1 ){
			sx+=256+16;
		}
		else {
			source+=30*32;
		}

		for( sy=0; sy<256; sy+=8 ){
			drawgfx( bitmap, gfx,
				*source++, color,
				0,0, /* no flip */
				sx,sy,
				&clip,
				TRANSPARENCY_NONE, 0xf );
		}
	}
}
static void draw_sprites( struct osd_bitmap *bitmap, int priority ){
	const struct GfxElement *gfx = Machine->gfx[3];
	struct rectangle clip = Machine->drv->visible_area;
	const unsigned char *source, *finish;

	static int yscroll = 0, xscroll = 255+20;

	if( priority ){
		source = spriteram + (sprite_layer&0x7c);
		finish = spriteram + 0x80;
	}
	else {
		source = spriteram;
		finish = spriteram + (sprite_layer&0x7c);
	}

	while( source<finish ){
		int attributes = source[3]; /* YBBX.CCCC */
		int tile_number = source[1];
		int sy = source[0] + ((attributes&0x10)?256:0) - yscroll;
		int sx = source[2] + ((attributes&0x80)?256:0) - xscroll;
		int color = attributes&0xf;

		if( attributes&0x40 ) tile_number += 256;
		if( attributes&0x20 ) tile_number += 512;

		drawgfx( bitmap,gfx,
			tile_number,
			color,
			0,0,
			(256-sx)&0x1ff,sy&0x1ff,
			&clip,TRANSPARENCY_PEN,7);

		source+=4;
	}
}

void marvins_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	tilemap_update( ALL_TILEMAPS );
	//palette_recalc();
	tilemap_render( ALL_TILEMAPS );

	tilemap_draw( bitmap,tilemap2,0 );
	draw_sprites( bitmap, 0 );
	tilemap_draw( bitmap,tilemap1,0 );
	draw_sprites( bitmap, 1 );
	tilemap_draw( bitmap,tilemap3,0 );
	draw_status( bitmap, spriteram+0x3400, text_color>>4, 0 );

	if( keyboard_key_pressed( KEYCODE_D ) ){ /* debug code: display video registers */
		char digit[16] = "0123456789abcdef";
		int i;
		for( i=0; i<=6; i++ ){
			int sy = 32+i*8;
			int data = videoreg[i];

			drawgfx( bitmap, Machine->uifont,
				digit[(data>>4)], 0,
				0, 0,
				32+0,sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_NONE,0);

			drawgfx( bitmap, Machine->uifont,
				digit[(data&0xf)], 0,
				0, 0,
				32+8,sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_NONE,0);
		}
	}
}
