/***************************************************************************
  Goindol

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "generic.h"

data8_t *goindol_bg_videoram;
data8_t *goindol_fg_videoram;
data8_t *goindol_fg_scrollx;
data8_t *goindol_fg_scrolly;

size_t goindol_fg_videoram_size;
size_t goindol_bg_videoram_size;
int goindol_char_bank;

static struct tilemap *bg_tilemap,*fg_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void goindol_vh_convert_color_prom(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code = goindol_fg_videoram[2*tile_index+1];
	int attr = goindol_fg_videoram[2*tile_index];
	SET_TILE_INFO(
			0,
			code | ((attr & 0x7) << 8) | (goindol_char_bank << 11),
			(attr & 0xf8) >> 3,
			0)
}

static void get_bg_tile_info(int tile_index)
{
	int code = goindol_bg_videoram[2*tile_index+1];
	int attr = goindol_bg_videoram[2*tile_index];
	SET_TILE_INFO(
			1,
			code | ((attr & 0x7) << 8) | (goindol_char_bank << 11),
			(attr & 0xf8) >> 3,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int goindol_vh_start(void)
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,      8,8,32,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( goindol_fg_videoram_w )
{
	if (goindol_fg_videoram[offset] != data)
	{
		goindol_fg_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset / 2);
	}
}

WRITE_HANDLER( goindol_bg_videoram_w )
{
	if (goindol_bg_videoram[offset] != data)
	{
		goindol_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset / 2);
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap, int gfxbank, unsigned char *sprite_ram)
{
	int offs,sx,sy,tile,palette;

	for (offs = 0 ;offs < spriteram_size; offs+=4)
	{
		sx = sprite_ram[offs];
		sy = 240-sprite_ram[offs+1];

		if ((sprite_ram[offs+1] >> 3) && (sx < 248))
		{
			tile	 = ((sprite_ram[offs+3])+((sprite_ram[offs+2] & 7) << 8));
			tile	+= tile;
			palette	 = sprite_ram[offs+2] >> 3;

			drawgfx(bitmap,Machine->gfx[gfxbank],
						tile,
						palette,
						0,0,
						sx,sy,
						&Machine->visible_area,
						TRANSPARENCY_PEN, 0);
			drawgfx(bitmap,Machine->gfx[gfxbank],
						tile+1,
						palette,
						0,0,
						sx,sy+8,
						&Machine->visible_area,
						TRANSPARENCY_PEN, 0);
		}
	}
}

void goindol_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(fg_tilemap,0,*goindol_fg_scrollx);
	tilemap_set_scrolly(fg_tilemap,0,*goindol_fg_scrolly);

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);
	draw_sprites(bitmap,1,spriteram);
	draw_sprites(bitmap,0,spriteram_2);
}
