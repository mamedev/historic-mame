#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "vidhrdw/generic.h"

unsigned char *fastlane_k007121_regs,*fastlane_videoram1,*fastlane_videoram2;
static struct tilemap *layer0, *layer1;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info0(int col,int row)
{
	int tile_index = 32*row+col;
	int color = fastlane_videoram1[tile_index];
	int gfxbank = ((color & 0x80) >> 7) | ((color & 0x70) >> 3)
					| ((K007121_ctrlram[0][0x04] & 0x08) << 1)
					| ((K007121_ctrlram[0][0x03] & 0x01) << 5);
	int code = fastlane_videoram1[tile_index + 0x400];

	code += 256*gfxbank;

	SET_TILE_INFO(0,code,1);
}

static void get_tile_info1(int col,int row)
{
	int tile_index = 32*row+col;
	int color = fastlane_videoram2[tile_index];
	int gfxbank = ((color & 0x80) >> 7) | ((color & 0x70) >> 3)
					| ((K007121_ctrlram[0][0x04] & 0x08) << 1)
					| ((K007121_ctrlram[0][0x03] & 0x01) << 5);
	int code = fastlane_videoram2[tile_index + 0x400];

	code += 256*gfxbank;

	SET_TILE_INFO(0,code,0);
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int fastlane_vh_start(void)
{
	layer0 = tilemap_create(get_tile_info0, TILEMAP_OPAQUE, 8, 8, 32, 32);
	layer1 = tilemap_create(get_tile_info1, TILEMAP_OPAQUE, 8, 8,  5, 32);

	tilemap_set_scroll_rows( layer0, 32 );

	if (layer0 && layer1)
	{
		struct rectangle clip = Machine->drv->visible_area;
		clip.min_x += 40;
		tilemap_set_clip(layer0,&clip);

		clip.max_x = 40;
		clip.min_x = 0;
		tilemap_set_clip(layer1,&clip);

		return 0;
	}
	return 1;
}

/***************************************************************************

  Memory Handlers

***************************************************************************/

void fastlane_vram1_w(int offset,int data)
{
	if (fastlane_videoram1[offset] != data)
	{
		tilemap_mark_tile_dirty(layer0, offset%32, (offset&0x3ff)/32);
		fastlane_videoram1[offset] = data;
	}
}

void fastlane_vram2_w(int offset,int data)
{
	if (fastlane_videoram2[offset] != data)
	{
		int col = offset%32;
		if (col < 5)
			tilemap_mark_tile_dirty(layer1, col, (offset&0x3ff)/32);
		fastlane_videoram2[offset] = data;
	}
}



/***************************************************************************

  Screen Refresh

***************************************************************************/

void fastlane_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i, xoffs;

	/* set scroll registers */
	xoffs = K007121_ctrlram[0][0x00];
	for( i=0; i<32; i++ ){
		tilemap_set_scrollx(layer0, i, fastlane_k007121_regs[0x20 + i] + xoffs - 40);
	}
	tilemap_set_scrolly( layer0, 0, K007121_ctrlram[0][0x02] );

	tilemap_update( ALL_TILEMAPS );
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render( ALL_TILEMAPS );

	tilemap_draw(bitmap,layer0,0);
	K007121_sprites_draw(0,bitmap,spriteram,0,40,0);
	tilemap_draw(bitmap,layer1,0);
}
