/***************************************************************************

   Caveman Ninja Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

Data East custom chip 55:  Generates two playfields, playfield 1 is underneath
playfield 2.  Caveman Ninja uses two of these chips.

	16 bytes of control registers per chip.

	Word 0:
		Mask 0x0080: Flip screen
		Mask 0x007f: ?
	Word 2:
		Mask 0xffff: Playfield 2 X scroll (top playfield)
	Word 4:
		Mask 0xffff: Playfield 2 Y scroll (top playfield)
	Word 6:
		Mask 0xffff: Playfield 1 X scroll (bottom playfield)
	Word 8:
		Mask 0xffff: Playfield 1 Y scroll (bottom playfield)
	Word 0xa:
		Mask 0xc000: Playfield 1 shape??
		Mask 0x3800: Playfield 1 rowscroll style
		Mask 0x0700: Playfield 1 colscroll style

		Mask 0x00c0: Playfield 2 shape??
		Mask 0x0038: Playfield 2 rowscroll style
		Mask 0x0007: Playfield 2 colscroll style
	Word 0xc:
		Mask 0x8000: Playfield 1 is 8*8 tiles else 16*16
		Mask 0x4000: Playfield 1 rowscroll enabled
		Mask 0x2000: Playfield 1 colscroll enabled
		Mask 0x1f00: ?

		Mask 0x0080: Playfield 2 is 8*8 tiles else 16*16
		Mask 0x0040: Playfield 2 rowscroll enabled
		Mask 0x0020: Playfield 2 colscroll enabled
		Mask 0x001f: ?
	Word 0xe:
		??

Colscroll style:
	0	64 columns across bitmap
	1	32 columns across bitmap

Rowscroll style:
	0	512 rows across bitmap


Locations 0 & 0xe are mostly unknown:

							 0		14
Caveman Ninja (bottom):		0053	1100 (see below)
Caveman Ninja (top):		0010	0081
Two Crude (bottom):			0053	0000
Two Crude (top):			0010	0041
Dark Seal (bottom):			0010	0000
Dark Seal (top):			0053	4101
Tumblepop:					0010	0000
Super Burger Time:			0010	0000

Location 14 in Cninja (bottom):
 1100 = pf2 uses graphics rom 1, pf3 uses graphics rom 2
 0000 = pf2 uses graphics rom 2, pf3 uses graphics rom 2
 1111 = pf2 uses graphics rom 1, pf3 uses graphics rom 1

**************************************************************************

Sprites - Data East custom chip 52

	8 bytes per sprite, unknowns bits seem unused.

	Word 0:
		Mask 0x8000 - ?
		Mask 0x4000 - Y flip
		Mask 0x2000 - X flip
		Mask 0x1000 - Sprite flash
		Mask 0x0800 - ?
		Mask 0x0600 - Sprite height (1x, 2x, 4x, 8x)
		Mask 0x01ff - Y coordinate

	Word 2:
		Mask 0xffff - Sprite number

	Word 4:
		Mask 0x8000 - ?
		Mask 0x4000 - Sprite is drawn beneath top 8 pens of playfield 4
		Mask 0x3e00 - Colour (32 palettes, most games only use 16)
		Mask 0x01ff - X coordinate

	Word 6:
		Always unused.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *cninja_pf1_data,*cninja_pf2_data;
data16_t *cninja_pf3_data,*cninja_pf4_data;
data16_t *cninja_pf1_rowscroll,*cninja_pf2_rowscroll;
data16_t *cninja_pf3_rowscroll,*cninja_pf4_rowscroll;

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;

static data16_t cninja_control_0[8];
static data16_t cninja_control_1[8];

static int cninja_pf2_bank,cninja_pf3_bank,cninja_pf4_bank;
static int bootleg,flipscreen;
static int robocop2_pri;

/* Function for all 16x16 1024x512 layers */
static UINT32 back_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

