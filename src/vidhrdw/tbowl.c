/* vidhrdw/tbowl.c */

/* see drivers/tbowl.c for more info */

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *tx_tilemap, *bg_tilemap, *bg2_tilemap;
data8_t *tbowl_txvideoram, *tbowl_bgvideoram, *tbowl_bg2videoram;

/*** Tilemap Stuff

***/

static UINT16 tbowl_xscroll;
static UINT16 tbowl_yscroll;

static UINT16 tbowl_bg2xscroll;
static UINT16 tbowl_bg2yscroll;

/* Foreground Layer (tx) Tilemap */

static void get_tx_tile_info(int tile_index)
{
	int tileno;
	int col;

	tileno = tbowl_txvideoram[tile_index] | ((tbowl_txvideoram[tile_index+0x800] & 0x07) << 8);
	col = (tbowl_txvideoram[tile_index+0x800] & 0xf0) >> 4;

	SET_TILE_INFO(0,tileno,col,0)
}

WRITE_HANDLER( tbowl_txvideoram_w )
{
	if (tbowl_txvideoram[offset] != data)
	{
		tbowl_txvideoram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0x7ff);
	}
}

/* Bottom BG Layer (bg) Tilemap */

static void get_bg_tile_info(int tile_index)
{
	int tileno;
	int col;

	tileno = tbowl_bgvideoram[tile_index] | ((tbowl_bgvideoram[tile_index+0x1000] & 0x0f) << 8);
	col = (tbowl_bgvideoram[tile_index+0x1000] & 0xf0) >> 4;

	SET_TILE_INFO(1,tileno,col,0)
}

WRITE_HANDLER( tbowl_bg2videoram_w )
{
	if (tbowl_bg2videoram[offset] != data)
	{
		tbowl_bg2videoram[offset] = data;
		tilemap_mark_tile_dirty(bg2_tilemap,offset & 0xfff);
	}
}

WRITE_HANDLER (tbowl_bgxscroll_lo)
{
tbowl_xscroll = (tbowl_xscroll & 0xff00) | data;
tilemap_set_scrollx(bg_tilemap, 0, tbowl_xscroll );
}

WRITE_HANDLER (tbowl_bgxscroll_hi)
{
tbowl_xscroll = (tbowl_xscroll & 0x00ff) | (data << 8);
tilemap_set_scrollx(bg_tilemap, 0, tbowl_xscroll );
}

WRITE_HANDLER (tbowl_bgyscroll_lo)
{
tbowl_yscroll = (tbowl_yscroll & 0xff00) | data;
tilemap_set_scrolly(bg_tilemap, 0, tbowl_yscroll );
}

WRITE_HANDLER (tbowl_bgyscroll_hi)
{
tbowl_yscroll = (tbowl_yscroll & 0x00ff) | (data << 8);
tilemap_set_scrolly(bg_tilemap, 0, tbowl_yscroll );
}

/* Middle BG Layer (bg2) Tilemaps */

static void get_bg2_tile_info(int tile_index)
{
	int tileno;
	int col;

	tileno = tbowl_bg2videoram[tile_index] | ((tbowl_bg2videoram[tile_index+0x1000] & 0x0f) << 8);
	tileno ^= 0x400;
	col = (tbowl_bg2videoram[tile_index+0x1000] & 0xf0) >> 4;

	SET_TILE_INFO(2,tileno,col,0)
}

WRITE_HANDLER( tbowl_bgvideoram_w )
{
	if (tbowl_bgvideoram[offset] != data)
	{
		tbowl_bgvideoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset & 0xfff);
	}
}

WRITE_HANDLER (tbowl_bg2xscroll_lo)
{
tbowl_bg2xscroll = (tbowl_bg2xscroll & 0xff00) | data;
tilemap_set_scrollx(bg2_tilemap, 0, tbowl_bg2xscroll );
}

WRITE_HANDLER (tbowl_bg2xscroll_hi)
{
tbowl_bg2xscroll = (tbowl_bg2xscroll & 0x00ff) | (data << 8);
tilemap_set_scrollx(bg2_tilemap, 0, tbowl_bg2xscroll );
}

WRITE_HANDLER (tbowl_bg2yscroll_lo)
{
tbowl_bg2yscroll = (tbowl_bg2yscroll & 0xff00) | data;
tilemap_set_scrolly(bg2_tilemap, 0, tbowl_bg2yscroll );
}

WRITE_HANDLER (tbowl_bg2yscroll_hi)
{
tbowl_bg2yscroll = (tbowl_bg2yscroll & 0x00ff) | (data << 8);
tilemap_set_scrolly(bg2_tilemap, 0, tbowl_bg2yscroll );
}


/*** Video Start / Update

 note: add checks for failure

*/

VIDEO_START( tbowl )
{
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	tilemap_set_transparent_pen(tx_tilemap,0);

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,128,32);
	tilemap_set_transparent_pen(bg_tilemap,0);

	bg2_tilemap = tilemap_create(get_bg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,128,32);
	tilemap_set_transparent_pen(bg2_tilemap,0);

	return 0;
}

VIDEO_UPDATE( tbowl )
{
	fillbitmap(bitmap,0x100,cliprect); /* is there a register controling the colour? looks odd when screen is blank */
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,bg2_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}


