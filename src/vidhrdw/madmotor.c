/***************************************************************************

  Mad Motor video emulation - Bryan McPhail, mish@tendril.co.uk

  Notes:  Playfield 3 can change size between 512x1024 and 2048x256

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *madmotor_pf1_rowscroll;
data16_t *madmotor_pf1_data,*madmotor_pf2_data,*madmotor_pf3_data;

static data16_t madmotor_pf1_control[16];
static data16_t madmotor_pf2_control[16];
static data16_t madmotor_pf3_control[16];

static int flipscreen;
static struct tilemap *madmotor_pf1_tilemap,*madmotor_pf2_tilemap,*madmotor_pf3_tilemap,*madmotor_pf3a_tilemap;




/* 512 by 512 playfield, 8 by 8 tiles */
static UINT32 pf1_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((row & 0x20) << 5) + ((col & 0x20) << 6);
}

static void get_pf1_tile_info(int tile_index)
{
	int tile,color;

	tile=madmotor_pf1_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

/* 512 by 512 playfield, 16 by 16 tiles */
static UINT32 pf2_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x0f) + ((row & 0x0f) << 4) + ((row & 0x10) << 4) + ((col & 0x10) << 5);
}

static void get_pf2_tile_info(int tile_index)
{
	int tile,color;

	tile=madmotor_pf2_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

/* 512 by 1024 playfield, 16 by 16 tiles */
static UINT32 pf3_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x0f) + ((row & 0x0f) << 4) + ((row & 0x30) << 4) + ((col & 0x10) << 6);
}

static void get_pf3_tile_info(int tile_index)
{
	int tile,color;

	tile=madmotor_pf3_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
}

/* 2048 by 256 playfield, 16 by 16 tiles */
static UINT32 pf3a_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x0f) + ((row & 0x0f) << 4) + ((col & 0x70) << 4);
}

static void get_pf3a_tile_info(int tile_index)
{
	int tile,color;

	tile=madmotor_pf3_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
}

/******************************************************************************/

int madmotor_vh_start(void)
{
	madmotor_pf1_tilemap = tilemap_create(get_pf1_tile_info, pf1_scan, TILEMAP_TRANSPARENT, 8, 8, 64,64);
	madmotor_pf2_tilemap = tilemap_create(get_pf2_tile_info, pf2_scan, TILEMAP_TRANSPARENT,16,16, 32,32);
	madmotor_pf3_tilemap = tilemap_create(get_pf3_tile_info, pf3_scan, TILEMAP_OPAQUE,     16,16, 32,64);
	madmotor_pf3a_tilemap= tilemap_create(get_pf3a_tile_info,pf3a_scan,TILEMAP_OPAQUE,     16,16,128,16);

	if (!madmotor_pf1_tilemap  || !madmotor_pf2_tilemap || !madmotor_pf3_tilemap || !madmotor_pf3a_tilemap)
		return 1;

	tilemap_set_transparent_pen(madmotor_pf1_tilemap,0);
	tilemap_set_transparent_pen(madmotor_pf2_tilemap,0);
	tilemap_set_scroll_rows(madmotor_pf1_tilemap,512);

	return 0;
}

/******************************************************************************/

READ16_HANDLER( madmotor_pf1_data_r )
{
	return madmotor_pf1_data[offset];
}

READ16_HANDLER( madmotor_pf2_data_r )
{
	return madmotor_pf2_data[offset];
}

READ16_HANDLER( madmotor_pf3_data_r )
{
	return madmotor_pf3_data[offset];
}

WRITE16_HANDLER( madmotor_pf1_data_w )
{
	int oldword = madmotor_pf1_data[offset];
	COMBINE_DATA(&madmotor_pf1_data[offset]);
	if (oldword != madmotor_pf1_data[offset])
		tilemap_mark_tile_dirty(madmotor_pf1_tilemap,offset);
}

