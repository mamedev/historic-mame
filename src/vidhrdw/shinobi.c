/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/system16.h"
#include "vidhrdw/s16sprit.c"

extern void drawgfxpicture(
	struct osd_bitmap *bitmap,
	const unsigned char *source,
	int width, int screenheight,
	const unsigned short *pal,
	int xflip, int yflip,
	int sx, int sy,
	int zoom,
	const struct rectangle *clip
);

/***************************************************************************

  system16 games don't have a color PROM
  Instead, 4096 bytes of RAM are mapped to specify colors.
  2 bytes are used for each color, yielding 2048 color entries.
  1024 of these are used for sprites, and 1024 are used for the backgrounds.

  Graphics use 4 bitplanes.

***************************************************************************/

static void refresh_palette(void);
static void refresh_palette(void)
{
	unsigned char *dirty = system16_colordirty;
	int indx;

	for( indx=0; indx<s16_paletteram_size/2; indx++ ){
		if( dirty[indx] ){
			int offset = indx*2;
			int palette = READ_WORD (&system16_paletteram[offset]);

			/*
				byte0		byte1
				GBGR BBBB	GGGG RRRR
				5444 3210	3210 3210
			*/
			int red		= ( palette & 0xf );
			int green	= ( palette >> 4 ) & 0x0f;
			int blue	= ( palette >> 8 ) & 0x0f;

			red = (red << 4) + red;
			green = (green << 4) + green;
			blue = (blue << 4) + blue;

			setgfxcolorentry (Machine->gfx[0], indx, red, green, blue);
			dirty[indx] = 0;
		}
	}
}

static void draw_background(struct osd_bitmap *bitmap);
static void draw_background(struct osd_bitmap *bitmap)
{
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;

	int scrollx = 320+scrollX[BG_OFFS];
	int scrolly = 256-scrollY[BG_OFFS];

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for (page=0; page < 4; page++){
		unsigned char *source = system16_backgroundram + bg_pages[page]*0x1000;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				int x = startx+col;
				int y = starty+row;
				if( x>320 ) x-=1024;
				if( y>224 ) y-=512;
				if( x>-8 && x<320 && y>-8 && y<224 ){
					int tile = READ_WORD( &source[0] );
					drawgfx( bitmap, gfx,
						tile + system16_background_bank*0x1000,
						(tile >> 6)%128,
						0, 0, x, y,
						clip, TRANSPARENCY_NONE, 0);
				}
				source+=2;
			}
		}
	} /* next page */
}

static void draw_foreground(struct osd_bitmap *bitmap, int priority );
static void draw_foreground(struct osd_bitmap *bitmap, int priority )
{
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;
	int scrollx = 320+scrollX[FG_OFFS];
	int scrolly = 256-scrollY[FG_OFFS];

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	for (page=0; page < 4; page++){
		unsigned char *source = system16_backgroundram + fg_pages[page]*0x1000;

		int startx = (page&1)*512+scrollx;
		int starty = (page>>1)*256+scrolly;

		int row,col;

		for( row=0; row<32*8; row+=8 ){
			for( col=0; col<64*8; col+=8 ){
				int tile = READ_WORD ( &source[0] );
				if( tile>>15 == priority ){
					int x = startx+col;
					int y = starty+row;
					if( x>320 ) x-=1024;
					if( y>224 ) y-=512;
					if( x>-8 && x<320 && y>-8 && y<224 ){
						if( tile )
						 drawgfx( bitmap, gfx,
							tile+(priority?8192:0),/*hack for altered beast */
							(tile >> 6)%128,
							0, 0, x, y,
							clip, TRANSPARENCY_PEN, 0);
					}
				}
				source+=2;
			}
		}
	} /* next page */
}

static void draw_text(struct osd_bitmap *bitmap);
static void draw_text(struct osd_bitmap *bitmap)
{
	const struct rectangle *clip = &Machine->drv->visible_area;

	int sx,sy;
	for (sx = 0; sx < 40; sx++) for (sy = 0; sy < 28; sy++){
			int offs = (sy * 64 + sx + (64-40)) << 1;
			int data = READ_WORD(&system16_videoram[offs]);
			int tile_number = data&0x1ff;
			if( tile_number ){ /* skip spaces */
				drawgfx(bitmap,Machine->gfx[0],
							tile_number,
							(data >> 9)%8, /* color */
							0,0, /* no flip*/
							8*sx,8*sy,
							clip,
							TRANSPARENCY_PEN,0);
			}
	}
}

static void draw_sprites(struct osd_bitmap *bitmap, int priority);
static void draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	struct sys16_sprite_info *sprite = sys16_sprite;
	struct sys16_sprite_info *finish = sprite+64;

	do{
		if( sprite->end_line == 0xff ) break;

		if( sprite->priority == priority ){
			const unsigned char *source = Machine->memory_region[2] +
				sprite->number * 4 + (s16_obj_bank[sprite->bank] << 17);

			const unsigned short *pal = Machine->gfx[0]->colortable + sprite->color+1024;

			int sx			= sprite->horizontal_position + system16_sprxoffset;
			int sy			= sprite->begin_line;
			int height		= sprite->end_line - sy;
			int width		= sprite->width*4;

			if( sprite->vertical_flip ){
				width = 512-width;
			}

			if( sprite->horizontal_flip ){
				source += 4;
				if( sprite->vertical_flip ) source -= width*2;
			}
			else{
				if( sprite->vertical_flip ) source -= width; else source += width;
			}

			drawgfxpicture(
				bitmap,
				source,
				width, height,
				pal,
				sprite->horizontal_flip, sprite->vertical_flip,
				sx, sy,
				sprite->zoom,
				&Machine->drv->visible_area
			);
		}
		sprite++;
	}while( sprite<finish );
}


void system16_vh_screenrefresh(struct osd_bitmap *bitmap);
void system16_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	refresh_palette();

	if( system16_refreshenable_r(0) ){
		draw_background(bitmap);

		draw_sprites(bitmap,1);

		draw_foreground(bitmap,0);

		draw_sprites(bitmap,2);

		draw_foreground(bitmap,1);

		draw_text(bitmap);

		draw_sprites(bitmap,3);
	}
}
