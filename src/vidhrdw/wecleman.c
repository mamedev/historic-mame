#include "driver.h"

#define SPRITE_FLIPX					0x01
#define SPRITE_FLIPY					0x02
#define SPRITE_VISIBLE					0x08

struct sprite {
	int priority, flags;

	const UINT8 *pen_data;	/* points to top left corner of tile data */
	int line_offset;

	const pen_t *pal_data;
	UINT32 pen_usage;

	int x_offset, y_offset;
	int tile_width, tile_height;
	int total_width, total_height;	/* in screen coordinates */
	int x, y;

	/* private */ const struct sprite *next;
	/* private */ long mask_offset;
};

struct sprite_list {
	int num_sprites;
	int max_priority;
	int transparent_pen;

	struct sprite *sprite;
	struct sprite_list *next; /* resource tracking */
};



#define SWAP(X,Y) { int temp = X; X = Y; Y = temp; }

/*
	The Sprite Manager provides a service that would be difficult or impossible using drawgfx.
	It allows sprite-to-sprite priority to be orthogonal to sprite-to-tilemap priority.

	The sprite manager also abstract a nice chunk of generally useful functionality.

	Drivers making use of Sprite Manager will NOT necessarily be any faster than traditional
	drivers using drawgfx.

	Currently supported features include:
	- sprite layering order, FRONT_TO_BACK / BACK_TO_FRONT (can be easily switched from a driver)
	- priority masking (needed for Gaiden, Blood Brothers, and surely many others)
	  this allows sprite-to-sprite priority to be orthogonal to sprite-to-layer priority.
	- resource tracking (sprite_init and sprite_close must be called by mame.c)
	- flickering sprites
	- offscreen sprite skipping
	- palette usage tracking
	- support for graphics that aren't pre-rotated (i.e. System16)

There are three sprite types that a sprite_list may use.  With all three sprite types, sprite->x and sprite->y
are screenwise coordinates for the topleft pixel of a sprite, and sprite->total_width, sprite->total_width is the
sprite size in screen coordinates - the "footprint" of the sprite on the screen.  sprite->line_offset indicates
offset from one logical row of sprite pen data to the next.

		line_offset is pen skip to next line; tile_width and tile_height are logical sprite dimensions
		The sprite will be stretched to fit total_width, total_height, shrinking or magnifying as needed

	TBA:
	- GfxElement-oriented field-setting macros
	- cocktail support
	- "special" pen (hides pixels of previously drawn sprites) - for MCR games, Mr. Do's Castle, etc.
	- end-of-line marking pen (needed for Altered Beast, ESWAT)
*/

static int orientation, screen_width, screen_height;
static int screen_clip_left, screen_clip_top, screen_clip_right, screen_clip_bottom;
unsigned char *screen_baseaddr;
int screen_line_offset;

static struct sprite_list *first_sprite_list = NULL; /* used for resource tracking */

static UINT32 *shade_table;

static void sprite_order_setup( struct sprite_list *sprite_list, int *first, int *last, int *delta ){
	*delta = 1;
	*first = 0;
	*last = sprite_list->num_sprites-1;
}

/*********************************************************************

	The mask buffer is a dynamically allocated resource
	it is recycled each frame.  Using this technique reduced the runttime
	memory requirements of the Gaiden from 512k (worst case) to approx 6K.

	Sprites use offsets instead of pointers directly to the mask data, since it
	is potentially reallocated.

*********************************************************************/
static unsigned char *mask_buffer = NULL;
static int mask_buffer_size = 0; /* actual size of allocated buffer */
static int mask_buffer_used = 0;

static void mask_buffer_reset( void ){
	mask_buffer_used = 0;
}
static void mask_buffer_dispose( void ){
	free( mask_buffer );
	mask_buffer = NULL;
	mask_buffer_size = 0;
}
static long mask_buffer_alloc( long size ){
	long result = mask_buffer_used;
	long req_size = mask_buffer_used + size;
	if( req_size>mask_buffer_size ){
		mask_buffer = realloc( mask_buffer, req_size );
		mask_buffer_size = req_size;
		logerror("increased sprite mask buffer size to %d bytes.\n", mask_buffer_size );
		if( !mask_buffer ) logerror("Error! insufficient memory for mask_buffer_alloc\n" );
	}
	mask_buffer_used = req_size;
	memset( &mask_buffer[result], 0x00, size ); /* clear it */
	return result;
}

#define BLIT \
if( sprite->flags&SPRITE_FLIPX ){ \
	source += screenx + flipx_adjust; \
	for( y=y1; y<y2; y++ ){ \
		for( x=x1; x<x2; x++ ){ \
			if( OPAQUE(-x) ) dest[x] = COLOR(-x); \
		} \
		source += source_dy; dest += blit.line_offset; \
		NEXTLINE \
	} \
} \
else { \
	source -= screenx; \
	for( y=y1; y<y2; y++ ){ \
		for( x=x1; x<x2; x++ ){ \
			if( OPAQUE(x) ) dest[x] = COLOR(x); \
			\
		} \
		source += source_dy; dest += blit.line_offset; \
		NEXTLINE \
	} \
}

static struct {
	int transparent_pen;
	int clip_left, clip_right, clip_top, clip_bottom;
	unsigned char *baseaddr;
	int line_offset;
	int write_to_mask;
	int origin_x, origin_y;
} blit;

