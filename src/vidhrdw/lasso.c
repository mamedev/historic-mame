/***************************************************************************

		driver by Phil Stroffolino, Nicola Salmoria, Luca Elia


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q			shows the background
		W			shows the foreground (if present)
		A			shows the sprites

		Keys can be used together!


	Every game has 1 256 x 256 tilemap (non scrollable) made of 8 x 8 x 2
	tiles, and 16 x 16 x 2 sprites (some games use 32, some more).

	The graphics for tiles and sprites are held inside the same ROMs,
	but	aren't shared between the two:

	the first $100 tiles are for the tilemap, the following	$100 are
	for sprites. This constitutes the first graphics bank. There can
	be several.

	Lasso has an additional pixel layer (256 x 256 x 1) and a third
	CPU devoted to drawing into it (the lasso!)

	Wwjgtin has an additional $800 x $400 scrolling tilemap in ROM
	and $100 more 16 x 16 x 4 tiles for it.

	The colors are static ($40 colors, 2 PROMs) but the background
	color can be changed at runtime. Wwjgtin can change the last
	4 colors (= last palette) too.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "lasso.h"

/* variables only used here: */
static struct tilemap *background, *background1;
static int gfxbank, wwjgtin_bg1_enable;


/* variables needed externally: */
data8_t *lasso_vram; 	/* 0x2000 bytes for a 256 x 256 x 1 bitmap */
data8_t *wwjgtin_scroll;


/***************************************************************************


								Memory Handlers


***************************************************************************/

WRITE_HANDLER( lasso_videoram_w )
{
	if( videoram[offset]!=data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( background, offset&0x3ff );
	}
}

