#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *nmk_bgvideoram,*nmk_fgvideoram,*nmk_txvideoram;
data16_t *gunnail_scrollram;
static data16_t gunnail_scrolly;

static int redraw_bitmap;

static data16_t *spriteram_old,*spriteram_old2;
static int bgbank;
static int videoshift;
static int bioship_background_bank;
static int bioship_scroll[4];

static struct tilemap *bg_tilemap,*fg_tilemap,*tx_tilemap;
static struct osd_bitmap *background_bitmap;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static void macross_get_bg_tile_info(int tile_index)
{
	int code = nmk_bgvideoram[tile_index];
	SET_TILE_INFO(1,(code & 0xfff) + (bgbank << 12),code >> 12)
}

static void strahl_get_fg_tile_info(int tile_index)
{
	int code = nmk_fgvideoram[tile_index];
	SET_TILE_INFO(3,(code & 0xfff),code >> 12)
}

static void macross_get_tx_tile_info(int tile_index)
{
	int code = nmk_txvideoram[tile_index];
	SET_TILE_INFO(0,code & 0xfff,code >> 12)
}

static void bjtwin_get_bg_tile_info(int tile_index)
{
	int code = nmk_bgvideoram[tile_index];
	int bank = (code & 0x800) ? 1 : 0;
	SET_TILE_INFO(bank,(code & 0x7ff) + ((bank) ? (bgbank << 11) : 0),code >> 12)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void nmk_vh_stop(void)
{
	free(spriteram_old);
	free(spriteram_old2);
	spriteram_old = NULL;
	spriteram_old2 = NULL;
	if (background_bitmap) bitmap_free(background_bitmap);
}

int bioship_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);
	background_bitmap = bitmap_alloc(4096,256);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2 || !background_bitmap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap,15);
	tilemap_set_transparent_pen(tx_tilemap,15);
	bioship_background_bank=0;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */

	return 0;
}

int strahl_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	fg_tilemap = tilemap_create(strahl_get_fg_tile_info, bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !fg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,15);
	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;
	return 0;
}

int macross_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;

	return 0;
}

int gunnail_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);
	tilemap_set_scroll_rows(bg_tilemap,512);

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;

	return 0;
}

int macross2_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,128);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}

int bjtwin_vh_start(void)
{
	bg_tilemap = tilemap_create(bjtwin_get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,64,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ16_HANDLER( nmk_bgvideoram_r )
{
	return nmk_bgvideoram[offset];
}

WRITE16_HANDLER( nmk_bgvideoram_w )
{
	int oldword = nmk_bgvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_bgvideoram[offset] = newword;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

READ16_HANDLER( nmk_fgvideoram_r )
{
	return nmk_fgvideoram[offset];
}

WRITE16_HANDLER( nmk_fgvideoram_w )
{
	int oldword = nmk_fgvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_fgvideoram[offset] = newword;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}

READ16_HANDLER( nmk_txvideoram_r )
{
	return nmk_txvideoram[offset];
}

WRITE16_HANDLER( nmk_txvideoram_w )
{
	int oldword = nmk_txvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_txvideoram[offset] = newword;
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}

WRITE16_HANDLER( nmk_paletteram_w )
{
	int newword, r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = ((newword >> 11) & 0x1e) | ((newword >> 3) & 0x01);
	g = ((newword >>  7) & 0x1e) | ((newword >> 2) & 0x01);
	b = ((newword >>  3) & 0x1e) | ((newword >> 1) & 0x01);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset,r,g,b);

	/* This is very bad, ensure Bioships tilemap is redrawn if palette changes */
    if (offset<256) redraw_bitmap=1;
}

WRITE16_HANDLER( mustang_scroll_w )
{
	static UINT8 scroll[4];

	if (ACCESSING_MSB)
	{
		scroll[offset] = (data >> 8) & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( nmk_scroll_w )
{
	if (ACCESSING_LSB)
	{
		static UINT8 scroll[4];

		scroll[offset] = data & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( nmk_scroll_2_w )
{
	if (ACCESSING_LSB)
	{
		static UINT8 scroll[4];

		scroll[offset] = data & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(fg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(fg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( vandyke_scroll_w )
{
	static UINT16 scroll[4];

	scroll[offset] = data;

	tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + (scroll[1] >> 8));
	tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + (scroll[3] >> 8));
}

WRITE16_HANDLER( nmk_flipscreen_w )
{
	if (ACCESSING_LSB)
		flip_screen_set(data & 0x01);
}

WRITE16_HANDLER( nmk_tilebank_w )
{
	if (ACCESSING_LSB)
	{
		if (bgbank != (data & 0xff))
		{
			bgbank = data & 0xff;
			tilemap_mark_all_tiles_dirty(bg_tilemap);
		}
	}
}

WRITE16_HANDLER( bioship_scroll_w )
{
	if (ACCESSING_MSB)
		bioship_scroll[offset]=data>>8;
}

WRITE16_HANDLER( bioship_bank_w )
{
	if (ACCESSING_LSB)
	{
		if (bioship_background_bank!=((data&7)*0x1000))
			redraw_bitmap=1;
		bioship_background_bank=(data&7)*0x1000;
	}
}

WRITE16_HANDLER( gunnail_scrollx_w )
{
	COMBINE_DATA(&gunnail_scrollram[offset]);
}

WRITE16_HANDLER( gunnail_scrolly_w )
{
	COMBINE_DATA(&gunnail_scrolly);
}

/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap, int priority, int pri_mask)
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		if (spriteram_old2[offs])
		{
			int sx = (spriteram_old2[offs+4] & 0x1ff) + videoshift;
			int sy = (spriteram_old2[offs+6] & 0x1ff);
			int code = spriteram_old2[offs+3];
			int color = spriteram_old2[offs+7];
			int w = (spriteram_old2[offs+1] & 0x0f);
			int h = ((spriteram_old2[offs+1] & 0xf0) >> 4);
			int pri = spriteram_old2[offs+7]>>8;
			int xx,yy,x;
			int delta = 16;

			if ((pri&pri_mask)!=priority) continue;

			if (flip_screen)
			{
				sx = 368 - sx;
				sy = 240 - sy;
				delta = -16;
			}

			yy = h;
			do
			{
				x = sx;
				xx = w;
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							code,
							color,
							flip_screen, flip_screen,
							((x + 16) & 0x1ff) - 16,sy & 0x1ff,
							&Machine->visible_area,TRANSPARENCY_PEN,15);

					code++;
					x += delta;
				} while (--xx >= 0);

				sy += delta;
			} while (--yy >= 0);
		}
	}
}

static void mark_sprites_colors(void)
{
	int offs,pal_base,color_mask;

	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	color_mask = Machine->gfx[2]->total_colors - 1;

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		if (spriteram_old2[offs])
		{
			int color = spriteram_old2[offs+7] & color_mask;
			memset(&palette_used_colors[pal_base + 16*color],PALETTE_COLOR_USED,16);
		}
	}
}

static void mark_user_tilemap_colors(void)
{
	int offs,pal_base,colmask[16],color,code,i;
	data16_t *tilerom = (data16_t *)memory_region(REGION_USER1);

	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < 0x1000;offs++)
	{
		code = tilerom[offs+bioship_background_bank];
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= Machine->gfx[3]->pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}
}

void macross_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	tilemap_update(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0,0);
	draw_sprites(bitmap,0,0);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}

