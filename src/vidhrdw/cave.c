/***************************************************************************

							  -= Cave Games =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing:

		X/C/V/B/Z  with  Q   shows layer 0 (tiles with priority 0/1/2/3/All)
		X/C/V/B/Z  with  W   shows layer 1 (tiles with priority 0/1/2/3/All)
		X/C/V/B/Z  with  E   shows layer 2 (tiles with priority 0/1/2/3/All)
		X/C/V/B/Z  with  A   shows sprites (tiles with priority 0/1/2/3/All)

		Keys can be used togheter!

		[ 1, 2 or 3 Scrolling Layers ]

		Layer Size:				512 x 512
		Tiles:					16x16x8 (16x16x4 in some games)

		[ 1024 Zooming Sprites ]

		There are 2 spriterams. A hardware register's bit selects
		the one to display (sprites double buffering).

		The sprites are NOT tile based: the "tile" size and start
		address is selectable for each sprite with a 16 pixel granularity.


**************************************************************************/
#include "vidhrdw/generic.h"


/* Variables that driver has access to: */

data16_t *cave_videoregs;

data16_t *cave_vram_0, *cave_vctrl_0;
data16_t *cave_vram_1, *cave_vctrl_1;
data16_t *cave_vram_2, *cave_vctrl_2;

data16_t *cave_wkram;

/* Variables only used here: */

static struct tilemap *tilemap_0, *tilemap_1, *tilemap_2;

/* Variables defined in driver: */

extern int cave_spritetype;

#define CAVE_SPRITETYPE_ZBUF		0x01
#define CAVE_SPRITETYPE_ZOOM		0x02
static  int cave_spritetype2;

#define SPRITE_FLIPX_CAVE					0x01
#define SPRITE_FLIPY_CAVE					0x02
#define SPRITE_VISIBLE_CAVE					0x04
#define SWAP(X,Y) { int temp = X; X = Y; Y = temp; }

struct sprite_cave {
	int priority, flags;

	const UINT8 *pen_data;	/* points to top left corner of tile data */
	int line_offset;

	const UINT32 *pal_data;
//	UINT32 pen_usage;

	int x_offset, y_offset;
	int tile_width, tile_height;
	int total_width, total_height;	/* in screen coordinates */
	int x, y;
};

static int orientation, screen_width, screen_height;

static struct {
	int clip_left, clip_right, clip_top, clip_bottom;
	unsigned char *baseaddr;
	int line_offset;
	int origin_x, origin_y;
} blit;

#define MAX_PRIORITY 4
#define MAX_SPRITE_NUM 0x400
static int num_sprites;
static struct sprite_cave *sprite_cave;
static struct sprite_cave *sprite_table[MAX_PRIORITY][MAX_SPRITE_NUM+1];
static UINT16 *sprite_zbuf;
static int sprite_zbuf_size;
static UINT16 sprite_zbuf_baseval = 0x10000-MAX_SPRITE_NUM;

static void (*get_sprite_info)(void);
static void (*cave_sprite_draw)( int priority );

static int sprite_init_cave(void);
static void sprite_draw_cave( int priority );
static void sprite_draw_cave_zbuf( int priority );
static void sprite_draw_donpachi( int priority );
static void sprite_draw_donpachi_zbuf( int priority );
void cave_vh_stop(void);

static UINT16 *cave_prgrom;

/***************************************************************************

						Callbacks for the TileMap code

							  [ Tiles Format ]

Offset:

0.w			fe-- ---- ---- ---		Priority
			--dc ba98 ---- ----		Color
			---- ---- 7654 3210

2.w									Code



***************************************************************************/

#define DIM_NX		(0x20)
#define DIM_NY		(0x20)


