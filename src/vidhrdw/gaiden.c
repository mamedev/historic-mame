/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

unsigned char *gaiden_videoram;
unsigned char *gaiden_videoram2;
unsigned char *gaiden_videoram3;

void gaiden_vh_stop (void);

static struct tilemap *text_layer,*foreground,*background;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static const UINT16 *videoram1, *videoram2;
static int gfxbank;

static void get_fg_tile_info( int tile_index ){
	SET_TILE_INFO(gfxbank,videoram1[tile_index] & 0x7ff,(videoram2[tile_index] & 0xf0) >> 4)
}

static void get_bg_tile_info( int tile_index ){
	SET_TILE_INFO(gfxbank,videoram1[tile_index] & 0xfff,(videoram2[tile_index] & 0xf0) >> 4)
}

int gaiden_vh_start(void){
	if( tilemap_start()==0 ){

		text_layer = tilemap_create(
			TILEMAP_TRANSPARENT,
			8,8,	/* tile width, tile height */
			32,32,	/* number of columns, number of rows */
			1,1	/* scroll rows, scroll columns */
		);

		foreground = tilemap_create(TILEMAP_TRANSPARENT,16,16,64,32,1,1);

		background = tilemap_create(0,16,16,64,32,1,1);

		if( text_layer && foreground && background ){
			text_layer->tile_get_info = get_fg_tile_info;
			text_layer->transparent_pen = 0;

			foreground->tile_get_info = get_bg_tile_info;
			foreground->transparent_pen = 0;

			background->tile_get_info = get_bg_tile_info;

			palette_transparent_color = 0x200; /* background color */

			return 0;
		}
	}

	gaiden_vh_stop();
	return 1;
}

void gaiden_vh_stop(void){
	tilemap_dispose(background);
	tilemap_dispose(foreground);
	tilemap_dispose(text_layer);

	tilemap_stop();
}

/* scroll write handlers */

