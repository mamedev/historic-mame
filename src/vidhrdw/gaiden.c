/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

/* blit contains parameters for drawing a sprite */
struct {
	int x,y,w,h;
	int clip_left, clip_right, clip_top, clip_bottom;

	int flags;
	unsigned short *pal_data;
	int transparent_pen;

	unsigned char *mask_baseaddr;
	int mask_rowsize;

	unsigned char *source_baseaddr;
	int source_rowsize;

	unsigned char *dest_baseaddr;
	int dest_rowsize;
} blit;

#define SPRITE_FLIPX					0x01
#define SPRITE_FLIPY					0x02
#define SPRITE_TRANSPARENCY_THROUGH		0x04

#define DRAWSPRITE \
if( blit.flags&SPRITE_FLIPX ){ \
	source += blit.x + blit.w-1; \
	for( y=y1; y<y2; y++ ){ \
		for( x=x1; x<x2; x++ ){ \
			if( OPAQUE(-x) ) dest[x] = COLOR(-x); \
		} \
		NEXTLINE \
		source += source_offset; \
		dest += blit.dest_rowsize; \
	} \
} \
else { \
	source -= blit.x; \
	for( y=y1; y<y2; y++ ){ \
		for( x=x1; x<x2; x++ ){ \
			if( OPAQUE(x) ) dest[x] = COLOR(x); \
		} \
		NEXTLINE \
		source += source_offset; \
		dest += blit.dest_rowsize; \
	} \
}

/* sprite_draw is a general-purpose drawing utility, similar to draw_gfx */

static void sprite_draw( void ){
	int x1 = blit.x;
	int y1 = blit.y;
	int x2 = blit.w + blit.x;
	int y2 = blit.h + blit.y;

	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ){
		unsigned char *dest = blit.dest_baseaddr + y1*blit.dest_rowsize;
		unsigned char *source = blit.source_baseaddr;
		unsigned short *pal_data = blit.pal_data;
		int transparent_pen = blit.transparent_pen;
		int x,y,source_offset;

		if( blit.flags&SPRITE_FLIPY ){
			source_offset = -blit.source_rowsize;
			source += (y2-1-blit.y)*blit.source_rowsize;
		}
		else {
			source_offset = blit.source_rowsize;
			source += (y1-blit.y)*blit.source_rowsize;
		}

		if( blit.mask_baseaddr ){ /* draw a masked sprite */
			unsigned char *mask = blit.mask_baseaddr + (y1-blit.y)*blit.mask_rowsize-blit.x;
			#define OPAQUE(X) (mask[x]==0 && source[X]!=transparent_pen)
			#define COLOR(X) (pal_data[source[X]])
			#define NEXTLINE mask+=blit.mask_rowsize;
			DRAWSPRITE
			#undef OPAQUE
			#undef COLOR
			#undef NEXTLINE
		}
		else if( blit.flags&SPRITE_TRANSPARENCY_THROUGH ){
			int color = Machine->pens[palette_transparent_color];
			#define OPAQUE(X) (dest[x]==color && source[X]!=transparent_pen)
			#define COLOR(X) (pal_data[source[X]])
			#define NEXTLINE
			DRAWSPRITE
			#undef OPAQUE
			#undef COLOR
			#undef NEXTLINE
		}
		else if( pal_data ){
			#define OPAQUE(X) (source[X]!=transparent_pen)
			#define COLOR(X) (pal_data[source[X]])
			#define NEXTLINE
			DRAWSPRITE
			#undef OPAQUE
			#undef COLOR
			#undef NEXTLINE
		}
		else {
			#define OPAQUE(X) (source[X]!=transparent_pen)
			#define COLOR(X) 0xff
			#define NEXTLINE
			DRAWSPRITE
			#undef OPAQUE
			#undef COLOR
			#undef NEXTLINE
		}
	}
}

/***************************************************************************/

unsigned char *gaiden_videoram;
unsigned char *gaiden_videoram2;
unsigned char *gaiden_videoram3;

void gaiden_vh_stop (void);

