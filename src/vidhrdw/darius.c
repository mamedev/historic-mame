#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"


static struct tilemap *fg_tilemap;
data16_t *darius_fg_ram;


/*
// TODO: change sprite routines over to use pdrawgfx //
struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;
*/

static void actual_get_fg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code = (ram[tile_index + 0x2000] & 0x7ff);
	UINT16 attr = ram[tile_index];

	SET_TILE_INFO(gfxnum,code,((attr & 0xff) << 2));
	tile_info.flags = TILE_FLIPYX((attr & 0xc000) >> 14);
}

static void get_fg_tile_info(int tile_index)
{
	actual_get_fg_tile_info(darius_fg_ram,2,tile_index);
}

void (*darius_fg_get_tile_info[1])(int tile_index) =
{
	get_fg_tile_info
};

/***************************************************************************/

int darius_vh_start (void)
{
	/* (chips, gfxnum, x_offs, y_offs, y_invert, opaque, dblwidth) */
  /*	if (PC080SN_vh_start(1,1,0,8,0,1,1))*/
  if( PC080SN_vh_start(1,1,-16,8,0,1,1) )/* 16dot shift right //hiro-shi */
    return 1;

  fg_tilemap = tilemap_create(darius_fg_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);

  if (!fg_tilemap)
    return 1;

  tilemap_set_transparent_pen(fg_tilemap,0);

  return 0;
}

void darius_vh_stop(void)
{
	PC080SN_vh_stop();
}

/***************************************************************************/

READ16_HANDLER( darius_fg_layer_r )
{
	return darius_fg_ram[offset];
}

WRITE16_HANDLER( darius_fg_layer_w )
{
	int oldword = darius_fg_ram[offset];

	COMBINE_DATA(&darius_fg_ram[offset]);
	if (oldword != darius_fg_ram[offset])
	{
		if (offset < 0x4000)
			tilemap_mark_tile_dirty(fg_tilemap,(offset & 0x1fff));
	}
}

/***************************************************************************/

void darius_update_palette(void)
{
	int offs,color,i;
	UINT16 tile_mask = 0x1fff; //(Machine->gfx[0]->total_elements) - 1 is 0x17ff
	int colmask[256];

	memset(colmask, 0, sizeof(colmask));

	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		int code = spriteram16[offs+2] &tile_mask;

		if (code)
		{
		  color = (spriteram16[offs+3] & 0x7f);
		  colmask[color] |= Machine->gfx[0]->pen_usage[code];
		}
	}

	for (color = 0;color < 256;color++)
	{
		for (i = 0; i < 16; i++)
			if (colmask[color] & (1 << i))
				palette_used_colors[color * 16 + i] = PALETTE_COLOR_USED;
	}
}

void darius_draw_sprites(struct osd_bitmap *bitmap,int y_offs)
{
	int offs,tile;
	UINT16 tile_mask = 0x1fff; // (Machine->gfx[0]->total_elements) - 1 is 0x17ff

	/* Draw the sprites. 256 sprites in total */
	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		tile = spriteram16[offs+2] &tile_mask;
		if (tile)
		{
			int sx,sy,color,data1;
			int flipx,flipy;

			/* subtracting 6 from x seems to align sprites better
			   with tilemaps; but when first boss appears you can
			   see his sprites pop-up on RHS. */

			sx = (spriteram16[offs+1] - 0) & 0x3ff;
			if (sx > 900) sx = sx - 1024;
 			sy = (256-spriteram16[offs]) & 0x1ff;
			sy += y_offs;
			if (sy > 400) sy = sy - 512;

			/* 0x7f (not 0xff) fixes your ship color just after continue,
			   and makes your bullet on rock explosions white (not red) */
			color = (spriteram16[offs+3] & 0x7f);

			data1 = spriteram16[offs+2];
			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}


void darius_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layer[2];

	PC080SN_tilemap_update();

	/* top layer is in fixed position */
	tilemap_set_scrollx(fg_tilemap,0,0);
	tilemap_set_scrolly(fg_tilemap,0,-8);
	tilemap_update(fg_tilemap);

	palette_init_used_colors();
	darius_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = 0;
	layer[1] = 1;

	fillbitmap(priority_bitmap,0,NULL);

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

 	PC080SN_tilemap_draw(bitmap,0,layer[0],0,0);
	darius_draw_sprites(bitmap,-8);	/* move by hiro-shi */
	PC080SN_tilemap_draw(bitmap,0,layer[1],0,0);

	//darius_draw_sprites(bitmap,-8);

	tilemap_draw(bitmap,fg_tilemap,0,0);
}