static void do_blit_zoom( const struct sprite *sprite ){
	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = (x1-blit.clip_right)*sprite->tile_width;
			x1 = blit.clip_right;
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 = (blit.clip_left-x1)*sprite->tile_width;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = (y1-blit.clip_bottom)*sprite->tile_height;
			y1 = blit.clip_bottom;
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 = (blit.clip_top-y1)*sprite->tile_height;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const pen_t *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy;
		unsigned char *dest = blit.baseaddr + blit.line_offset*y1;
		int ycount = ycount0;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			int xcount = xcount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				unsigned char *dest1;

				ycount = ycount0;
				while( xcount>=sprite->total_width ){
					xcount -= sprite->total_width;
					pen_data+=sprite->line_offset;
				}
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if( pen==0xff ) goto skip1; /* marker for right side of sprite; needed for AltBeast, ESwat */
/*					if( pen==10 ) *dest1 = shade_table[*dest1];
					else */if( pen ) *dest1 = pal_data[pen];
					ycount+= sprite->tile_height;
					dest1 += pitch;
				}
skip1:
				xcount += sprite->tile_width;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount = xcount0;
				const unsigned char *source;
				while( ycount>=sprite->total_height ){
					ycount -= sprite->total_height;
					pen_data += sprite->line_offset;
				}
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if( pen==0xff ) goto skip; /* marker for right side of sprite; needed for AltBeast, ESwat */
/*					if( pen==10 ) dest[x] = shade_table[dest[x]];
					else */if( pen ) dest[x] = pal_data[pen];
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
			}
		}
	}
}


static void do_blit_zoom16( const struct sprite *sprite ){
	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = (x1-blit.clip_right)*sprite->tile_width;
			x1 = blit.clip_right;
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 = (blit.clip_left-x1)*sprite->tile_width;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = (y1-blit.clip_bottom)*sprite->tile_height;
			y1 = blit.clip_bottom;
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 = (blit.clip_top-y1)*sprite->tile_height;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const pen_t *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy/2;
		UINT16 *dest = (UINT16 *)(blit.baseaddr + blit.line_offset*y1);
		int ycount = ycount0;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			int xcount = xcount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT16 *dest1;

				ycount = ycount0;
				while( xcount>=sprite->total_width ){
					xcount -= sprite->total_width;
					pen_data+=sprite->line_offset;
				}
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if( pen==0xff ) goto skip1; /* marker for right side of sprite; needed for AltBeast, ESwat */
/*					if( pen==10 ) *dest1 = shade_table[*dest1];
					else */if( pen ) *dest1 = pal_data[pen];
					ycount+= sprite->tile_height;
					dest1 += pitch;
				}
skip1:
				xcount += sprite->tile_width;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount = xcount0;
				const unsigned char *source;
				while( ycount>=sprite->total_height ){
					ycount -= sprite->total_height;
					pen_data += sprite->line_offset;
				}
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if( pen==0xff ) goto skip; /* marker for right side of sprite; needed for AltBeast, ESwat */
/*					if( pen==10 ) dest[x] = shade_table[dest[x]];
					else */if( pen ) dest[x] = pal_data[pen];
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
			}
		}
	}
}

/*********************************************************************/

static void sprite_init( void ){
	const struct rectangle *clip = &Machine->visible_area;
	int left = clip->min_x;
	int top = clip->min_y;
	int right = clip->max_x+1;
	int bottom = clip->max_y+1;

	struct mame_bitmap *bitmap = Machine->scrbitmap;
	screen_baseaddr = bitmap->line[0];
	screen_line_offset = ((UINT8 *)bitmap->line[1])-((UINT8 *)bitmap->line[0]);

	orientation = Machine->orientation;
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;

	if( orientation & ORIENTATION_SWAP_XY ){
		SWAP(left,top)
		SWAP(right,bottom)
	}
	if( orientation & ORIENTATION_FLIP_X ){
		SWAP(left,right)
		left = screen_width-left;
		right = screen_width-right;
	}
	if( orientation & ORIENTATION_FLIP_Y ){
		SWAP(top,bottom)
		top = screen_height-top;
		bottom = screen_height-bottom;
	}

	screen_clip_left = left;
	screen_clip_right = right;
	screen_clip_top = top;
	screen_clip_bottom = bottom;
}

static void sprite_close( void ){
	struct sprite_list *sprite_list = first_sprite_list;
	mask_buffer_dispose();

	while( sprite_list ){
		struct sprite_list *next = sprite_list->next;
		free( sprite_list->sprite );
		free( sprite_list );
		sprite_list = next;
	}
	first_sprite_list = NULL;
}

static struct sprite_list *sprite_list_create( int num_sprites ){
	struct sprite *sprite = calloc( num_sprites, sizeof(struct sprite) );
	struct sprite_list *sprite_list = calloc( 1, sizeof(struct sprite_list) );

	sprite_list->num_sprites = num_sprites;
	sprite_list->sprite = sprite;

	/* resource tracking */
	sprite_list->next = first_sprite_list;
	first_sprite_list = sprite_list;

	return sprite_list; /* warning: no error checking! */
}

