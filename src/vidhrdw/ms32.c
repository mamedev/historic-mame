/* Jaleco MegaSystem 32 Video Hardware */

/* The Video Hardware is Similar to the Non-MS32 Version of Tetris Plus 2 */

/* Plenty to do, see list in drivers/ms32.c */

#include "driver.h"

void ms32_dump_ram(void);
extern data32_t* ms32_bgram;
extern data32_t* ms32_spram;
extern data32_t* ms32_txram;
extern data32_t* ms32_fce00000;

/********** Tilemaps **********/

static struct tilemap *ms32_tx_tilemap, *ms32_bg_tilemap;

/* note for now we don't have any fg tiles because they're all encrypted */

static void get_ms32_tx_tile_info(int tile_index)
{
	int tileno, colour;

	tileno = ms32_txram[tile_index *2] & 0x0000ffff;
	colour = ms32_txram[tile_index *2+1] & 0x000000f;
	tile_info.priority=0;

	if ( (tileno == 0x20) || (tileno == 0x00) ) // HACK since the tiles are encrypted avoid drawing which should be spaces so we can see the screen
		tile_info.priority=4;
	SET_TILE_INFO(3,tileno,colour,0)
}

WRITE32_HANDLER( ms32_txram_w )
{
	COMBINE_DATA(&ms32_txram[offset]);
	tilemap_mark_tile_dirty(ms32_tx_tilemap,offset/2);
}

static void get_ms32_bg_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = ms32_bgram[tile_index *2] & 0x0000ffff;
	colour = ms32_bgram[tile_index *2+1] & 0x000000f;

	SET_TILE_INFO(1,tileno,colour,0)
}

WRITE32_HANDLER( ms32_bgram_w )
{
	COMBINE_DATA(&ms32_bgram[offset]);
	tilemap_mark_tile_dirty(ms32_bg_tilemap,offset/2);
}

/* SPRITES based on tetrisp2 for now, readd priority bits later */

static void ms32_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, data32_t *sprram_top, size_t sprram_size, int gfxnum)
{
/***************************************************************************


								Sprites Drawing

	Offset:		Bits:					Meaning:

	0.w			fedc ba98 ---- ----
				---- ---- 7654 ----		Priority
				---- ---- ---- 3---
				---- ---- ---- -2--		Draw this sprite
				---- ---- ---- --1-		Flip Y
				---- ---- ---- ---0		Flip X

	1.w			fedc ba98 ---- ----		Tile's Y position in the tile page (*)
				---- ---- 7654 3210		Tile's X position in the tile page (*)

	2.w			fedc ---- ---- ----		Color
				---- ba98 7--- ----
				---- ---- -654 3210		Tile Page (32x32 tiles = 256x256 pixels each)

	3.w			fedc ba98 ---- ----		Y Size - 1 (*)
				---- ---- 7654 3210		X Size - 1 (*)

	4.w			fedc ba-- ---- ----
				---- --98 7654 3210		Y (Signed)

	5.w			fedc b--- ---- ----
				---- -a98 7654 3210		X (Signed)

	6.w			fedc ba98 7654 3210		Zoom Y

	7.w			fedc ba98 7654 3210		Zoom X

(*)	1 pixel granularity

***************************************************************************/

	int x, y, tx, ty, sx, sy, flipx, flipy;
	int xsize, ysize, xnum, ynum, xzoom, yzoom;
	int xstart, ystart, xend, yend, xinc, yinc;
	int code, attr, color, size, pri, trans;
	int dx, dy, new_dx, new_dy;

	int min_x = cliprect->min_x;
	int max_x = cliprect->max_x;
	int min_y = cliprect->min_y;
	int max_y = cliprect->max_y;

	data32_t		*source	= sprram_top;
	const data32_t	*finish	= sprram_top + (sprram_size - 0x10) / 4;
	int spnum;

	spnum = 0;

	for (; source <= finish; source += 0x10/4 )
	{
		struct rectangle clip;


		attr	=	source[ 0 ];

		spnum++;

		if ((attr & 0x0004) == 0)			continue;

		flipx	=	attr & 1;
		flipy	=	attr & 2;

		pri = (attr >> 4)&0xf;

		code	=	source[ 1 ];
		color	=	source[ 2 ];

		tx		=	(code >> 0) & 0xff;
		ty		=	(code >> 8) & 0xff;

		code	=	(tx / 8) +
					(ty / 8) * (0x100/8) +
					(color & 0x07ff) * (0x100/8) * (0x100/8);

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

		xzoom	=	(source[ 6 ]&0xffff);
		yzoom	=	(source[ 7 ]&0xffff);

		if (!yzoom || !xzoom)				continue;

		yzoom = 0x1000000/yzoom;
		xzoom = 0x1000000/xzoom;

		trans = TRANSPARENCY_PEN; // there are surely also shadows (see gametngk) but how they're enabled we don't know

		/* Clip the sprite if its width or height is not an integer
		   multiple of 8 pixels (1 tile) */

		clip.min_x	=	sx;
		clip.max_x	=	sx + ((xsize*xzoom)>>16) - 1;
		clip.min_y	=	sy;
		clip.max_y	=	sy + ((ysize*yzoom)>>16) - 1;

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

		dy = sy + (((ystart*yzoom*8)+0x8000) >> 16);

		for (y = ystart; y != yend; y += yinc)
		{
			new_dy = sy + ((((y+yinc)*yzoom*8)+0x8000) >> 16);

			dx = sx + (((xstart*xzoom*8)+0x8000) >> 16);
			for (x = xstart; x != xend; x += xinc)
			{
				new_dx = sx + ((((x+xinc)*xzoom*8)+0x8000) >> 16);

				mdrawgfxzoom(bitmap, Machine->gfx[gfxnum],
						code++,
						color,
						flipx, flipy,
						dx, dy,
						&clip,
						trans, 0,
						abs(new_dx-dx)*0x2000, abs(new_dy-dy)*0x2000, 0); // Zoom to fill the gaps completely
				dx = new_dx;
			}

			dy = new_dy;
			code	+=	(0x100/8) - xnum;
		}
	}	/* end sprite loop */
}

/* Video Start / Update */

VIDEO_START( ms32 )
{
	ms32_tx_tilemap = tilemap_create(get_ms32_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	tilemap_set_transparent_pen(ms32_tx_tilemap,0);

	ms32_bg_tilemap = tilemap_create(get_ms32_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 16, 16,128,128); // or 4 tilemaps?
	return 0;
}


VIDEO_UPDATE( ms32 )
{

#ifdef MAME_DEBUG
	if (keyboard_pressed(KEYCODE_O)) ms32_dump_ram();
#endif

	{
		int xzoom = ms32_fce00000[0x610/4];
		int yzoom = ms32_fce00000[0x620/4];
		int scrollx = ms32_fce00000[0x630/4];
		int scrolly = ms32_fce00000[0x634/4];

		if (!strcmp(Machine->gamedrv->name,"gratia")) /* go figure ... */
		{
			scrollx = ms32_fce00000[0x600/4]+1024;
			scrolly = ms32_fce00000[0x604/4]+1024;// ?
		}

	tilemap_draw_roz(bitmap, cliprect, ms32_bg_tilemap,
			scrollx<<16, scrolly<<16, xzoom<<8, 0, 0, yzoom<<8,
			1, // Wrap
			0, 0);
	}

	ms32_draw_sprites(bitmap,cliprect, ms32_spram, 0x40000, 0);

	tilemap_draw(bitmap,cliprect,ms32_tx_tilemap,0,0);
}