static struct tilemap *text_layer,*foreground,*background;

static const UINT16 *videoram1, *videoram2;
static int gfxbank;

static void get_fg_tile_info( int col, int row ){
	int tile_index = row*32+col;
	SET_TILE_INFO(gfxbank,videoram1[tile_index] & 0x7ff,(videoram2[tile_index] & 0xf0) >> 4)
}

static void get_bg_tile_info( int col, int row ){
	int tile_index = row*64+col;
	SET_TILE_INFO(
		gfxbank,
		videoram1[tile_index] & 0xfff,
		(videoram2[tile_index] & 0xf0) >> 4
	)
}

/********************************************************************************/

#define NUMSPRITES 128
#define MAXSPRITESIZE 64
struct sprite_info {
	unsigned char *pen_data;
	unsigned short *pal_data;
	int pen_usage;
	int x, y, w, h;

	/* to be packed */
	int visible;
	int priority;
	int flipx, flipy;

	/*
		The following bit of mechanism is used so that one sprite can force the
		pixels of another sprite to be hidden.
	*/
	int masked;
	/*
		"masked" flags the sprites which are overlaped and need special treatment.
		This way, the performance impact is limited only to the sprites which are
		covered and need masking.
	*/
	unsigned char hidepixel[MAXSPRITESIZE*MAXSPRITESIZE];
	/* this currently eats a lot of memory */
} *sprite_info;

/* sprite format:
 *
 *	word		bit					usage
 * --------+-fedcba9876543210-+----------------
 *    0    | ---------------x | flip x
 *         | --------------x- | flip y
 *         | -------------x-- | enable
 *         | ----------x----- | auto-flicker
 *         | --------xx------ | sprite-tile priority
 *    1    | xxxxxxxxxxxxxxxx | number
 *    2    | --------xxxx---- | palette
 *         | --------------xx | size: 8x8, 16x16, 32x32, 64x64
 *    3    | xxxxxxxxxxxxxxxx | y position
 *    4    | xxxxxxxxxxxxxxxx | x position
 *    5,6,7|                  | unused
 */

/* get_sprite_info is the only routine to inspect spriteram.  It's needed as a
preprocessing step so we can do palette management and sprite layering effects */

static void get_sprite_info( void ){
	static int flicker = 0;
	const struct GfxElement *gfx = Machine->gfx[3];
	const unsigned short *source = (const unsigned short *)spriteram;
	const unsigned short *finish = source+0x400;
	struct sprite_info *sprite = sprite_info;

	flicker = 1-flicker;

	while( source<finish ){
		int attributes = source[0];
		sprite->visible = attributes&0x04;
		if( (attributes&0x20) && flicker ) sprite->visible = 0;

		if( sprite->visible ){
			sprite->priority = (attributes>>6)&3;
			sprite->y = source[3] & 0x1ff;
			sprite->x = source[4] & 0x1ff;
			if( sprite->x >= 256) sprite->x -= 512;
			if( sprite->y >= 256) sprite->y -= 512;

			if( sprite->x <= -64 || sprite->x >= 256 ||
				sprite->y <= -64 || sprite->y >= 256 ){
				sprite->visible = 0; /* offscreen */
			}
			else {
				int number = source[1]&0x7fff;
				int color = source[2];
				int span = 0;

				switch( color&0x3 ){
					case 0: span = 8; break;
					case 1: span = 16; break;//number = (number/4)*4; break;
					case 2: span = 32; break;//number = (number/16)*16; break;
					case 3: span = 64; break;//number = (number/64)*64; break;
				}

				color = (color>>4)&0xf;

				sprite->masked = 0;
				sprite->w = sprite->h = span;
				sprite->flipx = attributes&1;
				sprite->flipy = attributes&2;
				sprite->pal_data = &gfx->colortable[gfx->color_granularity * color];
				sprite->pen_usage = gfx->pen_usage[number/64];

				sprite->pen_data = gfx->gfxdata->line[(number/64)*64];
				if( number&0x01 ) sprite->pen_data += 8;
				if( number&0x02 ) sprite->pen_data += 8*64;
				if( number&0x04 ) sprite->pen_data += 16;
				if( number&0x08 ) sprite->pen_data += 16*64;
				if( number&0x10 ) sprite->pen_data += 32;
				if( number&0x20 ) sprite->pen_data += 32*64;
			}
		}
		sprite++;
		source += 8;
	}
}