static void get_pf2_tile_info(int tile_index)
{
	int tile=cninja_pf2_data[tile_index];

	SET_TILE_INFO(1,(tile&0xfff)|cninja_pf2_bank,(tile>>12)+48,0)
}

static void get_pf3_tile_info(int tile_index)
{
	int tile=cninja_pf3_data[tile_index];

	SET_TILE_INFO(1,(tile&0xfff)|cninja_pf3_bank,tile>>12,0)
}

static void get_pf4_tile_info(int tile_index)
{
	int tile=cninja_pf4_data[tile_index];

	SET_TILE_INFO(2,(tile&0xfff)|cninja_pf4_bank,tile>>12,0)
}

static void get_pf1_tile_info(int tile_index)
{
	int tile=cninja_pf1_data[tile_index];

	SET_TILE_INFO(0,tile&0xfff,tile>>12,0)
}

/******************************************************************************/

static int common_vh_start(void)
{
	pf2_tilemap = tilemap_create(get_pf2_tile_info,back_scan,TILEMAP_OPAQUE,16,16,64,32);
	pf3_tilemap = tilemap_create(get_pf3_tile_info,back_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_pf4_tile_info,back_scan,TILEMAP_SPLIT,16,16,64,32);
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	tilemap_set_transmask(pf4_tilemap,0,0x00ff,0xff01);

	return 0;
}

int cninja_vh_start(void)
{
	if (common_vh_start()) return 1;
	bootleg=0;
	return 0;
}

int stoneage_vh_start(void)
{
	if (common_vh_start()) return 1;
	bootleg=1; /* The bootleg has broken scroll registers */
	return 0;
}

