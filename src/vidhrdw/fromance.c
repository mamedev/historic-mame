/******************************************************************************

	Video Hardware for Video System Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/04 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static struct osd_bitmap *fromance_tmpbitmap[2];
static unsigned char *fromance_videoram[2];
static unsigned char *fromance_dirty[2];
static unsigned char *fromance_paletteram;
static int fromance_scrollx[2], fromance_scrolly[2];
static int fromance_gfxreg;
static int fromance_flipscreen;


void fromance_vh_stop(void);


/******************************************************************************


******************************************************************************/
READ_HANDLER( fromance_paletteram_r )
{
	int palram = (fromance_gfxreg & 0x40) ? 1 : 0;

	if (palram)
	{
		return fromance_paletteram[(offset + 0x800)];
	}
	else
	{
		return fromance_paletteram[offset];
	}
}

WRITE_HANDLER( fromance_paletteram_w )
{
	int palram = (fromance_gfxreg & 0x40) ? 1 : 0;
	int r, g, b;

	if (palram)
	{
		fromance_paletteram[(offset + 0x800)] = data;

		offset &= 0x7fe;

		// xRRR_RRGG_GGGB_BBBB
		r = ((fromance_paletteram[(offset + 0x800) + 1] & 0x7c) >> 2);
		g = (((fromance_paletteram[(offset + 0x800) + 1] & 0x03) << 3) | ((fromance_paletteram[(offset + 0x800) + 0] & 0xe0) >> 5));
		b = ((fromance_paletteram[(offset + 0x800) + 0] & 0x1f) >> 0);

		r = ((r << 3) | (r >> 2));
		g = ((g << 3) | (g >> 2));
		b = ((b << 3) | (b >> 2));

		palette_change_color(((offset >> 1) + 0x400), r, g, b);
	}
	else
	{
		fromance_paletteram[offset] = data;

		offset &= 0x7fe;

		// xRRR_RRGG_GGGB_BBBB
		r = ((fromance_paletteram[(offset + 0x000) + 1] & 0x7c) >> 2);
		g = (((fromance_paletteram[(offset + 0x000) + 1] & 0x03) << 3) | ((fromance_paletteram[(offset + 0x000) + 0] & 0xe0) >> 5));
		b = ((fromance_paletteram[(offset + 0x000) + 0] & 0x1f) >> 0);

		r = ((r << 3) | (r >> 2));
		g = ((g << 3) | (g >> 2));
		b = ((b << 3) | (b >> 2));

		palette_change_color(((offset >> 1) + 0x000), r, g, b);
	}
}

/******************************************************************************


******************************************************************************/
READ_HANDLER( fromance_videoram_r )
{
	int vram = (fromance_gfxreg & 0x02) ? 0 : 1;

	return fromance_videoram[vram][offset];
}

WRITE_HANDLER( fromance_videoram_w )
{
	int vram = (fromance_gfxreg & 0x02) ? 0 : 1;

	if (fromance_videoram[vram][offset] != data)
	{
		fromance_videoram[vram][offset] = data;
		fromance_dirty[vram][offset & 0x0fff] = 1;
	}
}

WRITE_HANDLER( fromance_gfxreg_w )
{
	if (fromance_gfxreg != data)
	{
		fromance_gfxreg = data;
		fromance_flipscreen = (data & 0x01);
		memset(fromance_dirty[0], 1, 0x1000);
		memset(fromance_dirty[1], 1, 0x1000);
	}
}