static void mark_sprite_colors( void ){
	int i,j;

	unsigned int colmask[16];
	memset( colmask, 0, 16*sizeof(unsigned int) );

	get_sprite_info();

	blit.clip_left = 0;
	blit.clip_top = 0;
	blit.dest_rowsize = 64;
	blit.transparent_pen = 0;
	blit.mask_baseaddr = 0;
	blit.pal_data = 0;
	blit.source_rowsize = 64;

	/*
		The following loop ensures that even though we are drawing all priority 3
		sprites before drawing the priority 2 sprites, and priority 2 sprites before the
		priority 1 sprites, that the sprite order as a whole still dictates
		sprite-to-sprite priority when sprite pixels overlap and aren't obscured by a
		background.  Checks are done to avoid special handling for the cases where
		masking isn't needed.
	*/
	for( i=0; i<NUMSPRITES; i++ ){
		struct sprite_info *sprite = &sprite_info[i];
		if( sprite->visible ){
			int priority = sprite->priority;

			colmask[(sprite->pal_data - Machine->colortable)/16]
				|= sprite->pen_usage;

			if( priority<3 ){
				/*
					priority 3 sprites are always drawn first, so we
					don't need to do anything special to cause them to
					be obscured by other sprites
				*/
				blit.clip_right = sprite->w;
				blit.clip_bottom = sprite->h;
				blit.dest_baseaddr = sprite->hidepixel;

				for( j=i+1; j<NUMSPRITES; j++ ){
					struct sprite_info *front = &sprite_info[j];
					if( front->visible && front->priority>priority ){
						blit.x = front->x - sprite->x;
						blit.y = front->y - sprite->y;
						blit.w = front->w;
						blit.h = front->h;
						if( blit.x<blit.clip_right &&
							blit.y<blit.clip_bottom &&
							blit.x+blit.w>blit.clip_left &&
							blit.y+blit.h>blit.clip_top ){

							blit.flags = 0;
							if( front->flipx ) blit.flags |= SPRITE_FLIPX;
							if( front->flipy ) blit.flags |= SPRITE_FLIPY;

							if( !sprite->masked ){
								memset( sprite->hidepixel, 0, 64*64 );
								sprite->masked = 1;
							}
							blit.source_baseaddr = front->pen_data;
							sprite_draw();
						}
					}
				}
			}
		}
	}

	{
		unsigned char *pen_used = &palette_used_colors[Machine->drv->gfxdecodeinfo[3].color_codes_start];
		for( i = 0; i<16; i++ ){
			int bit;
			for( bit = 1; bit<16; bit++ ){
				if( colmask[i] & (1 << bit) ) pen_used[bit] = PALETTE_COLOR_USED;
			}
			pen_used += 16;
		}
	}
}

static void draw_sprites( struct osd_bitmap *bitmap, int priority ){
	int base_flags = (priority==3)?SPRITE_TRANSPARENCY_THROUGH:0;
	/* priority 3 sprites are drawn behind the background layer */

	int i;

	{ /* set constants */
		const struct rectangle *clip = &Machine->drv->visible_area;
		blit.clip_left = clip->min_x;
		blit.clip_right = clip->max_x+1;
		blit.clip_top = clip->min_y;
		blit.clip_bottom = clip->max_y+1;
		blit.dest_baseaddr = bitmap->line[0];
		blit.dest_rowsize = bitmap->line[1]-bitmap->line[0];
		blit.transparent_pen = 0;
		blit.mask_rowsize = 64;
		blit.source_rowsize = 64;
	}

	for( i=0; i<NUMSPRITES; i++ ){
		struct sprite_info *sprite = &sprite_info[i];
		if( sprite->visible && (sprite->priority==priority) ){
			blit.pal_data = sprite->pal_data;
			blit.source_baseaddr = sprite->pen_data;
			blit.mask_baseaddr = sprite->masked?sprite->hidepixel:0;
			blit.x = sprite->x;
			blit.y = sprite->y;
			blit.w = sprite->w;
			blit.h = sprite->h;
			blit.flags = base_flags;
			if( sprite->flipx ) blit.flags |= SPRITE_FLIPX;
			if( sprite->flipy ) blit.flags |= SPRITE_FLIPY;
			sprite_draw();
		}
	}
}

