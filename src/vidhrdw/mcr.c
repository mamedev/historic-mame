/***************************************************************************

    Midway MCR systems

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


INT8 mcr12_sprite_xoffs;
INT8 mcr12_sprite_xoffs_flip;

static struct tilemap *bg_tilemap;



/*************************************
 *
 *  Tilemap callbacks
 *
 *************************************/

/*
    The 90009 board uses 1 byte per tile:

    Byte 0:
        pppppppp = picture index

 */
static void mcr_90009_get_tile_info(int tile_index)
{
	SET_TILE_INFO(0, videoram[tile_index], 0, 0);

	/* sprite color base is constant 0x10 */
	tile_info.priority = 1;
}


/*
    The 90010 board uses 2 adjacent bytes per tile:

    Byte 0:
        pppppppp = picture index (low 8 bits)

    Byte 1:
        ss------ = sprite palette bank
        ---cc--- = tile palette bank
        -----y-- = Y flip
        ------x- = X flip
        -------p = picture index (high 1 bit)

 */
static void mcr_90010_get_tile_info(int tile_index)
{
	int data = videoram[tile_index * 2] | (videoram[tile_index * 2 + 1] << 8);
	int code = data & 0x1ff;
	int color = (data >> 11) & 3;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((data >> 9) & 3));

	/* sprite color base comes from the top 2 bits */
	tile_info.priority = (data >> 14) & 3;
}


static void twotiger_get_tile_info(int tile_index)
{
	int data = videoram[tile_index] | (videoram[tile_index + 0x400] << 8);
	int code = data & 0x1ff;
	int color = (data >> 11) & 3;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((data >> 9) & 3));

	/* sprite color base comes from the top 2 bits */
	tile_info.priority = (data >> 14) & 3;
}



/*************************************
 *
 *  Common video startup/shutdown
 *
 *************************************/

VIDEO_START( mcr )
{
	/* the tilemap callback is based on the CPU board */
	switch (mcr_cpu_board)
	{
		case 90009:
			bg_tilemap = tilemap_create(mcr_90009_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
			break;

		case 90010:
			bg_tilemap = tilemap_create(mcr_90010_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
			break;
	}
	if (!bg_tilemap)
		return 1;
	return 0;
}


VIDEO_START( twotiger )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(twotiger_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
	if (!bg_tilemap)
		return 1;
	return 0;
}



/*************************************
 *
 *  Videoram writes
 *
 *************************************/

WRITE8_HANDLER( mcr1_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}


WRITE8_HANDLER( mcr2_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);

	/* palette RAM is mapped into the upper 0x80 bytes here */
	if ((offset & 0x780) == 0x780)
	{
		/* bit 2 of the red component is taken from bit 0 of the address */
		int idx = (offset >> 1) & 0x3f;
		int r = ((offset & 1) << 2) + (data >> 6);
		int g = (data >> 0) & 7;
		int b = (data >> 3) & 7;

		/* up to 8 bits */
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 5) | (b << 2) | (b >> 1);

		palette_set_color(idx, r, g, b);
	}
}


WRITE8_HANDLER( twotiger_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset & 0x3ff);

	/* palette RAM is mapped into the upper 0x40 bytes of each bank */
	if ((offset & 0x3c0) == 0x3c0)
	{
		/* bit 2 of the red component is taken from bit 0 of the address */
		int idx = ((offset & 0x400) >> 5) | ((offset >> 1) & 0x1f);
		int r = ((offset & 1) << 2) + (data >> 6);
		int g = (data >> 0) & 7;
		int b = (data >> 3) & 7;

		/* up to 8 bits */
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 5) | (b << 2) | (b >> 1);

		palette_set_color(idx, r, g, b);
	}
}



/*************************************
 *
 *  Common sprite update
 *
 *************************************/

static void render_sprites_91399(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	int offs;

	/* render the sprites into the bitmap, ORing together */
	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		int code, x, y, sx, sy, hflip, vflip;

		/* extract the bits of information */
		code = spriteram[offs + 1] & 0x3f;
		hflip = (spriteram[offs + 1] & 0x40) ? 31 : 0;
		vflip = (spriteram[offs + 1] & 0x80) ? 31 : 0;
		sx = (spriteram[offs + 2] - 4) * 2;
		sy = (240 - spriteram[offs]) * 2;

		/* apply cocktail mode */
		if (mcr_cocktail_flip)
		{
			hflip ^= 31;
			vflip ^= 31;
			sx = 466 - sx + mcr12_sprite_xoffs_flip;
			sy = 450 - sy;
		}
		else
			sx += mcr12_sprite_xoffs;

		/* clamp within 512 */
		sx &= 0x1ff;
		sy &= 0x1ff;

		/* loop over lines in the sprite */
		for (y = 0; y < 32; y++, sy = (sy + 1) & 0x1ff)
			if (sy >= cliprect->min_y && sy <= cliprect->max_y)
			{
				UINT8 *src = gfx->gfxdata + gfx->char_modulo * code + gfx->line_modulo * (y ^ vflip);
				UINT16 *dst = (UINT16 *)bitmap->line[sy];
				UINT8 *pri = priority_bitmap->line[sy];

				/* loop over columns */
				for (x = 0; x < 32; x++)
				{
					int tx = (sx + x) & 0x1ff;
					int pix = pri[tx] | src[x ^ hflip];

					/* update the effective sprite pixel */
					pri[tx] = pix;

					/* only draw if the low 3 bits are set */
					if (pix & 0x07)
						dst[tx] = pix;
				}
			}
	}
}



/*************************************
 *
 *  Main refresh routines
 *
 *************************************/

VIDEO_UPDATE( mcr )
{
	/* update the flip state */
	tilemap_set_flip(bg_tilemap, mcr_cocktail_flip ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

	/* draw the background */
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0x00);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 1, 0x10);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 2, 0x20);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 3, 0x30);

	/* update the sprites and render them */
	switch (mcr_sprite_board)
	{
		case 91399:
			render_sprites_91399(bitmap, cliprect);
			break;
	}
}


VIDEO_UPDATE( journey )
{
	/* update the flip state */
	tilemap_set_flip(bg_tilemap, mcr_cocktail_flip ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

	/* draw the background */
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0x00);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 1, 0x10);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 2, 0x20);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 3, 0x30);

	/* draw the sprites */
	mcr3_update_sprites(bitmap, cliprect, 0x03, 0, 0, 0);
}