WRITE_HANDLER( fromance_scroll_w )
{
	if (fromance_flipscreen)
	{
		switch (offset)
		{
			case	0:
				fromance_scrollx[1] = (data + (((fromance_gfxreg & 0x08) >> 3) * 0x100) - 0x159);
				break;
			case	1:
				fromance_scrolly[1] = (data + (((fromance_gfxreg & 0x04) >> 2) * 0x100) - 0x10);
				break;
			case	2:
				fromance_scrollx[0] = (data + (((fromance_gfxreg & 0x20) >> 5) * 0x100) - 0x159);
				break;
			case	3:
				fromance_scrolly[0] = (data + (((fromance_gfxreg & 0x10) >> 4) * 0x100) - 0x10);
				break;
		}
	}
	else
	{
		switch (offset)
		{
			case	0:
				fromance_scrollx[1] = -(data + (((fromance_gfxreg & 0x08) >> 3) * 0x100) - 0x1f7);
				break;
			case	1:
				fromance_scrolly[1] = -(data + (((fromance_gfxreg & 0x04) >> 2) * 0x100) - 0xfa);
				break;
			case	2:
				fromance_scrollx[0] = -(data + (((fromance_gfxreg & 0x20) >> 5) * 0x100) - 0x1f7);
				break;
			case	3:
				fromance_scrolly[0] = -(data + (((fromance_gfxreg & 0x10) >> 4) * 0x100) - 0xfa);
				break;
		}
	}
}

/******************************************************************************


******************************************************************************/
int fromance_vh_start(void)
{
	fromance_videoram[0] = malloc(0x1000 * 3);
	fromance_videoram[1] = malloc(0x1000 * 3);

	fromance_tmpbitmap[0] = bitmap_alloc(512, 256);
	fromance_tmpbitmap[1] = bitmap_alloc(512, 256);

	fromance_dirty[0] = malloc(0x1000);
	fromance_dirty[1] = malloc(0x1000);

	fromance_paletteram = malloc(0x800 * 2);

	if ((!fromance_videoram[0]) || (!fromance_videoram[1]) || (!fromance_tmpbitmap[0]) || (!fromance_tmpbitmap[1]) || (!fromance_dirty[0]) || (!fromance_dirty[1]) || (!fromance_paletteram))
	{
		fromance_vh_stop();
		return 1;
	}

	memset(fromance_videoram[0], 0x00, 0x3000);
	memset(fromance_videoram[1], 0x00, 0x3000);

	memset(fromance_dirty[0], 1, 0x1000);
	memset(fromance_dirty[1], 1, 0x1000);

	return 0;
}

void fromance_vh_stop(void)
{
	if (fromance_paletteram) free(fromance_paletteram);
	fromance_paletteram = 0;

	if (fromance_dirty[1]) free(fromance_dirty[1]);
	fromance_dirty[1] = 0;
	if (fromance_dirty[0]) free(fromance_dirty[0]);
	fromance_dirty[0] = 0;

	if (fromance_tmpbitmap[1]) bitmap_free(fromance_tmpbitmap[1]);
	fromance_tmpbitmap[1] = 0;
	if (fromance_tmpbitmap[0]) bitmap_free(fromance_tmpbitmap[0]);
	fromance_tmpbitmap[0] = 0;

	if (fromance_videoram[1]) free(fromance_videoram[1]);
	fromance_videoram[1] = 0;
	if (fromance_videoram[0]) free(fromance_videoram[0]);
	fromance_videoram[0] = 0;
}

