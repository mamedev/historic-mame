/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *tilemap0;
static struct tilemap *tilemap1;
static struct tilemap *textlayer;
static struct tilemap *foreground0;
static struct tilemap *foreground1;
static int changed;

unsigned char *combatsc_io_ram;
static int combatsc_vreg;
unsigned char combatsc_vflags;
unsigned char* banked_area;

extern int combatsc_video_circuit;

extern unsigned char *combatsc_page0;
extern unsigned char *combatsc_page1;
extern unsigned char *combatsc_workram0;
extern unsigned char *combatsc_workram1;

void combatsc_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom ){

	int i,pal,clut = 0;
	for( pal=0; pal<8; pal++ ){
		switch( pal ){
			case 0: /* other sprites */
			case 2: /* other sprites(alt) */
			clut = 2; /* ? */
			break;

			case 4: /* player sprites */
			case 6: /* player sprites(alt) */
			clut = 1;
			break;

			case 1: /* background */
			case 3: /* background(alt) */
			clut = 2;
			break;

			case 5: /* foreground tiles */
			case 7: /* foreground tiles(alt) */
			clut = 3;
			break;
		}

		for( i=0; i<256; i++ ){
			colortable[pal*256+i] = pal*16+color_prom[clut*256+i];
		}
	}
}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info0( int col, int row ){
	int tile_index = row*32 + col;
	unsigned char attributes = combatsc_page0[tile_index];
	int bank = 4*((combatsc_vreg & 0x0f) - 1);
	int number, pal, color;

	if (bank < 0) bank = 0;
	if ((attributes & 0xf0) == 0) bank = 0;	/* text bank */

	if (attributes & 0x10) bank += 2;
	if (attributes & 0x20) bank += 4;
	if (attributes & 0x80) bank += 1;

	pal = (bank == 0 || bank >= 0x1c || (attributes & 0x40)) ? 1 : 3;
	color = pal*16 + (attributes & 0x0f);
	number = combatsc_page0[tile_index + 0x400] + 256*bank;

	SET_TILE_INFO(0, number, color)
}

static void get_tile_info1( int col, int row ){
	int tile_index = row*32 + col;
	unsigned char attributes = combatsc_page1[tile_index];
	int bank = 4*((combatsc_vreg >> 4) - 1);
	int number, pal, color;

	if (bank < 0) bank = 0;
	if ((attributes & 0xf0) == 0) bank = 0;	/* text bank */

	if (attributes & 0x10) bank += 2;
	if (attributes & 0x20) bank += 4;
	if (attributes & 0x80) bank += 1;

	pal = (bank == 0 || bank >= 0x1c || (attributes & 0x40)) ? 5 : 7;
	color = pal*16 + (attributes & 0x0f);
	number = combatsc_page1[tile_index + 0x400] + 256*bank;

	SET_TILE_INFO(1, number, color)
}

static void get_text_info( int col, int row ){
	int tile_index = row*32 + col + 0x800;
	unsigned char attributes = combatsc_page0[tile_index];
	int number = combatsc_page0[tile_index + 0x400];
	int color = attributes & 0x0f;

	SET_TILE_INFO(1, number, color)
}

static void get_foreground_info0( int col, int row ){
	int tile_index = row*32 + col;
	unsigned char attributes = combatsc_page0[tile_index];
	int bank = 4*(((combatsc_vreg & 0x0f) - 1) % 4);
	int number, color;

	if (attributes & 0x10) bank += 2;
	if (attributes & 0x20) bank += 4;
	if (attributes & 0x80) bank += 1;
	if (attributes & 0x40)
		number = combatsc_page0[tile_index + 0x400] + 256*bank;
	else
		number = 0;

	color = 1*16 + (attributes & 0x0f);

	SET_TILE_INFO(0, number, color)
}

