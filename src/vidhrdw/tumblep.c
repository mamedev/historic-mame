/***************************************************************************

   Tumblepop Video emulation - Bryan McPhail, mish@tendril.co.uk

*********************************************************************

Uses Data East custom chip 55 for backgrounds, custom chip 52 for sprites.

See Dark Seal & Caveman Ninja drivers for info on these chips.

Tumblepop is one of few games to take advantage of the playfields ability
to switch between 8*8 tiles and 16*16 tiles.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static data16_t tumblep_control_0[8];
data16_t *tumblep_pf1_data,*tumblep_pf2_data;
static struct tilemap *pf1_tilemap,*pf1_alt_tilemap,*pf2_tilemap;
static data16_t *gfx_base;
static int gfx_bank,flipscreen;

/******************************************************************************/

static void tumblep_mark_sprite_colours(void)
{
	int offs,color,i,pal_base,colmask[16];
    unsigned int *pen_usage;

	palette_init_used_colors();

	pen_usage=Machine->gfx[3]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,multi;

		sprite = spriteram16[offs+1] & 0x3fff;
		if (!sprite) continue;

		y = spriteram16[offs];
		x = spriteram16[offs+2];
		color = (x >>9) &0xf;

		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		sprite &= ~multi;

		while (multi >= 0)
		{
			colmask[color] |= pen_usage[sprite + multi];
			multi--;
		}
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	palette_recalc();
}

static void tumblep_drawsprites(struct osd_bitmap *bitmap)
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
		colour = (x >>9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

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
			drawgfx(bitmap,Machine->gfx[3],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

/******************************************************************************/

WRITE16_HANDLER( tumblep_pf1_data_w )
{
	data16_t oldword=tumblep_pf1_data[offset];
	COMBINE_DATA(&tumblep_pf1_data[offset]);
	if (oldword!=tumblep_pf1_data[offset]) {
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
		tilemap_mark_tile_dirty(pf1_alt_tilemap,offset);

	}
}

WRITE16_HANDLER( tumblep_pf2_data_w )
{
	data16_t oldword=tumblep_pf2_data[offset];
	COMBINE_DATA(&tumblep_pf2_data[offset]);
	if (oldword!=tumblep_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( tumblep_control_0_w )
{
	COMBINE_DATA(&tumblep_control_0[offset]);
}

/******************************************************************************/

static UINT32 tumblep_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

static void get_bg_tile_info(int tile_index)
{
	int tile,color;

	tile=gfx_base[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(gfx_bank,tile,color)
}

static void get_fg_tile_info(int tile_index)
{
	int tile=tumblep_pf1_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;
	SET_TILE_INFO(0,tile,color)
}

int tumblep_vh_start(void)
{
	pf1_tilemap =     tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf1_alt_tilemap = tilemap_create(get_bg_tile_info,tumblep_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf2_tilemap =     tilemap_create(get_bg_tile_info,tumblep_scan,TILEMAP_OPAQUE,     16,16,64,32);

	if (!pf1_tilemap || !pf1_alt_tilemap || !pf2_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf1_alt_tilemap,0);

	return 0;
}

/******************************************************************************/

void tumblep_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	flipscreen=tumblep_control_0[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	if (flipscreen) offs=1; else offs=-1;

	tilemap_set_scrollx( pf1_tilemap,0, tumblep_control_0[1]+offs );
	tilemap_set_scrolly( pf1_tilemap,0, tumblep_control_0[2] );
	tilemap_set_scrollx( pf1_alt_tilemap,0, tumblep_control_0[1]+offs );
	tilemap_set_scrolly( pf1_alt_tilemap,0, tumblep_control_0[2] );
	tilemap_set_scrollx( pf2_tilemap,0, tumblep_control_0[3]+offs );
	tilemap_set_scrolly( pf2_tilemap,0, tumblep_control_0[4] );

	gfx_bank=1;
	gfx_base=tumblep_pf2_data;
	tilemap_update(pf2_tilemap);
	gfx_bank=2;
	gfx_base=tumblep_pf1_data;
	tilemap_update(pf1_alt_tilemap);
	tilemap_update(pf1_tilemap);

	tumblep_mark_sprite_colours();

	tilemap_draw(bitmap,pf2_tilemap,0,0);
	if (tumblep_control_0[6]&0x80)
		tilemap_draw(bitmap,pf1_tilemap,0,0);
	else
		tilemap_draw(bitmap,pf1_alt_tilemap,0,0);
	tumblep_drawsprites(bitmap);
}

void tumblepb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,offs2;

	flipscreen=tumblep_control_0[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	if (flipscreen) offs=1; else offs=-1;
	if (flipscreen) offs2=-3; else offs2=-5;

	tilemap_set_scrollx( pf1_tilemap,0, tumblep_control_0[1]+offs2 );
	tilemap_set_scrolly( pf1_tilemap,0, tumblep_control_0[2] );
	tilemap_set_scrollx( pf1_alt_tilemap,0, tumblep_control_0[1]+offs2 );
	tilemap_set_scrolly( pf1_alt_tilemap,0, tumblep_control_0[2] );
	tilemap_set_scrollx( pf2_tilemap,0, tumblep_control_0[3]+offs );
	tilemap_set_scrolly( pf2_tilemap,0, tumblep_control_0[4] );

	gfx_bank=1;
	gfx_base=tumblep_pf2_data;
	tilemap_update(pf2_tilemap);
	gfx_bank=2;
	gfx_base=tumblep_pf1_data;
	tilemap_update(pf1_tilemap);
	tilemap_update(pf1_alt_tilemap);

	tumblep_mark_sprite_colours();

	tilemap_draw(bitmap,pf2_tilemap,0,0);
	if (tumblep_control_0[6]&0x80)
		tilemap_draw(bitmap,pf1_tilemap,0,0);
	else
		tilemap_draw(bitmap,pf1_alt_tilemap,0,0);
	tumblep_drawsprites(bitmap);
}