/******************************************************************************


******************************************************************************/
static void mark_background_colors(void)
{
	int mask1 = Machine->gfx[0]->total_elements - 1;
	int mask2 = Machine->gfx[1]->total_elements - 1;
	int colormask = Machine->gfx[0]->total_colors - 1;
	UINT16 used_colors[128];
	int code, color, offs;

	/* reset all color codes */
	memset(used_colors, 0, sizeof(used_colors));

	/* loop over tiles */
	for (offs = 0; offs < 64*64; offs++)
	{
		/* consider background 0 */
		color = fromance_videoram[0][offs] & colormask;
		code = (fromance_videoram[0][offs + 0x1000] << 8) | fromance_videoram[0][offs + 0x2000];
		code |= ((fromance_videoram[0][offs] & 0x80) >> 7) * 0x10000;
		used_colors[color] |= Machine->gfx[0]->pen_usage[code & mask1];

		/* consider background 1 */
		color = fromance_videoram[1][offs] & colormask;
		code = (fromance_videoram[1][offs + 0x1000] << 8) | fromance_videoram[1][offs + 0x2000];
		code |= ((fromance_videoram[1][offs] & 0x80) >> 7) * 0x10000;
		used_colors[color] |= Machine->gfx[1]->pen_usage[code & mask2] & 0x7fff;
	}

	/* fill in the final table */
	for (offs = 0; offs <= colormask; offs++)
	{
		UINT16 used = used_colors[offs];
		if (used)
		{
			for (color = 0; color < 15; color++)
				if (used & (1 << color))
					palette_used_colors[offs * 16 + color] = PALETTE_COLOR_USED;
			if (used & 0x8000)
				palette_used_colors[offs * 16 + 15] = PALETTE_COLOR_USED;
			else
				palette_used_colors[offs * 16 + 15] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}

void fromance_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	unsigned short saved_pens[128];
	int offs;

	/* update the palette usage */
	palette_init_used_colors();
	mark_background_colors();

	if (palette_recalc() || full_refresh)
	{
		memset(fromance_dirty[0], 1, 0x1000);
		memset(fromance_dirty[1], 1, 0x1000);
	}

	for (offs = 0; offs < 0x1000; offs++)
	{
		// bg layer
		if (fromance_dirty[0][offs])
		{
			int tile;
			int color;
			int sx, sy;
			int flipx, flipy;

			fromance_dirty[0][offs] = 0;

			tile  = ((fromance_videoram[0][0x1000 + offs] << 8) | fromance_videoram[0][0x2000 + offs]);
			tile |= ((fromance_videoram[0][offs] & 0x80) >> 7) * 0x10000;
			color = (fromance_videoram[0][offs] & 0x7f);
			flipx = 0;
			flipy = 0;
			sx = (offs % 64);
			sy = (offs / 64);

			if (fromance_flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}

			drawgfx(fromance_tmpbitmap[0], Machine->gfx[0],
					tile,
					color,
					flipx, flipy,
					(8 * sx), (4 * sy),
					0, TRANSPARENCY_NONE, 0);
		}
	}

	/* mark the transparent pens transparent before drawing to background 2 */
	for (offs = 0; offs < 128; offs++)
	{
		saved_pens[offs] = Machine->gfx[0]->colortable[offs * 16 + 15];
		Machine->gfx[0]->colortable[offs * 16 + 15] = palette_transparent_pen;
	}

	for (offs = 0; offs < 0x1000; offs++)
	{
		// fg layer
		if (fromance_dirty[1][offs])
		{
			int tile;
			int color;
			int sx, sy;
			int flipx, flipy;

			fromance_dirty[1][offs] = 0;

			tile  = ((fromance_videoram[1][0x1000 + offs] << 8) | fromance_videoram[1][0x2000 + offs]);
			tile |= ((fromance_videoram[1][offs] & 0x80) >> 7) * 0x10000;
			color = (fromance_videoram[1][offs] & 0x7f);
			flipx = 0;
			flipy = 0;
			sx = (offs % 64);
			sy = (offs / 64);

			if (fromance_flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}

			drawgfx(fromance_tmpbitmap[1], Machine->gfx[1],
					tile,
					color,
					flipx, flipy,
					(8 * sx), (4 * sy),
					0, TRANSPARENCY_NONE, 0);
		}
	}

	/* restore the saved pens */
	for (offs = 0; offs < 128; offs++)
	{
		Machine->gfx[0]->colortable[offs * 16 + 15] = saved_pens[offs];
	}

	copyscrollbitmap(bitmap, fromance_tmpbitmap[0], 1, &fromance_scrollx[0],  1, &fromance_scrolly[0], &Machine->visible_area, TRANSPARENCY_NONE, 0);
	copyscrollbitmap(bitmap, fromance_tmpbitmap[1], 1, &fromance_scrollx[1],  1, &fromance_scrolly[1], &Machine->visible_area, TRANSPARENCY_PEN, palette_transparent_pen);
}
