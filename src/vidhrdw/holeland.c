/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int flipscreen[2];
static int palette_offset = 0;
static struct tilemap *bg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void holeland_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		color_prom++;
	}
}

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int tile_number = videoram[tile_index] | ((attr & 0x03) << 8);

	SET_TILE_INFO(0, tile_number, palette_offset + ((attr >> 4) & 0x0f));
	tile_info.flags = TILE_FLIPYX((attr >> 2) & 0x03) | TILE_SPLIT((attr >> 4) & 1);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int holeland_vh_start( void )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,32,32);

	if (!bg_tilemap)
		return 1;

	tilemap_set_transmask(bg_tilemap,0,0xff); /* split type 0 is totally transparent in front half */
	tilemap_set_transmask(bg_tilemap,1,0x01); /* split type 1 has pen 0? transparent in front half */
	return 0;
}

WRITE_HANDLER( holeland_videoram_w )
{
	if( videoram[offset]!=data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset );
	}
}

WRITE_HANDLER( holeland_colorram_w )
{
	if( colorram[offset]!=data )
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset );
	}
}

WRITE_HANDLER( holeland_pal_offs_w )
{
	static int po[2];
	if ((data & 1) != po[offset])
	{
		po[offset] = data & 1;
		palette_offset = (po[0] + (po[1] << 1)) << 4;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( holeland_flipscreen_w )
{
	int attributes = 0;

	flipscreen[offset] = data;

	if (flipscreen[0])
		attributes |= TILEMAP_FLIPX;
	if (flipscreen[1])
		attributes |= TILEMAP_FLIPY;

	tilemap_set_flip(ALL_TILEMAPS, attributes);
}

void holeland_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,code,sx,sy,color,flipx, flipy;

	tilemap_update(ALL_TILEMAPS);
	tilemap_draw(bitmap,bg_tilemap,TILEMAP_BACK,0);

	/* Draw the sprites. */

	/* Weird, sprites entries don't start on DWORD boundary */
	for (offs = 3;offs < spriteram_size - 1;offs += 4)
	{
		sy = spriteram[offs];
		sx = spriteram[offs+2];
		if ((sy == 0xf8) || (sx == 0)) continue;

		/* Bit 7 unknown */
		code = spriteram[offs+1] & 0x7f;
		sy = 236 - sy;
		color = palette_offset + (spriteram[offs+3] >> 4);

		/* Bit 0, 1 unknown */
		flipx = spriteram[offs+3] & 0x04;
		flipy = spriteram[offs+3] & 0x08;

		if (flipscreen[0])
		{
			flipx = !flipx;
			sx = 240 - sx;
		}

		if (flipscreen[1])
		{
			flipy = !flipy;
			sy = 240 - sy;
		}

		drawgfx(bitmap, Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
	tilemap_draw(bitmap,bg_tilemap,TILEMAP_FRONT,0);
}