#define CAVE_TILEMAP(_n_) \
static void get_tile_info_##_n_( int tile_index ) \
{ \
	UINT32 code		=	(cave_vram_##_n_[ tile_index * 2 + 0] << 16 )+ \
						 cave_vram_##_n_[ tile_index * 2 + 1]; \
	SET_TILE_INFO( _n_ ,  code & 0x00ffffff , (code & 0x3f000000) >> (32-8) ); \
	tile_info.priority = (code & 0xc0000000)>> (32-2); \
} \
\
WRITE16_HANDLER( cave_vram_##_n_##_w ) \
{ \
	COMBINE_DATA(&cave_vram_##_n_[offset]); \
	if ( (offset/2) < DIM_NX * DIM_NY ) \
		tilemap_mark_tile_dirty(tilemap_##_n_, offset/2 ); \
} \
\
WRITE16_HANDLER( cave_vram_##_n_##_8x8_w ) \
{ \
	offset %= ( (DIM_NX * 2) * (DIM_NY * 2) * 2); /* mirrored RAM */ \
	COMBINE_DATA(&cave_vram_##_n_[offset]); \
	tilemap_mark_tile_dirty(tilemap_##_n_, offset/2 ); \
}

CAVE_TILEMAP(0)
CAVE_TILEMAP(1)
CAVE_TILEMAP(2)






/***************************************************************************

								Vh_Start

***************************************************************************/


/* 3 Layers (layer 3 is made of 8x8 tiles!) */
int ddonpach_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	/* 8x8 tiles here! */
	tilemap_2 = tilemap_create(	get_tile_info_2,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								DIM_NX*2,DIM_NY*2 );


	if (tilemap_0 && tilemap_1 && tilemap_2 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,0);

		if (strcmp(Machine->gamedrv->name,"donpachi"))
		{	/* ddonpach */
			tilemap_set_scrolldx( tilemap_0, -0x6c +1, -0x57    );
			tilemap_set_scrolldx( tilemap_1, -0x6d +1, -0x56    );
			tilemap_set_scrolldx( tilemap_2, -0x6e -7, -0x55 +8 );
		}
		else
		{	/* donpachi */
			tilemap_set_scrolldx( tilemap_0, -0x6c +3, -0x57    );
			tilemap_set_scrolldx( tilemap_1, -0x6d +3, -0x56    );
			tilemap_set_scrolldx( tilemap_2, -0x6e -5, -0x55 +8 );
		}

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_2, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}


/* 3 Layers (like esprade but with different scroll delta's) */
int guwange_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_2 = tilemap_create(	get_tile_info_2,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );


	if (tilemap_0 && tilemap_1 && tilemap_2 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,0);

		tilemap_set_scrolldx( tilemap_0, -0x6c +2, -0x57 );
		tilemap_set_scrolldx( tilemap_1, -0x6d +2, -0x56 );
		tilemap_set_scrolldx( tilemap_2, -0x6e +2, -0x55 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_2, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}



/* 3 Layers */
int esprade_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_2 = tilemap_create(	get_tile_info_2,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );


	if (tilemap_0 && tilemap_1 && tilemap_2 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,0);

		tilemap_set_scrolldx( tilemap_0, -0x6c, -0x57 );
		tilemap_set_scrolldx( tilemap_1, -0x6d, -0x56 );
		tilemap_set_scrolldx( tilemap_2, -0x6e, -0x55 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_2, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}



/* 2 Layers */
int dfeveron_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_2 = 0;


	if (tilemap_0 && tilemap_1 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

/*
	Scroll registers (on dfeveron logo screen):
		8195	a1f7 (both)	=	200-6b	200-9	(flip off)
		01ac	2108 (both)	=	200-54	100+8	(flip on)
	Video registers:
		0183	0001		=	200-7d	001		(flip off)
		81bf	80f0		=	200-41	100-10	(flip on)
*/

		tilemap_set_scrolldx( tilemap_0, -0x6c +1, -0x57 +2 );
		tilemap_set_scrolldx( tilemap_1, -0x6d +1, -0x56 +2 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}



/* 1 Layer */
int uopoko_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = 0;

	tilemap_2 = 0;


	if (tilemap_0 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scrolldx( tilemap_0, -0x6d, -0x54 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}



/* 3 Layers */
int hotdogst_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );

	tilemap_2 = tilemap_create(	get_tile_info_2,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								DIM_NX,DIM_NY );


	if (tilemap_0 && tilemap_1 && tilemap_2 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,0);

		tilemap_set_scrolldx( tilemap_0, -0x6c, -0x57 + 0x40 );
		tilemap_set_scrolldx( tilemap_1, -0x6d, -0x56 + 0x40 );
		tilemap_set_scrolldx( tilemap_2, -0x6e, -0x55 + 0x40 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_2, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}

/* 2 Layers */
int mazinger_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								DIM_NX,DIM_NY );

	tilemap_2 = 0;


	if (tilemap_0 && tilemap_1 && !sprite_init_cave())
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scrolldx( tilemap_0, -0x6c +1, -0x57 +2 );
		tilemap_set_scrolldx( tilemap_1, -0x6d +1, -0x56 +2 );

		tilemap_set_scrolldy( tilemap_0, -0x11, -0x100 );
		tilemap_set_scrolldy( tilemap_1, -0x11, -0x100 );

		return 0;
	}
	else {cave_vh_stop(); return 1;}
}


/***************************************************************************

							Vh_Init_Palette

***************************************************************************/

/* Function needed for games with 4 bit sprites, rather than 8 bit */


void dfeveron_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int color, pen;

	/* Fill the 0-3fff range, used by sprites ($40 color codes * $100 pens)
	   Here sprites have 16 pens, but the sprite drawing routine always
	   multiplies the color code by $100 (for consistency).
	   That's why we need this function.	*/

	for( color = 0; color < 0x40; color++ )
		for( pen = 0; pen < 16; pen++ )
			colortable[color * 256 + pen] = color * 16 + pen;
}



void ddonpach_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int color, pen;

	/* Fill the 8000-83ff range ($40 color codes * $10 pens) for
	   layers 1 & 2 which are 4 bits deep rather than 8 bits deep
	   like layer 3, but use the first 16 color of every 256 for
	   any given color code. */

	for( color = 0; color < 0x40; color++ )
		for( pen = 0; pen < 16; pen++ )
			colortable[color * 16 + pen + 0x8000] = 0x4000 + color * 256 + pen;
}





/***************************************************************************

							Sprites Drawing

***************************************************************************/


/* --------------------------[ Sprites Format ]----------------------------

Offset:		Format:					Value:

00.w		fedc ba98 76-- ----		X Position
			---- ---- --54 3210

02.w		fedc ba98 76-- ----		Y Position
			---- ---- --54 3210

04.w		fe-- ---- ---- ----
			--dc ba98 ---- ----		Color
			---- ---- 76-- ----
			---- ---- --54 ----		Priority
			---- ---- ---- 3---		Flip X
			---- ---- ---- -2--		Flip Y
			---- ---- ---- --10		Code High Bit(s?)

06.w								Code Low Bits

08/0A.w								Zoom X / Y

0C.w		fedc ba98 ---- ----		Tile Size X
			---- ---- 7654 3210		Tile Size Y

0E.w								Unused

------------------------------------------------------------------------ */

static void get_sprite_info_cave(void)
{
	const int region				=	REGION_GFX4;

	const UINT32         *base_pal	=	Machine->remapped_colortable + 0;
	const unsigned char  *base_gfx	=	memory_region(region);
	const unsigned char  *gfx_max	=	base_gfx + memory_region_length(region);

	data16_t      *source			=	spriteram16 + ((spriteram_size/2) / 2) * (cave_videoregs[ 4 ] & 1);

	struct sprite_cave *sprite			=	sprite_cave;
	const struct sprite_cave *finish	=	sprite + num_sprites;

	int	glob_flipx	=	cave_videoregs[ 0 ] & 0x8000;
	int	glob_flipy	=	cave_videoregs[ 1 ] & 0x8000;

	int max_x		=	Machine->drv->screen_width;
	int max_y		=	Machine->drv->screen_height;

	for (; sprite < finish; sprite++,source+=8 )
	{
		int x,y,attr,code,zoomx,zoomy,size,flipx,flipy;

		if (cave_spritetype == 2)	/* Hot Dog Storm */
		{
			x		=		source[ 0 ] & 0x3ff;
			y		=		source[ 1 ] & 0x3ff;
		}
		else						/* all others */
		{
			x		=		source[ 0 ] >> 6;
			y		=		source[ 1 ] >> 6;
		}
		attr		=		source[ 2 ];
		code		=		source[ 3 ] + ((attr & 3) << 16);
		zoomx		=		source[ 4 ];
		zoomy		=		source[ 5 ];
		size		=		source[ 6 ];

		flipx		=		attr & 0x0008;
		flipy		=		attr & 0x0004;

		sprite->priority		=	(attr & 0x0030) >> 4;
		sprite->flags			=	SPRITE_VISIBLE_CAVE;

		sprite->tile_width		=	( (size >> 8) & 0x1f ) * 16;
		sprite->tile_height		=	( (size >> 0) & 0x1f ) * 16;

		sprite->total_width		=	(sprite->tile_width  * zoomx) / 0x100;
		sprite->total_height	=	(sprite->tile_height * zoomy) / 0x100;

		sprite->pen_data		=	base_gfx + (16*16) * code;
		sprite->line_offset		=	sprite->tile_width;

		sprite->pal_data		=	base_pal + (attr & 0x3f00);	// first 0x4000 colors

		/* Bound checking */
		if ((sprite->pen_data + sprite->tile_width * sprite->tile_height - 1) >= gfx_max )
			{sprite->flags = 0;	continue;}

		if (flipx) x += sprite->tile_width - sprite->total_width;
		if (flipy) y += sprite->tile_height - sprite->total_height;

		if (x > 0x1FF)	x -= 0x400;
		if (y > 0x1FF)	y -= 0x400;

		if (glob_flipx)	{ x = max_x - x - sprite->total_width;	flipx = !flipx; }
		if (glob_flipy)	{ y = max_y - y - sprite->total_height;	flipy = !flipy; }

		sprite->x				=	x;
		sprite->y				=	y;

		if (flipx)	sprite->flags |= SPRITE_FLIPX_CAVE;
		if (flipy)	sprite->flags |= SPRITE_FLIPY_CAVE;
	}
}

static void get_sprite_info_donpachi(void)
{
	const int region				=	REGION_GFX4;

	const UINT32         *base_pal	=	Machine->remapped_colortable + 0;
	const unsigned char  *base_gfx	=	memory_region(region);
	const unsigned char  *gfx_max	=	base_gfx + memory_region_length(region);

	data16_t      *source			=	spriteram16 + ((spriteram_size/2) / 2) * (cave_videoregs[ 4 ] & 1);

	struct sprite_cave *sprite			=	sprite_cave;
	const struct sprite_cave *finish	=	sprite + num_sprites;

	int	glob_flipx	=	cave_videoregs[ 0 ] & 0x8000;
	int	glob_flipy	=	cave_videoregs[ 1 ] & 0x8000;

	int max_x		=	Machine->drv->screen_width;
	int max_y		=	Machine->drv->screen_height;

	for (; sprite < finish; sprite++,source+=8 )
	{
		int x,y,attr,code,size,flipx,flipy;

		attr		=		source[ 0 ];
		code		=		source[ 1 ] + ((attr & 3) << 16);
		x			=		source[ 2 ] & 0x3FF;
		y			=		source[ 3 ] & 0x3FF;
		size		=		source[ 4 ];

		flipx		=		attr & 0x0008;
		flipy		=		attr & 0x0004;

		sprite->priority		=	(attr & 0x0030) >> 4;
		sprite->flags			=	SPRITE_VISIBLE_CAVE;

		sprite->tile_width		=	sprite->total_width		=	( (size >> 8) & 0x1f ) * 16;
		sprite->tile_height		=	sprite->total_height	=	( (size >> 0) & 0x1f ) * 16;

		sprite->pen_data		=	base_gfx + (16*16) * code;
		sprite->line_offset		=	sprite->tile_width;

		sprite->pal_data		=	base_pal + (attr & 0x3f00);	// first 0x4000 colors

		/* Bound checking */
		if ((sprite->pen_data + sprite->tile_width * sprite->tile_height - 1) >= gfx_max )
			{sprite->flags = 0;	continue;}

		if (x > 0x1FF)	x -= 0x400;
		if (y > 0x1FF)	y -= 0x400;

		if (glob_flipx)	{ x = max_x - x - sprite->total_width;	flipx = !flipx; }
		if (glob_flipy)	{ y = max_y - y - sprite->total_height;	flipy = !flipy; }

		sprite->x				=	x;
		sprite->y				=	y;

		if (flipx)	sprite->flags |= SPRITE_FLIPX_CAVE;
		if (flipy)	sprite->flags |= SPRITE_FLIPY_CAVE;
	}
}

static void sprite_set_clip( const struct rectangle *clip )
{
	int left = clip->min_x;
	int top = clip->min_y;
	int right = clip->max_x+1;
	int bottom = clip->max_y+1;

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

	blit.clip_left = left;
	blit.clip_top = top;
	blit.clip_right = right;
	blit.clip_bottom = bottom;
}

static int sprite_init_cave(void)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;

	orientation = Machine->orientation;
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;

	blit.origin_x = 0;
	blit.origin_y = 0;
	blit.baseaddr = bitmap->line[0];
	blit.line_offset = bitmap->line[1]-bitmap->line[0];
	sprite_set_clip(&Machine->visible_area);

	if (cave_spritetype == 0 || cave_spritetype == 2)	// most of the games
	{
		get_sprite_info = get_sprite_info_cave;
		cave_spritetype2 = CAVE_SPRITETYPE_ZOOM;
	}
	else						// donpachi ddonpach
	{
		get_sprite_info = get_sprite_info_donpachi;
		cave_spritetype2 = 0;
	}

	sprite_zbuf_size = Machine->drv->screen_width * Machine->drv->screen_height * 2;
	if(!(sprite_zbuf = malloc(sprite_zbuf_size))) return 1;

	num_sprites = spriteram_size / 0x10 / 2;
	if(!(sprite_cave = calloc( num_sprites, sizeof(struct sprite_cave) ))) return 1;

	cave_prgrom = (UINT16 *)memory_region(REGION_CPU1);

	return 0;
}

static void sprite_update_cave( void ){

	/* make a pass to adjust for screen orientation */
	if( orientation & ORIENTATION_SWAP_XY ){
		struct sprite_cave *sprite = sprite_cave;
		const struct sprite_cave *finish = &sprite[num_sprites];
		while( sprite<finish ){
			SWAP(sprite->x, sprite->y)
			SWAP(sprite->total_height,sprite->total_width)
			SWAP(sprite->tile_width,sprite->tile_height)
			SWAP(sprite->x_offset,sprite->y_offset)

			/* we must also swap the flipx and flipy bits (if they aren't identical) */
			if( sprite->flags&SPRITE_FLIPX_CAVE ){
				if( !(sprite->flags&SPRITE_FLIPY_CAVE) ){
					sprite->flags = (sprite->flags&~SPRITE_FLIPX_CAVE)|SPRITE_FLIPY_CAVE;
				}
			}
			else {
				if( sprite->flags&SPRITE_FLIPY_CAVE ){
					sprite->flags = (sprite->flags&~SPRITE_FLIPY_CAVE)|SPRITE_FLIPX_CAVE;
				}
			}
			sprite++;
		}
	}
	if( orientation & ORIENTATION_FLIP_X ){
		struct sprite_cave *sprite = sprite_cave;
		const struct sprite_cave *finish = &sprite[num_sprites];
		int toggle_bit = SPRITE_FLIPX_CAVE;
		while( sprite<finish ){
			sprite->x = screen_width - (sprite->x+sprite->total_width);
			sprite->flags ^= toggle_bit;

			/* extra processing for packed sprites */
			sprite->x_offset = sprite->tile_width - (sprite->x_offset+sprite->total_width);
			sprite++;
		}
	}
	if( orientation & ORIENTATION_FLIP_Y ){
		struct sprite_cave *sprite = sprite_cave;
		const struct sprite_cave *finish = &sprite[num_sprites];
		int toggle_bit = SPRITE_FLIPY_CAVE;
		while( sprite<finish ){
			sprite->y = screen_height - (sprite->y+sprite->total_height);
			sprite->flags ^= toggle_bit;

			/* extra processing for packed sprites */
			sprite->y_offset = sprite->tile_height - (sprite->y_offset+sprite->total_height);
			sprite++;
		}
	}
	{
		struct sprite_cave *sprite = sprite_cave;
		const struct sprite_cave *finish = &sprite[num_sprites];
		int i[4]={0,0,0,0};
		int priority_check = 0;
		int spritetype = cave_spritetype2;
		while( sprite<finish )
		{
			if( sprite->total_width>0 && sprite->total_height>0 && (sprite->flags&SPRITE_VISIBLE_CAVE) &&
				sprite->x + sprite->total_width>blit.clip_left && sprite->x<blit.clip_right &&
				sprite->y + sprite->total_height>blit.clip_top && sprite->y<blit.clip_bottom )
			{
				sprite_table[sprite->priority][i[sprite->priority]++] = sprite;

				if(Machine->color_depth == 8)	/* mark sprite colors */
				{
					int j,offs = sprite->pal_data - Machine->remapped_colortable;
					if(Machine->drv->total_colors == 0x8000)	/* 8 Bit Sprites */
						for(j=1;j<256;j++)
							palette_used_colors[offs+j] = PALETTE_COLOR_USED;
					else										/* 4 Bit Sprites */
						for(j=1;j<16;j++)
							palette_used_colors[(offs>>4)+j] = PALETTE_COLOR_USED;
				}

				if(!(spritetype&CAVE_SPRITETYPE_ZBUF))
				{
					if(priority_check > sprite->priority)
						spritetype |= CAVE_SPRITETYPE_ZBUF;
					else
						priority_check = sprite->priority;
				}
			}
			sprite++;
		}

		sprite_table[0][i[0]] = 0;
		sprite_table[1][i[1]] = 0;
		sprite_table[2][i[2]] = 0;
		sprite_table[3][i[3]] = 0;

		switch (spritetype)
		{
			case CAVE_SPRITETYPE_ZOOM:
				cave_sprite_draw = sprite_draw_cave;
				break;

			case CAVE_SPRITETYPE_ZOOM | CAVE_SPRITETYPE_ZBUF:
				cave_sprite_draw = sprite_draw_cave_zbuf;
				if(!(sprite_zbuf_baseval += MAX_SPRITE_NUM))
					memset( sprite_zbuf, 0x00, sprite_zbuf_size );
				break;

			case CAVE_SPRITETYPE_ZBUF:
				cave_sprite_draw = sprite_draw_donpachi_zbuf;
				if(!(sprite_zbuf_baseval += MAX_SPRITE_NUM))
					memset( sprite_zbuf, 0x00, sprite_zbuf_size );
				break;

			default:
			case 0:
				cave_sprite_draw = sprite_draw_donpachi;
		}
	}
}

static void do_blit_zoom16_cave( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
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
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
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
		const UINT32        *pal_data = sprite->pal_data;
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
				if (xcount >= sprite->tile_width) goto skip1;
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if ((ycount < sprite->tile_height) && pen) *dest1 = pal_data[pen];
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
				if (ycount >= sprite->tile_height) goto skip;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if ((xcount < sprite->tile_width) && pen) dest[x] = pal_data[pen];
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
			}
		}
	}
}


