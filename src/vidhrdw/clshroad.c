/***************************************************************************

							-= Clash Road =-

					driver by	Luca Elia (l.elia@tin.it)

Note:	if MAME_DEBUG is defined, pressing Z with:

		Q			shows layer 0 A
		W			shows layer 0 B
		E			shows layer 1
		A			shows the sprites

		Keys can be used together!


	[ 2 Horizontally Scrolling Layers ]

		Size :	512 x 256
		Tiles:	16 x 16 x 4.

		These 2 layers share the same graphics and X scroll value.
		The tile codes are stuffed together in memory too: first one
		layer's row, then the other's (and so on for all the rows).

	[ 1 Fixed Layer ]

		Size :	(256 + 32) x 256
		Tiles:	8 x 8 x 4.

		This is like a 32x32 tilemap, but the top and bottom rows (that
		fall outside the visible area) are used to widen the tilemap
		horizontally, adding 2 vertical columns both sides.

		The result is a 36x28 visible tilemap.

	[ 64? sprites ]

		Sprites are 16 x 16 x 4.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables only used here: */

static struct tilemap *tilemap_0a, *tilemap_0b, *tilemap_1;

/* Variables & functions needed by drivers: */

data8_t *clshroad_vram_0, *clshroad_vram_1;
data8_t *clshroad_vregs;

WRITE_HANDLER( clshroad_vram_0_w );
WRITE_HANDLER( clshroad_vram_1_w );
WRITE_HANDLER( clshroad_flipscreen_w );

void clshroad_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
}

WRITE_HANDLER( clshroad_flipscreen_w )
{
	flip_screen_set( data & 1 );
}

/***************************************************************************

						Callbacks for the TileMap code

***************************************************************************/

/***************************************************************************

						  Layers 0 Tiles Format

Offset:

	00-3f:	Even bytes: Codes	Odd bytes: Colors	<- Layer B First Row
	40-7f:	Even bytes: Codes	Odd bytes: Colors	<- Layer A First Row
	..										<- 2nd Row
	..										<- 3rd Row
	etc.

***************************************************************************/

static void get_tile_info_0a( int tile_index )
{
	data8_t code, color;
	tile_index = (tile_index & 0x1f) + (tile_index & ~0x1f)*2;
	code	=	clshroad_vram_0[ tile_index * 2 + 0x40 ];
	color	=	clshroad_vram_0[ tile_index * 2 + 0x41 ];
	SET_TILE_INFO(1, code, color & 0x0f );
}

static void get_tile_info_0b( int tile_index )
{
	data8_t code, color;
	tile_index = (tile_index & 0x1f) + (tile_index & ~0x1f)*2;
	code	=	clshroad_vram_0[ tile_index * 2 + 0x00 ];
	color	=	clshroad_vram_0[ tile_index * 2 + 0x01 ];
	SET_TILE_INFO(1, code, color & 0x0f );
}

WRITE_HANDLER( clshroad_vram_0_w )
{
	if (clshroad_vram_0[offset] != data)
	{
		int tile_index = offset / 2;
		int tile = (tile_index & 0x1f) + (tile_index & ~0x3f)/2;
		clshroad_vram_0[offset] = data;
		if (tile_index & 0x20)	tilemap_mark_tile_dirty(tilemap_0a, tile);
		else					tilemap_mark_tile_dirty(tilemap_0b, tile);
	}
}

/***************************************************************************

						  Layer 1 Tiles Format

Offset:

	000-3ff		Code
	400-7ff		7654----	Code (High bits)
				----3210	Color

	This is like a 32x32 tilemap, but the top and bottom rows (that
	fall outside the visible area) are used to widen the tilemap
	horizontally, adding 2 vertical columns both sides.

	The result is a 36x28 visible tilemap.

***************************************************************************/

/* logical (col,row) -> memory offset */
static UINT32 tilemap_scan_rows_extra( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	// The leftmost columns come from the bottom rows
	if (col <= 0x01)	return row + (col + 0x1e) * 0x20;
	// The rightmost columns come from the top rows
	if (col >= 0x22)	return row + (col - 0x22) * 0x20;

	// These are not visible, but *must* be mapped to other tiles than
	// those used by the leftmost and rightmost columns (tilemap "bug"?)
	if (row <= 0x01)	return 0;
	if (row >= 0x1e)	return 0;

	// "normal" layout for the rest.
	return (col-2) + row * 0x20;
}

