/* Simple 156 based board

 code mostly from deco32.c, but modified

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *pf1_tilemap,*pf1a_tilemap,*pf2_tilemap,*pf2a_tilemap;
static int simpl156_pf1_bank,simpl156_pf2_bank;
static int simpl156_pf2_colourbank;
extern data32_t *simpl156_pf1_data,*simpl156_pf2_data;
extern data32_t *simpl156_pf12_control;
extern data32_t *simpl156_pf1_rowscroll,*simpl156_pf2_rowscroll;
static int simpl156_pf1_bank,simpl156_pf2_bank;
static int simpl156_pf1_flip,simpl156_pf2_flip;
extern data32_t *simpl156_pf12_control;
extern data32_t *simpl156_pf1_rowscroll,*simpl156_pf2_rowscroll;

WRITE32_HANDLER( simpl156_pf1_data_w )
{
	const data32_t oldword=simpl156_pf1_data[offset];
	COMBINE_DATA(&simpl156_pf1_data[offset]);

	if (oldword!=simpl156_pf1_data[offset]) {
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
		if (pf1a_tilemap && offset<0x400)
			tilemap_mark_tile_dirty(pf1a_tilemap,offset);
	}
}

WRITE32_HANDLER( simpl156_pf2_data_w )
{
	const data32_t oldword=simpl156_pf2_data[offset];
	COMBINE_DATA(&simpl156_pf2_data[offset]);

	if (oldword!=simpl156_pf2_data[offset]) {
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
		if (pf2a_tilemap && offset<0x400)
			tilemap_mark_tile_dirty(pf2a_tilemap,offset);
	}
}

static void simpl156_setup_scroll(struct tilemap *pf_tilemap, data16_t height, data8_t control0, data8_t control1, data16_t sy, data16_t sx, data32_t *rowdata, data32_t *coldata)
{
	int rows,offs;

	/* Colscroll - not fully supported yet! */
	if (control1&0x20 && coldata) {
		sy+=coldata[0];
		//usrintf_showmessage("%08x",coldata[0]);
	}

	/* Rowscroll enable */
	if (control1&0x40 && rowdata) {
		tilemap_set_scroll_cols(pf_tilemap,1);
		tilemap_set_scrolly( pf_tilemap,0, sy );

		/* Several different rowscroll styles */
		switch ((control0>>3)&0xf) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			case 8: rows=2; break;
			default: rows=1; break;
		}
		if (height<rows) rows/=2; /* 8x8 tile layers have half as many lines as 16x16 */

		tilemap_set_scroll_rows(pf_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf_tilemap,offs, sx + rowdata[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf_tilemap,1);
		tilemap_set_scroll_cols(pf_tilemap,1);
		tilemap_set_scrollx( pf_tilemap, 0, sx );
		tilemap_set_scrolly( pf_tilemap, 0, sy );
	}
}