static void do_blit_zoom16_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
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
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
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
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy/2;
		UINT16 *dest = (UINT16 *)(blit.baseaddr + blit.line_offset*y1);
		int pitchz = (blit.clip_right-blit.clip_left)*dy;
		UINT16 *zbf = (UINT16 *)((unsigned char *)sprite_zbuf + (blit.clip_right-blit.clip_left)*y1*2);
		UINT16 pri_sp = (UINT16)(sprite - sprite_cave) + sprite_zbuf_baseval;
		int ycount = ycount0;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			int xcount = xcount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT16 *dest1;
				UINT16 *zbf1;

				ycount = ycount0;
				while( xcount>=sprite->total_width ){
					xcount -= sprite->total_width;
					pen_data+=sprite->line_offset;
				}
				if (xcount >= sprite->tile_width) goto skip1;
				source = pen_data;
				dest1 = &dest[x];
				zbf1 = &zbf[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if ((ycount < sprite->tile_height) && pen && (*zbf1<=pri_sp))
					{
						*dest1 = pal_data[pen];
						*zbf1 = pri_sp;
					}
					ycount+= sprite->tile_height;
					dest1 += pitch;
					zbf1 += pitchz;
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
				if (ycount >= sprite->tile_height) goto skip;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if ((xcount < sprite->tile_width) && pen && (zbf[x]<=pri_sp))
					{
						dest[x] = pal_data[pen];
						zbf[x] = pri_sp;
					}
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
				zbf += pitchz;
			}
		}
	}
}

