/***************************************************************************
  Great Swordsman

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

size_t gsword_spritexy_size;

UINT8 *gsword_scrolly_ram;
UINT8 *gsword_spritexy_ram;
UINT8 *gsword_spritetile_ram;
UINT8 *gsword_spriteattrib_ram;

static int charbank, charpalbank, flipscreen;

static struct tilemap *bg_tilemap;

PALETTE_INIT( josvolly )
{
	/* sprite lookup table is not original but it is almost 98% correct */

	int sprite_lookup_table[16] = { 0x00,0x02,0x05,0x8C,0x49,0xDD,0xB7,0x06,
					0xD5,0x7A,0x85,0x8D,0x27,0x1A,0x03,0x0F };
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the sprite lookup table */

	/* characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* sprites */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = sprite_lookup_table[*(color_prom++)];
}

PALETTE_INIT( gsword )
{
	/* sprite palette is not original but it is almost 98% correct */

	int sprite_palette[16] = { 0x00,0x52,0xa4,0xf0,0x38,0x18,0x81,0x87,0x0c,0x16,0x0f,0x27,0x77,0x3f,0xff,0x00 };
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[256+i] >> 0) & 1;
		bit1 = (color_prom[256+i] >> 1) & 1;
		bit2 = (color_prom[256+i] >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[256+i] >> 3) & 1;
		bit1 = (color_prom[i] >> 0) & 1;
		bit2 = (color_prom[i] >> 1) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 2) & 1;
		bit2 = (color_prom[i] >> 3) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (sprite_palette[i] >> 0) & 1;
		bit1 = (sprite_palette[i] >> 1) & 1;
		bit2 = (sprite_palette[i] >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (sprite_palette[i] >> 3) & 1;
		bit1 = (sprite_palette[i] >> 4) & 1;
		bit2 = (sprite_palette[i] >> 5) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (sprite_palette[i] >> 6) & 1;
		bit2 = (sprite_palette[i] >> 7) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(256+i,r,g,b);
	}

	color_prom += 2*256;
	/* color_prom now points to the beginning of the sprite lookup table */

	/* characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* sprites */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = 256 + *(color_prom++);
}

WRITE8_HANDLER( gsword_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( gsword_charbank_w )
{
	if (charbank != data)
	{
		charbank = data;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE8_HANDLER( gsword_videoctrl_w )
{
	if (data & 0x8f)
	{
		usrintf_showmessage("videoctrl %02x",data);
	}

	/* bits 5-6 are char palette bank */

	if (charpalbank != ((data & 0x60) >> 5))
	{
		charpalbank = (data & 0x60) >> 5;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* bit 4 is flip screen */

	if (flipscreen != (data & 0x10))
	{
		flipscreen = data & 0x10;
	    tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* bit 0 could be used but unknown */

	/* other bits unused */
}

WRITE8_HANDLER( gsword_scroll_w )
{
	tilemap_set_scrolly(bg_tilemap, 0, data);
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((charbank & 0x03) << 8);
	int color = ((code & 0x3c0) >> 6) + 16 * charpalbank;
	int flags = flipscreen ? (TILE_FLIPX | TILE_FLIPY) : 0;

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( gsword )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 64);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

void gsword_draw_sprites(struct mame_bitmap *bitmap)
{
	int offs;

	for (offs = 0; offs < gsword_spritexy_size - 1; offs+=2)
	{
		int sx,sy,flipx,flipy,spritebank,tile;

		if (gsword_spritexy_ram[offs]!=0xf1)
		{
			spritebank = 0;
			tile = gsword_spritetile_ram[offs];
			sy = 241-gsword_spritexy_ram[offs];
			sx = gsword_spritexy_ram[offs+1]-56;
			flipx = gsword_spriteattrib_ram[offs] & 0x02;
			flipy = gsword_spriteattrib_ram[offs] & 0x01;

			// Adjust sprites that should be far far right!
			if (sx<0) sx+=256;

			// Adjuste for 32x32 tiles(#128-256)
			if (tile > 127)
			{
				spritebank = 1;
				tile -= 128;
				sy-=16;
			}
			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}
			drawgfx(bitmap,Machine->gfx[1+spritebank],
					tile,
					gsword_spritetile_ram[offs+1] & 0x3f,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_COLOR, 256+15);
		}
	}
}

VIDEO_UPDATE( gsword )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	gsword_draw_sprites(bitmap);
}
