/*******************************************************************************
 WWF Wrestlefest (C) 1991 Technos Japan  (vidhrdw/wwfwfest.c)
********************************************************************************
 driver by David Haywood

 see (drivers/wwfwfest.c) for more notes
*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern data16_t *fg0_videoram, *bg0_videoram, *bg1_videoram;
extern int pri;
extern int bg0_scrollx, bg0_scrolly, bg1_scrollx, bg1_scrolly;
static struct tilemap *fg0_tilemap, *bg0_tilemap, *bg1_tilemap;

/*******************************************************************************
 Write Handlers
********************************************************************************
 for writes to Video Ram
*******************************************************************************/

WRITE16_HANDLER( wwfwfest_fg0_videoram_w )
{
	int oldword = fg0_videoram[offset];
	COMBINE_DATA(&fg0_videoram[offset]);
	if (oldword != fg0_videoram[offset])
		tilemap_mark_tile_dirty(fg0_tilemap,offset/2);
}

WRITE16_HANDLER( wwfwfest_bg0_videoram_w )
{
	int oldword = bg0_videoram[offset];
	COMBINE_DATA(&bg0_videoram[offset]);
	if (oldword != bg0_videoram[offset])
		tilemap_mark_tile_dirty(bg0_tilemap,offset/2);
}

WRITE16_HANDLER( wwfwfest_bg1_videoram_w )
{
	int oldword = bg1_videoram[offset];
	COMBINE_DATA(&bg1_videoram[offset]);
	if (oldword != bg1_videoram[offset])
		tilemap_mark_tile_dirty(bg1_tilemap,offset);
}

/*******************************************************************************
 Tilemap Related Functions
*******************************************************************************/
static void get_fg0_tile_info(int tile_index)
{
	/*- FG0 RAM Format -**

	  4 bytes per tile

	  ---- ----  tttt tttt  ---- ----  ???? TTTT

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)

	  other bits unknown / unused

	  basically the same as WWF Superstar's FG0 Ram but
	  more of it and the used bytes the other way around

	**- End of Comments -*/

	data16_t *tilebase;
	int tileno;
	int colbank;
	tilebase =  &fg0_videoram[tile_index*2];
	tileno =  (tilebase[0] & 0x00ff) | ((tilebase[1] & 0x000f) << 8);
	colbank = (tilebase[1] & 0x00f0) >> 4;
	SET_TILE_INFO(
			0,
			tileno,
			colbank,
			0)
}

static void get_bg0_tile_info(int tile_index)
{
	/*- BG0 RAM Format -**

	  4 bytes per tile

	  ---- ----  fF-- CCCC  ---- TTTT tttt tttt

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)
	  f = Flip Y
	  F = Flip X

	  other bits unknown / unused

	**- End of Comments -*/

	data16_t *tilebase;
	int tileno,colbank;

	tilebase =  &bg0_videoram[tile_index*2];
	tileno =  (tilebase[1] & 0x0fff);
	colbank = (tilebase[0] & 0x000f);
	SET_TILE_INFO(
			2,
			tileno,
			colbank,
			TILE_FLIPYX((tilebase[0] & 0x00c0) >> 6))
}

static void get_bg1_tile_info(int tile_index)
{
	/*- BG1 RAM Format -**

	  2 bytes per tile

	  CCCC TTTT tttt tttt

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)

	**- End of Comments -*/

	data16_t *tilebase;
	int tileno;
	int colbank;
	tilebase =  &bg1_videoram[tile_index];
	tileno =  (tilebase[0] & 0x0fff);
	colbank = (tilebase[0] & 0xf000) >> 12;
	SET_TILE_INFO(
			3,
			tileno,
			colbank,
			0)
}

/*******************************************************************************
 Sprite Related Functions
********************************************************************************
 sprite drawing could probably be improved a bit
*******************************************************************************/
static void wwfwfest_sprites_mark_colors(void)
{
	/* see wwfwfest_drawsprites for bits being used */
	int i;
	data16_t *source = spriteram16;
	data16_t *finish = source + 0x2000/2;

	while( source<finish )
	{
		int colourbank;
		char enable;

		enable = (source[1] & 0x0001);
		if (enable)	{
			colourbank = (source[4] & 0x000f);
			for (i = 0;i < 16;i++)
				palette_used_colors[0x400 + 16 * colourbank + i] = PALETTE_COLOR_USED;
		}
		source += 5;
	}
}