static void do_blit_16_cave( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = x1-blit.clip_right;
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
			xcount0 = blit.clip_left-x1;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = y1-blit.clip_bottom;
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
			ycount0 = blit.clip_top-y1;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy/2;
		UINT16 *dest = (UINT16 *)(blit.baseaddr + blit.line_offset*y1);

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			pen_data+=sprite->line_offset*xcount0+ycount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT16 *dest1;
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					pen = *source;
					if (pen) *dest1 = pal_data[pen];
					source ++;
					dest1 += pitch;
				}
				pen_data+=sprite->line_offset;
			}
		}
		else {
			pen_data+=sprite->line_offset*ycount0+xcount0;
			for( y=y1; y!=y2; y+=dy ){
				const unsigned char *source;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					pen = *source;
					if (pen) dest[x] = pal_data[pen];
					source++;
				}
				pen_data += sprite->line_offset;
				dest += pitch;
			}
		}
	}
}


static void do_blit_16_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = x1-blit.clip_right;
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
			xcount0 = blit.clip_left-x1;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = y1-blit.clip_bottom;
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
			ycount0 = blit.clip_top-y1;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy/2;
		UINT16 *dest = (UINT16 *)(blit.baseaddr + blit.line_offset*y1);
		int pitchz = (blit.clip_right-blit.clip_left)*dy;
		UINT16 *zbf = (UINT16 *)((unsigned char *)sprite_zbuf + (blit.clip_right-blit.clip_left)*y1*2);
		UINT16 pri_sp = (UINT16)(sprite - sprite_cave) + sprite_zbuf_baseval;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			pen_data+=sprite->line_offset*xcount0+ycount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT16 *dest1;
				UINT16 *zbf1;
				source = pen_data;
				dest1 = &dest[x];
				zbf1 = &zbf[x];
				for( y=y1; y!=y2; y+=dy ){
					pen = *source;
					if (pen && (*zbf1<=pri_sp))
					{
						*dest1 = pal_data[pen];
						*zbf1 = pri_sp;
					}
					source ++;
					dest1 += pitch;
					zbf1 += pitchz;
				}
				pen_data+=sprite->line_offset;
			}
		}
		else {
			pen_data+=sprite->line_offset*ycount0+xcount0;
			for( y=y1; y!=y2; y+=dy ){
				const unsigned char *source;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					pen = *source;
					if ( pen && (zbf[x]<=pri_sp))
					{
						dest[x] = pal_data[pen];
						zbf[x] = pri_sp;
					}
					source++;
				}
				pen_data += sprite->line_offset;
				dest += pitch;
				zbf += pitchz;
			}
		}
	}
}

//ks s
static void do_blit_zoom8_cave( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
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
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
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
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy;
		UINT8 *dest = blit.baseaddr + blit.line_offset*y1;
		int ycount = ycount0;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			int xcount = xcount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT8 *dest1;

				ycount = ycount0;
				while( xcount>=sprite->total_width ){
					xcount -= sprite->total_width;
					pen_data+=sprite->line_offset;
				}
				if (xcount >= sprite->tile_width) goto skip1;
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if ((ycount < sprite->tile_height) && pen) *dest1 = pal_data[pen];
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
				if (ycount >= sprite->tile_height) goto skip;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if ((xcount < sprite->tile_width) && pen) dest[x] = pal_data[pen];
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
			}
		}
	}
}


