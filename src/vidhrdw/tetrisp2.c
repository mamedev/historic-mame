/***************************************************************************

							  -= Tetris Plus 2 =-

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows the background
		W		shows the foreground
		A		shows the sprites

		Keys can be used together!


	[ 2 Scrolling Layers ]

	The Background is a 64 x 64 Tilemap with 16 x 16 x 8 tiles (1024 x 1024).
	The Foreground is a 64 x 64 Tilemap with 8 x 8 x 8 tiles (512 x 512).
	Each tile needs 4 bytes.

	[ 1024? Sprites ]

	Sprites are "nearly" tile based: if the data in the ROMs is decoded
	as 8x8 tiles, and each 32 consecutive tiles are drawn like a column
	of 256 pixels, it turns out that every 32 x 32 tiles form a picture
	of 256 x 256 pixels (a "page").

	Sprites are portions of any size from those page's graphics rendered
	to screen.

To Do:

-	There is a 3rd unimplemented layer capable of rotation (not used by
	the game, can be tested in service mode).
-	Priority RAM is not taken into account.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables needed by driver: */

data16_t *tetrisp2_vram_0, *tetrisp2_scroll_0;
data16_t *tetrisp2_vram_1, *tetrisp2_scroll_1;

data16_t *tetrisp2_priority;
data16_t *tetrisp2_vregs;


/***************************************************************************


									Palette


***************************************************************************/

/* BBBBBGGGGGRRRRRx xxxxxxxxxxxxxxxx */
WRITE16_HANDLER( tetrisp2_palette_w )
{
	data = COMBINE_DATA(&paletteram16[offset]);
	if ((offset & 1) == 0)
	{
		int r = (data >>  1) & 0x1f;
		int g = (data >>  6) & 0x1f;
		int b = (data >> 11) & 0x1f;
		palette_change_color(offset/2,(r << 3) | (r >> 2),(g << 3) | (g >> 2),(b << 3) | (b >> 2));
	}
}


/***************************************************************************


									Priority


***************************************************************************/

WRITE16_HANDLER( tetrisp2_priority_w )
{
	if (ACCESSING_LSB)	data = (data >> 0) & 0xff;
	else				data = (data >> 8) & 0xff;
	tetrisp2_priority[offset] = (data << 8) + data;
}



/***************************************************************************


									Tilemaps


	Offset:		Bits:					Value:

		0.w								Code
		2.w		fedc ba98 7654 ----
				---- ---- ---- 3210		Color


***************************************************************************/

static struct tilemap *tilemap_0, *tilemap_1;

#define NX_0  (0x40)
#define NY_0  (0x40)

static void get_tile_info_0(int tile_index)
{
	data16_t code_hi = tetrisp2_vram_0[ 2 * tile_index + 0];
	data16_t code_lo = tetrisp2_vram_0[ 2 * tile_index + 1];
	SET_TILE_INFO( 1, code_hi, code_lo & 0xf);
}

WRITE16_HANDLER( tetrisp2_vram_0_w )
{
	data16_t old_data	=	tetrisp2_vram_0[offset];
	data16_t new_data	=	COMBINE_DATA(&tetrisp2_vram_0[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_0,offset/2);
}



#define NX_1  (0x40)
#define NY_1  (0x40)

static void get_tile_info_1(int tile_index)
{
	data16_t code_hi = tetrisp2_vram_1[ 2 * tile_index + 0];
	data16_t code_lo = tetrisp2_vram_1[ 2 * tile_index + 1];
	SET_TILE_INFO( 2, code_hi, code_lo & 0xf);
}

WRITE16_HANDLER( tetrisp2_vram_1_w )
{
	data16_t old_data	=	tetrisp2_vram_1[offset];
	data16_t new_data	=	COMBINE_DATA(&tetrisp2_vram_1[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_1,offset/2);
}


int tetrisp2_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,NX_0,NY_0);

	tilemap_1 = tilemap_create(	get_tile_info_1,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,NX_1,NY_1);

	if (!tilemap_0 || !tilemap_1)	return 1;

	tilemap_set_transparent_pen(tilemap_0,0);
	tilemap_set_transparent_pen(tilemap_1,0);

	tilemap_set_scrolldx(tilemap_0, 4, 4);

	return 0;
}





/***************************************************************************


								Sprites Drawing

	Offset:		Bits:					Meaning:

	0.w			fedc ba98 ---- ----
				---- ---- 7654 ----		Priority
				---- ---- ---- 3---
				---- ---- ---- -2--		Draw this sprite
				---- ---- ---- --1-		Flip Y
				---- ---- ---- ---0		Flip X

	2.w			fedc ba98 ---- ----		Tile's Y position in the tile page (*)
				---- ---- 7654 3210		Tile's X position in the tile page (*)

	4.w			fedc ---- ---- ----		Color
				---- ba98 7--- ----
				---- ---- -654 3210		Tile Page (32x32 tiles = 256x256 pixels each)

	6.w			fedc ba98 ---- ----
				---- ---- 7654 ----		Y Size - 1 (*)
				---- ---- ---- 3210		X Size - 1 (*)

	8.w			fedc ba-- ---- ----
				---- --98 7654 3210		Y (Signed)

	A.w			fedc b--- ---- ----
				---- -a98 7654 3210		X (Signed)

	C.w

	E.w

(*)	1 pixel granularity

***************************************************************************/

