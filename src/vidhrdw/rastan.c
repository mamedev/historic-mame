/***************************************************************************

  Functions to emulate game video hardware:
  - rastan
  - rainbow islands
  - jumping

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


size_t rastan_videoram_size;
data16_t *rastan_videoram1;
data16_t *rastan_videoram3;
data16_t *rastan_scrollx;
data16_t *rastan_scrolly;

static struct tilemap *bg_tilemap, *fg_tilemap;

static int flipscreen;
static UINT16 sprite_ctrl = 0;
static UINT16 sprites_flipscreen = 0;


static void get_bg_tile_info(int tile_index)
{
	int color = rastan_videoram1[tile_index*2 ];
	int tile  = rastan_videoram1[tile_index*2 + 1];

	SET_TILE_INFO(0, tile & 0x3fff, color & 0x007f )
	tile_info.flags = TILE_FLIPYX( (color & 0xc000)>>14);
}

static void get_fg_tile_info(int tile_index)
{
	int color = rastan_videoram3[tile_index*2 ];
	int tile  = rastan_videoram3[tile_index*2 + 1];

	SET_TILE_INFO(0, tile & 0x3fff, color & 0x007f )
	tile_info.flags = TILE_FLIPYX( (color & 0xc000)>>14);
}

int rastan_vh_start (void)
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	flipscreen = 0;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

int jumping_vh_start(void)
{
	if (rastan_vh_start())
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,15);

	return 0;
}

WRITE16_HANDLER( rastan_videoram1_w )
{
	data16_t oldword = rastan_videoram1[offset];
	COMBINE_DATA (&rastan_videoram1[offset]);
	if (oldword != rastan_videoram1[offset])
		tilemap_mark_tile_dirty(bg_tilemap, offset/2 );
}

READ16_HANDLER( rastan_videoram1_r )
{
   return rastan_videoram1[offset];
}

WRITE16_HANDLER( rastan_videoram3_w )
{
	data16_t oldword = rastan_videoram3[offset];
	COMBINE_DATA (&rastan_videoram3[offset]);
	if (oldword != rastan_videoram3[offset])
		tilemap_mark_tile_dirty(fg_tilemap, offset/2 );
}

READ16_HANDLER( rastan_videoram3_r )
{
   return rastan_videoram3[offset];
}

WRITE16_HANDLER( rastan_spritectrl_w )
{
	if (offset == 0)
	{
		sprite_ctrl = data;

		/* bits 0 and 1 are coin lockout */
		coin_lockout_w(1,~data & 0x01);
		coin_lockout_w(0,~data & 0x02);

		/* bits 2 and 3 are the coin counters */
		coin_counter_w(1,data & 0x04);
		coin_counter_w(0,data & 0x08);

		/* bits 5-7 are the sprite palette bank */
		/* other bits unknown */
	}
}

WRITE16_HANDLER( rainbow_spritectrl_w )
{
	if (offset == 0)
	{
		sprite_ctrl = data;

		/* bits 0 and 1 always set [Jumping waits 15 seconds before doing this] */
		/* bits 5-7 are the sprite palette bank */
		/* other bits unknown */
	}
}

WRITE16_HANDLER( rastan_spriteflip_w )
{
	sprites_flipscreen = data;
}

WRITE16_HANDLER( rastan_flipscreen_w )
{
	if (offset == 0)
	{
		/* flipscreen when bit 0 set */
		if (flipscreen != (data & 1) )
		{
			flipscreen = data & 1;
			flip_screen_set( flipscreen );
		}
	}
}


void rastan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;

	/* Update tilemaps */
	tilemap_set_scrollx( bg_tilemap,0, -(rastan_scrollx[0] - 16) );
	tilemap_set_scrolly( bg_tilemap,0, -(rastan_scrolly[0]) );
	tilemap_set_scrollx( fg_tilemap,0, -(rastan_scrollx[1] - 16) );
	tilemap_set_scrolly( fg_tilemap,0, -(rastan_scrolly[1]) );

	tilemap_update(bg_tilemap);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	{
		int color,i;
		int colmask[128];

		memset(colmask, 0, sizeof(colmask));

		for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
		{
			int code = spriteram16[offs+2] & 0x0fff;
			if (code)
			{
				color = (spriteram16[offs] & 0x0f) | sprite_colbank;
				colmask[color] |= Machine->gfx[1]->pen_usage[code];
			}
		}
		for (color = 0;color < 128;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}
	palette_recalc();

	/* Draw playfields */
	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);

	/* Draw the sprites. 256 sprites in total */
	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		int tile = spriteram16[offs+2];
		if (tile)
		{
			int sx,sy,color,data1;
			int flipx,flipy;

			sx = spriteram16[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
 			sy = spriteram16[offs+1] & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = spriteram16[offs];
			color = (data1 & 0x0f) | sprite_colbank;
			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if ((sprites_flipscreen &1) == 0)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}

			drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}


