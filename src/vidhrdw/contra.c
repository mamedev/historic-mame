/***************************************************************************

  gryzor: vidhrdw.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int spriteram_offset;

unsigned char *contra_fg_vertical_scroll;
unsigned char *contra_fg_horizontal_scroll;
unsigned char *contra_bg_vertical_scroll;
unsigned char *contra_bg_horizontal_scroll;
unsigned char *contra_fg_vram,*contra_fg_cram;
unsigned char *contra_text_vram,*contra_text_cram;
unsigned char *contra_bg_vram,*contra_bg_cram;

static int fg_palette_bank,bg_palette_bank;
static int flipscreen;

static struct tilemap *bg_tilemap, *fg_tilemap, *text_tilemap;

/***************************************************************************
**
**	Contra has palette RAM, but it also has four lookup table PROMs
**
**	0	sprites
**	1	foreground
**	2	sprites
**	3	background
**
***************************************************************************/

void contra_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,pal,clut = 0;
	for( pal=0; pal<8; pal++ ){
		switch( pal ){
			/* sprite lookup tables */
			case 0:
			case 2:
			clut = 0;
			break;

			case 4:
			case 6:
			clut = 2;
			break;

			/* background lookup tables */
			case 1:
			case 3:
			clut = 1;
			break;

			case 5:
			case 7:
			clut = 3;
			break;
		}

		for( i=0; i<256; i++ ){
			colortable[pal*256+i] = pal*16+color_prom[clut*256+i];
		}
	}
}

/***************************************************************************
**
**	Tilemap Manager Callbacks
**
***************************************************************************/

static void get_fg_tile_info( int col, int row ){
	int offs = row*32 + col;
	int attr = contra_fg_cram[offs];
	int bank = (attr & 0xf8) >> 3;
	bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);
	SET_TILE_INFO(0, contra_fg_vram[offs]+bank*256, ((fg_palette_bank&0x30)*2+16)+(attr&7) )
}

static void get_bg_tile_info( int col, int row ){
	int offs = row*32 + col;
	int attr = contra_bg_cram[offs];
	int bank = (attr & 0xf8) >> 3;
	bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);
	SET_TILE_INFO(1, contra_bg_vram[offs]+bank*256, ((bg_palette_bank&0x30)*2+16)+(attr&7) )
}

static void get_text_tile_info( int col, int row ){
	int offs = row*32 + col;
	unsigned char attr = contra_text_cram[offs];
	int bank = (attr & 0xf8) >> 3;
	bank = ((bank & 0x0f) << 1) | ((bank & 0x10) >> 4);
	SET_TILE_INFO(0,contra_text_vram[offs] + 256*bank, 1*16+(attr&7) )
}

/***************************************************************************
**
**	Memory Write Handlers
**
***************************************************************************/

void contra_fg_vram_w(int offset,int data){
	if (contra_fg_vram[offset] != data){
		tilemap_mark_tile_dirty( fg_tilemap, offset%32, offset/32 );
		contra_fg_vram[offset] = data;
	}
}

void contra_fg_cram_w(int offset,int data){
	if (contra_fg_cram[offset] != data){
		tilemap_mark_tile_dirty( fg_tilemap, offset%32, offset/32 );
		contra_fg_cram[offset] = data;
	}
}

void contra_bg_vram_w(int offset,int data){
	if (contra_bg_vram[offset] != data){
		tilemap_mark_tile_dirty( bg_tilemap, offset%32, offset/32 );
		contra_bg_vram[offset] = data;
	}
}

void contra_bg_cram_w(int offset,int data){
	if (contra_bg_cram[offset] != data){
		tilemap_mark_tile_dirty( bg_tilemap, offset%32, offset/32 );
		contra_bg_cram[offset] = data;
	}
}

void contra_text_vram_w(int offset,int data){
	int col = offset%32;
	if( col<5 ){
		if( contra_text_vram[offset] == data) return;
		tilemap_mark_tile_dirty( text_tilemap, col, offset/32 );
	}
	contra_text_vram[offset] = data;
}

void contra_text_cram_w(int offset,int data){
	int col = offset%32;
	if( col<5 ){
		if( contra_text_cram[offset] == data) return;
		tilemap_mark_tile_dirty( text_tilemap, col, offset/32 );
	}
	contra_text_cram[offset] = data;
}

void contra_0007_w(int offset,int data)
{
	flipscreen = data & 0x08;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}

