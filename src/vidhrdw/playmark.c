#include "driver.h"
#include "vidhrdw/generic.h"



data16_t *bigtwin_bgvideoram;
size_t bigtwin_bgvideoram_size;
data16_t *wbeachvl_videoram1,*wbeachvl_videoram2,*wbeachvl_videoram3;

static struct osd_bitmap *bgbitmap;
static int bgscrollx,bgscrolly;
static struct tilemap *tx_tilemap,*fg_tilemap,*bg_tilemap;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void bigtwin_get_tx_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram1[2*tile_index];
	UINT16 color = wbeachvl_videoram1[2*tile_index+1];
	SET_TILE_INFO(2,code,color)
}

static void bigtwin_get_fg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram2[2*tile_index];
	UINT16 color = wbeachvl_videoram2[2*tile_index+1];
	SET_TILE_INFO(1,code,color)
}


static void wbeachvl_get_tx_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram1[2*tile_index];
	UINT16 color = wbeachvl_videoram1[2*tile_index+1];
	SET_TILE_INFO(2,code,color / 4)
}

static void wbeachvl_get_fg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram2[2*tile_index];
	UINT16 color = wbeachvl_videoram2[2*tile_index+1];
	SET_TILE_INFO(1,code & 0x7fff,color / 4 + 8)
	tile_info.flags = (code & 0x8000) ? TILE_FLIPX : 0;
}

static void wbeachvl_get_bg_tile_info(int tile_index)
{
	UINT16 code = wbeachvl_videoram3[2*tile_index];
	UINT16 color = wbeachvl_videoram3[2*tile_index+1];
	SET_TILE_INFO(1,code & 0x7fff,color / 4)
	tile_info.flags = (code & 0x8000) ? TILE_FLIPX : 0;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void bigtwin_vh_stop(void)
{
	bitmap_free(bgbitmap);
	bgbitmap = 0;
}

int bigtwin_vh_start(void)
{
	bgbitmap = bitmap_alloc(512,512);

	tx_tilemap = tilemap_create(bigtwin_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	fg_tilemap = tilemap_create(bigtwin_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,16);

	if (!tx_tilemap || !fg_tilemap || !bgbitmap)
	{
		bigtwin_vh_stop();
		return 1;
	}

	tilemap_set_transparent_pen(tx_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}


int wbeachvl_vh_start(void)
{
	tx_tilemap = tilemap_create(wbeachvl_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	fg_tilemap = tilemap_create(wbeachvl_get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	bg_tilemap = tilemap_create(wbeachvl_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,64,32);

	if (!tx_tilemap || !fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,0);
	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( wbeachvl_txvideoram_w )
{
	int oldword = wbeachvl_videoram1[offset];
	COMBINE_DATA(&wbeachvl_videoram1[offset]);
	if (oldword != wbeachvl_videoram1[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset / 2);
}

WRITE16_HANDLER( wbeachvl_fgvideoram_w )
{
	int oldword = wbeachvl_videoram2[offset];
	COMBINE_DATA(&wbeachvl_videoram2[offset]);
	if (oldword != wbeachvl_videoram2[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset / 2);
}

WRITE16_HANDLER( wbeachvl_bgvideoram_w )
{
	int oldword = wbeachvl_videoram3[offset];
	COMBINE_DATA(&wbeachvl_videoram3[offset]);
	if (oldword != wbeachvl_videoram3[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset / 2);
}


WRITE16_HANDLER( bigtwin_paletteram_w )
{
	int r,g,b,val;


	COMBINE_DATA(&paletteram16[offset]);

	val = paletteram16[offset];
	r = (val >> 11) & 0x1e;
	g = (val >>  7) & 0x1e;
	b = (val >>  3) & 0x1e;

	r |= ((val & 0x08) >> 3);
	g |= ((val & 0x04) >> 2);
	b |= ((val & 0x02) >> 1);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset,r,g,b);
}

WRITE16_HANDLER( bigtwin_bgvideoram_w )
{
	int sx,sy,color;


	COMBINE_DATA(&bigtwin_bgvideoram[offset]);

	sx = offset % 512;
	sy = offset / 512;

	color = bigtwin_bgvideoram[offset] & 0xff;

	plot_pixel(bgbitmap,sx,sy,Machine->pens[256 + color]);
}


WRITE16_HANDLER( bigtwin_scroll_w )
{
	static data16_t scroll[6];


	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data+2); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);   break;
		case 2: bgscrollx = -(data+4);                    break;
		case 3: bgscrolly = (-data) & 0x1ff;              break;
		case 4: tilemap_set_scrollx(fg_tilemap,0,data+6); break;
		case 5: tilemap_set_scrolly(fg_tilemap,0,data);   break;
	}
}

WRITE16_HANDLER( wbeachvl_scroll_w )
{
	static data16_t scroll[6];


	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0: tilemap_set_scrollx(tx_tilemap,0,data+2); break;
		case 1: tilemap_set_scrolly(tx_tilemap,0,data);   break;
		case 2: tilemap_set_scrollx(fg_tilemap,0,data+4); break;
		case 3: tilemap_set_scrolly(fg_tilemap,0,data);   break;
		case 4: tilemap_set_scrollx(bg_tilemap,0,data+6); break;
		case 5: tilemap_set_scrolly(bg_tilemap,0,data);   break;
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap,int codeshift)
{
	int offs;
	int height = Machine->gfx[0]->height;
	int colordiv = Machine->gfx[0]->color_granularity / 16;

	for (offs = 4;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,code,color,flipx;

		sy = spriteram16[offs+3-4];	/* -4? what the... ??? */
		if (sy == 0x2000) return;	/* end of list marker */

		flipx = sy & 0x4000;
		sx = (spriteram16[offs+1] & 0x01ff) - 16-7;
		sy = (256-8-height - sy) & 0xff;
		code = spriteram16[offs+2] >> codeshift;
		color = (spriteram16[offs+1] & 0xfe00) >> 9;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color/colordiv,
				flipx,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}


void bigtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_used_colors[256] = PALETTE_COLOR_TRANSPARENT;	/* keep the background black */
	if (palette_recalc())
	{
		int offs;

		for (offs = 0;offs < bigtwin_bgvideoram_size/2;offs++)
			bigtwin_bgvideoram_w(offs,bigtwin_bgvideoram[offs],0);
	}

	copyscrollbitmap(bitmap,bgbitmap,1,&bgscrollx,1,&bgscrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);
	draw_sprites(bitmap,4);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}

void wbeachvl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_recalc();

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);
	draw_sprites(bitmap,0);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}