static void sprite_update_helper( struct sprite_list *sprite_list ){
	struct sprite *sprite_table = sprite_list->sprite;

	/* initialize constants */
	blit.transparent_pen = sprite_list->transparent_pen;
	blit.write_to_mask = 1;
	blit.clip_left = 0;
	blit.clip_top = 0;

	/* make a pass to adjust for screen orientation */
	if( orientation & ORIENTATION_SWAP_XY ){
		struct sprite *sprite = sprite_table;
		const struct sprite *finish = &sprite[sprite_list->num_sprites];
		while( sprite<finish ){
			SWAP(sprite->x, sprite->y)
			SWAP(sprite->total_height,sprite->total_width)
			SWAP(sprite->tile_width,sprite->tile_height)
			SWAP(sprite->x_offset,sprite->y_offset)

			/* we must also swap the flipx and flipy bits (if they aren't identical) */
			if( sprite->flags&SPRITE_FLIPX ){
				if( !(sprite->flags&SPRITE_FLIPY) ){
					sprite->flags = (sprite->flags&~SPRITE_FLIPX)|SPRITE_FLIPY;
				}
			}
			else {
				if( sprite->flags&SPRITE_FLIPY ){
					sprite->flags = (sprite->flags&~SPRITE_FLIPY)|SPRITE_FLIPX;
				}
			}
			sprite++;
		}
	}
	if( orientation & ORIENTATION_FLIP_X ){
		struct sprite *sprite = sprite_table;
		const struct sprite *finish = &sprite[sprite_list->num_sprites];
		int toggle_bit = SPRITE_FLIPX;
		while( sprite<finish ){
			sprite->x = screen_width - (sprite->x+sprite->total_width);
			sprite->flags ^= toggle_bit;

			/* extra processing for packed sprites */
			sprite->x_offset = sprite->tile_width - (sprite->x_offset+sprite->total_width);
			sprite++;
		}
	}
	if( orientation & ORIENTATION_FLIP_Y ){
		struct sprite *sprite = sprite_table;
		const struct sprite *finish = &sprite[sprite_list->num_sprites];
		int toggle_bit = SPRITE_FLIPY;
		while( sprite<finish ){
			sprite->y = screen_height - (sprite->y+sprite->total_height);
			sprite->flags ^= toggle_bit;

			/* extra processing for packed sprites */
			sprite->y_offset = sprite->tile_height - (sprite->y_offset+sprite->total_height);
			sprite++;
		}
	}
	{ /* visibility check */
		struct sprite *sprite = sprite_table;
		const struct sprite *finish = &sprite[sprite_list->num_sprites];
		while( sprite<finish ){
			if(
				sprite->total_width<=0 || sprite->total_height<=0 ||
				sprite->x + sprite->total_width<=0 || sprite->x>=screen_width ||
				sprite->y + sprite->total_height<=0 || sprite->y>=screen_height ){
				sprite->flags &= (~SPRITE_VISIBLE);
			}
			sprite++;
		}
	}
}

static void sprite_update( void ){
	struct sprite_list *sprite_list = first_sprite_list;
	mask_buffer_reset();
	while( sprite_list ){
		sprite_update_helper( sprite_list );
		sprite_list = sprite_list->next;
	}
}

static void sprite_draw( struct sprite_list *sprite_list, int priority ){
	const struct sprite *sprite_table = sprite_list->sprite;


	{ /* set constants */
		blit.origin_x = 0;
		blit.origin_y = 0;

		blit.baseaddr = screen_baseaddr;
		blit.line_offset = screen_line_offset;
		blit.transparent_pen = sprite_list->transparent_pen;
		blit.write_to_mask = 0;

		blit.clip_left = screen_clip_left;
		blit.clip_top = screen_clip_top;
		blit.clip_right = screen_clip_right;
		blit.clip_bottom = screen_clip_bottom;
	}

	{
		int i, dir, last;
		void (*do_blit)( const struct sprite * );

		if (Machine->scrbitmap->depth == 16) /* 16 bit */
			do_blit = do_blit_zoom16;
		else
			do_blit = do_blit_zoom;

		sprite_order_setup( sprite_list, &i, &last, &dir );
		for(;;){
			const struct sprite *sprite = &sprite_table[i];
			if( (sprite->flags&SPRITE_VISIBLE) && (sprite->priority==priority) ) do_blit( sprite );
			if( i==last ) break;
			i+=dir;
		}
	}
}


static void sprite_set_shade_table(UINT32 *table)
{
	shade_table=table;
}


/***************************************************************************
						WEC Le Mans 24  &   Hot Chase

					      (C)   1986 & 1988 Konami

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows background layer
		W		shows foreground layer
		E		shows text layer
		R		shows road layer
		A		shows sprites
		B		toggles the custom gfx browser on/off

		Keys can be used togheter!

							[WEC Le Mans 24]

[ 2 Scrolling Layers ]
	[Background]
	[Foreground]
		Tile Size:				8x8

		Tile Format:
										Colour?
				---- ba98 7654 3210		Code

		Layer Size:				4 Pages -	Page0 Page1
											Page2 Page3
								each page is 512 x 256 (64 x 32 tiles)

		Page Selection Reg.:	108efe	[Bg]
								108efc	[Fg]
								4 pages to choose from

		Scrolling Columns:		1
		Scrolling Columns Reg.:	108f26	[Bg]
								108f24	[Fg]

		Scrolling Rows:			224 / 8 (Screen-wise scrolling)
		Scrolling Rows Reg.:	108f82/4/6..	[Bg]
								108f80/2/4..	[Fg]

[ 1 Text Layer ]
		Tile Size:				8x8

		Tile Format:
				fedc ba9- ---- ----		Colour: ba9 fedc
				---- ba98 7654 3210		Code

		Layer Size:				1 Page: 512 x 256 (64 x 32 tiles)

		Scrolling:				-

[ 1 Road Layer ]

[ 256 Sprites ]
	Zooming Sprites, see below



								[Hot Chase]

[ 3 Zooming Layers ]
	[Background]
	[Foreground (text)]
	[Road]

[ 256 Sprites ]
	Zooming Sprites, see below


**************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

/* Variables only used here: */

