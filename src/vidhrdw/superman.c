/***************************************************************************

  vidhrdw/superman.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

size_t supes_videoram_size;
size_t supes_attribram_size;

data16_t *supes_videoram;
data16_t *supes_attribram;



void superman_update_palette (void)
{
	unsigned short palette_map[32]; /* range of color table is 0-31 */
	int i;

	memset (palette_map, 0, sizeof (palette_map));

	/* Find colors used in the background tile plane */
	for (i = 0; i < 0x400/2; i += 0x40/2)
	{
		int i2;

		for (i2 = i; i2 < (i + 0x40/2); i2 += 2/2)
		{
			int tile;
			int color;

			color = 0;

			tile = supes_videoram[0x800/2 + i2] & 0x3fff;
			if (tile)
				color = supes_videoram[0xc00/2 + i2] >> 11;

			palette_map[color] |= Machine->gfx[0]->pen_usage[tile];

		}
	}

	/* Find colors used in the sprite plane */
	for (i = 0x3fe/2; i >= 0; i -= 2/2)
	{
		int tile;
		int color;

		color = 0;

		tile = supes_videoram[i] & 0x3fff;
		if (tile)
			color = supes_videoram[0x400/2 + i] >> 11;

		palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
	}

	/* Now tell the palette system about those colors */
	for (i = 0;i < 32;i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
				else
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
		}
		else
			memset(&palette_used_colors[i * 16],PALETTE_COLOR_UNUSED,16);
	}

	palette_recalc ();

}

void superman_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;

	superman_update_palette ();

	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

	/* Refresh the background tile plane */
	for (i = 0; i < 0x400/2; i += 0x40/2)
	{
		UINT32 x1, y1;
		int i2;

		x1 = supes_attribram[0x408/2 + (i>>1)];
		y1 = supes_attribram[0x400/2 + (i>>1)];

		for (i2 = i; i2 < (i + 0x40/2); i2 += 2/2)
		{
			int tile;

			tile = supes_videoram[0x800/2 + i2] & 0x3fff;
			if (tile)
			{
				int x, y;

				x = (        x1 + ((i2 & 0x01) << 4))  & 0x1ff;
				y = ((265 - (y1 - ((i2 & 0x1e) << 3))) &  0xff);

//				if ((x > 0) && (y > 0) && (x < 388) && (y < 272))
				{
					UINT32 flipx = supes_videoram[0x800/2 + i2] & 0x4000;
					UINT32 flipy = supes_videoram[0x800/2 + i2] & 0x8000;
					UINT32 color = supes_videoram[0xc00/2 + i2] >> 11;

					/* Some tiles are transparent, e.g. the gate, so we use TRANSPARENCY_PEN */
					drawgfx(bitmap,Machine->gfx[0],
						tile,
						color,
						flipx,flipy,
						x,y,
						&Machine->visible_area,
						TRANSPARENCY_PEN,0);
				}
			}
		}
	}

	/* Refresh the sprite plane */
	for (i = 0x3fe/2; i >= 0; i -= 2/2)
	{
		int sprite;

		sprite = supes_videoram[i] & 0x3fff;
		if (sprite)
		{
			int x, y;

			x = (      supes_videoram [0x400/2 + i])  & 0x1ff;
			y = (250 - supes_attribram[i          ])  & 0xff;

//			if ((x > 0) && (y > 0) && (x < 388) && (y < 272))
			{
				int flipy = supes_videoram[i] & 0x4000;
				int flipx = supes_videoram[i] & 0x8000;
				int color = supes_videoram[0x400/2 + i] >> 11;

				drawgfx(bitmap,Machine->gfx[0],
					sprite,
					color,
					flipx,flipy,
					x,y,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0);
			}
		}
	}
}