WRITE16_HANDLER( madmotor_pf2_data_w )
{
	int oldword = madmotor_pf2_data[offset];
	COMBINE_DATA(&madmotor_pf2_data[offset]);
	if (oldword != madmotor_pf2_data[offset])
		tilemap_mark_tile_dirty(madmotor_pf2_tilemap,offset);
}

WRITE16_HANDLER( madmotor_pf3_data_w )
{
	int oldword = madmotor_pf3_data[offset];
	COMBINE_DATA(&madmotor_pf3_data[offset]);
	if (oldword != madmotor_pf3_data[offset])
	{
		/* Mark the dirty position on the 512 x 1024 version */
		tilemap_mark_tile_dirty(madmotor_pf3_tilemap,offset);

		/* Mark the dirty position on the 2048 x 256 version */
		tilemap_mark_tile_dirty(madmotor_pf3a_tilemap,offset);
	}
}

WRITE16_HANDLER( madmotor_pf1_control_w )
{
	COMBINE_DATA(&madmotor_pf1_control[offset]);
}

WRITE16_HANDLER( madmotor_pf2_control_w )
{
	COMBINE_DATA(&madmotor_pf2_control[offset]);
}

WRITE16_HANDLER( madmotor_pf3_control_w )
{
	COMBINE_DATA(&madmotor_pf3_control[offset]);
}

READ16_HANDLER( madmotor_pf1_rowscroll_r )
{
	return madmotor_pf1_rowscroll[offset];
}

WRITE16_HANDLER( madmotor_pf1_rowscroll_w )
{
	COMBINE_DATA(&madmotor_pf1_rowscroll[offset]);
}

/******************************************************************************/

static void dec0_drawsprites(struct osd_bitmap *bitmap,int pri_mask,int pri_val)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		y = spriteram16[offs];
		if ((y&0x8000) == 0) continue;

		x = spriteram16[offs+2];
		colour = x >> 12;
		if ((colour & pri_mask) != pri_val) continue;

		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */
											/* multi = 0   1   3   7 */

		sprite = spriteram16[offs+1] & 0x1fff;

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

		if (flipscreen) {
			y=240-y;
			x=240-x;
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

void madmotor_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* Update flipscreen */
	if (madmotor_pf1_control[0]&0x80)
		flipscreen=1;
	else
		flipscreen=0;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Setup scroll registers */
	for (offs = 0;offs < 512;offs++)
		tilemap_set_scrollx( madmotor_pf1_tilemap,offs, madmotor_pf1_control[0x08] + madmotor_pf1_rowscroll[0x200+offs] );
	tilemap_set_scrolly( madmotor_pf1_tilemap,0, madmotor_pf1_control[0x09] );
	tilemap_set_scrollx( madmotor_pf2_tilemap,0, madmotor_pf2_control[0x08] );
	tilemap_set_scrolly( madmotor_pf2_tilemap,0, madmotor_pf2_control[0x09] );
	tilemap_set_scrollx( madmotor_pf3_tilemap,0, madmotor_pf3_control[0x08] );
	tilemap_set_scrolly( madmotor_pf3_tilemap,0, madmotor_pf3_control[0x09] );
	tilemap_set_scrollx( madmotor_pf3a_tilemap,0, madmotor_pf3_control[0x08] );
	tilemap_set_scrolly( madmotor_pf3a_tilemap,0, madmotor_pf3_control[0x09] );

	/* Draw playfields & sprites */
	if (madmotor_pf3_control[0x03]==2)
		tilemap_draw(bitmap,madmotor_pf3_tilemap,0,0);
	else
		tilemap_draw(bitmap,madmotor_pf3a_tilemap,0,0);
	tilemap_draw(bitmap,madmotor_pf2_tilemap,0,0);
	dec0_drawsprites(bitmap,0x00,0x00);
	tilemap_draw(bitmap,madmotor_pf1_tilemap,0,0);
}