void gaiden_txscrollx_w( int offset,int data ){
	int oldword = -tilemap_get_scrollx( text_layer, 0 );
	tilemap_set_scrollx( text_layer,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_txscrolly_w( int offset,int data ){
	int oldword = -tilemap_get_scrolly( text_layer, 0 );
	tilemap_set_scrolly( text_layer,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_fgscrollx_w( int offset,int data ){
	int oldword = -tilemap_get_scrollx( foreground, 0 );
	tilemap_set_scrollx( foreground,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_fgscrolly_w( int offset,int data ){
	int oldword = -tilemap_get_scrolly( foreground, 0 );
	tilemap_set_scrolly( foreground,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_bgscrollx_w( int offset,int data ){
	int oldword = -tilemap_get_scrollx( background, 0 );
	tilemap_set_scrollx( background,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_bgscrolly_w( int offset,int data ){
	int oldword = -tilemap_get_scrolly( background, 0 );
	tilemap_set_scrolly( background,0, -COMBINE_WORD(oldword,data) );
}

void gaiden_videoram3_w( int offset,int data ){
	int oldword = READ_WORD(&gaiden_videoram3[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		WRITE_WORD(&gaiden_videoram3[offset],newword);
		tilemap_mark_tile_dirty( background,(offset/2)&0x7ff );
	}
}

int gaiden_videoram3_r(int offset){
   return READ_WORD (&gaiden_videoram3[offset]);
}

void gaiden_videoram2_w(int offset,int data){
	int oldword = READ_WORD(&gaiden_videoram2[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		WRITE_WORD(&gaiden_videoram2[offset],newword);
		tilemap_mark_tile_dirty(foreground, (offset/2)&0x7ff );
	}
}

int gaiden_videoram2_r(int offset){
   return READ_WORD (&gaiden_videoram2[offset]);
}

void gaiden_videoram_w(int offset,int data){
	int oldword = READ_WORD(&gaiden_videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		WRITE_WORD(&gaiden_videoram[offset],newword);
		tilemap_mark_tile_dirty(text_layer,(offset/2)&0x3ff );
	}
}

int gaiden_videoram_r(int offset){
	return READ_WORD (&gaiden_videoram[offset]);
}

void gaiden_spriteram_w(int offset,int data){
	COMBINE_WORD_MEM (&spriteram[offset],data);
}

int gaiden_spriteram_r(int offset){
	return READ_WORD (&spriteram[offset]);
}

/* sprite format:
 *
 *	word		bit					usage
 * --------+-fedcba9876543210-+----------------
 *    0    | ---------------x | flip x
 *         | --------------x- | flip y
 *         | -------------x-- | enable
 *         | ----------x----- | flash
 *         | --------xx------ | priority
 *    1    | xxxxxxxxxxxxxxxx | number
 *    2    | --------xxxx---- | palette
 *         | --------------xx | size: 8x8, 16x16, 32x32, 64x64
 *    3    | xxxxxxxxxxxxxxxx | y position
 *    4    | xxxxxxxxxxxxxxxx | x position
 */

void gaiden_drawsprites(struct osd_bitmap *bitmap, int priority){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[3];
	const unsigned short *source = (const unsigned short *)spriteram;
	const unsigned short *finish = source+0x400;

	while( source<finish ){
		int attributes = source[0];
		if( (attributes&0x4) && priority == ((attributes>>6)&3)
			&& (!(attributes & 0x20) || (cpu_getcurrentframe() & 1)))
		{
			int sy = source[3] & 0x1ff;
			int sx = source[4] & 0x1ff;
			if (sx >= 256) sx -= 512;
			if (sy >= 256) sy -= 512;

			if( sx > -64 && sx < 256 && sy > -64 && sy < 256 ){
				int sprite_number = source[1];
				int color = source[2];
				int size = color&0x3;
				int num_tiles = 1<<(size*2);

				int span = (1<<size)-1;
				int i;

				color = (color>>4)&0xf;

				for( i=0; i<num_tiles; i++ ){
					int dx = ((i>>0)&1)|((i>>1)&2)|((i>>2)&4);
					int dy = ((i>>1)&1)|((i>>2)&2)|((i>>3)&4);
					if( attributes&1 ) dx = span-dx;
					if( attributes&2 ) dy = span-dy;
					drawgfx(bitmap, gfx,
						(sprite_number+i)&0x7fff,
						color,
						attributes&1,attributes&2, /* flip */
						sx+8*dx,sy+8*dy,
						clip,
						priority == 3 ? TRANSPARENCY_THROUGH : TRANSPARENCY_PEN,
						priority == 3 ? palette_transparent_pen : 0);
				}
			} /* clip */
		} /* priority */
		source += 8;
	} /* process next sprite */
}

static void mark_sprite_colors( void ){
	const struct GfxElement *gfx = Machine->gfx[3];
	unsigned char *pen_used = &palette_used_colors[Machine->drv->gfxdecodeinfo[3].color_codes_start];
	const unsigned short *source = (const unsigned short *)spriteram;
	const unsigned short *finish = source+0x400;

	int i;
	unsigned int colmask[16];
	memset( colmask, 0, 16*sizeof(unsigned int) );

	while( source<finish ){
		if( source[0]&4 ){
			int sx = source[4]&0x1ff;
			int sy = source[3]&0x1ff;
			if (sx >= 256) sx -= 512;
			if (sy >= 256) sy -= 512;
			if( sx > -64 && sx < 256 && sy > -64 && sy < 256){
				int code = source[1];
				int color = source[2];
				int size = color&0x3;
				int num_tiles = 1<<(size*2);
				color = (color>>4)&0xf;

				for( i=0; i<num_tiles; i++ ){
					colmask[color] |= gfx->pen_usage[(code+i)&0x7fff];
				}
			}
		}
		source+=8;
	}

	for( i = 0; i<16; i++ ){
		int bit;
		for( bit = 1; bit<16; bit++ ){
			if( colmask[i] & (1 << bit) ) pen_used[bit] = PALETTE_COLOR_USED;
		}
		pen_used += 16;
	}
}

static void pre_update_background( void ){
	gfxbank = 1;
	videoram1 = (const unsigned short *)&gaiden_videoram3[0x1000];
	videoram2 = (const unsigned short *)gaiden_videoram3;
}

static void pre_update_foreground( void ){
	gfxbank = 2;
	videoram1 = (const unsigned short *)&gaiden_videoram2[0x1000];
	videoram2 = (const unsigned short *)gaiden_videoram2;
}

static void pre_update_text_layer( void ){
	gfxbank = 0;
	videoram1 = (const unsigned short *)&gaiden_videoram[0x0800];
	videoram2 = (const unsigned short *)gaiden_videoram;
}

void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

	pre_update_background(); tilemap_update(background);
	pre_update_foreground(); tilemap_update(foreground);
	pre_update_text_layer(); tilemap_update(text_layer);
	tilemap_mark_palette();
	mark_sprite_colors();

	/* the following is required to make the colored background work */
	for (i = 0;i < Machine->drv->total_colors;i += 16)
		palette_used_colors[i] = PALETTE_COLOR_TRANSPARENT;

	if( palette_recalc() ){
		tilemap_mark_all_pixels_dirty(text_layer);
		tilemap_mark_all_pixels_dirty(foreground);
		tilemap_mark_all_pixels_dirty(background);
	}

	tilemap_render(background);
	tilemap_render(foreground);
	tilemap_render(text_layer);

	tilemap_draw(bitmap,background,0);
	/* sprites will be drawn with TRANSPARENCY_THROUGH and appear behind the background */
	gaiden_drawsprites(bitmap,3);
	gaiden_drawsprites(bitmap,2);
	tilemap_draw(bitmap,foreground,0);
	gaiden_drawsprites(bitmap,1);
	tilemap_draw(bitmap,text_layer,0);
	gaiden_drawsprites(bitmap,0);	/* is this correct? */
}

