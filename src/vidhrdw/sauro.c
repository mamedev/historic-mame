/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *sauro_videoram2;
extern unsigned char *sauro_colorram2;

static int scroll1;
static int scroll2;

static struct tilemap *trckydoc_bg;


WRITE_HANDLER( sauro_scroll1_w )
{
	scroll1 = data;
}


static int scroll2_map     [8] = {2, 1, 4, 3, 6, 5, 0, 7};
static int scroll2_map_flip[8] = {0, 7, 2, 1, 4, 3, 6, 5};

WRITE_HANDLER( sauro_scroll2_w )
{
	int* map = (flip_screen ? scroll2_map_flip : scroll2_map);

	scroll2 = (data & 0xf8) | map[data & 7];
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( sauro )
{
	int offs,code,sx,sy,color,flipx;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer,1,videoram_size);
	}


	/* for every character in the backround RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < videoram_size; offs ++)
	{
		if (!dirtybuffer[offs]) continue;

		dirtybuffer[offs] = 0;

		code = videoram[offs] + ((colorram[offs] & 0x07) << 8);
		sx = 8 * (offs / 32);
		sy = 8 * (offs % 32);
		color = (colorram[offs] >> 4) & 0x0f;

		flipx = colorram[offs] & 0x08;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = 248 - sx;
			sy = 248 - sy;
		}

		drawgfx(tmpbitmap,Machine->gfx[1],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				0,TRANSPARENCY_NONE,0);
	}

	if (!flip_screen)
	{
		int scroll = -scroll1;
		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll ,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
	else
	{
		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll1,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0; offs < videoram_size; offs++)
	{
		code = sauro_videoram2[offs] + ((sauro_colorram2[offs] & 0x07) << 8);

		/* Skip spaces */
		if (code == 0x19) continue;

		sx = 8 * (offs / 32);
		sy = 8 * (offs % 32);
		color = (sauro_colorram2[offs] >> 4) & 0x0f;

		flipx = sauro_colorram2[offs] & 0x08;

		sx = (sx - scroll2) & 0xff;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = 248 - sx;
			sy = 248 - sy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	};

	/* Draw the sprites. The order is important for correct priorities */

	/* Weird, sprites entries don't start on DWORD boundary */
	for (offs = 3;offs < spriteram_size - 1;offs += 4)
	{
		sy = spriteram[offs];
		if (sy == 0xf8) continue;

		code = spriteram[offs+1] + ((spriteram[offs+3] & 0x03) << 8);
		sx = spriteram[offs+2];
		sy = 236 - sy;
		color = (spriteram[offs+3] >> 4) & 0x0f;

		/* I'm not really sure how this bit works */
		if (spriteram[offs+3] & 0x08)
		{
			if (sx > 0xc0)
			{
				/* Sign extend */
				sx = (signed int)(signed char)sx;
			}
		}
		else
		{
			if (sx < 0x40) continue;
		}

		flipx = spriteram[offs+3] & 0x04;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = (235 - sx) & 0xff;  /* The &0xff is not 100% percent correct */
			sy = 240 - sy;
		}

		drawgfx(bitmap, Machine->gfx[2],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/* Tricky Doc is using tilemaps, we should convert Sauro to use them as well */

WRITE_HANDLER( trckydoc_bg_scroll_w )
{
	tilemap_set_scrollx(trckydoc_bg, 0, data);
}

WRITE_HANDLER( trckydoc_bg_videoram_w )
{
	if( videoram[offset] != data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( trckydoc_bg, offset );
	}
}

WRITE_HANDLER( trckydoc_bg_colorram_w )
{
	if( colorram[offset] != data )
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty( trckydoc_bg, offset );
	}
}

WRITE_HANDLER ( trckydoc_spriteram_mirror_w )
{
	spriteram[offset] = data;
}

static void get_tile_info_bg(int tile_index)
{
	int code, color, flag;
	code = videoram[tile_index] + ((colorram[tile_index] & 0x07) << 8);
	color = (colorram[tile_index] >> 4) & 0x0f;
	flag = colorram[tile_index] & 8 ? TILE_FLIPX : 0;

	SET_TILE_INFO(
		0,
		code,
		color,
		flag)
}


VIDEO_START( trckydoc )
{
	if((trckydoc_bg = tilemap_create(get_tile_info_bg, tilemap_scan_cols, TILEMAP_OPAQUE, 8, 8, 32, 32)) == 0)
		return 1;

	return 0;
}

VIDEO_UPDATE( trckydoc )
{
	int offs,code,sy,color,flipx,sx;

	tilemap_draw(bitmap, cliprect, trckydoc_bg, 0, 0);

	/* Weird, sprites entries don't start on DWORD boundary */
	for (offs = 3;offs < spriteram_size - 1;offs += 4)
	{
		sy = spriteram[offs];

		code = spriteram[offs+1] + ((spriteram[offs+3] & 0x01) << 8);

		sx = spriteram[offs+2]-2;
		color = (spriteram[offs+3] >> 4) & 0x0f;

		sy = 236 - sy;

		/* similar to sauro but different bit is used .. */
		if (spriteram[offs+3] & 0x02)
		{
			if (sx > 0xc0)
			{
				/* Sign extend */
				sx = (signed int)(signed char)sx;
			}
		}
		else
		{
			if (sx < 0x40) continue;
		}

		flipx = spriteram[offs+3] & 0x04;

		if (flip_screen)
		{
			flipx = !flipx;
			sx = (235 - sx) & 0xff;  /* The &0xff is not 100% percent correct */
			sy = 240 - sy;
		}
		drawgfx(bitmap, Machine->gfx[1],
				code,
				color,
				flipx,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