/* based on dietgo.c */
static void simpl156_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	flip_screen = 1; // until the eeprom works ...

	for (offs = 0;offs < 0x800;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram32[offs+1]&0xffff;
		if (!sprite) continue;

		y = spriteram32[offs]&0xffff;
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram32[offs+2]&0xffff;
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

		if (flip_screen)
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
			drawgfx(bitmap,Machine->gfx[4],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}


VIDEO_UPDATE( simpl156 )
{
	/* Dirty tilemaps if any globals change */
	if (simpl156_pf1_flip!=((simpl156_pf12_control[6]>>0)&0x3))
		tilemap_mark_all_tiles_dirty(pf1_tilemap);
	if (simpl156_pf2_flip!=((simpl156_pf12_control[6]>>8)&0x3))
		tilemap_mark_all_tiles_dirty(pf2_tilemap);

	if (simpl156_pf1_bank!=  ((simpl156_pf12_control[7]>> 4)&0xf)<<12)
	{
		simpl156_pf1_bank = ((simpl156_pf12_control[7]>> 4)&0xf)<<12;
		tilemap_mark_all_tiles_dirty(pf1_tilemap);
		tilemap_mark_all_tiles_dirty(pf1a_tilemap);
	}

	if (simpl156_pf2_bank!=  ((simpl156_pf12_control[7]>> 4)&0xf)<<12)
	{
		simpl156_pf2_bank = ((simpl156_pf12_control[7]>> 12)&0xf)<<12;
		tilemap_mark_all_tiles_dirty(pf2_tilemap);
		tilemap_mark_all_tiles_dirty(pf2a_tilemap);
	}

	/* Enable registers */
	tilemap_set_enable(pf1_tilemap, simpl156_pf12_control[5]&0x0080);
	tilemap_set_enable(pf2_tilemap, simpl156_pf12_control[5]&0x8000);

	/* Setup scroll registers */
	simpl156_setup_scroll(pf1_tilemap,  256,(simpl156_pf12_control[5]>>0)&0xff,(simpl156_pf12_control[6]>>0)&0xff,simpl156_pf12_control[2],simpl156_pf12_control[1],simpl156_pf1_rowscroll,simpl156_pf1_rowscroll+0x200);
	simpl156_setup_scroll(pf1a_tilemap, 512,(simpl156_pf12_control[5]>>0)&0xff,(simpl156_pf12_control[6]>>0)&0xff,simpl156_pf12_control[2],simpl156_pf12_control[1],simpl156_pf1_rowscroll,simpl156_pf1_rowscroll+0x200);
	simpl156_setup_scroll(pf2_tilemap,  256,(simpl156_pf12_control[5]>>8)&0xff,(simpl156_pf12_control[6]>>8)&0xff,simpl156_pf12_control[4],simpl156_pf12_control[3],simpl156_pf2_rowscroll,simpl156_pf2_rowscroll+0x200);
	simpl156_setup_scroll(pf2a_tilemap, 512,(simpl156_pf12_control[5]>>8)&0xff,(simpl156_pf12_control[6]>>8)&0xff,simpl156_pf12_control[4],simpl156_pf12_control[3],simpl156_pf2_rowscroll,simpl156_pf2_rowscroll+0x200);

	/* Draw screen */
	fillbitmap(bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,pf2a_tilemap,0,16);
	/* PF1 can be in 8x8 mode or 16x16 mode */
	if (simpl156_pf12_control[6]&0x80)
		tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
	else
		tilemap_draw(bitmap,cliprect,pf1a_tilemap,0,0);

	simpl156_drawsprites(bitmap,cliprect);
}


static void get_simpl156_pf1_tile_info(int tile_index)
{
	int tile=simpl156_pf1_data[tile_index];
	SET_TILE_INFO(0,(tile&0xfff)|simpl156_pf1_bank,(tile>>12)&0xf,0)
}

static void get_simpl156_pf1a_tile_info(int tile_index)
{

	data32_t tile=simpl156_pf1_data[tile_index];
	data8_t	colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((simpl156_pf12_control[6]>>0)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((simpl156_pf12_control[6]>>0)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(1,(tile&0xfff)|simpl156_pf1_bank,colour,flags)
}

static void get_simpl156_pf2_tile_info(int tile_index)
{
	data32_t tile=simpl156_pf2_data[tile_index];
	data8_t	colour=(tile>>12)&0xf;
	data8_t flags=0;

	SET_TILE_INFO(2,(tile&0xfff)|simpl156_pf2_bank,colour+simpl156_pf2_colourbank,flags)
}

static void get_simpl156_pf2a_tile_info(int tile_index)
{
	data32_t tile=simpl156_pf2_data[tile_index];
	data8_t	colour=(tile>>12)&0xf;
	data8_t flags=0;


	if (tile&0x8000) {
		if ((simpl156_pf12_control[6]>>8)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((simpl156_pf12_control[6]>>8)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(3,(tile&0xfff)|simpl156_pf2_bank,colour+simpl156_pf2_colourbank,flags)
}

static UINT32 deco16_scan_rows(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

VIDEO_START( simpl156 )
{
	pf1_tilemap = tilemap_create(get_simpl156_pf1_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	pf1a_tilemap =tilemap_create(get_simpl156_pf1a_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf2_tilemap = tilemap_create(get_simpl156_pf2_tile_info,    tilemap_scan_rows,TILEMAP_TRANSPARENT,  8, 8,64,32);
	pf2a_tilemap =tilemap_create(get_simpl156_pf2a_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);


	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf1a_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf2a_tilemap,0);

	return 0;
}
