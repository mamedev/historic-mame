#ifndef DECLARE
/***************************************************************************

							  -= Cave Games =-

				driver by	Luca Elia (l.elia@tin.it)


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
	int tile_width, tile_height;
	int total_width, total_height;	/* in screen coordinates */
	int x, y, xcount0, ycount0;
	int zoomx_re, zoomy_re;
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
	SET_TILE_INFO( _n_ ,  code & 0x00ffffff , (code & 0x3f000000) >> (32-8), 0 ); \
	tile_info.priority = (code & 0xc0000000)>> (32-2); \
} \
\
WRITE16_HANDLER( cave_vram_##_n_##_w ) \
{ \
	if((cave_vram_##_n_[offset] & ~mem_mask)==(data & ~mem_mask)) return; \
	COMBINE_DATA(&cave_vram_##_n_[offset]); \
	if ( (offset/2) < DIM_NX * DIM_NY ) \
		tilemap_mark_tile_dirty(tilemap_##_n_, offset/2 ); \
} \
\
WRITE16_HANDLER( cave_vram_##_n_##_8x8_w ) \
{ \
	offset %= ( (DIM_NX * 2) * (DIM_NY * 2) * 2); /* mirrored RAM */ \
	if((cave_vram_##_n_[offset] & ~mem_mask)==(data & ~mem_mask)) return; \
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
	data16_t      *finish			=	source + ((spriteram_size/2) / 2);

	struct sprite_cave *sprite			=	sprite_cave;

	int	glob_flipx	=	cave_videoregs[ 0 ] & 0x8000;
	int	glob_flipy	=	cave_videoregs[ 1 ] & 0x8000;

	int max_x		=	Machine->drv->screen_width;
	int max_y		=	Machine->drv->screen_height;

	for (; source < finish; source+=8 )
	{
		int x,y,attr,code,zoomx,zoomy,size,flipx,flipy;
		int total_width_f,total_height_f;

		if (cave_spritetype == 2)	/* Hot Dog Storm */
		{
			x		=		(source[ 0 ] & 0x3ff) << 8;
			y		=		(source[ 1 ] & 0x3ff) << 8;
		}
		else						/* all others */
		{
			x		=		source[ 0 ] << 2;
			y		=		source[ 1 ] << 2;
		}
		attr		=		source[ 2 ];
		code		=		source[ 3 ] + ((attr & 3) << 16);
		zoomx		=		source[ 4 ];
		zoomy		=		source[ 5 ];
		size		=		source[ 6 ];

		sprite->tile_width		=	( (size >> 8) & 0x1f ) * 16;
		sprite->tile_height		=	( (size >> 0) & 0x1f ) * 16;
		sprite->pen_data		=	base_gfx + (16*16) * code;

		/* Bound checking */
		if (((sprite->pen_data + sprite->tile_width * sprite->tile_height - 1) >= gfx_max ) ||
		    !sprite->tile_width || !sprite->tile_height ){continue;}

		flipx		=		attr & 0x0008;
		flipy		=		attr & 0x0004;

		sprite->total_width		=	(total_width_f  = sprite->tile_width  * zoomx) / 0x100;
		sprite->total_height	=	(total_height_f = sprite->tile_height * zoomy) / 0x100;

		if (sprite->total_width <= 1)
		{
			sprite->total_width  = 1;
			sprite->zoomx_re = sprite->tile_width << 16;
			sprite->xcount0 = sprite->zoomx_re / 2;
			x -= 0x80;
		}
		else
		{
			sprite->zoomx_re = 0x1000000 / zoomx;
			sprite->xcount0 = sprite->zoomx_re - 1;
		}

		if (sprite->total_height <= 1)
		{
			sprite->total_height = 1;
			sprite->zoomy_re = sprite->tile_height << 16;
			sprite->ycount0 = sprite->zoomy_re / 2;
			y -= 0x80;
		}
		else
		{
			sprite->zoomy_re = 0x1000000 / zoomy;
			sprite->ycount0 = sprite->zoomy_re - 1;
		}

		if (flipx && (zoomx != 0x100)) x += (sprite->tile_width<<8) - total_width_f - 0x80;
		if (flipy && (zoomy != 0x100)) y += (sprite->tile_height<<8) - total_height_f - 0x80;

		x >>= 8;
		y >>= 8;

		if (x > 0x1FF)	x -= 0x400;
		if (y > 0x1FF)	y -= 0x400;

		if (x + sprite->total_width<=0 || x>=max_x || y + sprite->total_height<=0 || y>=max_y )
		{continue;}

		sprite->priority		=	(attr & 0x0030) >> 4;
		sprite->flags			=	SPRITE_VISIBLE_CAVE;
		sprite->line_offset		=	sprite->tile_width;
		sprite->pal_data		=	base_pal + (attr & 0x3f00);	// first 0x4000 colors

		if (glob_flipx)	{ x = max_x - x - sprite->total_width;	flipx = !flipx; }
		if (glob_flipy)	{ y = max_y - y - sprite->total_height;	flipy = !flipy; }

		sprite->x				=	x;
		sprite->y				=	y;

		if (flipx)	sprite->flags |= SPRITE_FLIPX_CAVE;
		if (flipy)	sprite->flags |= SPRITE_FLIPY_CAVE;

		sprite++;
	}
	num_sprites = sprite - sprite_cave;
}

static void get_sprite_info_donpachi(void)
{
	const int region				=	REGION_GFX4;

	const UINT32         *base_pal	=	Machine->remapped_colortable + 0;
	const unsigned char  *base_gfx	=	memory_region(region);
	const unsigned char  *gfx_max	=	base_gfx + memory_region_length(region);

	data16_t      *source			=	spriteram16 + ((spriteram_size/2) / 2) * (cave_videoregs[ 4 ] & 1);
	data16_t      *finish			=	source + ((spriteram_size/2) / 2);

	struct sprite_cave *sprite			=	sprite_cave;

	int	glob_flipx	=	cave_videoregs[ 0 ] & 0x8000;
	int	glob_flipy	=	cave_videoregs[ 1 ] & 0x8000;

	int max_x		=	Machine->drv->screen_width;
	int max_y		=	Machine->drv->screen_height;

	for (; source < finish; source+=8 )
	{
		int x,y,attr,code,size,flipx,flipy;

		attr		=		source[ 0 ];
		code		=		source[ 1 ] + ((attr & 3) << 16);
		x			=		source[ 2 ] & 0x3FF;
		y			=		source[ 3 ] & 0x3FF;
		size		=		source[ 4 ];

		sprite->tile_width		=	sprite->total_width		=	( (size >> 8) & 0x1f ) * 16;
		sprite->tile_height		=	sprite->total_height	=	( (size >> 0) & 0x1f ) * 16;
		sprite->pen_data		=	base_gfx + (16*16) * code;
		if (x > 0x1FF)	x -= 0x400;
		if (y > 0x1FF)	y -= 0x400;

		/* Bound checking */
		if (((sprite->pen_data + sprite->tile_width * sprite->tile_height - 1) >= gfx_max ) ||
		    !sprite->tile_width || !sprite->tile_height ||
			x + sprite->total_width<=0 || x>=max_x || y + sprite->total_height<=0 || y>=max_y )
		{continue;}

		flipx		=		attr & 0x0008;
		flipy		=		attr & 0x0004;

		sprite->priority		=	(attr & 0x0030) >> 4;
		sprite->flags			=	SPRITE_VISIBLE_CAVE;
		sprite->line_offset		=	sprite->tile_width;
		sprite->pal_data		=	base_pal + (attr & 0x3f00);	// first 0x4000 colors

		if (glob_flipx)	{ x = max_x - x - sprite->total_width;	flipx = !flipx; }
		if (glob_flipy)	{ y = max_y - y - sprite->total_height;	flipy = !flipy; }

		sprite->x				=	x;
		sprite->y				=	y;

		if (flipx)	sprite->flags |= SPRITE_FLIPX_CAVE;
		if (flipy)	sprite->flags |= SPRITE_FLIPY_CAVE;

		sprite++;
	}
	num_sprites = sprite - sprite_cave;
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
			SWAP(sprite->zoomx_re,sprite->zoomy_re)
			SWAP(sprite->xcount0,sprite->ycount0)
			SWAP(sprite->tile_width,sprite->tile_height)

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
/*
			if( (sprite->flags&SPRITE_VISIBLE_CAVE)
			  && sprite->x + sprite->total_width>blit.clip_left && sprite->x<blit.clip_right &&
				sprite->y + sprite->total_height>blit.clip_top && sprite->y<blit.clip_bottom
				)
*/
			{
				sprite_table[sprite->priority][i[sprite->priority]++] = sprite;

				if(Machine->color_depth == 8)	/* mark sprite colors */
				{
					int j,offs = sprite->pal_data - Machine->remapped_colortable;
					if(Machine->drv->total_colors == 0x8000)	/* 8 Bit Sprites */
					{
						if(palette_used_colors[offs+1] != PALETTE_COLOR_USED)
							for(j=1;j<256;j++)
								palette_used_colors[offs+j] = PALETTE_COLOR_USED;
					}
					else										/* 4 Bit Sprites */
					{
						if(palette_used_colors[(offs>>4)+1] != PALETTE_COLOR_USED)
							for(j=1;j<16;j++)
								palette_used_colors[(offs>>4)+j] = PALETTE_COLOR_USED;
					}
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
	int xcount0 = 0x10000 + sprite->xcount0, ycount0 = 0x10000 + sprite->ycount0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 += (x1-blit.clip_right)* sprite->zoomx_re;
			x1 = blit.clip_right;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1--;}
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 += (blit.clip_left-x1)*sprite->zoomx_re;
			x1 = blit.clip_left;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1++;}
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
			ycount0 += (y1-blit.clip_bottom)*sprite->zoomy_re;
			y1 = blit.clip_bottom;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1--;}
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 += (blit.clip_top-y1)*sprite->zoomy_re;
			y1 = blit.clip_top;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1++;}
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data -1 -sprite->line_offset;
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

				if (xcount&0xffff0000){
					ycount = ycount0;
					pen_data+=sprite->line_offset*(xcount>>16);
					xcount &= 0xffff;
					source = pen_data;
					dest1 = &dest[x];
					for( y=y1; y!=y2; y+=dy ){
						if (ycount&0xffff0000){
							source+=ycount>>16;
							ycount &= 0xffff;
							pen = *source;
							if (pen) *dest1 = pal_data[pen];
						}
						ycount+= sprite->zoomy_re;
						dest1 += pitch;
					}
				}
				xcount += sprite->zoomx_re;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount;
				const unsigned char *source;

				if (ycount&0xffff0000){
					xcount = xcount0;
					pen_data+=sprite->line_offset*(ycount>>16);
					ycount &= 0xffff;
					source = pen_data;
					for( x=x1; x!=x2; x+=dx ){
						if (xcount&0xffff0000){
							source+=xcount>>16;
							xcount &= 0xffff;
							pen = *source;
							if (pen) dest[x] = pal_data[pen];
						}
						xcount += sprite->zoomx_re;
					}
				}
				ycount += sprite->zoomy_re;
				dest += pitch;
			}
		}
	}
}


static void do_blit_zoom16_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0x10000 + sprite->xcount0, ycount0 = 0x10000 + sprite->ycount0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 += (x1-blit.clip_right)* sprite->zoomx_re;
			x1 = blit.clip_right;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1--;}
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 += (blit.clip_left-x1)*sprite->zoomx_re;
			x1 = blit.clip_left;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1++;}
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
			ycount0 += (y1-blit.clip_bottom)*sprite->zoomy_re;
			y1 = blit.clip_bottom;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1--;}
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 += (blit.clip_top-y1)*sprite->zoomy_re;
			y1 = blit.clip_top;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1++;}
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data -1 -sprite->line_offset;
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

				if (xcount&0xffff0000){
					ycount = ycount0;
					pen_data+=sprite->line_offset*(xcount>>16);
					xcount &= 0xffff;
					source = pen_data;
					dest1 = &dest[x];
					zbf1 = &zbf[x];
					for( y=y1; y!=y2; y+=dy ){
						if (ycount&0xffff0000){
							source+=ycount>>16;
							ycount &= 0xffff;
							pen = *source;
							if (pen && (*zbf1<=pri_sp)){
								*dest1 = pal_data[pen];
								*zbf1 = pri_sp;
							}
						}
						ycount+= sprite->zoomy_re;
						dest1 += pitch;
						zbf1 += pitchz;
					}
				}
				xcount += sprite->zoomx_re;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount;
				const unsigned char *source;

				if (ycount&0xffff0000){
					xcount = xcount0;
					pen_data+=sprite->line_offset*(ycount>>16);
					ycount &= 0xffff;
					source = pen_data;
					for( x=x1; x!=x2; x+=dx ){
						if (xcount&0xffff0000){
							source+=xcount>>16;
							xcount &= 0xffff;
							pen = *source;
							if (pen && (zbf[x]<=pri_sp)){
								dest[x] = pal_data[pen];
								zbf[x] = pri_sp;
							}
						}
						xcount += sprite->zoomx_re;
					}
				}
				ycount += sprite->zoomy_re;
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

