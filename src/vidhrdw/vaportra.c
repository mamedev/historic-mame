/***************************************************************************

   Vapour Trail Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

	2 Data East 55 chips for playfields (same as Dark Seal, etc)
	1 Data East MXC-06 chip for sprites (same as Bad Dudes, etc)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *vaportra_pf1_data,*vaportra_pf2_data,*vaportra_pf3_data,*vaportra_pf4_data;

static data16_t vaportra_control_0[8];
static data16_t vaportra_control_1[8];
static data16_t vaportra_control_2[2];

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static data16_t *gfx_base;
static int gfx_bank,flipscreen;

/* Function for all 16x16 1024x1024 layers */
static UINT32 vaportra_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5) + ((row & 0x20) << 6);
}

static void get_bg_tile_info(int tile_index)
{
	int tile,color;

	tile=gfx_base[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(gfx_bank,tile,color)
}

/* 8x8 top layer */
static void get_fg_tile_info(int tile_index)
{
	int tile=vaportra_pf1_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(0,tile,color)
}

/******************************************************************************/

int vaportra_vh_start(void)
{
	pf1_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	pf2_tilemap = tilemap_create(get_bg_tile_info,vaportra_scan,    TILEMAP_TRANSPARENT,16,16,64,32);
	pf3_tilemap = tilemap_create(get_bg_tile_info,vaportra_scan,    TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_bg_tile_info,vaportra_scan,    TILEMAP_TRANSPARENT,16,16,64,32);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);

	return 0;
}

/******************************************************************************/

WRITE16_HANDLER( vaportra_pf1_data_w )
{
	data16_t oldword=vaportra_pf1_data[offset];
	COMBINE_DATA(&vaportra_pf1_data[offset]);
	if (oldword!=vaportra_pf1_data[offset])
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( vaportra_pf2_data_w )
{
	data16_t oldword=vaportra_pf2_data[offset];
	COMBINE_DATA(&vaportra_pf2_data[offset]);
	if (oldword!=vaportra_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( vaportra_pf3_data_w )
{
	data16_t oldword=vaportra_pf3_data[offset];
	COMBINE_DATA(&vaportra_pf3_data[offset]);
	if (oldword!=vaportra_pf3_data[offset])
		tilemap_mark_tile_dirty(pf3_tilemap,offset);
}

WRITE16_HANDLER( vaportra_pf4_data_w )
{
	data16_t oldword=vaportra_pf4_data[offset];
	COMBINE_DATA(&vaportra_pf4_data[offset]);
	if (oldword!=vaportra_pf4_data[offset])
		tilemap_mark_tile_dirty(pf4_tilemap,offset);
}

WRITE16_HANDLER( vaportra_control_0_w )
{
	COMBINE_DATA(&vaportra_control_0[offset]);
}

WRITE16_HANDLER( vaportra_control_1_w )
{
	COMBINE_DATA(&vaportra_control_1[offset]);
}

WRITE16_HANDLER( vaportra_control_2_w )
{
	COMBINE_DATA(&vaportra_control_2[offset]);
}

/******************************************************************************/

static void update_24bitcol(int offset)
{
	UINT8 r,g,b;

	r = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset] >> 8) & 0xff;
	b = (paletteram16_2[offset] >> 0) & 0xff;

	palette_change_color(offset,r,g,b);
}

WRITE16_HANDLER( vaportra_palette_24bit_rg_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_24bitcol(offset);
}

WRITE16_HANDLER( vaportra_palette_24bit_b_w )
{
	COMBINE_DATA(&paletteram16_2[offset]);
	update_24bitcol(offset);
}

/******************************************************************************/

static void vaportra_update_palette(void)
{
	int offs,color,i,pal_base;
	int colmask[16];

	palette_init_used_colors();

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[4].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,multi;

		y = buffered_spriteram16[offs];
		if ((y&0x8000) == 0) continue;

		sprite = buffered_spriteram16[offs+1] & 0x1fff;

		x = buffered_spriteram16[offs+2];
		color = (x >> 12) &0xf;

		x = x & 0x01ff;
		if (x >= 256) x -= 512;
		x = 240 - x;
		if (x>256) continue; /* Speedup */

		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */
		sprite &= ~multi;

		while (multi >= 0)
		{
			colmask[color] |= Machine->gfx[4]->pen_usage[sprite + multi];
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

static void vaportra_drawsprites(struct osd_bitmap *bitmap, int pri)
{
	int offs,priority_value;

	priority_value=vaportra_control_2[1];

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		y = buffered_spriteram16[offs];
		if ((y&0x8000) == 0) continue;

		sprite = buffered_spriteram16[offs+1] & 0x1fff;
		x = buffered_spriteram16[offs+2];
		colour = (x >> 12) &0xf;
		if (pri && (colour>=priority_value)) continue;
		if (!pri && !(colour>=priority_value)) continue;

		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		if (x>256) continue; /* Speedup */

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
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[4],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}


void vaportra_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int pri=vaportra_control_2[0];

	/* Update flipscreen */
	if (vaportra_control_1[0]&0x80)
		flipscreen=0;
	else
		flipscreen=1;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Update scroll registers */
	tilemap_set_scrollx( pf1_tilemap,0, vaportra_control_1[1] );
	tilemap_set_scrolly( pf1_tilemap,0, vaportra_control_1[2] );
	tilemap_set_scrollx( pf2_tilemap,0, vaportra_control_0[1] );
	tilemap_set_scrolly( pf2_tilemap,0, vaportra_control_0[2] );
	tilemap_set_scrollx( pf3_tilemap,0, vaportra_control_1[3] );
	tilemap_set_scrolly( pf3_tilemap,0, vaportra_control_1[4] );
	tilemap_set_scrollx( pf4_tilemap,0, vaportra_control_0[3] );
	tilemap_set_scrolly( pf4_tilemap,0, vaportra_control_0[4] );

	pri&=0x3;

	gfx_bank=1;
	gfx_base=vaportra_pf2_data;
	tilemap_update(pf2_tilemap);

	gfx_bank=2;
	gfx_base=vaportra_pf3_data;
	tilemap_update(pf3_tilemap);

	gfx_bank=3;
	gfx_base=vaportra_pf4_data;
	tilemap_update(pf4_tilemap);

	tilemap_update(pf1_tilemap);
	vaportra_update_palette();

	/* Draw playfields */
	if (pri==0) {
		tilemap_draw(bitmap,pf4_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		tilemap_draw(bitmap,pf2_tilemap,0,0);
		vaportra_drawsprites(bitmap,0);
		tilemap_draw(bitmap,pf3_tilemap,0,0);
	}
	else if (pri==1) {
		tilemap_draw(bitmap,pf2_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		tilemap_draw(bitmap,pf4_tilemap,0,0);
		vaportra_drawsprites(bitmap,0);
		tilemap_draw(bitmap,pf3_tilemap,0,0);
	}
	else if (pri==2) {
		tilemap_draw(bitmap,pf4_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		tilemap_draw(bitmap,pf3_tilemap,0,0);
		vaportra_drawsprites(bitmap,0);
		tilemap_draw(bitmap,pf2_tilemap,0,0);
	}
	else {
		tilemap_draw(bitmap,pf2_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
		tilemap_draw(bitmap,pf3_tilemap,0,0);
		vaportra_drawsprites(bitmap,0);
		tilemap_draw(bitmap,pf4_tilemap,0,0);
	}

	vaportra_drawsprites(bitmap,1);
	tilemap_draw(bitmap,pf1_tilemap,0,0);
}
