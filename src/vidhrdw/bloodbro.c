/***************************************************************************

	Video Hardware for Blood Brothers

***************************************************************************/

#include "vidhrdw/generic.h"

data16_t *bloodbro_txvideoram;
data16_t *bloodbro_bgvideoram,*bloodbro_fgvideoram;
data16_t *bloodbro_scroll;

static struct tilemap *bg_tilemap,*fg_tilemap,*tx_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int code = bloodbro_bgvideoram[tile_index];
	SET_TILE_INFO(1,code & 0xfff,code >> 12)
}

static void get_fg_tile_info(int tile_index)
{
	int code = bloodbro_fgvideoram[tile_index];
	SET_TILE_INFO(2,code & 0xfff,code >> 12)
}

static void get_tx_tile_info(int tile_index)
{
	int code = bloodbro_txvideoram[tile_index];
	SET_TILE_INFO(0,code & 0xfff,code >> 12)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int bloodbro_vh_start(void)
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,32,16);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,16);
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,15);
	tilemap_set_transparent_pen(tx_tilemap,15);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( bloodbro_bgvideoram_w )
{
	int oldword = bloodbro_bgvideoram[offset];
	COMBINE_DATA(&bloodbro_bgvideoram[offset]);
	if (oldword != bloodbro_bgvideoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE16_HANDLER( bloodbro_fgvideoram_w )
{
	int oldword = bloodbro_fgvideoram[offset];
	COMBINE_DATA(&bloodbro_fgvideoram[offset]);
	if (oldword != bloodbro_fgvideoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( bloodbro_txvideoram_w )
{
	int oldword = bloodbro_txvideoram[offset];
	COMBINE_DATA(&bloodbro_txvideoram[offset]);
	if (oldword != bloodbro_txvideoram[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset);
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* SPRITE INFO (8 bytes)

   D-F?P?SS SSSSCCCC
   ---TTTTT TTTTTTTT
   -------X XXXXXXXX
   -------- YYYYYYYY */
static void bloodbro_draw_sprites( struct osd_bitmap *bitmap)
{
	int offs;
	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,x,y,width,height,attributes,tile_number,color,flipx,flipy,pri_mask;

		attributes = spriteram16[offs+0];
		if (attributes & 0x8000) continue;	/* disabled */

		width = ((attributes>>7)&7);
		height = ((attributes>>4)&7);
		pri_mask = (attributes & 0x0800) ? 0x02 : 0;
		tile_number = spriteram16[offs+1]&0x1fff;
		sx = spriteram16[offs+2]&0x1ff;
		sy = spriteram16[offs+3]&0x1ff;
		if (sx >= 256) sx -= 512;
		if (sy >= 256) sy -= 512;

		flipx = attributes & 0x2000;
		flipy = attributes & 0x4000;	/* ?? */
		color = attributes & 0xf;

		for (x = 0;x <= width;x++)
		{
			for (y = 0;y <= height;y++)
			{
				pdrawgfx(bitmap,Machine->gfx[3],
						tile_number++,
						color,
						flipx,flipy,
						flipx ? (sx + 16*(width-x)) : (sx + 16*x),flipy ? (sy + 16*(height-y)) : (sy + 16*y),
						&Machine->visible_area,TRANSPARENCY_PEN,15,
						pri_mask);
			}
		}
	}
}

static void bloodbro_mark_sprite_colors(void)
{
	int offs,i;
	int color, colmask[0x80];
	int pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0;offs <spriteram_size/2;offs += 4 )
	{
		color = spriteram16[offs+0] & 0x000f;
		colmask[color] |= 0xffff;
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] |= PALETTE_COLOR_VISIBLE;
		}
	}
}

/* SPRITE INFO (8 bytes)

   D------- YYYYYYYY
   ---TTTTT TTTTTTTT
   CCCC--F? -?--????  Priority??
   -------X XXXXXXXX
*/

static void weststry_draw_sprites( struct osd_bitmap *bitmap, int priority)
{
	int offs;

	/* TODO: the last two entries are not sprites - control registers? */
	for (offs = 0;offs < spriteram_size/2 - 8;offs += 4)
	{
		int data = spriteram16[offs+2];
		int data0 = spriteram16[offs+0];
		int code = spriteram16[offs+1]&0x1fff;
		int sx = spriteram16[offs+3]&0x1ff;
		int sy = 0xf0-(data0&0xff);
		int flipx = data & 0x200;
		int flipy = data & 0x400;	/* ??? */
		int color = (data&0xf000)>>12;
		int pri_mask = (data & 0x0080) ? 0x02 : 0;

		if (sx >= 256) sx -= 512;

		if (data0 & 0x8000) continue;	/* disabled */

		/* Remap code 0x800 <-> 0x1000 */
		code = (code&0x7ff) | ((code&0x800)<<1) | ((code&0x1000)>>1);

		pdrawgfx(bitmap,Machine->gfx[3],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,15,
				pri_mask);
	}
}

static void weststry_mark_sprite_colors(void)
{
	int offs,i;
	int colmask[0x80],pal_base,color;

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	/* TODO: the last two entries are not sprites - control registers? */
	for (offs = 0;offs <spriteram_size/2 - 8;offs += 4 )
	{
		color = spriteram16[offs+2]>>12;
		colmask[color] |= 0xffff;
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] |= PALETTE_COLOR_VISIBLE;
		}
	}
}



void bloodbro_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
	tilemap_set_scrollx(bg_tilemap,0,bloodbro_scroll[0x10]);	/* ? */
	tilemap_set_scrolly(bg_tilemap,0,bloodbro_scroll[0x11]);	/* ? */
	tilemap_set_scrollx(fg_tilemap,0,bloodbro_scroll[0x12]);
	tilemap_set_scrolly(fg_tilemap,0,bloodbro_scroll[0x13]);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	bloodbro_mark_sprite_colors();
	palette_recalc();

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,1);
	bloodbro_draw_sprites(bitmap);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}

void weststry_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
//	tilemap_set_scrollx(bg_tilemap,0,bloodbro_scroll[0x10]);	/* ? */
//	tilemap_set_scrolly(bg_tilemap,0,bloodbro_scroll[0x11]);	/* ? */
//	tilemap_set_scrollx(fg_tilemap,0,bloodbro_scroll[0x12]);
//	tilemap_set_scrolly(fg_tilemap,0,bloodbro_scroll[0x13]);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	weststry_mark_sprite_colors();
	palette_recalc();

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,1);
	weststry_draw_sprites(bitmap,1);
	tilemap_draw(bitmap,tx_tilemap,0,0);
}