static void wwfwfest_drawsprites( struct osd_bitmap *bitmap )
{
	/*- SPR RAM Format -**

	  16 bytes per sprite

	  ---- ----  yyyy yyyy  ---- ----  lllF fXYE  ---- ----  nnnn nnnn  ---- ----  NNNN NNNN
	  ---- ----  ---- CCCC  ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ----

	  Yy = sprite Y Position
	  Xx = sprite X Position
	  C  = colour bank
	  f  = flip Y
	  F  = flip X
	  l  = chain sprite
	  E  = sprite enable
	  Nn = Sprite Number

	  other bits unused

	**- End of Comments -*/

	const struct rectangle *clip = &Machine->visible_area;
	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *source = spriteram16;
	data16_t *finish = source + 0x2000/2;

	while( source<finish )
	{
		int xpos, ypos, colourbank, flipx, flipy, chain, enable, number, count;

		enable = (source[1] & 0x0001);

		if (enable)	{
			xpos = +(source[5] & 0x00ff) | (source[1] & 0x0004) << 6;
			if (xpos>512-16) xpos -=512;
			ypos = (source[0] & 0x00ff) | (source[1] & 0x0002) << 7;
			ypos = (256 - ypos) & 0x1ff;
			ypos -= 16 ;
			flipx = (source[1] & 0x0010) >> 4;
			flipy = (source[1] & 0x0008) >> 3;
			chain = (source[1] & 0x00e0) >> 5;
			chain += 1;
			number = (source[2] & 0x00ff) | (source[3] & 0x00ff) << 8;
			colourbank = (source[4] & 0x000f);

			for (count=0;count<chain;count++) {
				if (flipy) {
					drawgfx(bitmap,gfx,number+count,colourbank,flipx,flipy,xpos,ypos-(16*(chain-1))+(16*count),clip,TRANSPARENCY_PEN,0);
				} else {
					drawgfx(bitmap,gfx,number+count,colourbank,flipx,flipy,xpos,ypos-16*count,clip,TRANSPARENCY_PEN,0);
				}
			}
		}
	source+=8;
	}
}

/*******************************************************************************
 Video Start and Refresh Functions
********************************************************************************
 Draw Order / Priority seems to affect where the scroll values are used also.
*******************************************************************************/

int wwfwfest_vh_start(void)
{
	fg0_tilemap = tilemap_create(get_fg0_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	bg1_tilemap = tilemap_create(get_bg1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,32,32);
	bg0_tilemap = tilemap_create(get_bg0_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,32,32);

	if (!fg0_tilemap || !bg0_tilemap || !bg1_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg0_tilemap,0);
	tilemap_set_transparent_pen(bg1_tilemap,0);
	tilemap_set_transparent_pen(bg0_tilemap,0);

	return 0;
}

void wwfwfest_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	if (pri == 0x0078) {
		tilemap_set_scrolly( bg0_tilemap, 0, bg0_scrolly  );
		tilemap_set_scrollx( bg0_tilemap, 0, bg0_scrollx  );
		tilemap_set_scrolly( bg1_tilemap, 0, bg1_scrolly  );
		tilemap_set_scrollx( bg1_tilemap, 0, bg1_scrollx  );
	} else {
		tilemap_set_scrolly( bg1_tilemap, 0, bg0_scrolly  );
		tilemap_set_scrollx( bg1_tilemap, 0, bg0_scrollx  );
		tilemap_set_scrolly( bg0_tilemap, 0, bg1_scrolly  );
		tilemap_set_scrollx( bg0_tilemap, 0, bg1_scrollx  );
	}

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	wwfwfest_sprites_mark_colors();
	palette_recalc();

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area); /* as order can change all layers have to be transparent so we have to blank bg */

	/* todo : which bits of pri are significant to the order */

	if (pri == 0x007b) {
		tilemap_draw(bitmap,bg0_tilemap,0,0);
		tilemap_draw(bitmap,bg1_tilemap,0,0);
		wwfwfest_drawsprites(bitmap);
		tilemap_draw(bitmap,fg0_tilemap,0,0);
	}

	if (pri == 0x007c) {
		tilemap_draw(bitmap,bg0_tilemap,0,0);
		wwfwfest_drawsprites(bitmap);
		tilemap_draw(bitmap,bg1_tilemap,0,0);
		tilemap_draw(bitmap,fg0_tilemap,0,0);
	}

	if (pri == 0x0078) {
		tilemap_draw(bitmap,bg1_tilemap,0,0);
		tilemap_draw(bitmap,bg0_tilemap,0,0);
		wwfwfest_drawsprites(bitmap);
		tilemap_draw(bitmap,fg0_tilemap,0,0);
	}
}
