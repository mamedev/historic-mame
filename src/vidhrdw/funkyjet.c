/***************************************************************************

   Funky Jet Video emulation - Bryan McPhail, mish@tendril.co.uk

*********************************************************************

Uses Data East custom chip 56 for backgrounds, custom chip 52 for sprites.

See Dark Seal & Caveman Ninja drivers for info on these chips.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *funkyjet_pf2_data,*funkyjet_pf1_data,*funkyjet_pf1_row;
static data16_t funkyjet_control_0[8];
static struct tilemap *pf1_tilemap,*pf2_tilemap;
static int flipscreen;

/******************************************************************************/

static void funkyjet_drawsprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1] & 0x3fff;
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

/******************************************************************************/

WRITE16_HANDLER( funkyjet_pf2_data_w )
{
	data16_t oldword=funkyjet_pf2_data[offset];
	COMBINE_DATA(&funkyjet_pf2_data[offset]);
	if (oldword!=funkyjet_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( funkyjet_pf1_data_w )
{
	data16_t oldword=funkyjet_pf1_data[offset];
	COMBINE_DATA(&funkyjet_pf1_data[offset]);
	if (oldword!=funkyjet_pf1_data[offset])
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( funkyjet_control_0_w )
{
	COMBINE_DATA(&funkyjet_control_0[offset]);
}

/******************************************************************************/

static UINT32 funkyjet_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

static void get_bg_tile_info(int tile_index)
{
	int tile,color;

	tile=funkyjet_pf2_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(1,tile,color,0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile=funkyjet_pf1_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;
	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( funkyjet )
{
	pf1_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	pf2_tilemap = tilemap_create(get_bg_tile_info,funkyjet_scan,    TILEMAP_OPAQUE,     16,16,64,32);

	if (!pf1_tilemap || !pf2_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);

	return 0;
}

/******************************************************************************/

VIDEO_UPDATE( funkyjet )
{
	flipscreen=funkyjet_control_0[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	tilemap_set_scrollx( pf2_tilemap,0, funkyjet_control_0[3] );
	tilemap_set_scrolly( pf2_tilemap,0, funkyjet_control_0[4] );
	tilemap_set_scrolly( pf1_tilemap,0, funkyjet_control_0[2] );

	/* Pf1 - rowscroll, there are probably different types, but only '4 rows' mode seems used */
	if (funkyjet_control_0[6]==0xc0) {
		tilemap_set_scroll_rows(pf1_tilemap,4);
		tilemap_set_scrollx( pf1_tilemap,0, funkyjet_control_0[1] + funkyjet_pf1_row[0] );
		tilemap_set_scrollx( pf1_tilemap,1, funkyjet_control_0[1] + funkyjet_pf1_row[1] );
		tilemap_set_scrollx( pf1_tilemap,2, funkyjet_control_0[1] + funkyjet_pf1_row[2] );
		tilemap_set_scrollx( pf1_tilemap,3, funkyjet_control_0[1] + funkyjet_pf1_row[3] );
	} else {
		tilemap_set_scroll_rows(pf1_tilemap,1);
		tilemap_set_scrollx( pf1_tilemap,0, funkyjet_control_0[1] );
	}

	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
	funkyjet_drawsprites(bitmap,cliprect);
}