static void do_blit_zoom8_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
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
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
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
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy;
		UINT8 *dest = blit.baseaddr + blit.line_offset*y1;
		int pitchz = (blit.clip_right-blit.clip_left)*dy;
		UINT16 *zbf = (UINT16 *)((unsigned char *)sprite_zbuf + (blit.clip_right-blit.clip_left)*y1*2);
		UINT16 pri_sp = (UINT16)(sprite - sprite_cave) + sprite_zbuf_baseval;
		int ycount = ycount0;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			int xcount = xcount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT8 *dest1;
				UINT16 *zbf1;

				ycount = ycount0;
				while( xcount>=sprite->total_width ){
					xcount -= sprite->total_width;
					pen_data+=sprite->line_offset;
				}
				if (xcount >= sprite->tile_width) goto skip1;
				source = pen_data;
				dest1 = &dest[x];
				zbf1 = &zbf[x];
				for( y=y1; y!=y2; y+=dy ){
					while( ycount>=sprite->total_height ){
						ycount -= sprite->total_height;
						source ++;
					}
					pen = *source;
					if ((ycount < sprite->tile_height) && pen && (*zbf1<=pri_sp))
					{
						*dest1 = pal_data[pen];
						*zbf1 = pri_sp;
					}
					ycount+= sprite->tile_height;
					dest1 += pitch;
					zbf1 += pitchz;
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
				if (ycount >= sprite->tile_height) goto skip;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					while( xcount>=sprite->total_width ){
						xcount -= sprite->total_width;
						source++;
					}
					pen = *source;
					if ((xcount < sprite->tile_width) && pen && (zbf[x]<=pri_sp))
					{
						dest[x] = pal_data[pen];
						zbf[x] = pri_sp;
					}
					xcount += sprite->tile_width;
				}
skip:
				ycount += sprite->tile_height;
				dest += pitch;
				zbf += pitchz;
			}
		}
	}
}

static void do_blit_8_cave( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = x1-blit.clip_right;
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
			xcount0 = blit.clip_left-x1;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = y1-blit.clip_bottom;
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
			ycount0 = blit.clip_top-y1;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy;
		UINT8 *dest = blit.baseaddr + blit.line_offset*y1;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			pen_data+=sprite->line_offset*xcount0+ycount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT8 *dest1;
				source = pen_data;
				dest1 = &dest[x];
				for( y=y1; y!=y2; y+=dy ){
					pen = *source;
					if (pen) *dest1 = pal_data[pen];
					source ++;
					dest1 += pitch;
				}
				pen_data+=sprite->line_offset;
			}
		}
		else {
			pen_data+=sprite->line_offset*ycount0+xcount0;
			for( y=y1; y!=y2; y+=dy ){
				const unsigned char *source;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					pen = *source;
					if (pen) dest[x] = pal_data[pen];
					source++;
				}
				pen_data += sprite->line_offset;
				dest += pitch;
			}
		}
	}
}


static void do_blit_8_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0, ycount0 = 0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 = x1-blit.clip_right;
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
			xcount0 = blit.clip_left-x1;
			x1 = blit.clip_left;
		}
		if( x2>blit.clip_right ) x2 = blit.clip_right;
		if( x1>=x2 ) return;
	}
	if( sprite->flags & SPRITE_FLIPY_CAVE ){
		y2 = sprite->y;
		y1 = y2+sprite->total_height;
		dy = -1;
		if( y2<blit.clip_top ) y2 = blit.clip_top;
		if( y1>blit.clip_bottom ){
			ycount0 = y1-blit.clip_bottom;
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
			ycount0 = blit.clip_top-y1;
			y1 = blit.clip_top;
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data;
		const UINT32        *pal_data = sprite->pal_data;
		int x,y;
		unsigned char pen;
		int pitch = blit.line_offset*dy;
		UINT8 *dest = blit.baseaddr + blit.line_offset*y1;
		int pitchz = (blit.clip_right-blit.clip_left)*dy;
		UINT16 *zbf = (UINT16 *)((unsigned char *)sprite_zbuf + (blit.clip_right-blit.clip_left)*y1*2);
		UINT16 pri_sp = (UINT16)(sprite - sprite_cave) + sprite_zbuf_baseval;

		if( orientation & ORIENTATION_SWAP_XY ){ /* manually rotate the sprite graphics */
			pen_data+=sprite->line_offset*xcount0+ycount0;
			for( x=x1; x!=x2; x+=dx ){
				const unsigned char *source;
				UINT8 *dest1;
				UINT16 *zbf1;
				source = pen_data;
				dest1 = &dest[x];
				zbf1 = &zbf[x];
				for( y=y1; y!=y2; y+=dy ){
					pen = *source;
					if (pen && (*zbf1<=pri_sp))
					{
						*dest1 = pal_data[pen];
						*zbf1 = pri_sp;
					}
					source ++;
					dest1 += pitch;
					zbf1 += pitchz;
				}
				pen_data+=sprite->line_offset;
			}
		}
		else {
			pen_data+=sprite->line_offset*ycount0+xcount0;
			for( y=y1; y!=y2; y+=dy ){
				const unsigned char *source;
				source = pen_data;
				for( x=x1; x!=x2; x+=dx ){
					pen = *source;
					if ( pen && (zbf[x]<=pri_sp))
					{
						dest[x] = pal_data[pen];
						zbf[x] = pri_sp;
					}
					source++;
				}
				pen_data += sprite->line_offset;
				dest += pitch;
				zbf += pitchz;
			}
		}
	}
}


static void sprite_draw_cave( int priority )
{
	int i=0;
if(Machine->color_depth == 16)
	while(sprite_table[priority][i])
	{
		const struct sprite_cave *sprite = sprite_table[priority][i++];
		if ((sprite->tile_width == sprite->total_width) && (sprite->tile_height == sprite->total_height))
			do_blit_16_cave( sprite );
		else
			do_blit_zoom16_cave( sprite );
	}
else
	while(sprite_table[priority][i])
	{
		const struct sprite_cave *sprite = sprite_table[priority][i++];
		if ((sprite->tile_width == sprite->total_width) && (sprite->tile_height == sprite->total_height))
			do_blit_8_cave( sprite );
		else
			do_blit_zoom8_cave( sprite );
	}
}

static void sprite_draw_cave_zbuf( int priority )
{
	int i=0;
if(Machine->color_depth == 16)
	while(sprite_table[priority][i])
	{
		const struct sprite_cave *sprite = sprite_table[priority][i++];
		if ((sprite->tile_width == sprite->total_width) && (sprite->tile_height == sprite->total_height))
			do_blit_16_cave_zb( sprite );
		else
			do_blit_zoom16_cave_zb( sprite );
	}
else
	while(sprite_table[priority][i])
	{
		const struct sprite_cave *sprite = sprite_table[priority][i++];
		if ((sprite->tile_width == sprite->total_width) && (sprite->tile_height == sprite->total_height))
			do_blit_8_cave_zb( sprite );
		else
			do_blit_zoom8_cave_zb( sprite );
	}
}