void contra_fg_palette_bank_w(int offset,int data){
	if (fg_palette_bank != data){
		fg_palette_bank = data;
		tilemap_mark_all_tiles_dirty( fg_tilemap );
	}
}

void contra_bg_palette_bank_w(int offset,int data){
	if (bg_palette_bank != data ){
		bg_palette_bank = data;
		tilemap_mark_all_tiles_dirty( bg_tilemap );
	}
}

void contra_sprite_buffer_select( int offset, int data ){
	spriteram_offset = (data&0x08)?0x000:0x800;
}

/***************************************************************************
**
**	Video Driver Initialization
**
***************************************************************************/

int contra_vh_start(void){
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
		TILEMAP_OPAQUE,
		8,8,	/* tile width, tile height */
		5,32	/* number of columns, number of rows */
	);

	if( bg_tilemap && fg_tilemap && text_tilemap ){
		struct rectangle clip = Machine->drv->visible_area;
		clip.min_x += 40;
		tilemap_set_clip( bg_tilemap, &clip );
		tilemap_set_clip( fg_tilemap, &clip );

		clip.max_x = 40;
		clip.min_x = 0;
		tilemap_set_clip( text_tilemap, &clip );

		fg_tilemap->transparent_pen = 0;

		return 0;
	}

	return 1;
}

static void draw_sprites( struct osd_bitmap *bitmap, int bank )
{
	const struct rectangle *clip = &Machine->drv->visible_area;
	struct GfxElement *gfx = Machine->gfx[bank];

//	unsigned char *RAM = Machine->memory_region[0];
//	int limit = (bank)? (RAM[0xc2]*256 + RAM[0xc3]) : (RAM[0xc0]*256 + RAM[0xc1]);

	const unsigned char *source = spriteram + bank*0x2000 + spriteram_offset;
	const unsigned char *finish = source+(40)*5;
	int base_color = bank?2:4;
/*
	spriteram[0]	tile_number
	spriteram[1]	XXXX		color
					    XX		bank (least significant bits of tile_number)
					      XX	bank
	spriteram[2]	ypos
	spriteram[3]	xpos
	spriteram[4]	XX			bank
					  XX		flip
					    XXX		size
					       X	most significant bit of xpos
*/

	while( source<finish ){
		int attributes = source[4];
		int sx = source[3];
		int sy = source[2];
		int tile_number = source[0];
		int color =  source[1];

		int yflip		= 0x20 & attributes;
		int xflip		= 0x10 & attributes;
		int width = 0, height = 0;
		if( attributes&0x01 ) sx -= 256;

		tile_number += ((color&0x3)<<8) + ((attributes&0xc0)<<4);
		tile_number = tile_number<<2;
		tile_number += (color>>2)&3;

		switch( attributes&0xe ){
			case 0x00: width = height = 2; tile_number &= (~3); break;
			case 0x08: width = height = 4; tile_number &= (~3); break;
			case 0x06: width = height = 1; break;
		}

		{
			static int x_offset[4] = {0x0,0x1,0x4,0x5};
			static int y_offset[4] = {0x0,0x2,0x8,0xa};
			int x,y, ex, ey;
			int c = base_color*16+(color>>4);

			for( y=0; y<height; y++ ){
				for( x=0; x<width; x++ ){
					ex = xflip?(width-1-x):x;
					ey = yflip?(height-1-y):y;

					drawgfx(bitmap,gfx,
						tile_number+x_offset[ex]+y_offset[ey],
						c,
						xflip,yflip,
						40+sx+x*8,sy+y*8,
						clip,TRANSPARENCY_PEN,0);
				}
			}
		}
		source += 5;
	}
}

void contra_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx( fg_tilemap,0, *contra_fg_vertical_scroll - 40 );
	tilemap_set_scrolly( fg_tilemap,0, *contra_fg_horizontal_scroll );
	tilemap_set_scrollx( bg_tilemap,0, *contra_bg_vertical_scroll - 40 );
	tilemap_set_scrolly( bg_tilemap,0, *contra_bg_horizontal_scroll );

	tilemap_update( ALL_TILEMAPS );
	//palette_recalc();
	tilemap_render( ALL_TILEMAPS );

	tilemap_draw( bitmap, bg_tilemap, 0 );
	tilemap_draw( bitmap, fg_tilemap, 0 );
	draw_sprites( bitmap, 0 );
	draw_sprites( bitmap, 1 );
	tilemap_draw( bitmap, text_tilemap, 0 );
}