static void do_blit_zoom8_cave( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0x10000 + sprite->xcount0, ycount0 = 0x10000 + sprite->ycount0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 += (x1-blit.clip_right)* sprite->zoomx_re;
			x1 = blit.clip_right;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1--;}
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 += (blit.clip_left-x1)*sprite->zoomx_re;
			x1 = blit.clip_left;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1++;}
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
			ycount0 += (y1-blit.clip_bottom)*sprite->zoomy_re;
			y1 = blit.clip_bottom;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1--;}
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 += (blit.clip_top-y1)*sprite->zoomy_re;
			y1 = blit.clip_top;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1++;}
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data -1 -sprite->line_offset;
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

				if (xcount&0xffff0000){
					ycount = ycount0;
					pen_data+=sprite->line_offset*(xcount>>16);
					xcount &= 0xffff;
					source = pen_data;
					dest1 = &dest[x];
					for( y=y1; y!=y2; y+=dy ){
						if (ycount&0xffff0000){
							source+=ycount>>16;
							ycount &= 0xffff;
							pen = *source;
							if (pen) *dest1 = pal_data[pen];
						}
						ycount+= sprite->zoomy_re;
						dest1 += pitch;
					}
				}
				xcount += sprite->zoomx_re;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount;
				const unsigned char *source;

				if (ycount&0xffff0000){
					xcount = xcount0;
					pen_data+=sprite->line_offset*(ycount>>16);
					ycount &= 0xffff;
					source = pen_data;
					for( x=x1; x!=x2; x+=dx ){
						if (xcount&0xffff0000){
							source+=xcount>>16;
							xcount &= 0xffff;
							pen = *source;
							if (pen) dest[x] = pal_data[pen];
						}
						xcount += sprite->zoomx_re;
					}
				}
				ycount += sprite->zoomy_re;
				dest += pitch;
			}
		}
	}
}