void gunnail_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;

	for (i = 0;i < 256;i++)
		tilemap_set_scrollx(bg_tilemap,(i+gunnail_scrolly) & 0x1ff,gunnail_scrollram[0] + gunnail_scrollram[i] - videoshift);
	tilemap_set_scrolly(bg_tilemap,0,gunnail_scrolly);

	macross_vh_screenrefresh(bitmap,full_refresh);
}

void bioship_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	data16_t *tilerom = (data16_t *)memory_region(REGION_USER1);
	int scrollx=-((bioship_scroll[1]&0xff) + (bioship_scroll[0]&0xff)*256);
	int scrolly=-((bioship_scroll[3]&0xff) + (bioship_scroll[2]&0xff)*256);

	tilemap_set_scrollx(tx_tilemap,0,-videoshift);
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();
	mark_user_tilemap_colors();

	if (palette_recalc()) {
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		redraw_bitmap=1;
	}

	if (redraw_bitmap) {
		int sx=0, sy=-1, offs;
		redraw_bitmap=0;

        /* Draw background from tile rom */
        for (offs = 0;offs <0x1000;offs++) {
                data16_t data = tilerom[offs+bioship_background_bank];
                int numtile = data&0xfff;
                int color = (data&0xf000)>>12;
                sy++;
				if (sy==16) {sy=0; sx++;}

                drawgfx(background_bitmap,Machine->gfx[3],
                        numtile,
                        color,
                        0,0,   /* no flip */
                        16*sx,16*sy,
                        0,TRANSPARENCY_NONE,0);

				data = tilerom[offs+0x800+bioship_background_bank];
				numtile = data&0xfff;
				color = (data&0xf000)>>12;
		        drawgfx(background_bitmap,Machine->gfx[3],
                      	numtile,
                        color,
                        0,0,   /* no flip */
                        16*sx,(16*sy)+256,
                        0,TRANSPARENCY_NONE,0);
        }
	}

	tilemap_update(ALL_TILEMAPS);
	copyscrollbitmap(bitmap,background_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
//	draw_sprites(bitmap,5,~5); /* Is this right? */
	tilemap_draw(bitmap,bg_tilemap,0,0);
	draw_sprites(bitmap,0,0);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}

void strahl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	tilemap_update(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);
	draw_sprites(bitmap,0,0);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}

void bjtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(bg_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	tilemap_update(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0,0);
	draw_sprites(bitmap,0,0);
}

void nmk_eof_callback(void)
{
	/* looks like sprites are *two* frames ahead */
	memcpy(spriteram_old2,spriteram_old,spriteram_size);
	memcpy(spriteram_old,spriteram16,spriteram_size);
}
