/***************************************************************************

  vidhrdw/mcr2.c

  Functions to emulate the video hardware of the machine.

  Journey is an MCR/II game with a MCR/III sprite board so it has it's own
  routines.

***************************************************************************/

#include "driver.h"
#include "machine/mcr.h"
#include "vidhrdw/generic.h"


static UINT8 last_cocktail_flip;



/*************************************
 *
 *	Generic MCR2 palette writes
 *
 *************************************/

WRITE_HANDLER( mcr2_paletteram_w )
{
	int r, g, b;

	paletteram[offset] = data;

	/* bit 2 of the red component is taken from bit 0 of the address */
	r = ((offset & 1) << 2) + (data >> 6);
	g = (data >> 0) & 7;
	b = (data >> 3) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_change_color(offset / 2, r, g, b);
}



/*************************************
 *
 *	Generic MCR2 videoram writes
 *
 *************************************/

WRITE_HANDLER( mcr2_videoram_w )
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset & ~1] = 1;
		videoram[offset] = data;
	}
}



/*************************************
 *
 *	Background update
 *
 *************************************/

static void mcr2_update_background(struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2; offs >= 0; offs -= 2)
	{
		if (dirtybuffer[offs])
		{
			int mx = (offs / 2) % 32;
			int my = (offs / 2) / 32;

			int attr = videoram[offs + 1];
			int code = videoram[offs] + 256 * (attr & 0x01);
			int hflip = attr & 0x02;
			int vflip = attr & 0x04;
			int color = (attr & 0x18) >> 3;

			if (!mcr_cocktail_flip)
				drawgfx(bitmap, Machine->gfx[0], code, color, hflip, vflip,
						16 * mx, 16 * my, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
			else
				drawgfx(bitmap, Machine->gfx[0], code, color, !hflip, !vflip,
						16 * (31 - mx), 16 * (29 - my), &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

			dirtybuffer[offs] = 0;
		}
	}
}



/*************************************
 *
 *	Sprite update
 *
 *************************************/

void mcr2_update_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		int code, x, y, xtile, ytile, xcount, ycount, hflip, vflip, sx, sy;

		/* skip if zero */
		if (spriteram[offs] == 0)
			continue;

		/* extract the bits of information */
		code = spriteram[offs + 1] & 0x3f;
		hflip = spriteram[offs + 1] & 0x40;
		vflip = spriteram[offs + 1] & 0x80;
		x = (spriteram[offs + 2] - 3) * 2;
		y = (241 - spriteram[offs]) * 2;

		/* break into tiles, arranged on the background */
		sx = x / 16;
		sy = y / 16;
		xcount = (x & 15) ? 4 : 3;
		ycount = (y & 15) ? 4 : 3;

		for (ytile = sy; ytile < sy + ycount; ytile++)
			for (xtile = sx; xtile < sx + xcount; xtile++)
				if (xtile >= 0 && xtile < 32 && ytile >= 0 && ytile < 30)
				{
					int toffs = 32 * ytile + xtile;
					int color = videoram[toffs * 2 + 1] >> 6;

					/* make a clipping rectangle */
					struct rectangle clip;
					if (!mcr_cocktail_flip)
					{
						clip.min_x = xtile * 16;
						clip.min_y = ytile * 16;
					}
					else
					{
						clip.min_x = (31 - xtile) * 16;
						clip.min_y = (29 - ytile) * 16;
					}
					clip.max_x = clip.min_x + 15;
					clip.max_y = clip.min_y + 15;

					/* draw the sprite */
					if (!mcr_cocktail_flip)
						drawgfx(bitmap, Machine->gfx[1], code, color, hflip, vflip,
								x, y, &clip, TRANSPARENCY_PENS, 0x101);
					else
						drawgfx(bitmap, Machine->gfx[1], code, color, !hflip, !vflip,
								466 - x, 448 - y, &clip, TRANSPARENCY_PENS, 0x101);

					/* mark the tile underneath as dirty */
					dirtybuffer[toffs * 2] = 1;
				}
	}
}



/*************************************
 *
 *	Generic MCR2 redraw
 *
 *************************************/

void mcr2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* mark everything dirty on a full refresh or cocktail flip change */
	if (palette_recalc() || full_refresh || last_cocktail_flip != mcr_cocktail_flip)
		memset(dirtybuffer, 1, videoram_size);
	last_cocktail_flip = mcr_cocktail_flip;

	/* redraw the background */
	mcr2_update_background(bitmap);

	/* draw the sprites */
	mcr2_update_sprites(bitmap);
}



/*************************************
 *
 *	Journey-specific MCR2 redraw
 *
 *	Uses the MCR3 sprite drawing
 *
 *************************************/

extern void mcr3_update_sprites(struct osd_bitmap *bitmap, int color_mask, int code_xor, int dx, int dy);

void journey_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* mark everything dirty on a cocktail flip change */
	if (palette_recalc() || last_cocktail_flip != mcr_cocktail_flip)
		memset(dirtybuffer, 1, videoram_size);
	last_cocktail_flip = mcr_cocktail_flip;

	/* redraw the background */
	mcr2_update_background(tmpbitmap);

	/* copy it to the destination */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* draw the sprites */
	mcr3_update_sprites(bitmap, 0x03, 0, 0, 0);
}