static void get_foreground_info1( int col, int row ){
	int tile_index = row*32 + col;
	unsigned char attributes = combatsc_page1[tile_index];
	int bank = 4*(((combatsc_vreg & 0x0f) - 1));
	int number, color;

	if (attributes & 0x10) bank += 2;
	if (attributes & 0x20) bank += 4;
	if (attributes & 0x80) bank += 1;
	if (attributes & 0x40)
		number = combatsc_page1[tile_index + 0x400] + 256*bank;
	else
		number = 0;

	color = 5*16 + (attributes & 0x0f);

	SET_TILE_INFO(1, number, color)
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

void combatsc_vh_stop( void ){
}

int combatsc_vh_start( void ){

	combatsc_vreg = -1;

	tilemap0 = tilemap_create(
		get_tile_info0,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap_set_scroll_rows( tilemap0, 32 );

	tilemap1 = tilemap_create(
		get_tile_info1,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap_set_scroll_rows( tilemap1, 32 );

	textlayer = tilemap_create(
		get_text_info,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);

	foreground0 = tilemap_create(
		get_foreground_info0,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap_set_scroll_rows( foreground0, 32 );

	foreground1 = tilemap_create(
		get_foreground_info1,
		TILEMAP_TRANSPARENT,
		8,8,	/* tile width, tile height */
		32,32	/* number of columns, number of rows */
	);
	tilemap_set_scroll_rows( foreground1, 32 );

	if( tilemap0 && tilemap1 && textlayer && foreground0 && foreground1 ){
		tilemap0->transparent_pen = 0;
		tilemap1->transparent_pen = 0;
		textlayer->transparent_pen = 0;
		foreground0->transparent_pen = 0;
		foreground1->transparent_pen = 0;

		tilemap_set_enable( foreground0, 0 );
		tilemap_set_enable( foreground1, 0 );

		return 0;
	}

	return 1;
}

/***************************************************************************

	Memory handlers

***************************************************************************/

int combatsc_video_r( int offset ){
	return videoram[offset];
}

void combatsc_video_w( int offset, int data ){
	if( videoram[offset]!=data ){
		videoram[offset] = data;
		if( offset<0x800 ){
			offset = offset&0x3ff;
			if (combatsc_video_circuit){
				tilemap_mark_tile_dirty( tilemap1, offset%32, offset/32 );
				tilemap_mark_tile_dirty( foreground1, offset%32, offset/32 );
			}
			else{
				tilemap_mark_tile_dirty( tilemap0, offset%32, offset/32 );
				tilemap_mark_tile_dirty( foreground0, offset%32, offset/32 );
			}
		}
		else if( offset<0x1000 && combatsc_video_circuit==0 ){
			offset = offset&0x3ff;
			tilemap_mark_tile_dirty( textlayer, offset%32, offset/32 );
		}
	}
}


void combatsc_vreg_w( int offset, int data ){
	if(data != combatsc_vreg )	{
		changed = 1;
		tilemap_mark_all_tiles_dirty( textlayer );
		if ((data & 0x0f) != (combatsc_vreg & 0x0f))
			tilemap_mark_all_tiles_dirty( tilemap0 );
		if ((data >> 4) != (combatsc_vreg >> 4))
			tilemap_mark_all_tiles_dirty( tilemap1 );
		combatsc_vreg = data;
		/* enable foreground layers only when they are used to speed up emulation */
		if (combatsc_vreg == 0x55 || combatsc_vreg == 0x44){
			tilemap_set_enable( foreground0, 1 );
			tilemap_set_enable( foreground1, 1 );
			tilemap_mark_all_tiles_dirty( foreground0 );
			tilemap_mark_all_tiles_dirty( foreground1 );
		}
		else{
			tilemap_set_enable( foreground0, 0 );
			tilemap_set_enable( foreground1, 0 );
		}
	}
}

void combatsc_vflag_w( int offset, int data ){
	combatsc_vflags = data;
}

void combatsc_sh_irqtrigger_w(int offset, int data){
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,0xff);
}

int combatsc_io_r( int offset ){
	if ((offset <= 0x403) && (offset >= 0x400)){
		switch (offset)	{
			case 0x400:	return input_port_0_r(0); break;
			case 0x401:	return input_port_1_r(0); break;
			case 0x402:	return input_port_2_r(0); break;
			case 0x403:	return input_port_3_r(0); break;
		}
	}
	return banked_area[offset];
}

void combatsc_io_w( int offset, int data ){

	switch (offset){
		case 0x400: combatsc_vflag_w(0, data); break;
		case 0x800: combatsc_sh_irqtrigger_w(0, data); break;
		case 0xc00:	combatsc_vreg_w(0, data); break;
		default:
			combatsc_io_ram[offset] = data;
	}
}

/***************************************************************************

	bootleg Combat School sprites. Each sprite has 5 bytes:

byte #0:	sprite number
byte #1:	y position
byte #2:	x position
byte #3:
	bit 0:		x position (bit 0)
	bits 1..3:	???
	bit 4:		flip x
	bit 5:		unused?
	bit 6:		sprite bank # (bit 2)
	bit 7:		???
byte #4:
	bits 0,1:	sprite bank # (bits 0 & 1)
	bits 2,3:	unused?
	bits 4..7:	sprite color

***************************************************************************/

static void draw_sprites( struct osd_bitmap *bitmap, const unsigned char *source, int circuit, int priority ){
	const struct GfxElement *gfx = Machine->gfx[circuit+2];
	const struct rectangle *clip = &Machine->drv->visible_area;

	unsigned char *RAM = Machine->memory_region[0];
	int limit = ( circuit) ? (RAM[0xc2]*256 + RAM[0xc3]) : (RAM[0xc0]*256 + RAM[0xc1]);
	const unsigned char *finish;

	source+=0x1000;
	finish = source;
	source+=0x400;
	limit = (0x3400-limit)/8;
	if( limit>=0 ) finish = source-limit*8;
	source-=8;

	while( source>finish ){
		unsigned char attributes = source[3]; /* PBxF ?xxX */
		{
			int number = source[0];
			int x = source[2] - 71 + (attributes & 0x01)*256;
			int y = 242 - source[1];
			unsigned char color = source[4]; /* CCCC xxBB */

			int bank = (color & 0x03) | ((attributes & 0x40) >> 4);

			number = ((number & 0x02) << 1) | ((number & 0x04) >> 1) | (number & (~6));
			number += 256*bank;

			color = (circuit*4)*16 + (color >> 4);

			/*	hacks to select alternate palettes */
			if(combatsc_vreg == 0x40 && (attributes & 0x40)) color += 1*16;
			if(combatsc_vreg == 0x23 && (attributes & 0x02)) color += 1*16;
			if(combatsc_vreg == 0x66 ) color += 2*16;

			drawgfx( bitmap, gfx,
				number, color,
				attributes & 0x10,0, /* flip */
				x,y,
				clip, TRANSPARENCY_PEN, 0 );
		}
		source -= 8;
	}
}

/***************************************************************************

	Konami Combat School sprites (very preliminary). Each sprite has 5 bytes:

byte #0:	sprite number
byte #1:
	bits 0..2:	sprite bank #?
	bit 3:		???
	bits 4..7:	sprite color
byte #2:	y position
byte #3:	x position
byte #4:
	bit 0:		???
	bit 1:		???
	bit 2:		???
	bit 3:		2x sprite?
	bit 4:		flip x
	bit 5:		???
	bit 6:		???
	bit 7:		???

***************************************************************************/

static void draw_sprites_2(struct osd_bitmap *bitmap, const unsigned char *source, int circuit, int priority, int desp){
	const struct GfxElement *gfx = Machine->gfx[circuit+2];
	const unsigned char *finish;

	source = source + desp;
	finish = source + 0x400;

	while( source < finish ){
		int number = source[0];				/* sprite number */
		int sprite_bank = source[1] & 0x07;	/* sprite bank */
		int sx = source[3];					/* vertical position */
		int sy = source[2];					/* horizontal position */
		int attr = source[4];				/* attributes */
		int xflip = source[4] & 0x10;		/* flip x */
		int color = source[1] & 0xf0;		/* color */

		number = ((number & 0x02) << 1) | ((number & 0x04) >> 1) | (number & (~6));
		number += sprite_bank*256;
		color = (circuit*4)*16 + (color >> 4);

		drawgfx( bitmap, gfx, number, color, xflip, 0, sx, sy, 0, TRANSPARENCY_PEN, 0 );
		if (attr & 0x08){	/* 2x sprite? */
			drawgfx( bitmap, gfx, number+1, color, xflip, 0, sx+16, sy, 0, TRANSPARENCY_PEN, 0 );
		}
		source += 5;
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

void cmbatscb_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	int i;

	if (!changed){
		for( i=0; i<32; i++ ){
			tilemap_set_scrollx( tilemap0,i, combatsc_io_ram[0x040+i]+5 );
			tilemap_set_scrollx( tilemap1,i, combatsc_io_ram[0x060+i]+3 );
			tilemap_set_scrollx( foreground0,i, combatsc_io_ram[0x040+i]+5 );
			tilemap_set_scrollx( foreground1,i, combatsc_io_ram[0x060+i]+3 );
		}
		tilemap_set_scrolly( tilemap0,0, combatsc_io_ram[0x000] );
		tilemap_set_scrolly( tilemap1,0, combatsc_io_ram[0x020] );
		tilemap_set_scrolly( foreground0,0, combatsc_io_ram[0x000] );
		tilemap_set_scrolly( foreground1,0, combatsc_io_ram[0x020] );

		tilemap_update( ALL_TILEMAPS );
		palette_recalc();
		tilemap_render( ALL_TILEMAPS );

		if( (combatsc_vflags & 0x20) == 0 ){
			tilemap_draw( bitmap,tilemap1,TILEMAP_IGNORE_TRANSPARENCY );
			draw_sprites( bitmap, combatsc_page1, 1, 0 );
			tilemap_draw( bitmap,tilemap0,0 );
			draw_sprites( bitmap, combatsc_page0, 0, 0 );
		}
		else {
			tilemap_draw( bitmap,tilemap0,TILEMAP_IGNORE_TRANSPARENCY );
			draw_sprites( bitmap, combatsc_page1, 1, 0 );
			tilemap_draw( bitmap,tilemap1,0 );
			draw_sprites( bitmap, combatsc_page0, 0, 0 );
		}
		draw_sprites( bitmap, combatsc_page0, 0, 1 );
		draw_sprites( bitmap, combatsc_page1, 1, 1 );

		tilemap_draw( bitmap,foreground1,0 );
		tilemap_draw( bitmap,foreground0,0 );
		tilemap_draw( bitmap,textlayer,0 );
	}
	else
		changed = 0;
}

void combatsc_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh ){
	int i;

	if (!changed){
		if (!combatsc_workram0[0x00] && combatsc_workram0[0x01] != 8)
			for( i=0; i<32; i++ ){
				tilemap_set_scrollx( tilemap0,i, combatsc_workram0[0x20+i] );
				tilemap_set_scrollx( foreground0,i, combatsc_workram0[0x20+i] );
			}
		else
			for( i=0; i<32; i++ ){
				tilemap_set_scrollx( tilemap0,i, combatsc_workram0[0] );
				tilemap_set_scrollx( foreground0,i, combatsc_workram0[0] );
			}
		if (!combatsc_workram1[0x00] && combatsc_workram1[0x01] != 8)
			for( i=0; i<32; i++ ){
				tilemap_set_scrollx( tilemap1,i, combatsc_workram1[0x20+i] );
				tilemap_set_scrollx( foreground1,i, combatsc_workram1[0x20+i] );
			}
		else
			for( i=0; i<32; i++ ){
				tilemap_set_scrollx( tilemap1,i, combatsc_workram1[0] );
				tilemap_set_scrollx( foreground1,i, combatsc_workram1[0] );
			}

		tilemap_set_scrolly( tilemap0,0, combatsc_workram0[0x02] );
		tilemap_set_scrolly( tilemap1,0, combatsc_workram1[0x02] );
		tilemap_set_scrolly( foreground0,0, combatsc_workram0[0x02] );
		tilemap_set_scrolly( foreground1,0, combatsc_workram1[0x02] );

		tilemap_update( ALL_TILEMAPS );
		palette_recalc();
		tilemap_render( ALL_TILEMAPS );

		if( (combatsc_vflags & 0x20) == 0 ){
			tilemap_draw( bitmap,tilemap1,TILEMAP_IGNORE_TRANSPARENCY );
			draw_sprites_2( bitmap, combatsc_page1, 1, 0, 0x1800 );
			tilemap_draw( bitmap,tilemap0,0 );
			draw_sprites_2( bitmap, combatsc_page0, 0, 0, 0x1800 );
		}
		else {
			tilemap_draw( bitmap,tilemap0,TILEMAP_IGNORE_TRANSPARENCY );
			draw_sprites_2( bitmap, combatsc_page1, 1, 0, 0x1800 );
			tilemap_draw( bitmap,tilemap1,0 );
			draw_sprites_2( bitmap, combatsc_page0, 0, 0, 0x1800);
		}
		draw_sprites_2( bitmap, combatsc_page0, 0, 1, 0x1800 );
		draw_sprites_2( bitmap, combatsc_page1, 1, 1, 0x1800 );

		tilemap_draw( bitmap,foreground1,0 );
		tilemap_draw( bitmap,foreground0,0 );
		tilemap_draw( bitmap,textlayer,0 );
	}
	else
		changed = 0;
}