int edrandy_vh_start(void)
{
	pf2_tilemap = tilemap_create(get_pf2_tile_info,back_scan,TILEMAP_OPAQUE,16,16,64,32);
	pf3_tilemap = tilemap_create(get_pf3_tile_info,back_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_pf4_tile_info,back_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	bootleg=0;

	return 0;
}

int robocop2_vh_start(void)
{
	pf2_tilemap = tilemap_create(get_pf2_tile_info,back_scan,TILEMAP_OPAQUE,16,16,64,32);
	pf3_tilemap = tilemap_create(get_pf3_tile_info,back_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_pf4_tile_info,back_scan,TILEMAP_TRANSPARENT,16,16,64,32);
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	bootleg=0;

	return 0;
}

/******************************************************************************/

WRITE16_HANDLER( robocop2_pri_w )
{
	robocop2_pri=data;
}

WRITE16_HANDLER( cninja_pf1_data_w )
{
	data16_t oldword=cninja_pf1_data[offset];
	COMBINE_DATA(&cninja_pf1_data[offset]);
	if (oldword!=cninja_pf1_data[offset])
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( cninja_pf2_data_w )
{
	data16_t oldword=cninja_pf2_data[offset];
	COMBINE_DATA(&cninja_pf2_data[offset]);
	if (oldword!=cninja_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( cninja_pf3_data_w )
{
	data16_t oldword=cninja_pf3_data[offset];
	COMBINE_DATA(&cninja_pf3_data[offset]);
	if (oldword!=cninja_pf3_data[offset])
		tilemap_mark_tile_dirty(pf3_tilemap,offset);
}

WRITE16_HANDLER( cninja_pf4_data_w )
{
	data16_t oldword=cninja_pf4_data[offset];
	COMBINE_DATA(&cninja_pf4_data[offset]);
	if (oldword!=cninja_pf4_data[offset])
		tilemap_mark_tile_dirty(pf4_tilemap,offset);
}

WRITE16_HANDLER( cninja_control_0_w )
{
//	if (bootleg && offset==6) {
//		COMBINE_DATA(&cninja_control_0[offset],data+0xa);
//		return;
//	}
	COMBINE_DATA(&cninja_control_0[offset]);
}

WRITE16_HANDLER( cninja_control_1_w )
{
//	if (bootleg) {
//		switch (offset) {
//			case 1:
//				COMBINE_DATA(&cninja_control_1[offset],data-2);
//				return;
//			case 3:
////				COMBINE_DATA(&cninja_control_1[offset],data+0xa);
//				return;
//		}
//	}
	COMBINE_DATA(&cninja_control_1[offset]);
}

/******************************************************************************/

WRITE16_HANDLER( cninja_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}

/******************************************************************************/

static void cninja_drawsprites(struct osd_bitmap *bitmap, int pri)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;
		sprite = buffered_spriteram16[offs+1];
		if (!sprite) continue;

		x = buffered_spriteram16[offs+2];

		/* Sprite/playfield priority */
		if ((x&0x4000) && pri==1) continue;
		if (!(x&0x4000) && pri==0) continue;

		y = buffered_spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;
		colour = (x >> 9) &0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

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

static void robocop2_drawsprites(struct osd_bitmap *bitmap, int pri)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;
		sprite = buffered_spriteram16[offs+1];
		if (!sprite) continue;

		x = buffered_spriteram16[offs+2];

		/* Sprite/playfield priority */
		if ((x&0xc000)!=pri) continue;

		y = buffered_spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;
		colour = (x >> 9) &0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		x = 304 - x;
		y = 240 - y;

		if (x>320) continue; /* Speedup */

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen) {
			y=304-y;
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

static void setup_scrolling(void)
{
	int pf23_control,pf14_control,offs;

	/* Setup scrolling */
	pf23_control=cninja_control_0[6];
	pf14_control=cninja_control_1[6];

	/* Background - Rowscroll enable */
	if (pf23_control&0x4000) {
		int scrollx=cninja_control_0[3],rows;
		tilemap_set_scroll_cols(pf2_tilemap,1);
		tilemap_set_scrolly( pf2_tilemap,0, cninja_control_0[4] );

		/* Several different rowscroll styles! */
		switch ((cninja_control_0[5]>>11)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf2_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf2_tilemap,offs, scrollx + cninja_pf2_rowscroll[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf2_tilemap,1);
		tilemap_set_scroll_cols(pf2_tilemap,1);
		tilemap_set_scrollx( pf2_tilemap,0, cninja_control_0[3] );
		tilemap_set_scrolly( pf2_tilemap,0, cninja_control_0[4] );
	}

	/* Playfield 3 */
	if (pf23_control&0x40) { /* Rowscroll */
		int scrollx=cninja_control_0[1],rows;
		tilemap_set_scroll_cols(pf3_tilemap,1);
		tilemap_set_scrolly( pf3_tilemap,0, cninja_control_0[2] );

		/* Several different rowscroll styles! */
		switch ((cninja_control_0[5]>>3)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf3_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf3_tilemap,offs, scrollx + cninja_pf3_rowscroll[offs] );
	}
	else if (pf23_control&0x20) { /* Colscroll */
		int scrolly=cninja_control_0[2];
		tilemap_set_scroll_rows(pf3_tilemap,1);
		tilemap_set_scroll_cols(pf3_tilemap,64);
		tilemap_set_scrollx( pf3_tilemap,0, cninja_control_0[1] );

		/* Used in lava level & Level 1 */
		for (offs=0 ; offs < 32;offs++)
			tilemap_set_scrolly( pf3_tilemap,offs+32, scrolly + cninja_pf3_rowscroll[offs+0x200] );
	}
	else {
		tilemap_set_scroll_rows(pf3_tilemap,1);
		tilemap_set_scroll_cols(pf3_tilemap,1);
		tilemap_set_scrollx( pf3_tilemap,0, cninja_control_0[1] );
		tilemap_set_scrolly( pf3_tilemap,0, cninja_control_0[2] );
	}

	/* Top foreground */
	if (pf14_control&0x4000) {
		int scrollx=cninja_control_1[3],rows;
		tilemap_set_scroll_cols(pf4_tilemap,1);
		tilemap_set_scrolly( pf4_tilemap,0, cninja_control_1[4] );

		/* Several different rowscroll styles! */
		switch ((cninja_control_1[5]>>11)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf4_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf4_tilemap,offs, scrollx + cninja_pf4_rowscroll[offs] );
	}
	else if (pf14_control&0x2000) { /* Colscroll */
		if (((cninja_control_1[5]>>8)&7)==4) {
			int scrolly=cninja_control_1[4];
			tilemap_set_scroll_rows(pf4_tilemap,1);
			tilemap_set_scroll_cols(pf4_tilemap,3);
			tilemap_set_scrollx( pf4_tilemap,0, cninja_control_0[1] );

			/* Used in Robocop 2 Japan intro */
			for (offs=0 ; offs < 3;offs++)
				tilemap_set_scrolly( pf4_tilemap,offs, scrolly + cninja_pf4_rowscroll[offs+0x200] );
		} else {
			int scrolly=cninja_control_1[4];
			tilemap_set_scroll_rows(pf4_tilemap,1);
			tilemap_set_scroll_cols(pf4_tilemap,64);
			tilemap_set_scrollx( pf4_tilemap,0, cninja_control_0[1] );

			/* Used in first lava level */
			for (offs=0 ; offs < 64;offs++)
				tilemap_set_scrolly( pf4_tilemap,offs, scrolly + cninja_pf4_rowscroll[offs+0x200] );
		}
	}
	else {
		tilemap_set_scroll_rows(pf4_tilemap,1);
		tilemap_set_scroll_cols(pf4_tilemap,1);
		tilemap_set_scrollx( pf4_tilemap,0, cninja_control_1[3] );
		tilemap_set_scrolly( pf4_tilemap,0, cninja_control_1[4] );
	}

	/* Playfield 1 - 8 * 8 Text */
	if (pf14_control&0x40) { /* Rowscroll */
		int scrollx=cninja_control_1[1],rows;
		tilemap_set_scroll_cols(pf1_tilemap,1);
		tilemap_set_scrolly( pf1_tilemap,0, cninja_control_1[2] );

		/* Several different rowscroll styles! */
		switch ((cninja_control_1[5]>>3)&7) {
			case 0: rows=256; break;
			case 1: rows=128; break;
			case 2: rows=64; break;
			case 3: rows=32; break;
			case 4: rows=16; break;
			case 5: rows=8; break;
			case 6: rows=4; break;
			case 7: rows=2; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf1_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf1_tilemap,offs, scrollx + cninja_pf1_rowscroll[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf1_tilemap,1);
		tilemap_set_scroll_cols(pf1_tilemap,1);
		tilemap_set_scrollx( pf1_tilemap,0, cninja_control_1[1] );
		tilemap_set_scrolly( pf1_tilemap,0, cninja_control_1[2] );
	}
}

void cninja_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	static int last_pf2_bank, last_pf3_bank;
	int pf23_control;

	/* Update flipscreen */
	flipscreen = cninja_control_1[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Handle gfx rom switching */
	pf23_control=cninja_control_0[7];
	if ((pf23_control&0xff)==0x00)
		cninja_pf3_bank=0x1000;
	else
		cninja_pf3_bank=0x0000;

	if ((pf23_control&0xff00)==0x00)
		cninja_pf2_bank=0x1000;
	else
		cninja_pf2_bank=0x0000;

	if (last_pf2_bank!=cninja_pf2_bank) tilemap_mark_all_tiles_dirty(pf2_tilemap);
	if (last_pf3_bank!=cninja_pf3_bank) tilemap_mark_all_tiles_dirty(pf3_tilemap);
	last_pf2_bank=cninja_pf2_bank;
	last_pf3_bank=cninja_pf3_bank;
	cninja_pf4_bank=0;
	setup_scrolling();

	/* Draw playfields */
	tilemap_draw(bitmap,pf2_tilemap,0,0);
	tilemap_draw(bitmap,pf3_tilemap,0,0);
	tilemap_draw(bitmap,pf4_tilemap,TILEMAP_BACK,0);
	cninja_drawsprites(bitmap,0);
	tilemap_draw(bitmap,pf4_tilemap,TILEMAP_FRONT,0);
	cninja_drawsprites(bitmap,1);
	tilemap_draw(bitmap,pf1_tilemap,0,0);
}

void edrandy_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int pf23_control;

	/* Update flipscreen */
	flipscreen = cninja_control_1[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Handle gfx rom switching */
	pf23_control=cninja_control_0[7];
	if ((pf23_control&0xff)==0x00)
		cninja_pf3_bank=0x1000;
	else
		cninja_pf3_bank=0x0000;

	if ((pf23_control&0xff00)==0x00)
		cninja_pf2_bank=0x1000;
	else
		cninja_pf2_bank=0x0000;

	cninja_pf4_bank=0;
	setup_scrolling();

	/* Draw playfields */
	tilemap_draw(bitmap,pf2_tilemap,0,0);
	tilemap_draw(bitmap,pf3_tilemap,0,0);
	tilemap_draw(bitmap,pf4_tilemap,0,0);
	cninja_drawsprites(bitmap,0);
	cninja_drawsprites(bitmap,1);
	tilemap_draw(bitmap,pf1_tilemap,0,0);
}

void robocop2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	static int last_pf2_bank,last_pf3_bank,last_pf4_bank;
	int pf23_control,pf14_control;

	/* Update flipscreen */
	flipscreen = !(cninja_control_1[0]&0x80);
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Handle gfx rom switching */
	pf23_control=cninja_control_0[7];
	pf14_control=cninja_control_1[7];
	switch (pf23_control&0xff) {
		case 0x00: cninja_pf3_bank=0x0000; break;
		case 0x11: cninja_pf3_bank=0x1000; break;
		case 0x29: cninja_pf3_bank=0x2000; break;
		default:   cninja_pf3_bank=0x0000; break;
	}
	switch (pf23_control>>8) {
		case 0x00: cninja_pf2_bank=0x0000; break;
		case 0x11: cninja_pf2_bank=0x1000; break;
		case 0x29: cninja_pf2_bank=0x2000; break;
		default:   cninja_pf2_bank=0x0000; break;
	}
	if ((pf14_control&0xff00)==0x00)
		cninja_pf4_bank=0;
	else
		cninja_pf4_bank=0x1000;

	if (last_pf2_bank!=cninja_pf2_bank) tilemap_mark_all_tiles_dirty(pf2_tilemap);
	if (last_pf3_bank!=cninja_pf3_bank) tilemap_mark_all_tiles_dirty(pf3_tilemap);
	if (last_pf4_bank!=cninja_pf4_bank) tilemap_mark_all_tiles_dirty(pf4_tilemap);
	last_pf2_bank=cninja_pf2_bank;
	last_pf3_bank=cninja_pf3_bank;
	last_pf4_bank=cninja_pf4_bank;

	setup_scrolling();

	/* Draw playfields */
	tilemap_draw(bitmap,pf2_tilemap,0,0);

	/* I think bit 0x4 is really 'turn on special graphics mode' which is used to display
		the hicolour pictures in attract mode */
	switch (robocop2_pri) {
		case 8:
			robocop2_drawsprites(bitmap,0x8000);
			tilemap_draw(bitmap,pf4_tilemap,0,0);
			robocop2_drawsprites(bitmap,0x4000);
			tilemap_draw(bitmap,pf3_tilemap,0,0);
			break;
		default:
		case 4:
		case 0:
			robocop2_drawsprites(bitmap,0x8000);
			tilemap_draw(bitmap,pf3_tilemap,0,0);
			robocop2_drawsprites(bitmap,0x4000);
			tilemap_draw(bitmap,pf4_tilemap,0,0);
			break;

	}
	robocop2_drawsprites(bitmap,0);
	tilemap_draw(bitmap,pf1_tilemap,0,0);
}