static struct tilemap *bg_tilemap, *fg_tilemap, *txt_tilemap;
static struct sprite_list *sprite_list;


/* Variables that driver has acces to: */

data16_t *wecleman_pageram, *wecleman_txtram, *wecleman_roadram, *wecleman_unknown;
size_t wecleman_roadram_size;
int wecleman_bgpage[4], wecleman_fgpage[4], *wecleman_gfx_bank;



/* Variables defined in driver: */

extern int wecleman_selected_ip, wecleman_irqctrl;



/***************************************************************************
							Common routines
***************************************************************************/

/* Useful defines - for debug */
#define KEY(_k_,_action_) \
	if (keyboard_pressed_memory(KEYCODE_##_k_))	{ _action_ }
#define KEY_SHIFT(_k_,_action_) \
	if ( (keyboard_pressed(KEYCODE_LSHIFT)||keyboard_pressed(KEYCODE_RSHIFT)) && \
	      keyboard_pressed_memory(KEYCODE_##_k_) )	{ _action_ }
#define KEY_FAST(_k_,_action_) \
	if (keyboard_pressed(KEYCODE_##_k_))	{ _action_ }


/* WEC Le Mans 24 and Hot Chase share the same sprite hardware */
#define NUM_SPRITES 256


WRITE16_HANDLER( paletteram16_SBGRBBBBGGGGRRRR_word_w )
{
	/*	byte 0    	byte 1		*/
	/*	SBGR BBBB 	GGGG RRRR	*/
	/*	S000 4321 	4321 4321	*/
	/*  S = Shade				*/

	int newword = COMBINE_DATA(&paletteram16[offset]);

	int r = ((newword << 1) & 0x1E ) | ((newword >> 12) & 0x01);
	int g = ((newword >> 3) & 0x1E ) | ((newword >> 13) & 0x01);
	int b = ((newword >> 7) & 0x1E ) | ((newword >> 14) & 0x01);

	/* This effect can be turned on/off actually ... */
	if (newword & 0x8000)	{ r /= 2;	 g /= 2;	 b /= 2; }

	palette_set_color( offset,	 (r * 0xFF) / 0x1F,
									 (g * 0xFF) / 0x1F,
									 (b * 0xFF) / 0x1F	 );
}




/***************************************************************************

					  Callbacks for the TileMap code

***************************************************************************/



/***************************************************************************
								WEC Le Mans 24
***************************************************************************/

#define PAGE_NX  		(0x40)
#define PAGE_NY  		(0x20)
#define PAGE_GFX		(0)
#define TILEMAP_DIMY	(PAGE_NY * 2 * 8)

/*------------------------------------------------------------------------
				[ Frontmost (text) layer + video registers ]
------------------------------------------------------------------------*/



void wecleman_get_txt_tile_info( int tile_index )
{
	data16_t code = wecleman_txtram[tile_index];
	SET_TILE_INFO(
			PAGE_GFX,
			code & 0xfff,
			(code >> 12) + ((code >> 5) & 0x70),
			0)
}


WRITE16_HANDLER( wecleman_txtram_w )
{
	data16_t old_data = wecleman_txtram[offset];
	data16_t new_data = COMBINE_DATA(&wecleman_txtram[offset]);

	if ( old_data != new_data )
	{
		if (offset >= 0xE00/2 )	/* Video registers */
		{
			/* pages selector for the background */
			if (offset == 0xEFE/2)
			{
				wecleman_bgpage[0] = (new_data >> 0x4) & 3;
				wecleman_bgpage[1] = (new_data >> 0x0) & 3;
				wecleman_bgpage[2] = (new_data >> 0xc) & 3;
				wecleman_bgpage[3] = (new_data >> 0x8) & 3;
				tilemap_mark_all_tiles_dirty(bg_tilemap);
			}

			/* pages selector for the foreground */
			if (offset == 0xEFC/2)
			{
				wecleman_fgpage[0] = (new_data >> 0x4) & 3;
				wecleman_fgpage[1] = (new_data >> 0x0) & 3;
				wecleman_fgpage[2] = (new_data >> 0xc) & 3;
				wecleman_fgpage[3] = (new_data >> 0x8) & 3;
				tilemap_mark_all_tiles_dirty(fg_tilemap);
			}

			/* Parallactic horizontal scroll registers follow */

		}
		else
			tilemap_mark_tile_dirty(txt_tilemap, offset);
	}
}




/*------------------------------------------------------------------------
							[ Background ]
------------------------------------------------------------------------*/

void wecleman_get_bg_tile_info( int tile_index )
{
	int page = wecleman_bgpage[(tile_index%(PAGE_NX*2))/PAGE_NX+2*(tile_index/(PAGE_NX*2*PAGE_NY))];
	int code = wecleman_pageram[(tile_index%PAGE_NX) + PAGE_NX*((tile_index/(PAGE_NX*2))%PAGE_NY) + page*PAGE_NX*PAGE_NY];
	SET_TILE_INFO(
			PAGE_GFX,
			code & 0xfff,
			(code >> 12) + ((code >> 5) & 0x70),
			0)
}



/*------------------------------------------------------------------------
							[ Foreground ]
------------------------------------------------------------------------*/

void wecleman_get_fg_tile_info( int tile_index )
{
	int page = wecleman_fgpage[(tile_index%(PAGE_NX*2))/PAGE_NX+2*(tile_index/(PAGE_NX*2*PAGE_NY))];
	int code = wecleman_pageram[(tile_index%PAGE_NX) + PAGE_NX*((tile_index/(PAGE_NX*2))%PAGE_NY) + page*PAGE_NX*PAGE_NY];
	SET_TILE_INFO(
			PAGE_GFX,
			code & 0xfff,
			(code >> 12) + ((code >> 5) & 0x70),
			0)
}






/*------------------------------------------------------------------------
					[ Pages (Background & Foreground) ]
------------------------------------------------------------------------*/



/* Pages that compose both the background and the foreground */

WRITE16_HANDLER( wecleman_pageram_w )
{
	data16_t old_data = wecleman_pageram[offset];
	data16_t new_data = COMBINE_DATA(&wecleman_pageram[offset]);

	if ( old_data != new_data )
	{
		int page,col,row;

		page	=	( offset ) / (PAGE_NX * PAGE_NY);

		col		=	( offset )           % PAGE_NX;
		row		=	( offset / PAGE_NX ) % PAGE_NY;

		/* background */
		if (wecleman_bgpage[0] == page)	tilemap_mark_tile_dirty(bg_tilemap, (col+PAGE_NX*0) + (row+PAGE_NY*0)*PAGE_NX*2 );
		if (wecleman_bgpage[1] == page)	tilemap_mark_tile_dirty(bg_tilemap, (col+PAGE_NX*1) + (row+PAGE_NY*0)*PAGE_NX*2 );
		if (wecleman_bgpage[2] == page)	tilemap_mark_tile_dirty(bg_tilemap, (col+PAGE_NX*0) + (row+PAGE_NY*1)*PAGE_NX*2 );
		if (wecleman_bgpage[3] == page)	tilemap_mark_tile_dirty(bg_tilemap, (col+PAGE_NX*1) + (row+PAGE_NY*1)*PAGE_NX*2 );

		/* foreground */
		if (wecleman_fgpage[0] == page)	tilemap_mark_tile_dirty(fg_tilemap, (col+PAGE_NX*0) + (row+PAGE_NY*0)*PAGE_NX*2 );
		if (wecleman_fgpage[1] == page)	tilemap_mark_tile_dirty(fg_tilemap, (col+PAGE_NX*1) + (row+PAGE_NY*0)*PAGE_NX*2 );
		if (wecleman_fgpage[2] == page)	tilemap_mark_tile_dirty(fg_tilemap, (col+PAGE_NX*0) + (row+PAGE_NY*1)*PAGE_NX*2 );
		if (wecleman_fgpage[3] == page)	tilemap_mark_tile_dirty(fg_tilemap, (col+PAGE_NX*1) + (row+PAGE_NY*1)*PAGE_NX*2 );
	}
}



/*------------------------------------------------------------------------
						[ Video Hardware Start ]
------------------------------------------------------------------------*/

int wecleman_vh_start(void)
{

/*
 Sprite banking - each bank is 0x20000 bytes (we support 0x40 bank codes)
 This game has ROMs for 16 banks
*/

	static int bank[0x40] = {	0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
								8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
								0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,
								8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15	};

	wecleman_gfx_bank = bank;


	sprite_init();

	bg_tilemap = tilemap_create(wecleman_get_bg_tile_info,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,	/* We draw part of the road below */
								8,8,
								PAGE_NX * 2, PAGE_NY * 2 );

	fg_tilemap = tilemap_create(wecleman_get_fg_tile_info,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								PAGE_NX * 2, PAGE_NY * 2);

	txt_tilemap = tilemap_create(wecleman_get_txt_tile_info,
								 tilemap_scan_rows,
								 TILEMAP_TRANSPARENT,
								 8,8,
								 PAGE_NX * 1, PAGE_NY * 1);


	sprite_list = sprite_list_create( NUM_SPRITES );


	if (bg_tilemap && fg_tilemap && txt_tilemap && sprite_list)
	{
		tilemap_set_scroll_rows(bg_tilemap, TILEMAP_DIMY);	/* Screen-wise scrolling */
		tilemap_set_scroll_cols(bg_tilemap, 1);
		tilemap_set_transparent_pen(bg_tilemap,0);

		tilemap_set_scroll_rows(fg_tilemap, TILEMAP_DIMY);	/* Screen-wise scrolling */
		tilemap_set_scroll_cols(fg_tilemap, 1);
		tilemap_set_transparent_pen(fg_tilemap,0);

		tilemap_set_scroll_rows(txt_tilemap, 1);
		tilemap_set_scroll_cols(txt_tilemap, 1);
		tilemap_set_transparent_pen(txt_tilemap,0);
		tilemap_set_scrollx(txt_tilemap,0, 512-320-16 );	/* fixed scrolling? */
		tilemap_set_scrolly(txt_tilemap,0, 0 );

		sprite_list->max_priority = 0;

		return 0;
	}
	else return 1;
}

void wecleman_vh_stop(void)
{
	sprite_close();
}













/***************************************************************************
								Hot Chase
***************************************************************************/


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

#define ZOOMROM0_MEM_REGION REGION_GFX2
#define ZOOMROM1_MEM_REGION REGION_GFX3

static void zoom_callback_0(int *code,int *color)
{
	*code |= (*color & 0x03) << 8;
	*color = (*color & 0xfc) >> 2;
}

static void zoom_callback_1(int *code,int *color)
{
	*code |= (*color & 0x01) << 8;
	*color = ((*color & 0x3f) << 1) | ((*code & 0x80) >> 7);
}



/*------------------------------------------------------------------------
						[ Video Hardware Start ]
------------------------------------------------------------------------*/

int hotchase_vh_start(void)
{
/*
 Sprite banking - each bank is 0x20000 bytes (we support 0x40 bank codes)
 This game has ROMs for 0x30 banks
*/
	static int bank[0x40] = {	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
								16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
								32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
								0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15	};
	wecleman_gfx_bank = bank;


	sprite_init();

	if (K051316_vh_start_0(ZOOMROM0_MEM_REGION,4,TILEMAP_TRANSPARENT,0,zoom_callback_0))
		return 1;

	if (K051316_vh_start_1(ZOOMROM1_MEM_REGION,4,TILEMAP_TRANSPARENT,0,zoom_callback_1))
	{
		K051316_vh_stop_0();
		return 1;
	}

	sprite_list = sprite_list_create( NUM_SPRITES );

	if (sprite_list)
	{
		K051316_wraparound_enable(0,1);
//		K051316_wraparound_enable(1,1);
		K051316_set_offset(0, -0xB0/2, -16);
		K051316_set_offset(1, -0xB0/2, -16);

		sprite_list->max_priority = 0;

		return 0;
	}
	else return 1;
}

void hotchase_vh_stop(void)
{
	K051316_vh_stop_0();
	K051316_vh_stop_1();

	sprite_close();
}







/***************************************************************************

								Road Drawing

***************************************************************************/


/***************************************************************************
								WEC Le Mans 24
***************************************************************************/

#define ROAD_COLOR(x)	(0x00 + ((x)&0xff))


/*

	This layer is composed of horizontal lines gfx elements
	There are 256 lines in ROM, each is 512 pixels wide

	Offset:		Elements:	Data:

	0000-01ff	100 Words	Code

		fedcba98--------	Priority?
		--------76543210	Line Number

	0200-03ff	100 Words	Horizontal Scroll
	0400-05ff	100 Words	Color
	0600-07ff	100 Words	??

	We draw each line using a bunch of 64x1 tiles

*/

void wecleman_draw_road(struct mame_bitmap *bitmap,int priority)
{
	struct rectangle rect = Machine->visible_area;
	int curr_code, sx,sy;

/* Referred to what's in the ROMs */
#define XSIZE 512
#define YSIZE 256

	/* Let's draw from the top to the bottom of the visible screen */
	for (sy = rect.min_y ; sy <= rect.max_y ; sy ++)
	{
		int code		=	wecleman_roadram[ YSIZE*0+sy ];
		int scrollx 	=	wecleman_roadram[ YSIZE*1+sy ] + 24;	// fudge factor :)
		int attr		=	wecleman_roadram[ YSIZE*2+sy ];

		/* high byte is a priority information? */
		if ((code>>8) != priority)	continue;

		/* line number converted to tile number (each tile is 64x1) */
		code		=	(code % YSIZE) * (XSIZE/64);

		/* Scroll value applies to a "picture" twice as wide as the gfx
		   in ROM: left half is color 15, right half is the gfx */
		scrollx %= XSIZE * 2;

		if (scrollx >= XSIZE)	{curr_code = code+(scrollx-XSIZE)/64;	code = 0;}
		else					{curr_code = 0   + scrollx/64;}

		for (sx = -(scrollx % 64) ; sx <= rect.max_x ; sx += 64)
		{
			drawgfx(bitmap,Machine->gfx[1],
				curr_code++,
				ROAD_COLOR(attr),
				0,0,
				sx,sy,
				&rect,
				TRANSPARENCY_NONE,0);

			if ( (curr_code % (XSIZE/64)) == 0)	curr_code = code;
		}
	}

#undef XSIZE
#undef YSIZE
}






/***************************************************************************
							Hot Chase
***************************************************************************/




/*
	This layer is composed of horizontal lines gfx elements
	There are 512 lines in ROM, each is 512 pixels wide

	Offset:		Elements:	Data:

	0000-03ff	00-FF		Code (4 bytes)

	Code:

		00.w
				fedc ba98 ---- ----		Unused?
				---- ---- 7654 ----		color
				---- ---- ---- 3210		scroll x

		02.w	fedc ba-- ---- ----		scroll x
				---- --9- ---- ----		?
				---- ---8 7654 3210		code

	We draw each line using a bunch of 64x1 tiles

*/

void hotchase_draw_road(struct mame_bitmap *bitmap)
{
	int sy;


/* Referred to what's in the ROMs */
#define XSIZE 512
#define YSIZE 512


	/* Let's draw from the top to the bottom of the visible screen */
	for (sy = Machine->visible_area.min_y;sy <= Machine->visible_area.max_y;sy++)
	{
		int sx;
		int code	=	  wecleman_roadram[ sy *4/2 + 2/2 ] +
						( wecleman_roadram[ sy *4/2 + 0/2 ] << 16 );
		int color	=	(( code & 0x00f00000 ) >> 20) + 0x70;
		int scrollx = 2*(( code & 0x0007fc00 ) >> 10);
		code		=	 ( code & 0x000001ff ) >>  0;


		/* convert line number in gfx element number: */
		/* code is the tile code of the start of this line */
		code	= code * ( XSIZE / 32 );

		for (sx = 0;sx < 2*XSIZE;sx += 64)
		{
			drawgfx(bitmap,Machine->gfx[0],
					code++,
					color,
					0,0,
					((sx-scrollx)&0x3ff)-(384-32),sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

#undef XSIZE
#undef YSIZE
}




/***************************************************************************

							Sprites Drawing

***************************************************************************/

/* Hot Chase: shadow of trees is pen 0x0a - Should it be black like it is now */

/*

	Sprites: 256 entries, 16 bytes each, first ten bytes used (and tested)

	Offset	Bits					Meaning

	00.w	fedc ba98 ---- ----		Screen Y start
			---- ---- 7654 3210		Screen Y stop

	02.w	fedc ba-- ---- ----		High bits of sprite "address"
			---- --9- ---- ----		Flip Y ?
			---- ---8 7654 3210		Screen X start

	04.w	fedc ba98 ---- ----		Color
			---- ---- 7654 3210		Source Width / 8

	06.w	f--- ---- ---- ----		Flip X
			-edc ba98 7654 3210		Low bits of sprite "address"

	08.w	--dc ba98 ---- ----		Y? Shrink Factor
			---- ---- --54 3210		X? Shrink Factor

Sprite "address" is the index of the pixel the hardware has to start
fetching data from, divided by 8. Only the on-screen height and source data
width are provided, along with two shrinking factors. So on screen width
and source height are calculated by the hardware using the shrink factors.
The factors are in the range 0 (no shrinking) - 3F (half size).

*/

static void get_sprite_info(void)
{
	const pen_t          *base_pal	= Machine->remapped_colortable;
	const unsigned char  *base_gfx	= memory_region(REGION_GFX1);

	const int gfx_max = memory_region_length(REGION_GFX1);

	data16_t *source			=	spriteram16;

	struct sprite *sprite		=	sprite_list->sprite;
	const struct sprite *finish	=	sprite + NUM_SPRITES;

	int visibility = SPRITE_VISIBLE;


#define SHRINK_FACTOR(x) \
	(1.0 - ( ( (x) & 0x3F ) / 63.0) * 0.5)

	for (; sprite < finish; sprite++,source+=0x10/2)
	{
		int code, gfx, zoom;

		sprite->priority = 0;

		sprite->y	= source[ 0x00/2 ];
		if (sprite->y == 0xFFFF)	{ visibility = 0; }

		sprite->flags = visibility;
		if (visibility==0) continue;

		sprite->total_height = (sprite->y >> 8) - (sprite->y & 0xFF);
		if (sprite->total_height < 1) {sprite->flags = 0;	continue;}

		sprite->x	 		=	source[ 0x02/2 ];
		sprite->tile_width	=	source[ 0x04/2 ];
		code				=	source[ 0x06/2 ];
		zoom				=	source[ 0x08/2 ];

		gfx	= (wecleman_gfx_bank[(sprite->x >> 10) & 0x3f] << 15) +  (code & 0x7fff);
		sprite->pal_data = base_pal + ( (sprite->tile_width >> 4) & 0x7f0 );	// 16 colors = 16 shorts

		if (code & 0x8000)
		{	sprite->flags |= SPRITE_FLIPX;	gfx += 1 - (sprite->tile_width & 0xFF);	};

		if (sprite->x & 0x0200)		/* ?flip y? */
		{	sprite->flags |= SPRITE_FLIPY; }

		gfx *= 8;

		sprite->pen_data = base_gfx + gfx;

		sprite->tile_width	= (sprite->tile_width & 0xFF) * 8;
		if (sprite->tile_width < 1) {sprite->flags = 0;	continue;}

		sprite->tile_height = sprite->total_height * ( 1.0 / SHRINK_FACTOR(zoom>>8) );
		sprite->x   = (sprite->x & 0x1ff) - 0xc0;
		sprite->y   = (sprite->y & 0xff);
		sprite->total_width = sprite->tile_width * SHRINK_FACTOR(zoom & 0xFF);

		sprite->line_offset = sprite->tile_width;

		/* Bound checking */
		if ((gfx + sprite->tile_width * sprite->tile_height - 1) >= gfx_max )
			{sprite->flags = 0;	continue;}
	}
}



/***************************************************************************

							Browse the graphics

***************************************************************************/

/*
	Browse the sprites

	Use:
	* LEFT, RIGHT, UP, DOWN and PGUP/PGDN to move around
	* SHIFT + PGUP/PGDN to move around faster
	* SHIFT + LEFT/RIGHT to change the width of the graphics
	* SHIFT + RCTRL to go back to the start of the gfx

*/
#ifdef MAME_DEBUG
static void browser(struct mame_bitmap *bitmap)
{
	const pen_t          *base_pal	=	Machine->gfx[0]->colortable + 0;
	const unsigned char  *base_gfx	=	memory_region(REGION_GFX1);

	const int gfx_max				=	memory_region_length(REGION_GFX1);

	struct sprite *sprite			=	sprite_list->sprite;
	const struct sprite *finish		=	sprite + NUM_SPRITES;

	static int w = 32, gfx;
	char buf[80];

	for ( ; sprite < finish ; sprite++)	sprite->flags = 0;

	sprite = sprite_list->sprite;

	sprite->flags = SPRITE_VISIBLE;
	sprite->x = 0;
	sprite->y = 0;
	sprite->tile_height = sprite->total_height = 224;
	sprite->pal_data = base_pal;

	KEY_FAST(LEFT,	gfx-=8;)
	KEY_FAST(RIGHT,	gfx+=8;)
	KEY_FAST(UP,	gfx-=w;)
	KEY_FAST(DOWN,	gfx+=w;)

	KEY_SHIFT(PGDN,	gfx -= 0x100000;)
	KEY_SHIFT(PGUP,	gfx += 0x100000;)

	KEY(PGDN,gfx+=w*sprite->tile_height;)
	KEY(PGUP,gfx-=w*sprite->tile_height;)

	KEY_SHIFT(RCONTROL,	gfx = 0;)

	gfx %= gfx_max;
	if (gfx < 0)	gfx += gfx_max;

	KEY_SHIFT(LEFT,		w-=8;)
	KEY_SHIFT(RIGHT,	w+=8;)
	w &= 0x1ff;

	sprite->pen_data = base_gfx + gfx;
	sprite->tile_width = sprite->total_width = sprite->line_offset = w;

	/* Bound checking */
	if ((gfx + sprite->tile_width * sprite->tile_height - 1) >= gfx_max )
		sprite->flags = 0;

	sprite_draw( sprite_list, 0 );

	sprintf(buf,"W:%02X GFX/8: %X",w,gfx / 8);
	usrintf_showmessage(buf);
}
#endif











/***************************************************************************

							Screen Drawing

***************************************************************************/

#define WECLEMAN_TVKILL \
	if ((wecleman_irqctrl & 0x40)==0) layers_ctrl = 0;	// TV-KILL

#define WECLEMAN_LAMPS \
	set_led_status(0,wecleman_selected_ip & 0x04); 		// Start lamp



/* You can activate each single layer of gfx */
#define WECLEMAN_LAYERSCTRL \
{ \
	static int browse = 0; \
	KEY(B, browse ^= 1;) \
	if (browse) \
	{ \
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area); \
		browser(bitmap); \
		return; \
	} \
	if (keyboard_pressed(KEYCODE_Z)) \
	{ \
	int msk = 0; \
	 \
		if (keyboard_pressed(KEYCODE_Q))	{ msk |= 0x01;} \
		if (keyboard_pressed(KEYCODE_W))	{ msk |= 0x02;} \
		if (keyboard_pressed(KEYCODE_E))	{ msk |= 0x04;} \
		if (keyboard_pressed(KEYCODE_A))	{ msk |= 0x08;} \
		if (keyboard_pressed(KEYCODE_R))	{ msk |= 0x10;} \
		if (msk != 0) layers_ctrl &= msk; \
	} \
}


/***************************************************************************
							WEC Le Mans 24
***************************************************************************/

void wecleman_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int i, layers_ctrl = -1;


	WECLEMAN_LAMPS

	WECLEMAN_TVKILL

#ifdef MAME_DEBUG
	WECLEMAN_LAYERSCTRL
#endif

{
/* Set the scroll values for the scrolling layers */

	/* y values */
	int bg_y = wecleman_txtram[( 0x0F24+2 )/2] & (TILEMAP_DIMY - 1);
	int fg_y = wecleman_txtram[( 0x0F24+0 )/2] & (TILEMAP_DIMY - 1);

	tilemap_set_scrolly(bg_tilemap, 0, bg_y );
	tilemap_set_scrolly(fg_tilemap, 0, fg_y );

	/* x values */
	for ( i = 0 ; i < 28; i++ )
	{
		int j;
		int bg_x = 0xB0 + wecleman_txtram[ 0xF80/2 + i*4/2 + 2/2 ];
		int fg_x = 0xB0 + wecleman_txtram[ 0xF80/2 + i*4/2 + 0/2 ];

		for ( j = 0 ; j < 8; j++ )
		{
			tilemap_set_scrollx(bg_tilemap, (bg_y + i*8 + j) & (TILEMAP_DIMY - 1), bg_x );
			tilemap_set_scrollx(fg_tilemap, (fg_y + i*8 + j) & (TILEMAP_DIMY - 1), fg_x );
		}
	}
}


	get_sprite_info();
	sprite_update();


	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* Draw the road (lines which have "priority" 0x02) */
	if (layers_ctrl & 16)	wecleman_draw_road(bitmap,0x02);

	/* Draw the background */
	if (layers_ctrl & 1)
		tilemap_draw(bitmap, bg_tilemap,  0,0);

	/* Draw the foreground */
	if (layers_ctrl & 2)
		tilemap_draw(bitmap, fg_tilemap,  0,0);

	/* Draw the road (lines which have "priority" 0x04) */
	if (layers_ctrl & 16)	wecleman_draw_road(bitmap,0x04);

	/* Draw the sprites */
	if (layers_ctrl & 8)	sprite_draw(sprite_list,0);

	/* Draw the text layer */
	if (layers_ctrl & 4)
		tilemap_draw(bitmap, txt_tilemap,  0,0);
}








/***************************************************************************
								Hot Chase
***************************************************************************/

void hotchase_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int layers_ctrl = -1;

	WECLEMAN_LAMPS

	WECLEMAN_TVKILL

#ifdef MAME_DEBUG
	WECLEMAN_LAYERSCTRL
#endif

	get_sprite_info();
	sprite_update();


	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* Draw the background */
	if (layers_ctrl & 1)
		K051316_zoom_draw_0(bitmap,0,0);

	/* Draw the road */
	if (layers_ctrl & 16)
		hotchase_draw_road(bitmap);

	/* Draw the sprites */
	if (layers_ctrl & 8)	sprite_draw(sprite_list,0);

	/* Draw the foreground (text) */
	if (layers_ctrl & 4)
		K051316_zoom_draw_1(bitmap,0,0);
}