void rainbow_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;

	/* Update tilemaps */
	tilemap_set_scrollx( bg_tilemap,0, -(rastan_scrollx[0] - 16) );
	tilemap_set_scrolly( bg_tilemap,0, -(rastan_scrolly[0]) );
	tilemap_set_scrollx( fg_tilemap,0, -(rastan_scrollx[1] - 16) );
	tilemap_set_scrolly( fg_tilemap,0, -(rastan_scrolly[1]) );

	tilemap_update(bg_tilemap);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	{
		int color,i;
		int colmask[128];

		memset(colmask, 0, sizeof(colmask));

		for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
		{
			int code = spriteram16[offs+2];
			if (code)
			{
				color = (spriteram16[offs] & 0x0f) | sprite_colbank;	/* was (spriteram16[offs] + 0x10) & 0x7f;*/

				if (code < 4096)
					colmask[color] |= Machine->gfx[1]->pen_usage[code];
				else
					colmask[color] |= Machine->gfx[2]->pen_usage[code-4096];
			}
		}
		for (color = 0;color < 128;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}
	palette_recalc();

	/* Draw playfields */
	tilemap_draw(bitmap,bg_tilemap,0,0);

	/* Draw the sprites. 256 sprites in total */
	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		int tile = spriteram16[offs+2];
		if (tile)
		{
			int sx,sy,color,data1;
			int flipx,flipy;

			sx = spriteram16[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
 			sy = spriteram16[offs+1] & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = spriteram16[offs];
			color = (data1 & 0x0f) | sprite_colbank;
			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if ((sprites_flipscreen &1) == 0)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}

			if (tile < 4096)
				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			else
				drawgfx(bitmap,Machine->gfx[2],
					tile-4096,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	tilemap_draw(bitmap,fg_tilemap,0,0);

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}


/* Jumping uses different sprite controller   */
/* than rainbow island. - values are remapped */
/* at address 0x2EA in the code. Apart from   */
/* physical layout, the main change is that   */
/* the Y settings are active low              */

void jumping_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sprite_colbank = (sprite_ctrl & 0xe0) >> 1;

	/* Update tilemaps */
	tilemap_set_scrollx( bg_tilemap,0, -(rastan_scrollx[0] - 16) );
	tilemap_set_scrolly( bg_tilemap,0,   rastan_scrolly[0]       );
	tilemap_set_scrollx( fg_tilemap,0, 16 );
	tilemap_set_scrolly( fg_tilemap,0, 0  );

	tilemap_update(bg_tilemap);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	{
		int color,i;
		int colmask[128];

		memset(colmask, 0, sizeof(colmask));

		for (offs = spriteram_size/2-8; offs >= 0; offs -= 8)
		{
			int code = spriteram16[offs];
			if (code < Machine->gfx[1]->total_elements)
			{
				color = (spriteram16[offs+4] & 0x0f) | sprite_colbank;
				colmask[color] |= Machine->gfx[1]->pen_usage[code];
			}
		}
		for (color = 0;color < 128;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[16 * color] = PALETTE_COLOR_USED;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}
	palette_recalc();

	/* Draw playfields */
	tilemap_draw(bitmap,bg_tilemap,0,0);

	/* Draw the sprites. 128 sprites in total */
	for (offs = spriteram_size/2-8; offs >= 0; offs -= 8)
	{
		int tile = spriteram16[offs];
		if (tile < Machine->gfx[1]->total_elements)
		{
			int sx,sy,color,data1;

			sy = ((spriteram16[offs+1] - 0xfff1) ^ 0xffff) & 0x1ff;
  			if (sy > 400) sy = sy - 512;
			sx = (spriteram16[offs+2] - 0x38) & 0x1ff;
			if (sx > 400) sx = sx - 512;

			data1 = spriteram16[offs+3];
			color = (spriteram16[offs+4] & 0x0f) | sprite_colbank;

			drawgfx(bitmap,Machine->gfx[1],
					tile,
					color,
					data1 & 0x40, data1 & 0x80,
					sx,sy+1,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}

	tilemap_draw(bitmap,fg_tilemap,0,0);

#if 0
	{
		char buf[80];
		sprintf(buf,"sprite_ctrl: %04x",sprite_ctrl);
		usrintf_showmessage(buf);
	}
#endif
}