WRITE_HANDLER( lasso_gfxbank_w )
{
	int bank = (data & 0x04) >> 2;

	flip_screen_set( data & 0x01 );

	if (gfxbank != bank)
	{
		gfxbank = bank;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( wwjgtin_gfxbank_w )
{
	int bank = ((data & 0x04) ? 0 : 1) + ((data & 0x10) ? 2 : 0);
	wwjgtin_bg1_enable = data & 0x08;

	flip_screen_set( data & 0x01 );

	if (gfxbank != bank)
	{
		gfxbank = bank;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

/***************************************************************************


							Colors (BBGGGRRR)


***************************************************************************/

void lasso_set_color(int i, int data)
{
	int bit0,bit1,bit2,r,g,b;

	/* red component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = (data >> 6) & 0x01;
	bit1 = (data >> 7) & 0x01;
	b = 0x4f * bit0 + 0xa8 * bit1;

	palette_set_color( i,r,g,b );
}

PALETTE_INIT( lasso )
{
	int i;

	for (i = 0;i < 0x40;i++)
	{
		lasso_set_color(i,*color_prom);
		color_prom++;
	}
}

/* 16 color tiles with a 4 color step for the palettes */
PALETTE_INIT( wwjgtin )
{
	int color, pen;

	palette_init_lasso(colortable,color_prom);

	for( color = 0; color < 0x10; color++ )
		for( pen = 0; pen < 16; pen++ )
			colortable[color * 16 + pen + 4*16] = (color * 4 + pen) % 0x40;
}

/* The background color can be changed */
WRITE_HANDLER( lasso_backcolor_w )
{
	int i;
	for( i=0; i<0x40; i+=4 ) /* stuff into color#0 of each palette */
		lasso_set_color(i,data);
}

/* The last 4 color (= last palette) entries can be changed */
WRITE_HANDLER( wwjgtin_lastcolor_w )
{
	lasso_set_color(0x3f - offset,data);
}

/***************************************************************************


								Tilemaps


***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int tile_number	=	videoram[tile_index];
	int attributes	=	videoram[tile_index + 0x400];
	SET_TILE_INFO(		0,
						tile_number + (gfxbank << 8),
						attributes & 0x0f,
						0	)
}

/* wwjgtin has an additional scrollable tilemap stored in ROM */
static void get_bg1_tile_info(int tile_index)
{
	data8_t *ROM = memory_region(REGION_GFX3);
	int tile_number	=	ROM[tile_index];
	int attributes	=	ROM[tile_index + 0x2000];
	SET_TILE_INFO(		2,
						tile_number,
						attributes & 0x0f,
						0	)
}


/***************************************************************************


						  	Video Hardware Init


***************************************************************************/

VIDEO_START( lasso )
{
	background = tilemap_create(	get_bg_tile_info, tilemap_scan_rows,
									TILEMAP_OPAQUE,		8,8,	32,32);

	if (!background)
		return 1;

	return 0;
}

VIDEO_START( wwjgtin )
{
	background = tilemap_create(	get_bg_tile_info, tilemap_scan_rows,
									TILEMAP_TRANSPARENT,	8,8,	32,32);

	background1 = tilemap_create(	get_bg1_tile_info, tilemap_scan_rows,
									TILEMAP_OPAQUE,			16,16,	0x80,0x40);

	if (!background || !background1)
		return 1;

	tilemap_set_transparent_pen(background,0);
	return 0;
}




/***************************************************************************

								Sprites Drawing


		Offset:		Format:			Value:

			0						Y (Bottom-up)

			1		7--- ----		Flip Y
					-6-- ----		Flip X
					--54 3210		Code

			2		7654 ----
					---- 3210		Color

			3						X

***************************************************************************/

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int reverse )
{
	const data8_t *finish, *source;
	int inc;

	if (reverse)
	{
		source	=	spriteram;
		finish	=	spriteram + spriteram_size;
		inc		=	4;
	}
	else
	{
		source	=	spriteram + spriteram_size - 4;
		finish	=	spriteram - 4;
		inc		=	-4;
	}

	while( source != finish )
	{
		int sy			=	source[0];
		int tile_number	=	source[1];
		int color		=	source[2];
		int sx			=	source[3];

		int flipx		=	(tile_number & 0x40);
		int flipy		=	(tile_number & 0x80);

		if( flip_screen )
		{
			sx = 240-sx;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sy = 240-sy;
		}
        drawgfx(	bitmap, Machine->gfx[1],
					(tile_number & 0x3f) + (gfxbank << 6),
					color,
					flipx, flipy,
					sx,sy,
					cliprect, TRANSPARENCY_PEN,0	);

		source += inc;
    }
}

/***************************************************************************


								Pixmap Drawing


***************************************************************************/

static void draw_lasso( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const unsigned char *source = lasso_vram;
	int x,y;
	int pen = Machine->pens[0x3f];
	for( y=0; y<256; y++ )
	{
		if (flip_screen)
		{
			if (255-y < cliprect->min_y || 255-y > cliprect->max_y)
				continue;
		}
		else
		{
			if (y < cliprect->min_y || y > cliprect->max_y)
				continue;
		}
		
		for( x=0; x<256; x+=8 )
		{
			int data = *source++;
			if( data )
			{
				int bit;
				if( flip_screen )
				{
					for( bit=0; bit<8; bit++ )
					{
						if( (data<<bit)&0x80 )
							plot_pixel( bitmap, 255-(x+bit), 255-y, pen );
					}
				}
				else
				{
					for( bit=0; bit<8; bit++ )
					{
						if( (data<<bit)&0x80 )
							plot_pixel( bitmap, x+bit, y, pen );
					}
				}
			}
		}
	}
}

/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( lasso )
{
	int layers_ctrl = -1;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_A))	msk |= 4;
	if (msk != 0) layers_ctrl &= msk;		}
#endif

	if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, background,  0,0);
	else					fillbitmap(bitmap,Machine->pens[0],cliprect);
	if (layers_ctrl & 2)	draw_lasso(bitmap,cliprect);
	if (layers_ctrl & 4)	draw_sprites(bitmap,cliprect, 0);
}

VIDEO_UPDATE( chameleo )
{
	int layers_ctrl = -1;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_A))	msk |= 4;
	if (msk != 0) layers_ctrl &= msk;		}
#endif

	if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, background,  0,0);
	else					fillbitmap(bitmap,Machine->pens[0],cliprect);
//	if (layers_ctrl & 2)	draw_lasso(bitmap,cliprect);
	if (layers_ctrl & 4)	draw_sprites(bitmap,cliprect, 0);
}


VIDEO_UPDATE( wwjgtin )
{
	int layers_ctrl = -1;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_A))	msk |= 4;
	if (msk != 0) layers_ctrl &= msk;		}
#endif

	tilemap_set_scrollx(background1,0,wwjgtin_scroll[0] + wwjgtin_scroll[1]*256);
	tilemap_set_scrolly(background1,0,wwjgtin_scroll[2] + wwjgtin_scroll[3]*256);

	if((layers_ctrl & 1) && wwjgtin_bg1_enable)
		tilemap_draw(bitmap,cliprect, background1, 0,0);
	else
		fillbitmap(bitmap,Machine->pens[0x40],cliprect);	// BLACK

	if (layers_ctrl & 4)	draw_sprites(bitmap,cliprect, 1);	// reverse order
	if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, background,  0,0);
}
