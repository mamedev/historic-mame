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


extern UINT16 tetrisp2_systemregs[0x10];
extern UINT16 rocknms_sub_systemregs[0x10];

/* Variables needed by driver: */

data16_t *tetrisp2_vram_bg, *tetrisp2_scroll_bg;
data16_t *tetrisp2_vram_fg, *tetrisp2_scroll_fg;
data16_t *tetrisp2_vram_rot, *tetrisp2_rotregs;

data16_t *tetrisp2_priority;

data16_t *rocknms_sub_vram_bg, *rocknms_sub_scroll_bg;
data16_t *rocknms_sub_vram_fg, *rocknms_sub_scroll_fg;
data16_t *rocknms_sub_vram_rot, *rocknms_sub_rotregs;

data16_t *rocknms_sub_priority;

static struct mame_bitmap *rocknms_main_tmpbitmap, *rocknms_sub_tmpbitmap;
static struct mame_bitmap *sprite_tmpbitmap;
static struct mame_bitmap *priority_tmpbitmap;


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
		palette_set_color(offset/2,(r << 3) | (r >> 2),(g << 3) | (g >> 2),(b << 3) | (b >> 2));
	}
}

WRITE16_HANDLER( rocknms_sub_palette_w )
{
	data = COMBINE_DATA(&paletteram16_2[offset]);
	if ((offset & 1) == 0)
	{
		int r = (data >>  1) & 0x1f;
		int g = (data >>  6) & 0x1f;
		int b = (data >> 11) & 0x1f;
		palette_set_color((0x8000 + (offset/2)),(r << 3) | (r >> 2),(g << 3) | (g >> 2),(b << 3) | (b >> 2));
	}
}


/***************************************************************************


									Priority


***************************************************************************/

READ16_HANDLER( tetrisp2_priority_r )
{
	return tetrisp2_priority[offset];
}

READ16_HANDLER( rocknms_sub_priority_r )
{
	return rocknms_sub_priority[offset];
}

WRITE16_HANDLER( tetrisp2_priority_w )
{
	if (ACCESSING_MSB)
	{
		data |= ((data & 0xff00) >> 8);
		tetrisp2_priority[offset] = data;
	}
}

WRITE16_HANDLER( rockn_priority_w )
{
	if (ACCESSING_MSB)
	{
		tetrisp2_priority[offset] = data;
	}
}

WRITE16_HANDLER( rocknms_sub_priority_w )
{
	if (ACCESSING_MSB)
	{
		rocknms_sub_priority[offset] = data;
	}
}



/***************************************************************************


									Tilemaps


	Offset:		Bits:					Value:

		0.w								Code
		2.w		fedc ba98 7654 ----
				---- ---- ---- 3210		Color


***************************************************************************/

static struct tilemap *tilemap_bg, *tilemap_fg, *tilemap_rot;
static struct tilemap *tilemap_sub_bg, *tilemap_sub_fg, *tilemap_sub_rot;

#define NX_0  (0x40)
#define NY_0  (0x40)