static void tetrisp2_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	int x, y, tx, ty, sx, sy, flipx, flipy;
	int xsize, ysize, xnum, ynum;
	int xstart, ystart, xend, yend, xinc, yinc;
	int code, attr, color, size;

	int min_x = Machine->visible_area.min_x;
	int max_x = Machine->visible_area.max_x;
	int min_y = Machine->visible_area.min_y;
	int max_y = Machine->visible_area.max_y;

	data16_t		*source	=	spriteram16 + (spriteram_size-0x10)/2;
	const data16_t	*finish	=	spriteram16;

	priority = (priority & 0xf) << 4;

	for (; source >= finish; source -= 0x10/2 )
	{
		struct rectangle clip;

		attr	=	source[ 0 ];

		if ((attr & 0x0004) == 0)			continue;
		if ((attr & 0x00f0) != priority)	continue;

		flipx	=	attr & 1;
		flipy	=	attr & 2;

		code	=	source[ 1 ];
		color	=	source[ 2 ];

		tx		=	(code >> 0) & 0xff;
		ty		=	(code >> 8) & 0xff;

		code	=	(tx / 8) +
					(ty / 8) * (0x100/8) +
					(color & 0x7f) * (0x100/8) * (0x100/8);

		color	=	(color >> 12) & 0xf;

		size	=	source[ 3 ];

		xsize	=	((size >> 0) & 0xff) + 1;
		ysize	=	((size >> 8) & 0xff) + 1;

		xnum	=	( ((tx + xsize) & ~7) + (((tx + xsize) & 7) ? 8 : 0) - (tx & ~7) ) / 8;
		ynum	=	( ((ty + ysize) & ~7) + (((ty + ysize) & 7) ? 8 : 0) - (ty & ~7) ) / 8;

		sy		=	source[ 4 ];
		sx		=	source[ 5 ];

		sx		=	(sx & 0x3ff) - (sx & 0x400);
		sy		=	(sy & 0x1ff) - (sy & 0x200);

		/* Flip Screen */
		if (tetrisp2_vregs[0] & 1)
		{
			sx = max_x + 1 - sx - xsize;	flipx = !flipx;
			sy = max_y + 1 - sy - ysize;	flipy = !flipy;
		}

		/* Clip the sprite if its width or height is not an integer
		   multiple of 8 pixels (1 tile) */

		clip.min_x	=	sx;
		clip.max_x	=	sx + xsize - 1;
		clip.min_y	=	sy;
		clip.max_y	=	sy + ysize - 1;

		if (clip.min_x > max_x)	continue;
		if (clip.max_x < min_x)	continue;

		if (clip.min_y > max_y)	continue;
		if (clip.max_y < min_y)	continue;

		if (clip.min_x < min_x)	clip.min_x = min_x;
		if (clip.max_x > max_x)	clip.max_x = max_x;

		if (clip.min_y < min_y)	clip.min_y = min_y;
		if (clip.max_y > max_y)	clip.max_y = max_y;

		if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1;  sx -= xnum*8 - xsize - (tx & 7); }
		else		{ xstart = 0;       xend = xnum;  xinc = +1;  sx -= tx & 7; }

		if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1;  sy -= ynum*8 - ysize - (ty & 7); }
		else		{ ystart = 0;       yend = ynum;  yinc = +1;  sy -= ty & 7; }


		for (y = ystart; y != yend; y += yinc)
		{
			for (x = xstart; x != xend; x += xinc)
			{
				drawgfx(bitmap,Machine->gfx[0],
						code++,
						color,
						flipx,flipy,
						sx + x * 8, sy + y * 8,
						&clip,
						TRANSPARENCY_PEN,0);
			}
			code	+=	(0x100/8) - xnum;
		}
	}	/* end sprite loop */
}





/***************************************************************************


								Screen Drawing


***************************************************************************/

void tetrisp2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,layers_ctrl = -1;

	/* Black background color ? */
	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

	tilemap_set_flip(tilemap_0, (tetrisp2_vregs[0] & 1) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0 );
	tilemap_set_flip(tilemap_1, (tetrisp2_vregs[0] & 1) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0 );

	tilemap_set_scrollx( tilemap_0, 0, tetrisp2_scroll_0[ 2 ] );
	tilemap_set_scrolly( tilemap_0, 0, tetrisp2_scroll_0[ 5 ] );

	tilemap_set_scrollx( tilemap_1, 0, tetrisp2_scroll_1[ 2 ] );
	tilemap_set_scrolly( tilemap_1, 0, tetrisp2_scroll_1[ 5 ] );

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
//	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;

#if 1
{	char buf[1024];
	sprintf(buf, "0] %04X %04X %04X %04X %04X %04X\n"
				 "1] %04X %04X %04X %04X %04X %04X\n",
//				 "V] %04X %04X %04X %04X",

				tetrisp2_scroll_0[0x00/2],tetrisp2_scroll_0[0x02/2],
				tetrisp2_scroll_0[0x04/2],tetrisp2_scroll_0[0x06/2],
				tetrisp2_scroll_0[0x08/2],tetrisp2_scroll_0[0x0a/2],

				tetrisp2_scroll_1[0x00/2],tetrisp2_scroll_1[0x02/2],
				tetrisp2_scroll_1[0x04/2],tetrisp2_scroll_1[0x06/2],
				tetrisp2_scroll_1[0x08/2],tetrisp2_scroll_1[0x0a/2]

//				tetrisp2_vregs[0x00/2],tetrisp2_vregs[0x02/2],
//				tetrisp2_vregs[0x04/2],tetrisp2_vregs[0x06/2]
			);
	usrintf_showmessage(buf);	}
#endif
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	memset(palette_used_colors,PALETTE_COLOR_USED,Machine->drv->total_colors);

	palette_recalc();

	if (layers_ctrl & 1)	tilemap_draw(bitmap,tilemap_0, 0, 0);
	if (layers_ctrl & 8)	for (i=0xf;i>=0;i--)	tetrisp2_draw_sprites(bitmap,i);
	if (layers_ctrl & 2)	tilemap_draw(bitmap,tilemap_1, 0, 0);
}
