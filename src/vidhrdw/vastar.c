/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



data8_t *vastar_bg1videoram,*vastar_bg2videoram,*vastar_fgvideoram;
data8_t *vastar_bg1_scroll,*vastar_bg2_scroll;
data8_t *vastar_sprite_priority;

static struct tilemap *fg_tilemap, *bg1_tilemap, *bg2_tilemap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/

void vastar_vh_convert_color_prom(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = vastar_fgvideoram[tile_index + 0x800] | (vastar_fgvideoram[tile_index + 0x400] << 8);
	color = vastar_fgvideoram[tile_index];
	SET_TILE_INFO(
			0,
			code,
			color & 0x3f,
			0)
}

static void get_bg1_tile_info(int tile_index)
{
	int code, color;

	code = vastar_bg1videoram[tile_index + 0x800] | (vastar_bg1videoram[tile_index] << 8);
	color = vastar_bg1videoram[tile_index + 0xc00];
	SET_TILE_INFO(
			4,
			code,
			color & 0x3f,
			0)
}

static void get_bg2_tile_info(int tile_index)
{
	int code, color;

	code = vastar_bg2videoram[tile_index + 0x800] | (vastar_bg2videoram[tile_index] << 8);
	color = vastar_bg2videoram[tile_index + 0xc00];
	SET_TILE_INFO(
			3,
			code,
			color & 0x3f,
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int vastar_vh_start(void)
{
	fg_tilemap  = tilemap_create(get_fg_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	bg1_tilemap = tilemap_create(get_bg1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	bg2_tilemap = tilemap_create(get_bg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fg_tilemap || !bg1_tilemap || !bg2_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transparent_pen(bg1_tilemap,0);
	tilemap_set_transparent_pen(bg2_tilemap,0);

	tilemap_set_scroll_cols(bg1_tilemap, 32);
	tilemap_set_scroll_cols(bg2_tilemap, 32);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( vastar_fgvideoram_w )
{
	vastar_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( vastar_bg1videoram_w )
{
	vastar_bg1videoram[offset] = data;
	tilemap_mark_tile_dirty(bg1_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( vastar_bg2videoram_w )
{
	vastar_bg2videoram[offset] = data;
	tilemap_mark_tile_dirty(bg2_tilemap,offset & 0x3ff);
}


READ_HANDLER( vastar_bg1videoram_r )
{
	return vastar_bg1videoram[offset];
}

READ_HANDLER( vastar_bg2videoram_r )
{
	return vastar_bg2videoram[offset];
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = 0; offs < spriteram_size; offs += 2)
	{
		int code, sx, sy, color, flipx, flipy;


		code = ((spriteram_3[offs] & 0xfc) >> 2) + ((spriteram_2[offs] & 0x01) << 6)
				+ ((offs & 0x20) << 2);

		sx = spriteram_3[offs + 1];
		sy = spriteram[offs];
		color = spriteram[offs + 1] & 0x3f;
		flipx = spriteram_3[offs] & 0x02;
		flipy = spriteram_3[offs] & 0x01;

		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
		}

		if (spriteram_2[offs] & 0x08)	/* double width */
		{
			if (!flip_screen)
				sy = 224 - sy;

			drawgfx(bitmap,Machine->gfx[2],
					code/2,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			/* redraw with wraparound */
			drawgfx(bitmap,Machine->gfx[2],
					code/2,
					color,
					flipx,flipy,
					sx,sy+256,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
		else
		{
			if (!flip_screen)
				sy = 240 - sy;

			drawgfx(bitmap,Machine->gfx[1],
					code,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

void vastar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		tilemap_set_scrolly(bg1_tilemap,i,vastar_bg1_scroll[i]);
		tilemap_set_scrolly(bg2_tilemap,i,vastar_bg2_scroll[i]);
	}

	switch (*vastar_sprite_priority)
	{
	case 0:
		tilemap_draw(bitmap, bg1_tilemap, TILEMAP_IGNORE_TRANSPARENCY,0);
		draw_sprites(bitmap);
		tilemap_draw(bitmap, bg2_tilemap, 0,0);
		tilemap_draw(bitmap, fg_tilemap, 0,0);
		break;

	case 2:
		tilemap_draw(bitmap, bg1_tilemap, TILEMAP_IGNORE_TRANSPARENCY,0);
		draw_sprites(bitmap);
		tilemap_draw(bitmap, bg1_tilemap, 0,0);
		tilemap_draw(bitmap, bg2_tilemap, 0,0);
		tilemap_draw(bitmap, fg_tilemap, 0,0);
		break;

	case 3:
		tilemap_draw(bitmap, bg1_tilemap, TILEMAP_IGNORE_TRANSPARENCY,0);
		tilemap_draw(bitmap, bg2_tilemap, 0,0);
		tilemap_draw(bitmap, fg_tilemap, 0,0);
		draw_sprites(bitmap);
		break;

	default:
		logerror("Unimplemented priority %X\n", *vastar_sprite_priority);
		break;
	}
}