static void do_blit_zoom8_cave_zb( const struct sprite_cave *sprite ){
	/*	assumes SPRITE_LIST_RAW_DATA flag is set */

	int x1,x2, y1,y2, dx,dy;
	int xcount0 = 0x10000 + sprite->xcount0, ycount0 = 0x10000 + sprite->ycount0;

	if( sprite->flags & SPRITE_FLIPX_CAVE ){
		x2 = sprite->x;
		x1 = x2+sprite->total_width;
		dx = -1;
		if( x2<blit.clip_left ) x2 = blit.clip_left;
		if( x1>blit.clip_right ){
			xcount0 += (x1-blit.clip_right)* sprite->zoomx_re;
			x1 = blit.clip_right;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1--;}
		}
		if( x2>=x1 ) return;
		x1--; x2--;
	}
	else {
		x1 = sprite->x;
		x2 = x1+sprite->total_width;
		dx = 1;
		if( x1<blit.clip_left ){
			xcount0 += (blit.clip_left-x1)*sprite->zoomx_re;
			x1 = blit.clip_left;
			while((xcount0&0xffff)>=sprite->zoomx_re){xcount0 += sprite->zoomx_re; x1++;}
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
			ycount0 += (y1-blit.clip_bottom)*sprite->zoomy_re;
			y1 = blit.clip_bottom;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1--;}
		}
		if( y2>=y1 ) return;
		y1--; y2--;
	}
	else {
		y1 = sprite->y;
		y2 = y1+sprite->total_height;
		dy = 1;
		if( y1<blit.clip_top ){
			ycount0 += (blit.clip_top-y1)*sprite->zoomy_re;
			y1 = blit.clip_top;
			while((ycount0&0xffff)>=sprite->zoomy_re){ycount0 += sprite->zoomy_re; y1++;}
		}
		if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;
		if( y1>=y2 ) return;
	}

	{
		const unsigned char *pen_data = sprite->pen_data -1 -sprite->line_offset;
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

				if (xcount&0xffff0000){
					ycount = ycount0;
					pen_data+=sprite->line_offset*(xcount>>16);
					xcount &= 0xffff;
					source = pen_data;
					dest1 = &dest[x];
					zbf1 = &zbf[x];
					for( y=y1; y!=y2; y+=dy ){
						if (ycount&0xffff0000){
							source+=ycount>>16;
							ycount &= 0xffff;
							pen = *source;
							if (pen && (*zbf1<=pri_sp)){
								*dest1 = pal_data[pen];
								*zbf1 = pri_sp;
							}
						}
						ycount+= sprite->zoomy_re;
						dest1 += pitch;
						zbf1 += pitchz;
					}
				}
				xcount += sprite->zoomx_re;
			}
		}
		else {
			for( y=y1; y!=y2; y+=dy ){
				int xcount;
				const unsigned char *source;

				if (ycount&0xffff0000){
					xcount = xcount0;
					pen_data+=sprite->line_offset*(ycount>>16);
					ycount &= 0xffff;
					source = pen_data;
					for( x=x1; x!=x2; x+=dx ){
						if (xcount&0xffff0000){
							source+=xcount>>16;
							xcount &= 0xffff;
							pen = *source;
							if (pen && (zbf[x]<=pri_sp)){
								dest[x] = pal_data[pen];
								zbf[x] = pri_sp;
							}
						}
						xcount += sprite->zoomx_re;
					}
				}
				ycount += sprite->zoomy_re;
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


/***************************************************************************

								Screen Drawing

***************************************************************************/

struct tilemap_mask {
	struct osd_bitmap *bitmask;
	int line_offset;
	UINT8 *data;
	UINT8 **data_row;
};

struct tilemap {
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	int *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	int logical_flip_to_cached_flip[4];

	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int memory_offset );

	UINT32 max_memory_offset;
	UINT32 num_tiles;
	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;
	UINT32 cached_tile_width, cached_tile_height;
	UINT32 cached_width, cached_height;

	struct cached_tile_info *cached_tile_info;

	int dx, dx_if_flipped;
	int dy, dy_if_flipped;
	int scrollx_delta, scrolly_delta;

	int enable;
	int attributes;

	int type;
	int transparent_pen;
	unsigned int transmask[4];

	int bNeedRender;

	void (*draw)( int, int );
	void (*draw_opaque)( int, int );
	void (*draw_alpha)( int, int );

	UINT8 *priority,	/* priority for each tile */
		**priority_row;

	UINT8 *visible; /* boolean flag for each tile */

	UINT8 *dirty_vram; /* boolean flag for each tile */

	UINT8 *dirty_pixels;

	int scroll_rows, scroll_cols;
	int *rowscroll, *colscroll;

	int orientation;
	int clip_left,clip_right,clip_top,clip_bottom;

	UINT16 tile_depth, tile_granularity;
	UINT8 *tile_dirty_map;

	/* cached color data */
	struct osd_bitmap *pixmap;
	int pixmap_line_offset;

	struct tilemap_mask *foreground;
	/* for transparent layers, or the front half of a split layer */

	struct tilemap_mask *background;
	/* for the back half of a split layer */

	struct tilemap *next; /* resource tracking */
};

enum {
	TILE_TRANSPARENT,
	TILE_MASKED,
	TILE_OPAQUE
};



/***********************************************************************************/
static struct {
	int clip_left, clip_top, clip_right, clip_bottom;
	int source_width, source_height;
	int dest_line_offset,source_line_offset,mask_line_offset;
	int dest_row_offset,source_row_offset,mask_row_offset;
	struct osd_bitmap *screen, *pixmap, *bitmask;
	UINT8 **mask_data_row;
	UINT8 **priority_data_row;
	int tile_priority;
//d	int tilemap_priority_code;
} tmblt;


#define DEPTH 8
#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DATA_TYPE UINT8
#define DECLARE(function,args,body) static void function##8x8x8BPP args body
#include "vidhrdw/cave.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DATA_TYPE UINT8
#define DECLARE(function,args,body) static void function##16x16x8BPP args body
#include "vidhrdw/cave.c"

#undef DEPTH

#define DEPTH 16
#define TILE_WIDTH	8
#define TILE_HEIGHT	8
#define DATA_TYPE UINT16
#define blend blend16
#define blendbitmask blendbitmask16
#define DECLARE(function,args,body) static void function##8x8x16BPP args body
#include "vidhrdw/cave.c"

#define TILE_WIDTH	16
#define TILE_HEIGHT	16
#define DATA_TYPE UINT16
#define DECLARE(function,args,body) static void function##16x16x16BPP args body
#include "vidhrdw/cave.c"

#undef DEPTH

/*********************************************************************************/

static void cave_tilemap_set_scrollx( struct tilemap *tilemap, int which, int value, int valuedy ){
	value = tilemap->scrollx_delta-value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY ){
		if( tilemap->orientation & ORIENTATION_FLIP_X ){
			which = tilemap->scroll_cols-1 - which;
			valuedy = -valuedy;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		tilemap->colscroll[which] = (value&0xffff)|(valuedy<<16);
	}
	else {
		if( tilemap->orientation & ORIENTATION_FLIP_Y ){
			which = tilemap->scroll_rows-1 - which;
			valuedy = -valuedy;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		tilemap->rowscroll[which] = (value&0xffff)|(valuedy<<16);
	}
}

/*********************************************************************************/

static void tilemap_draw_x( struct osd_bitmap *dest, struct tilemap *tilemap, UINT32 flags, UINT32 priority ){
	int xpos,ypos;
profiler_mark(PROFILER_TILEMAP_DRAW);
	tilemap_get_pixmap( tilemap );

	if( tilemap->enable ){
		void (*draw)( int, int );

		int rows = tilemap->scroll_rows;
		const int *rowscroll = tilemap->rowscroll;
		int cols = tilemap->scroll_cols;
		const int *colscroll = tilemap->colscroll;

		int left = tilemap->clip_left;
		int right = tilemap->clip_right;
		int top = tilemap->clip_top;
		int bottom = tilemap->clip_bottom;

		int tile_height = tilemap->cached_tile_height;

		tmblt.screen = dest;
		tmblt.dest_line_offset = dest->line[1] - dest->line[0];
		tmblt.pixmap = tilemap->pixmap;
		tmblt.source_line_offset = tilemap->pixmap_line_offset;
		tmblt.bitmask = tilemap->foreground->bitmask;
		tmblt.mask_line_offset = tilemap->foreground->line_offset;
		tmblt.mask_data_row = tilemap->foreground->data_row;
		tmblt.mask_row_offset = tile_height*tmblt.mask_line_offset;

		if( dest->depth==16 || dest->depth==15 ){
			tmblt.dest_line_offset /= 2;
			tmblt.source_line_offset /= 2;
			if(tilemap->cached_tile_width==8){draw=draw8x8x16BPP;}
			else{draw=draw16x16x16BPP;}
		}
		else{
			if(tilemap->cached_tile_width==8){draw=draw8x8x8BPP;}
			else{draw=draw16x16x8BPP;}
		}

		tmblt.source_row_offset = tile_height*tmblt.source_line_offset;
		tmblt.dest_row_offset = tile_height*tmblt.dest_line_offset;

		tmblt.priority_data_row = tilemap->priority_row;
		tmblt.source_width = tilemap->cached_width;
		tmblt.source_height = tilemap->cached_height;
		tmblt.tile_priority = flags&0xf;

		if( rows == 1 && cols == 1 ){ /* XY scrolling playfield */
			int scrollx = rowscroll[0] % tmblt.source_width;
			int scrolly = colscroll[0] % tmblt.source_height;
	 		tmblt.clip_left = left;
	 		tmblt.clip_top = top;
	 		tmblt.clip_right = right;
	 		tmblt.clip_bottom = bottom;
			for(
				ypos = scrolly - tmblt.source_height;
				ypos < tmblt.clip_bottom;
				ypos += tmblt.source_height
			){
				for(
					xpos = scrollx - tmblt.source_width;
					xpos < tmblt.clip_right;
					xpos += tmblt.source_width
				){
					draw( xpos,ypos );
				}
			}
		}
		else if( rows == 1 ){ /* scrolling columns + horizontal scroll */
			int col = 0;
			int colwidth = (right-left) / cols;
			tmblt.clip_top = top;
			tmblt.clip_bottom = bottom;
			while( col < cols ){
				int cons = 1;
				int scrolly = colscroll[col];
				int scrollx = (rowscroll[0] + (scrolly>>16)) % tilemap->cached_width;
				while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;
				scrolly %= tmblt.source_height;
				tmblt.clip_left = left + col * colwidth;
				tmblt.clip_right = tmblt.clip_left + cons * colwidth;
				if (tmblt.clip_right > right) tmblt.clip_right = right;
				for(
					ypos = scrolly - tmblt.source_height;
					ypos < tmblt.clip_bottom;
					ypos += tmblt.source_height
				){
					draw( scrollx,ypos );
				}
				for(
					ypos = scrolly - tmblt.source_height;
					ypos < tmblt.clip_bottom;
					ypos += tmblt.source_height
				){
					draw( scrollx - tmblt.source_width,ypos );
				}
				col += cons;
			}
		}
		else if( cols == 1 ){ /* scrolling rows + vertical scroll */
			int row = 0;
			int rowheight = (bottom - top) / rows;
			tmblt.clip_left = left;
			tmblt.clip_right = right;
			while( row < rows ){
				int cons = 1;
				int scrollx = rowscroll[row];
				int scrolly = (colscroll[0] + (scrollx>>16)) % tilemap->cached_height;
				while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
				scrollx %= tmblt.source_width;
				tmblt.clip_top = top + row * rowheight;
				tmblt.clip_bottom = tmblt.clip_top + cons * rowheight;
				if (tmblt.clip_bottom > bottom) tmblt.clip_bottom = bottom;
				for(
					xpos = scrollx - tmblt.source_width;
					xpos < tmblt.clip_right;
					xpos += tmblt.source_width
				){
					draw( xpos,scrolly );
				}
				for(
					xpos = scrollx - tmblt.source_width;
					xpos < tmblt.clip_right;
					xpos += tmblt.source_width
				){
					draw( xpos,scrolly - tmblt.source_height );
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}
/*********************************************************************************/

static UINT16 scrollx_0,scrolly_0,scrollx_1,scrolly_1,scrollx_2,scrolly_2;

#define CAVE_TILEMAP_SET_SCROLLX(_n_) \
{ \
	const int num_lines = Machine->drv->screen_height; \
	int line; \
	switch (((cave_vctrl_##_n_[0] & 0x4000)>>14) + ((cave_vctrl_##_n_[1] & 0x4000)>>13)) \
	{ \
		case 1:		/* x */ \
			tilemap_set_scroll_rows(tilemap_##_n_,num_lines); \
			for(line = 0; line < num_lines; line++) \
				cave_tilemap_set_scrollx(tilemap_##_n_, line, \
								scrollx_##_n_ + cave_vram_##_n_[(0x1000+((line*4)&0x7ff))/2], \
								0 ); \
			break; \
		case 2:		/* y */ \
			tilemap_set_scroll_rows(tilemap_##_n_,num_lines); \
			for(line = 0; line < num_lines; line++) \
				cave_tilemap_set_scrollx(tilemap_##_n_, line, \
								scrollx_##_n_, \
								line - cave_vram_##_n_[(0x1000+(((2+((line+delta)*4))&0x7ff)))/2] ); \
			break; \
		case 3:		/* x,y */ \
			tilemap_set_scroll_rows(tilemap_##_n_,num_lines); \
			for(line = 0; line < num_lines; line++) \
				cave_tilemap_set_scrollx(tilemap_##_n_, line, \
								scrollx_##_n_ + cave_vram_##_n_[(0x1000+((line*4)&0x7ff))/2], \
								line - cave_vram_##_n_[(0x1000+(((2+((line+delta)*4))&0x7ff)))/2] ); \
			break; \
		default: \
		case 0: \
			tilemap_set_scroll_rows(tilemap_##_n_,1); \
			tilemap_set_scrollx(tilemap_##_n_, 0, scrollx_##_n_ ); \
	} \
}


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
		if ((layers_ctrl&(1<<(pri+0))))				tilemap_draw_x(bitmap, tilemap_0, pri,0);
		if ((layers_ctrl&(1<<(pri+4)))&&tilemap_1)	tilemap_draw_x(bitmap, tilemap_1, pri,0);
		if ((layers_ctrl&(1<<(pri+8)))&&tilemap_2)	tilemap_draw_x(bitmap, tilemap_2, pri,0);
	}
}
#endif


/**************************************************************************/

void donpachi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int delta = 0;

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
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );
	CAVE_TILEMAP_SET_SCROLLX(1)

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrollx(tilemap_2, 0, scrollx_2 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );


	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x0f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	tilemap_draw_x(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	tilemap_draw_x(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	tilemap_draw_x(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
	tilemap_draw_x(bitmap, tilemap_2, 3,0);
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

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x4000 + 0x0f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	tilemap_draw_x(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	tilemap_draw_x(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	tilemap_draw_x(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
	tilemap_draw_x(bitmap, tilemap_2, 3,0);
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

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	tilemap_draw_x(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	tilemap_draw_x(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	tilemap_draw_x(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
	tilemap_draw_x(bitmap, tilemap_2, 3,0);
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
	tilemap_set_scrolly(tilemap_1, 0, scrolly_1 );
	CAVE_TILEMAP_SET_SCROLLX(1)

	tilemap_set_enable( tilemap_2,    cave_vctrl_2[2] & 1 );
	tilemap_set_scrolly(tilemap_2, 0, scrolly_2 );
	CAVE_TILEMAP_SET_SCROLLX(2)

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, delta);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	tilemap_draw_x(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	tilemap_draw_x(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	tilemap_draw_x(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
	tilemap_draw_x(bitmap, tilemap_2, 3,0);
#endif
}


void dfeveron_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int delta = 0;

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

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
#endif
}


void uopoko_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int delta = 0;

	scrollx_0=cave_vctrl_0[0];
	scrolly_0=cave_vctrl_0[1];

	tilemap_set_flip(ALL_TILEMAPS, ((cave_videoregs[ 0 ] & 0x8000) ? TILEMAP_FLIPX : 0) | ((cave_videoregs[ 1 ] & 0x8000) ? TILEMAP_FLIPY : 0) );

	tilemap_set_enable( tilemap_0,    cave_vctrl_0[2] & 1 );
	tilemap_set_scrolly(tilemap_0, 0, scrolly_0 );
	CAVE_TILEMAP_SET_SCROLLX(0)

	tilemap_update(ALL_TILEMAPS);

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
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

	if(Machine->color_depth == 8) palette_init_used_colors();

	(*get_sprite_info)();

	sprite_update_cave();

//#ifdef MAME_DEBUG
	if(Machine->color_depth == 8) palette_recalc();
//#endif

	fillbitmap(bitmap,Machine->remapped_colortable[0x3f00],&Machine->visible_area);

#ifdef MAME_DEBUG
	screenrefresh_debug(bitmap, 0);
#else
	(*cave_sprite_draw)( 0 );
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	tilemap_draw_x(bitmap, tilemap_2, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	tilemap_draw_x(bitmap, tilemap_2, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	tilemap_draw_x(bitmap, tilemap_2, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
	tilemap_draw_x(bitmap, tilemap_2, 3,0);
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
	tilemap_draw_x(bitmap, tilemap_0, 0,0);
	tilemap_draw_x(bitmap, tilemap_1, 0,0);
	(*cave_sprite_draw)( 1 );
	tilemap_draw_x(bitmap, tilemap_0, 1,0);
	tilemap_draw_x(bitmap, tilemap_1, 1,0);
	(*cave_sprite_draw)( 2 );
	tilemap_draw_x(bitmap, tilemap_0, 2,0);
	tilemap_draw_x(bitmap, tilemap_1, 2,0);
	(*cave_sprite_draw)( 3 );
	tilemap_draw_x(bitmap, tilemap_0, 3,0);
	tilemap_draw_x(bitmap, tilemap_1, 3,0);
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


/*********************************************************************************/
#else // DECLARE

DECLARE( draw, (int xpos, int ypos),
{
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+tmblt.source_width;
	int y2 = ypos+tmblt.source_height;

	/* clip source coordinates */
	if( x1<tmblt.clip_left ) x1 = tmblt.clip_left;
	if( x2>tmblt.clip_right ) x2 = tmblt.clip_right;
	if( y1<tmblt.clip_top ) y1 = tmblt.clip_top;
	if( y2>tmblt.clip_bottom ) y2 = tmblt.clip_bottom;

	if( x1<x2 && y1<y2 ){ /* do nothing if totally clipped */
		DATA_TYPE *dest_baseaddr = xpos + (DATA_TYPE *)tmblt.screen->line[y1];
		DATA_TYPE *dest_next;

		int priority = tmblt.tile_priority;
		const DATA_TYPE *source_baseaddr;
		const DATA_TYPE *source_next;
		const UINT8 *mask_baseaddr;
		const UINT8 *mask_next;

		int c1;
		int c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)tmblt.pixmap->line[y1];
		mask_baseaddr = tmblt.bitmask->line[y1];

		c1 = x1/TILE_WIDTH; /* round down */
		c2 = (x2+TILE_WIDTH-1)/TILE_WIDTH; /* round up */

		y = y1;
		y_next = TILE_HEIGHT*(y1/TILE_HEIGHT) + TILE_HEIGHT;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*tmblt.dest_line_offset;
			source_next = source_baseaddr + dy*tmblt.source_line_offset;
			mask_next = mask_baseaddr + dy*tmblt.mask_line_offset;
		}

		for(;;){
			int row = y/TILE_HEIGHT;
			UINT8 *mask_data = tmblt.mask_data_row[row];
			UINT8 *priority_data = tmblt.priority_data_row[row];

			int tile_type;
			int prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1;
			int x_end;

			int column;
			for( column=c1; column<=c2; column++ ){
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = mask_data[column];

				if( tile_type!=prev_tile_type ){
					x_end = column*TILE_WIDTH;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT ){
						if( prev_tile_type == TILE_MASKED ){
							const UINT8 *mask0 = mask_baseaddr;
							const DATA_TYPE *source0 = source_baseaddr;
							DATA_TYPE *dest0 = dest_baseaddr;
							const UINT8 *mask1;
							const DATA_TYPE *source1;
							DATA_TYPE *dest1;
							int i = y;
							int j;
							int cnt = x_end-x_start;
							int cnt1 = (-x_start)&7;
							int cnt2;
							int cnt3;
							if(cnt1>=cnt){
								cnt1=cnt;cnt2=0;cnt3=0;
							}
							else{
								cnt3 = x_end&7;
								if(cnt1+cnt3==cnt){
									cnt2=0;
								}
								else{
									cnt2=(cnt-cnt1)/8;
								}
							}
							for(;;){
								int xp=x_start;

								for(j=cnt1;j>0;j--){
									if( mask0[xp/8]&(0x80>>(xp&7)) ) dest0[xp] = source0[xp];
									xp++;
								}

								if(cnt2){
									source1 = &source0[xp];
									dest1 = &dest0[xp];
									mask1 = &mask0[xp/8];
									for(j=cnt2;j>0;j--){
										UINT8 data = *mask1++;
										if( data&0x80 ) dest1[0] = source1[0];
										if( data&0x40 ) dest1[1] = source1[1];
										if( data&0x20 ) dest1[2] = source1[2];
										if( data&0x10 ) dest1[3] = source1[3];
										if( data&0x08 ) dest1[4] = source1[4];
										if( data&0x04 ) dest1[5] = source1[5];
										if( data&0x02 ) dest1[6] = source1[6];
										if( data&0x01 ) dest1[7] = source1[7];
										source1+=8;
										dest1+=8;
									}
									xp+=cnt2*8;
								}

								for(j=cnt3;j>0;j--){
									if( mask0[xp/8]&(0x80>>(xp&7)) ) dest0[xp] = source0[xp];
									xp++;
								}

								if( ++i == y_next ) break;
								dest0 += tmblt.dest_line_offset;
								source0 += tmblt.source_line_offset;
								mask0 += tmblt.mask_line_offset;
							}
						}
						else { /* TILE_OPAQUE */
							int num_pixels = x_end - x_start;
							DATA_TYPE *dest0 = dest_baseaddr+x_start;
							const DATA_TYPE *source0 = source_baseaddr+x_start;
							int i = y;
							for(;;){
								memcpy( dest0, source0, num_pixels*sizeof(DATA_TYPE) );
								if( ++i == y_next ) break;
								dest0 += tmblt.dest_line_offset;
								source0 += tmblt.source_line_offset;
							}
						}
					}
					x_start = x_end;
				}
				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;

			y = y_next;
			y_next += TILE_HEIGHT;

			if( y_next>=y2 ){
				y_next = y2;
			}
			else {
				dest_next += tmblt.dest_row_offset;
				source_next += tmblt.source_row_offset;
				mask_next += tmblt.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})


#undef TILE_WIDTH
#undef TILE_HEIGHT
#undef DATA_TYPE
#undef DECLARE

#endif /* DECLARE */