static void get_tile_info_bg(int tile_index)
{
	data16_t code_hi = tetrisp2_vram_bg[ 2 * tile_index + 0];
	data16_t code_lo = tetrisp2_vram_bg[ 2 * tile_index + 1];
	SET_TILE_INFO(
			1,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( tetrisp2_vram_bg_w )
{
	data16_t old_data	=	tetrisp2_vram_bg[offset];
	data16_t new_data	=	COMBINE_DATA(&tetrisp2_vram_bg[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_bg,offset/2);
}



#define NX_1  (0x40)
#define NY_1  (0x40)

static void get_tile_info_fg(int tile_index)
{
	data16_t code_hi = tetrisp2_vram_fg[ 2 * tile_index + 0];
	data16_t code_lo = tetrisp2_vram_fg[ 2 * tile_index + 1];
	SET_TILE_INFO(
			3,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( tetrisp2_vram_fg_w )
{
	data16_t old_data	=	tetrisp2_vram_fg[offset];
	data16_t new_data	=	COMBINE_DATA(&tetrisp2_vram_fg[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_fg,offset/2);
}


static void get_tile_info_rot(int tile_index)
{
	data16_t code_hi = tetrisp2_vram_rot[ 2 * tile_index + 0];
	data16_t code_lo = tetrisp2_vram_rot[ 2 * tile_index + 1];
	SET_TILE_INFO(
			2,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( tetrisp2_vram_rot_w )
{
	data16_t old_data	=	tetrisp2_vram_rot[offset];
	data16_t new_data	=	COMBINE_DATA(&tetrisp2_vram_rot[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_rot,offset/2);
}



static void get_tile_info_rocknms_sub_bg(int tile_index)
{
	data16_t code_hi = rocknms_sub_vram_bg[ 2 * tile_index + 0];
	data16_t code_lo = rocknms_sub_vram_bg[ 2 * tile_index + 1];
	SET_TILE_INFO(
			5,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( rocknms_sub_vram_bg_w )
{
	data16_t old_data	=	rocknms_sub_vram_bg[offset];
	data16_t new_data	=	COMBINE_DATA(&rocknms_sub_vram_bg[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_sub_bg,offset/2);
}


static void get_tile_info_rocknms_sub_fg(int tile_index)
{
	data16_t code_hi = rocknms_sub_vram_fg[ 2 * tile_index + 0];
	data16_t code_lo = rocknms_sub_vram_fg[ 2 * tile_index + 1];
	SET_TILE_INFO(
			7,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( rocknms_sub_vram_fg_w )
{
	data16_t old_data	=	rocknms_sub_vram_fg[offset];
	data16_t new_data	=	COMBINE_DATA(&rocknms_sub_vram_fg[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_sub_fg,offset/2);
}


static void get_tile_info_rocknms_sub_rot(int tile_index)
{
	data16_t code_hi = rocknms_sub_vram_rot[ 2 * tile_index + 0];
	data16_t code_lo = rocknms_sub_vram_rot[ 2 * tile_index + 1];
	SET_TILE_INFO(
			6,
			code_hi,
			code_lo & 0xf,
			0)
}

WRITE16_HANDLER( rocknms_sub_vram_rot_w )
{
	data16_t old_data	=	rocknms_sub_vram_rot[offset];
	data16_t new_data	=	COMBINE_DATA(&rocknms_sub_vram_rot[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_sub_rot,offset/2);
}



VIDEO_START( tetrisp2 )
{
	tilemap_bg = tilemap_create(	get_tile_info_bg,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,NX_0,NY_0);

	tilemap_fg = tilemap_create(	get_tile_info_fg,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,NX_1,NY_1);

	tilemap_rot = tilemap_create(	get_tile_info_rot,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,NX_0*2,NY_0*2);

	sprite_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
 	priority_tmpbitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 8);

	if (!tilemap_bg || !tilemap_fg || !tilemap_rot)	return 1;
	if (!sprite_tmpbitmap || !priority_tmpbitmap) return 1;

	tilemap_set_transparent_pen(tilemap_bg,0);
	tilemap_set_transparent_pen(tilemap_fg,0);
	tilemap_set_transparent_pen(tilemap_rot,0);

	return 0;
}


VIDEO_START( rockntread )
{
	tilemap_bg = tilemap_create(	get_tile_info_bg,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16, 16, 256, 16);	// rockn ms(main),1,2,3,4

	tilemap_fg = tilemap_create(	get_tile_info_fg,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8, 8, 64, 64);

	tilemap_rot = tilemap_create(	get_tile_info_rot,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16, 16, 128, 128);

	sprite_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
 	priority_tmpbitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 8);

	if (!tilemap_bg || !tilemap_fg || !tilemap_rot)	return 1;
	if (!sprite_tmpbitmap || !priority_tmpbitmap) return 1;

	tilemap_set_transparent_pen(tilemap_bg, 0);
	tilemap_set_transparent_pen(tilemap_fg, 0);
	tilemap_set_transparent_pen(tilemap_rot, 0);

	return 0;
}


VIDEO_START( rocknms )
{
	tilemap_bg = tilemap_create(get_tile_info_bg,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					16, 16, 256, 16);	// rockn ms(main),1,2,3,4

	tilemap_fg = tilemap_create(get_tile_info_fg,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					8, 8, 64, 64);

	tilemap_rot = tilemap_create(get_tile_info_rot,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					16, 16, 128, 128);

	tilemap_sub_bg = tilemap_create(get_tile_info_rocknms_sub_bg,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					16, 16, 32, 256);	// rockn ms(sub)

	tilemap_sub_fg = tilemap_create(get_tile_info_rocknms_sub_fg,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					8, 8, 64, 64);

	tilemap_sub_rot = tilemap_create( get_tile_info_rocknms_sub_rot,tilemap_scan_rows,
					TILEMAP_TRANSPARENT,
					16, 16, 128, 128);

	rocknms_main_tmpbitmap = auto_bitmap_alloc(320*1, 224*4);
	rocknms_sub_tmpbitmap = auto_bitmap_alloc(320*1, 224*4);

	if (!tilemap_bg || !tilemap_fg || !tilemap_rot)	return 1;
	if (!tilemap_sub_bg || !tilemap_sub_fg || !tilemap_sub_rot)	return 1;
	if (!rocknms_main_tmpbitmap || !rocknms_sub_tmpbitmap)	return 1;

	tilemap_set_transparent_pen(tilemap_bg, 0);
	tilemap_set_transparent_pen(tilemap_fg, 0);
	tilemap_set_transparent_pen(tilemap_rot, 0);

	tilemap_set_transparent_pen(tilemap_sub_bg, 0);
	tilemap_set_transparent_pen(tilemap_sub_fg, 0);
	tilemap_set_transparent_pen(tilemap_sub_rot, 0);

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

	C.w			fedc ba98 7654 3210		ZOOM X (unused)

	E.w			fedc ba98 7654 3210		ZOOM Y (unused)

(*)	1 pixel granularity

***************************************************************************/

static UINT8 get_priority(struct mame_bitmap *pri, int x, int y)
{
	UINT8 *pritmp;
	int pwidth;
	int pheight;

	pwidth = pri->width;
	pheight = pri->height;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int tmp;
		tmp = x; x = y; y = tmp;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = (pwidth - x - 1);

	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = (pheight - y - 1);

	if (x >= 0 && x < pwidth && y >= 0 && y < pheight)
	{
		pritmp = (UINT8*)(pri->line[y]);
		return pritmp[x];
	}

	return 0;
}

static void set_priority(struct mame_bitmap *pri, int x, int y, int data)
{
	UINT8 *pritmp;
	int pwidth;
	int pheight;

	pwidth = pri->width;
	pheight = pri->height;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int tmp;
		tmp = x; x = y; y = tmp;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = (pwidth - x - 1);

	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = (pheight - y - 1);

	if (x >= 0 && x < pwidth && y >= 0 && y < pheight)
	{
		pritmp = (UINT8*)(pri->line[y]);
		pritmp[x] = data;
	}
}

static void tetrisp2_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, data16_t *sprram_top, size_t sprram_size, int gfxnum)
{
	int x, y, tx, ty, sx, sy, flipx, flipy;
	int xsize, ysize, xnum, ynum;
	int xstart, ystart, xend, yend, xinc, yinc;
	int code, attr, color, size;
	int flipscreen;
	int px, py;

	int min_x = cliprect->min_x;
	int max_x = cliprect->max_x;
	int min_y = cliprect->min_y;
	int max_y = cliprect->max_y;

	data16_t		*source	=	sprram_top + (sprram_size - 0x10) / 2;
	const data16_t	*finish	=	sprram_top;

	flipscreen = (tetrisp2_systemregs[0x00] & 0x02);

	for (; source >= finish; source -= 0x10/2 )
	{
		struct rectangle clip;

		attr	=	source[ 0 ];

		if ((attr & 0x0004) == 0)			continue;

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
		if (flipscreen)
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
				mdrawgfx(bitmap, Machine->gfx[gfxnum],
						code++,
						color,
						flipx, flipy,
						sx + x * 8, sy + y * 8,
						&clip,
						TRANSPARENCY_PEN, 0, 0);

				for (py = 0; py < 8; py++)
				{
					for (px = 0; px < 8; px++)
					{
						if (get_priority(priority_bitmap, sx + x * 8 + px, sy + y * 8 + py) == 0x1f)
						{
							set_priority(priority_bitmap, sx + x * 8 + px, sy + y * 8 + py, ( ( (attr & 0x00f0) >> 4 ) + 1 ));
						}
					}
				}
			}
			code	+=	(0x100/8) - xnum;
		}
	}	/* end sprite loop */
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( rockntread )
{
	static int flipscreen_old = -1;
	int flipscreen;
	int x, y;
	int priority_data;
	int asc_pri;
	int scr_pri;
	int rot_pri;
	int pen;
	int rot_ofsx, rot_ofsy;

	int min_x = cliprect->min_x;
	int max_x = cliprect->max_x;
	int min_y = cliprect->min_y;
	int max_y = cliprect->max_y;

	flipscreen = (tetrisp2_systemregs[0x00] & 0x02);

	/* Black background color */
	fillbitmap(bitmap, Machine->pens[0x0000], cliprect);
	fillbitmap(sprite_tmpbitmap, Machine->pens[0x0000], &Machine->visible_area);
	fillbitmap(priority_tmpbitmap, 0, NULL);
	fillbitmap(priority_bitmap, 0, NULL);

	/* Flip Screen */
	if (flipscreen != flipscreen_old)
	{
		flipscreen_old = flipscreen;
		tilemap_set_flip(ALL_TILEMAPS, flipscreen ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);
	}

	/* Flip Screen */
	if (flipscreen)
	{
		rot_ofsx = 0x053f;
		rot_ofsy = 0x04df;
	}
	else
	{
		rot_ofsx = 0x400;
		rot_ofsy = 0x400;
	}

	tilemap_set_scrollx(tilemap_bg, 0, (((tetrisp2_scroll_bg[ 0 ] + 0x0014) + tetrisp2_scroll_bg[ 2 ] ) & 0xffff));
	tilemap_set_scrolly(tilemap_bg, 0, (((tetrisp2_scroll_bg[ 3 ] + 0x0000) + tetrisp2_scroll_bg[ 5 ] ) & 0xffff));

	tilemap_set_scrollx(tilemap_fg, 0, tetrisp2_scroll_fg[ 2 ]);
	tilemap_set_scrolly(tilemap_fg, 0, tetrisp2_scroll_fg[ 5 ]);

	tilemap_set_scrollx(tilemap_rot, 0, (tetrisp2_rotregs[ 0 ] - rot_ofsx));
	tilemap_set_scrolly(tilemap_rot, 0, (tetrisp2_rotregs[ 2 ] - rot_ofsy));

	tetrisp2_draw_sprites(sprite_tmpbitmap,cliprect, spriteram16, spriteram_size, 0);

	copybitmap(priority_tmpbitmap, priority_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	fillbitmap(priority_bitmap, 0, cliprect);

	asc_pri = scr_pri = rot_pri = 0;

	if((tetrisp2_priority[0x2b00 / 2] & 0x00ff) == 0x0034)
		asc_pri++;
	else
		rot_pri++;

	if((tetrisp2_priority[0x2e00 / 2] & 0x00ff) == 0x0034)
		asc_pri++;
	else
		scr_pri++;

	if((tetrisp2_priority[0x3a00 / 2] & 0x00ff) == 0x000c)
		scr_pri++;
	else
		rot_pri++;

	if (rot_pri == 0)
		tilemap_draw(bitmap,cliprect, tilemap_rot, 0, 1 << 2);
	else if (scr_pri == 0)
		tilemap_draw(bitmap,cliprect, tilemap_bg,  0, 1 << 0);
	else if (asc_pri == 0)
		tilemap_draw(bitmap,cliprect, tilemap_fg,  0, 1 << 4);

	if (rot_pri == 1)
		tilemap_draw(bitmap,cliprect, tilemap_rot, 0, 1 << 2);
	else if (scr_pri == 1)
		tilemap_draw(bitmap,cliprect, tilemap_bg,  0, 1 << 0);
	else if (asc_pri == 1)
		tilemap_draw(bitmap,cliprect, tilemap_fg,  0, 1 << 4);

	if (rot_pri == 2)
		tilemap_draw(bitmap,cliprect, tilemap_rot, 0, 1 << 2);
	else if (scr_pri == 2)
		tilemap_draw(bitmap,cliprect, tilemap_bg,  0, 1 << 0);
	else if (asc_pri == 2)
		tilemap_draw(bitmap,cliprect, tilemap_fg,  0, 1 << 4);

	for (y = min_y; y < (max_y + 1); y++)
	{
		for (x = min_x; x < (max_x + 1); x++)
		{
			if (!get_priority(priority_tmpbitmap, x, y)) continue;

			priority_data = (((int)(~get_priority(priority_bitmap, x, y))) << 8) & 0x1500;
			priority_data |= 0x0a00;
			priority_data |= ((get_priority(priority_tmpbitmap, x, y) - 1) << 4);

			if (tetrisp2_priority[priority_data / 2] & 0x38) continue;

			pen = read_pixel(sprite_tmpbitmap, x, y);
			plot_pixel(bitmap, x, y, pen);
		}
	}
}

VIDEO_UPDATE( rocknms )
{
	struct rectangle rocknms_rect_main, rocknms_rect_sub;

	rocknms_rect_main.min_x = 0;
	rocknms_rect_main.min_y = 224;
	rocknms_rect_main.max_x = 320*2-1;
	rocknms_rect_main.max_y = 224*2-1;

	rocknms_rect_sub.min_x = 0;
	rocknms_rect_sub.min_y = 0;
	rocknms_rect_sub.max_x = 320-1;
	rocknms_rect_sub.max_y = 224-1;

	/* Black background color */
	fillbitmap(bitmap, Machine->pens[0x0000], cliprect);
	fillbitmap(rocknms_main_tmpbitmap, Machine->pens[0x0000], cliprect);
	fillbitmap(rocknms_sub_tmpbitmap, Machine->pens[0x8000], cliprect);

	tilemap_set_scrollx(tilemap_bg, 0, tetrisp2_scroll_bg[ 2 ] + 0x000);
	tilemap_set_scrolly(tilemap_bg, 0, tetrisp2_scroll_bg[ 5 ] + 0x000);

	tilemap_set_scrollx(tilemap_fg, 0, tetrisp2_scroll_fg[ 2 ] + 0x000);
	tilemap_set_scrolly(tilemap_fg, 0, tetrisp2_scroll_fg[ 5 ] + 0x000);

	tilemap_set_scrollx(tilemap_rot, 0, tetrisp2_rotregs[ 0 ] + 0x400);
	tilemap_set_scrolly(tilemap_rot, 0, tetrisp2_rotregs[ 2 ] + 0x400);

	tilemap_set_scrollx(tilemap_sub_bg, 0, rocknms_sub_scroll_bg[ 2 ] + 0x000);
	tilemap_set_scrolly(tilemap_sub_bg, 0, rocknms_sub_scroll_bg[ 5 ] + 0x000);

	tilemap_set_scrollx(tilemap_sub_fg, 0, rocknms_sub_scroll_fg[ 2 ] + 0x000);
	tilemap_set_scrolly(tilemap_sub_fg, 0, rocknms_sub_scroll_fg[ 5 ] + 0x000);

	tilemap_set_scrollx(tilemap_sub_rot, 0, rocknms_sub_rotregs[ 0 ] + 0x400);
	tilemap_set_scrolly(tilemap_sub_rot, 0, rocknms_sub_rotregs[ 2 ] + 0x400);

	tetrisp2_draw_sprites(bitmap,cliprect, spriteram16_2, spriteram_2_size, 4);
	tilemap_draw(bitmap,cliprect, tilemap_sub_rot, 0, 0);
	tetrisp2_draw_sprites(bitmap,cliprect, spriteram16_2, spriteram_2_size, 4);
	tilemap_draw(bitmap,cliprect, tilemap_sub_bg, 0, 0);
	tetrisp2_draw_sprites(bitmap,cliprect, spriteram16_2, spriteram_2_size, 4);
	tilemap_draw(bitmap,cliprect, tilemap_sub_fg, 0, 0);
	tetrisp2_draw_sprites(bitmap,cliprect, spriteram16_2, spriteram_2_size, 4);

	tetrisp2_draw_sprites(rocknms_main_tmpbitmap,cliprect, spriteram16, spriteram_size, 0);
	tilemap_draw(rocknms_main_tmpbitmap,cliprect, tilemap_rot, 0, 0);
	tetrisp2_draw_sprites(rocknms_main_tmpbitmap,cliprect, spriteram16, spriteram_size, 0);
	tilemap_draw(rocknms_main_tmpbitmap,cliprect, tilemap_bg, 0, 0);
	tetrisp2_draw_sprites(rocknms_main_tmpbitmap,cliprect, spriteram16, spriteram_size, 0);
	tilemap_draw(rocknms_main_tmpbitmap,cliprect, tilemap_fg, 0, 0);
	tetrisp2_draw_sprites(rocknms_main_tmpbitmap,cliprect, spriteram16, spriteram_size, 0);

	copybitmap(bitmap, rocknms_main_tmpbitmap, 0, 0,   0, 224, &rocknms_rect_main, TRANSPARENCY_NONE, 0);
//	copybitmap(bitmap, rocknms_sub_tmpbitmap,  0, 0,   0,   0, &rocknms_rect_sub,  TRANSPARENCY_NONE, 0);
}