static void get_tile_info_1( int tile_index )
{
	data8_t code	=	clshroad_vram_1[ tile_index + 0x000 ];
	data8_t color	=	clshroad_vram_1[ tile_index + 0x400 ];
	SET_TILE_INFO(2, code + ((color & 0xf0)<<4), color & 0x0f );
}

WRITE_HANDLER( clshroad_vram_1_w )
{
	if (clshroad_vram_1[offset] != data)
	{
		clshroad_vram_1[offset] = data;
		tilemap_mark_tile_dirty(tilemap_1, offset % 0x400);
	}
}


int clshroad_vh_start(void)
{
	/* These 2 use the graphics and scroll value */
	tilemap_0a	=	tilemap_create(	get_tile_info_0a, tilemap_scan_rows,
									TILEMAP_OPAQUE,			16,16,	0x20, 0x10 );
	tilemap_0b	=	tilemap_create(	get_tile_info_0b, tilemap_scan_rows,
									TILEMAP_TRANSPARENT,	16,16,	0x20, 0x10 );

	/* Text (No scrolling) */
	tilemap_1	=	tilemap_create(	get_tile_info_1, tilemap_scan_rows_extra,
									TILEMAP_TRANSPARENT,	8,8,	0x20+4, 0x20 );

	if (!tilemap_0a || !tilemap_0b || !tilemap_1)	return 1;

	tilemap_set_scroll_rows( tilemap_0a, 1);
	tilemap_set_scroll_rows( tilemap_0b, 1);
	tilemap_set_scroll_rows( tilemap_1,  1);

	tilemap_set_scroll_cols( tilemap_0a, 1);
	tilemap_set_scroll_cols( tilemap_0b, 1);
	tilemap_set_scroll_cols( tilemap_1,  1);

	tilemap_set_scrolldx( tilemap_0a, -0x30, -0xb5);
	tilemap_set_scrolldx( tilemap_0b, -0x30, -0xb5);

	tilemap_set_transparent_pen( tilemap_0a, 0 );
	tilemap_set_transparent_pen( tilemap_0b, 0 );
	tilemap_set_transparent_pen( tilemap_1,  0 );

	return 0;
}



/***************************************************************************

								Sprites Drawing

Offset:		Format:		Value:

	0

	1					Y (Bottom-up)

	2		765432--
			------10	Code (high bits)

	3		76------
			--543210	Code (low bits)

	4

	5					X (low bits)

	6					X (High bits)

	7					Color?

- Sprite flipping ?

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int i;
	int max_y = Machine->drv->screen_height - 1;

	int priority = clshroad_vregs[ 2 ];

	if (~priority & 8)	return;

	for (i = 0; i < spriteram_size ; i += 8)
	{
		int y		=	 spriteram[i+1];
		int code	=	(spriteram[i+3] & 0x3f) + (spriteram[i+2] << 6);
		int x		=	 spriteram[i+5]         + (spriteram[i+6] << 8);
		int attr	=	 spriteram[i+7];

		int flipx	=	0;
		int flipy	=	0;

		x -= 0x4a/2;
		if (flip_screen)	{ y = max_y - y - 16;	flipx = !flipx;	flipy = !flipy;	}

		drawgfx(bitmap,Machine->gfx[0],
				code,
				attr & 0x0f,
				flipx, flipy,
				x, 0x0f0 - y,
				&Machine->visible_area,TRANSPARENCY_PEN,0 );
	}
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

void clshroad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layers_ctrl = -1;
	int scrollx  = clshroad_vregs[ 0 ] + (clshroad_vregs[ 1 ] << 8);
	int priority = clshroad_vregs[ 2 ];

	/* Only horizontal scrolling (these 2 layers use the same value) */
	tilemap_set_scrollx(tilemap_0a, 0, scrollx);
	tilemap_set_scrollx(tilemap_0b, 0, scrollx);

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;
	usrintf_showmessage("Pri: %02X", priority);
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
//	memset(palette_used_colors,PALETTE_COLOR_USED,Machine->drv->total_colors);
	palette_recalc();

	if (layers_ctrl & 1)	tilemap_draw(bitmap,tilemap_0a,0,0);	// Opaque
	else					fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	if (layers_ctrl & 2)	tilemap_draw(bitmap,tilemap_0b,0,0);

	if ( (layers_ctrl & 8) && (priority & 1) )	draw_sprites(bitmap);

	if (layers_ctrl & 4)	tilemap_draw(bitmap,tilemap_1,0,0);

	if ( (layers_ctrl & 8) && (~priority & 1) )	draw_sprites(bitmap);
}