/********************************************************************************/

int gaiden_vh_start(void){
	sprite_info = (struct sprite_info *)malloc( sizeof(struct sprite_info)*NUMSPRITES );
	if( !sprite_info ) return 1; /* error */

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

	gaiden_vh_stop();

	return 1;
}

void gaiden_vh_stop(void){
	tilemap_dispose(background);
	tilemap_dispose(foreground);
	tilemap_dispose(text_layer);

	free( sprite_info );
}

/* scroll write handlers */

void gaiden_txscrollx_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrollx( text_layer,0, -oldword );
}

void gaiden_txscrolly_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrolly( text_layer,0, -oldword );
}

void gaiden_fgscrollx_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrollx( foreground,0, -oldword );
}

void gaiden_fgscrolly_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrolly( foreground,0, -oldword );
}

void gaiden_bgscrollx_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrollx( background,0, -oldword );
}

void gaiden_bgscrolly_w( int offset,int data ){
	static int oldword;
	oldword = COMBINE_WORD(oldword,data);
	tilemap_set_scrolly( background,0, -oldword );
}

void gaiden_videoram3_w( int offset,int data ){
	int oldword = READ_WORD(&gaiden_videoram3[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		int tile_index = (offset/2)&0x7ff;
		WRITE_WORD(&gaiden_videoram3[offset],newword);
		tilemap_mark_tile_dirty( background,tile_index%64,tile_index/64 );
	}
}

int gaiden_videoram3_r(int offset){
   return READ_WORD (&gaiden_videoram3[offset]);
}

void gaiden_videoram2_w(int offset,int data){
	int oldword = READ_WORD(&gaiden_videoram2[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		int tile_index = (offset/2)&0x7ff;
		WRITE_WORD(&gaiden_videoram2[offset],newword);
		tilemap_mark_tile_dirty(foreground,tile_index%64,tile_index/64 );
	}
}

int gaiden_videoram2_r(int offset){
   return READ_WORD (&gaiden_videoram2[offset]);
}

void gaiden_videoram_w(int offset,int data){
	int oldword = READ_WORD(&gaiden_videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword){
		int tile_index = (offset/2)&0x3ff;
		WRITE_WORD(&gaiden_videoram[offset],newword);
		tilemap_mark_tile_dirty(text_layer,tile_index%32,tile_index/32 );
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
	pre_update_background(); tilemap_update(background);
	pre_update_foreground(); tilemap_update(foreground);
	pre_update_text_layer(); tilemap_update(text_layer);

	palette_init_used_colors();
	mark_sprite_colors();

	{
		/* the following is required to make the colored background work */
		int i;
		for (i = 0;i < Machine->drv->total_colors;i += 16)
			palette_used_colors[i] = PALETTE_COLOR_TRANSPARENT;
	}

	if( palette_recalc() ){
		tilemap_mark_all_pixels_dirty(text_layer);
		tilemap_mark_all_pixels_dirty(foreground);
		tilemap_mark_all_pixels_dirty(background);
	}

	tilemap_render(background);
	tilemap_render(foreground);
	tilemap_render(text_layer);

	tilemap_draw(bitmap,background,0);
	draw_sprites(bitmap,3); /* behind bg */
	draw_sprites(bitmap,2); /* behind fg */
	tilemap_draw(bitmap,foreground,0);
	draw_sprites(bitmap,1); /* in front of fg */
	tilemap_draw(bitmap,text_layer,0);
	draw_sprites(bitmap,0); /* highest priority */
}