static void sprite_draw_donpachi( int priority )
{
	int i=0;
if(Machine->color_depth == 16)
	while(sprite_table[priority][i])
		do_blit_16_cave( sprite_table[priority][i++] );
else
	while(sprite_table[priority][i])
		do_blit_8_cave( sprite_table[priority][i++] );
}

static void sprite_draw_donpachi_zbuf( int priority )
{
	int i=0;
if(Machine->color_depth == 16)
	while(sprite_table[priority][i])
		do_blit_16_cave_zb( sprite_table[priority][i++] );
else
	while(sprite_table[priority][i])
		do_blit_8_cave_zb( sprite_table[priority][i++] );
}
//ks s

/***************************************************************************

								Screen Drawing

***************************************************************************/
static UINT16 scrollx_0,scrolly_0,scrollx_1,scrolly_1,scrollx_2,scrolly_2;

#define CAVE_TILEMAP_SET_SCROLLX(_n_) \
	if ((cave_vctrl_##_n_[0] & 0x4000)&&(!(cave_vctrl_##_n_[1] & 0x4000))) \
	{ \
		int line; \
		int sdy = (cave_videoregs[ 1 ] & 0x8000) ? -0x100 : -0x11; \
		tilemap_set_scroll_rows(tilemap_##_n_,512); \
		for(line = 0; line < 240; line++) \
			tilemap_set_scrollx(tilemap_##_n_, \
								(line + scrolly_##_n_ - sdy) & 511, \
								scrollx_##_n_ + cave_vram_##_n_[(0x1000+((line*4)&0x7ff))/2] ); \
	} \
	else \
	{ \
		tilemap_set_scroll_rows(tilemap_##_n_,1); \
		tilemap_set_scrollx(tilemap_##_n_, 0, scrollx_##_n_ ); \
	}


//ks s
#if 1 /* 1:old tilemap manager  0:new tilemap manager */

#define CAVE_TILEMAP_DRAW(_n_) \
INLINE void cave_tilemap_draw_##_n_(struct osd_bitmap *bitmap, int priority, int delta) \
{ \
	if (cave_vctrl_##_n_[1] & 0x4000) \
	{ \
		struct rectangle clip; \
		int sy, startline, endline, vramdata0, vramdata1; \
		int sdy = (cave_videoregs[ 1 ] & 0x8000) ? -0x100 : -0x11; \
 \
		clip.min_x = Machine->visible_area.min_x; \
		clip.max_x = Machine->visible_area.max_x; \
		for(startline = 0; startline < 240;) \
		{ \
			vramdata0 = vramdata1 = cave_vram_##_n_[(0x1000+(((2+((startline+delta)*4))&0x7ff)))/2]; \
			for(endline = startline + 1; endline < 241; endline++) \
				if((++vramdata1) != cave_vram_##_n_[(0x1000+(((2+((endline+delta)*4))&0x7ff)))/2]) break; \
 \
			sy = scrolly_##_n_ + vramdata0 - startline; \
			tilemap_set_scrolly(tilemap_##_n_, 0, sy ); \
			if (cave_vctrl_##_n_[0] & 0x4000) \
			{ \
				int line; \
				tilemap_set_scroll_rows(tilemap_##_n_,512); \
				for(line = startline; line < endline; line++) \
					tilemap_set_scrollx(tilemap_##_n_, \
										(line + sy - sdy) & 511, \
										scrollx_##_n_ + cave_vram_##_n_[(0x1000+((line*4)&0x7ff))/2] ); \
			} \
			clip.min_y = startline; \
			clip.max_y = endline - 1; \
			tilemap_set_clip(tilemap_##_n_,&clip); \
			tilemap_draw(bitmap, tilemap_##_n_, priority,0); \
 \
			startline = endline; \
		} \
		tilemap_set_clip(tilemap_##_n_,&Machine->visible_area); \
	} \
	else \
	{ \
		tilemap_draw(bitmap, tilemap_##_n_, priority, 0); \
	} \
}

#else		//new tilemap manager

#define CAVE_TILEMAP_DRAW(_n_) \
INLINE void cave_tilemap_draw_##_n_(struct osd_bitmap *bitmap, int priority, int delta) \
{ \
	if (cave_vctrl_##_n_[1] & 0x4000) \
	{ \
		struct rectangle clip; \
		int sy, startline, endline, vramdata0, vramdata1; \
		int sdy = (cave_videoregs[ 1 ] & 0x8000) ? -0x100 : -0x11; \
 \
		clip.min_x = Machine->visible_area.min_x; \
		clip.max_x = Machine->visible_area.max_x; \
		for(startline = 0; startline < 240;) \
		{ \
			vramdata0 = vramdata1 = cave_vram_##_n_[(0x1000+(((2+((startline+delta)*4))&0x7ff)))/2]; \
			for(endline = startline + 1; endline < 241; endline++) \
				if((++vramdata1) != cave_vram_##_n_[(0x1000+(((2+((endline+delta)*4))&0x7ff)))/2]) break; \
 \
			sy = scrolly_##_n_ + vramdata0 - startline; \
			tilemap_set_scrolly(tilemap_##_n_, 0, sy ); \
			if (cave_vctrl_##_n_[0] & 0x4000) \
			{ \
				int line; \
				tilemap_set_scroll_rows(tilemap_##_n_,512); \
				for(line = startline; line < endline; line++) \
					tilemap_set_scrollx(tilemap_##_n_, \
										(line + sy - sdy) & 511, \
										scrollx_##_n_ + cave_vram_##_n_[(0x1000+((line*4)&0x7ff))/2] ); \
			} \
			clip.min_y = startline; \
			clip.max_y = endline - 1; \
			tilemap_set_clip(tilemap_##_n_,&clip); \
			tilemap_update(tilemap_##_n_); \
			tilemap_draw(bitmap, tilemap_##_n_, priority,0); \
 \
			startline = endline; \
		} \
		tilemap_set_clip(tilemap_##_n_,&Machine->visible_area); \
	} \
	else \
	{ \
		tilemap_draw(bitmap, tilemap_##_n_, priority, 0); \
	} \
}
#endif		//new tilemap manager
//ks e


CAVE_TILEMAP_DRAW(0)
CAVE_TILEMAP_DRAW(1)
CAVE_TILEMAP_DRAW(2)



#ifdef MAME_DEBUG
INLINE void screenrefresh_debug(struct osd_bitmap *bitmap, int delta)
{
	int pri;
	int layers_ctrl = -1;

	{//ksdeb
		static int rasflag0 = 0;
		int rasflag = 0;

		if (cave_vctrl_0[0] & 0x4000) rasflag = 1;
		if (cave_vctrl_0[1] & 0x4000) rasflag += 10;
		if(keyboard_pressed(KEYCODE_J))
		{
			char buf[80];
			sprintf(buf,"x:%04X y:%04X",cave_vctrl_0[0],cave_vctrl_0[1]);
			usrintf_showmessage(buf);
		}
		if (tilemap_1)
		{
			if (cave_vctrl_1[0] & 0x4000) rasflag += 100;
			if (cave_vctrl_1[1] & 0x4000) rasflag += 1000;
			if(keyboard_pressed(KEYCODE_K))
			{
				char buf[80];
				sprintf(buf,"x:%04X y:%04X",cave_vctrl_1[0],cave_vctrl_1[1]);
				usrintf_showmessage(buf);
			}
		}
		if (tilemap_2)
		{
			if (cave_vctrl_2[0] & 0x4000) rasflag += 10000;
			if (cave_vctrl_2[1] & 0x4000) rasflag += 100000;
			if(keyboard_pressed(KEYCODE_L))
			{
				char buf[80];
				sprintf(buf,"x:%04X y:%04X",cave_vctrl_2[0],cave_vctrl_2[1]);
				usrintf_showmessage(buf);
			}
		}


		if(rasflag)
		{
			char buf[80];
			sprintf(buf,"line effect : %06u",rasflag);
			usrintf_showmessage(buf);
			if(rasflag!=rasflag0)
			{
				logerror("line effect : %06u\n",rasflag);
				rasflag0 = rasflag;
			}
		}
	}//ksdeb

	if ( keyboard_pressed(KEYCODE_Z) || keyboard_pressed(KEYCODE_X) || keyboard_pressed(KEYCODE_C) ||
	     keyboard_pressed(KEYCODE_V) || keyboard_pressed(KEYCODE_B) )
	{
		int msk = 0, val = 0;

		if (keyboard_pressed(KEYCODE_X))	val = 1;	// priority 0 only
		if (keyboard_pressed(KEYCODE_C))	val = 2;	// ""       1
		if (keyboard_pressed(KEYCODE_V))	val = 4;	// ""       2
		if (keyboard_pressed(KEYCODE_B))	val = 8;	// ""       3

		if (keyboard_pressed(KEYCODE_Z))	val = 1|2|4|8;	// All of the above priorities

		if (keyboard_pressed(KEYCODE_Q))	msk |= val << 0;	// for layer 0
		if (keyboard_pressed(KEYCODE_W))	msk |= val << 4;	// for layer 1
		if (keyboard_pressed(KEYCODE_E))	msk |= val << 8;	// for layer 2
		if (keyboard_pressed(KEYCODE_A))	msk |= val << 12;	// for sprites
		if (msk != 0) layers_ctrl &= msk;

		{
			char buf[80];
			sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",
					cave_videoregs[0], cave_videoregs[1],
					cave_videoregs[2], cave_videoregs[3],
					cave_videoregs[4], cave_videoregs[5],
					cave_videoregs[6], cave_videoregs[7] );
			usrintf_showmessage(buf);
		}
	}

	for ( pri = 0; pri < 4; pri++ )
	{
		if ((layers_ctrl&(1<<(pri+12))))			(*cave_sprite_draw)( pri );
		if ((layers_ctrl&(1<<(pri+0))))				cave_tilemap_draw_0(bitmap, pri, delta);
		if ((layers_ctrl&(1<<(pri+4)))&&tilemap_1)	cave_tilemap_draw_1(bitmap, pri, delta);
		if ((layers_ctrl&(1<<(pri+8)))&&tilemap_2)	cave_tilemap_draw_2(bitmap, pri, delta);
	}
}
#endif



/**************************************************************************/

void donpachi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	{		/* scroll hack */
		UINT32 addr = (cave_wkram[0x13CC/2]<<15)+(cave_wkram[0x13CE/2]>>1);
		UINT16 *scroll_delta;

		if(addr&(0xfff80000/2))
		{
			scroll_delta = &cave_wkram[addr&(0xffff/2)];
		}
		else
			scroll_delta = &cave_prgrom[addr];

		if(!((scrollx_0=cave_vctrl_0[0]) & 0x4000)) scrollx_0 = cave_wkram[0x13A8/2] + scroll_delta[0];
													scrolly_0 = cave_wkram[0x13AA/2] + scroll_delta[1];
													scrollx_1 = cave_wkram[0x13AC/2] + scroll_delta[2];
		if(!((scrolly_1=cave_vctrl_1[1]) & 0x4000)) scrolly_1 = cave_wkram[0x13AE/2] + scroll_delta[3];
													scrollx_2 = cave_wkram[0x13B0/2] + scroll_delta[4];
													scrolly_2 = cave_wkram[0x13B2/2] + scroll_delta[5];
	}

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );
	CAVE_TILEMAP_SET_SCROLLX(0)

	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, scrollx_1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );


	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x0f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	cave_tilemap_draw_1(bitmap, 0, 0);
	tilemap_draw(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	cave_tilemap_draw_1(bitmap, 1, 0);
	tilemap_draw(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	cave_tilemap_draw_1(bitmap, 2, 0);
	tilemap_draw(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	cave_tilemap_draw_1(bitmap, 3, 0);
	tilemap_draw(bitmap, tilemap_2, 3,0);
#endif
}


void ddonpach_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	{		/* scroll hack */
		UINT32 addr = (cave_wkram[0x13DC/2]<<15)+(cave_wkram[0x13DE/2]>>1);
		UINT16 *scroll_delta;

		if(addr&(0xfff00000/2))
		{
			scroll_delta = &cave_wkram[addr&(0xffff/2)];
		}
		else
			scroll_delta = &cave_prgrom[addr];

		scrollx_0 = cave_wkram[0x13B8/2] + scroll_delta[0];
		scrolly_0 = cave_wkram[0x13BA/2] + scroll_delta[1];
		scrollx_1 = cave_wkram[0x13BC/2] + scroll_delta[2];
		scrolly_1 = cave_wkram[0x13BE/2] + scroll_delta[3];
		scrollx_2 = cave_wkram[0x13C0/2] + scroll_delta[4];
		scrolly_2 = cave_wkram[0x13C2/2] + scroll_delta[5];
	}

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrollx(tilemap_0, 0, scrollx_0 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );

	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, scrollx_1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x4000 + 0x0f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	tilemap_draw(bitmap, tilemap_1, 0,0);
	tilemap_draw(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	tilemap_draw(bitmap, tilemap_1, 1,0);
	tilemap_draw(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	tilemap_draw(bitmap, tilemap_1, 2,0);
	tilemap_draw(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	tilemap_draw(bitmap, tilemap_1, 3,0);
	tilemap_draw(bitmap, tilemap_2, 3,0);
#endif
}


void esprade_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	{		/* scroll hack */
		UINT32 addr = (cave_wkram[0x13D6/2]<<15)+(cave_wkram[0x13D8/2]>>1);
		UINT16 *scroll_delta;

		if(addr&(0xfff00000/2))
		{
			scroll_delta = &cave_wkram[addr&(0xffff/2)];
		}
		else
			scroll_delta = &cave_prgrom[addr];

		scrollx_0 = cave_wkram[0x13B2/2] + scroll_delta[0];
		scrolly_0 = cave_wkram[0x13B4/2] + scroll_delta[1];
		scrollx_1 = cave_wkram[0x13B6/2] + scroll_delta[2];
		scrolly_1 = cave_wkram[0x13B8/2] + scroll_delta[3];
		scrollx_2 = cave_wkram[0x13BA/2] + scroll_delta[4];
		scrolly_2 = cave_wkram[0x13BC/2] + scroll_delta[5];
	}

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrollx(tilemap_0, 0, scrollx_0 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );

	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, scrollx_1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	tilemap_draw(bitmap, tilemap_1, 0,0);
	tilemap_draw(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	tilemap_draw(bitmap, tilemap_1, 1,0);
	tilemap_draw(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	tilemap_draw(bitmap, tilemap_1, 2,0);
	tilemap_draw(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	tilemap_draw(bitmap, tilemap_1, 3,0);
	tilemap_draw(bitmap, tilemap_2, 3,0);
#endif
}


void guwange_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int delta;

	{		/* scroll hack */
		UINT32 addr = (cave_wkram[0x1584/2]<<15)+(cave_wkram[0x1586/2]>>1);
		UINT16 *scroll_delta;

		if(addr&(0xfff00000/2))
		{
			scroll_delta = &cave_wkram[addr&(0xffff/2)];
		}
		else
			scroll_delta = &cave_prgrom[addr];

													scrollx_0 = cave_wkram[0x1560/2] + scroll_delta[0];
													scrolly_0 = cave_wkram[0x1562/2] + scroll_delta[1];
													scrollx_1 = cave_wkram[0x1564/2] + scroll_delta[2];
		if(!((scrolly_1=cave_vctrl_1[1]) & 0x4000))	scrolly_1 = cave_wkram[0x1566/2] + scroll_delta[3];
													scrollx_2 = cave_wkram[0x1568/2] + scroll_delta[4];
		if(!((scrolly_2=cave_vctrl_2[1]) & 0x4000))	scrolly_2 = cave_wkram[0x156A/2] + scroll_delta[5];
	}

	if(cave_videoregs[ 1 ] & 0x8000)
	{
		delta = 1;
		tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | TILEMAP_FLIPY );
	}
	else
	{
		delta = -1;
		tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) );
	}

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrollx(tilemap_0, 0, scrollx_0 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );

	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, scrollx_1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, delta);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	cave_tilemap_draw_1(bitmap, 0, delta);
	cave_tilemap_draw_2(bitmap, 0, delta);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	cave_tilemap_draw_1(bitmap, 1, delta);
	cave_tilemap_draw_2(bitmap, 1, delta);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	cave_tilemap_draw_1(bitmap, 2, delta);
	cave_tilemap_draw_2(bitmap, 2, delta);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	cave_tilemap_draw_1(bitmap, 3, delta);
	cave_tilemap_draw_2(bitmap, 3, delta);
#endif
}


void dfeveron_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	{		/* scroll hack */
		UINT32 addr = (cave_wkram[0x1762/2]<<15)+(cave_wkram[0x1764/2]>>1);
		UINT16 *scroll_delta;

		if(addr&(0xfff00000/2))
		{
			scroll_delta = &cave_wkram[addr&(0xffff/2)];
		}
		else
			scroll_delta = &cave_prgrom[addr];

		if(!((scrollx_0=cave_vctrl_0[0]) & 0x4000)) scrollx_0 = cave_wkram[0x173E/2] + scroll_delta[0] + ((cave_wkram[0xECE2/2]>>6)&0x1FF);
		if(!((scrolly_0=cave_vctrl_0[1]) & 0x4000)) scrolly_0 = cave_wkram[0x1740/2] + scroll_delta[1] + ((cave_wkram[0xECE4/2]>>6)&0x1FF);
		if(!((scrollx_1=cave_vctrl_1[0]) & 0x4000)) scrollx_1 = cave_wkram[0x1742/2] + scroll_delta[2] + ((cave_wkram[0xECE2/2]>>6)&0x1FF);
		if(!((scrolly_1=cave_vctrl_1[1]) & 0x4000)) scrolly_1 = cave_wkram[0x1744/2] + scroll_delta[3] + ((cave_wkram[0xECE4/2]>>6)&0x1FF);
	}

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );
	CAVE_TILEMAP_SET_SCROLLX(0)

	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );
	CAVE_TILEMAP_SET_SCROLLX(1)

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	cave_tilemap_draw_0(bitmap, 0, 0);
	cave_tilemap_draw_1(bitmap, 0, 0);
	(*cave_sprite_draw)( 1 );
	cave_tilemap_draw_0(bitmap, 1, 0);
	cave_tilemap_draw_1(bitmap, 1, 0);
	(*cave_sprite_draw)( 2 );
	cave_tilemap_draw_0(bitmap, 2, 0);
	cave_tilemap_draw_1(bitmap, 2, 0);
	(*cave_sprite_draw)( 3 );
	cave_tilemap_draw_0(bitmap, 3, 0);
	cave_tilemap_draw_1(bitmap, 3, 0);
#endif
}


void uopoko_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	scrollx_0=cave_vctrl_0[0];
	scrolly_0=cave_vctrl_0[1];

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );
	CAVE_TILEMAP_SET_SCROLLX(0)

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
#endif
}


void hotdogst_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	scrollx_0=cave_vctrl_0[0];
	scrolly_0=cave_vctrl_0[1];
	scrollx_1=cave_vctrl_1[0];
	scrolly_1=cave_vctrl_1[1];
	scrollx_2=cave_vctrl_2[0];
	scrolly_2=cave_vctrl_2[1];

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

//	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrollx(tilemap_0, 0, scrollx_0 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );

//	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, scrollx_1 );
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );

//	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();			//ks

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();			//ks
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	tilemap_draw(bitmap, tilemap_1, 0,0);
	tilemap_draw(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	tilemap_draw(bitmap, tilemap_1, 1,0);
	tilemap_draw(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	tilemap_draw(bitmap, tilemap_1, 2,0);
	tilemap_draw(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	tilemap_draw(bitmap, tilemap_1, 3,0);
	tilemap_draw(bitmap, tilemap_2, 3,0);
#endif
}

void mazinger_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

//	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrollx(tilemap_0, 0, cave_vctrl_0[0] );
	tilemap_set_scrolly(tilemap_0, 0, cave_vctrl_0[1] );

//	tilemap_set_enable( tilemap_1,    cave_vctrl_1[2] & 1 );
	tilemap_set_scrollx(tilemap_1, 0, cave_vctrl_1[0] );
	tilemap_set_scrolly(tilemap_1, 0, cave_vctrl_1[1] );

	tilemap_update(ALL_TILEMAPS);

	//palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

#ifdef MAME_DEBUG
	palette_recalc();
#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw(bitmap, tilemap_0, 0,0);
	tilemap_draw(bitmap, tilemap_1, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw(bitmap, tilemap_0, 1,0);
	tilemap_draw(bitmap, tilemap_1, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw(bitmap, tilemap_0, 2,0);
	tilemap_draw(bitmap, tilemap_1, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw(bitmap, tilemap_0, 3,0);
	tilemap_draw(bitmap, tilemap_1, 3,0);
#endif
}



/***************************************************************************

								Vh_Stop

***************************************************************************/
void cave_vh_stop(void)
{
	free(sprite_cave);
	sprite_cave = NULL;
	free(sprite_zbuf);
	sprite_zbuf = NULL;
}
