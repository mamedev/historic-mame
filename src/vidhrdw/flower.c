/* Flower Video Hardware */

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *flower_bg0_tilemap, *flower_bg1_tilemap, *flower_text_tilemap, *flower_text_right_tilemap;
data8_t *flower_textram, *flower_bg0ram, *flower_bg1ram, *flower_bg0_scroll, *flower_bg1_scroll;

static void flower_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data8_t *source = spriteram + 0x200;
	data8_t *finish = source - 0x200;
	int step;

	if(flip_screen)
		step = -16;
	else
		step = 16;

	source -= 8;

	while( source>=finish )
	{
		int xblock,yblock;
		int sy = 256-32-source[0]+1;
		int	sx = source[4]-55;
		int code = source[1] & 0x3f;

		int flipy = source[1] & 0x80; // wrong? sunflower needs it, ship afterwards breaks with it
		int flipx = source[1] & 0x40;

		int size = source[3];

		int xsize = ((size & 0x08)>>3);
		int ysize = ((size & 0x80)>>7);

		xsize++;
		ysize++;

		if (ysize==2) sy -= 16;

		code |= ((source[2] & 0x01) << 6);
		code |= ((source[2] & 0x08) << 4);

		if(flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
			sx = source[4] - 23;
			sy = source[0] + 25;

			if (ysize==2) sy += 16;
		}


		for (xblock = 0; xblock<xsize; xblock++)
		{
			for (yblock = 0; yblock<ysize; yblock++)
			{
				drawgfxzoom(bitmap,gfx,
						code+yblock+(8*xblock),
						0,
						flipx,flipy,
						sx+step*xblock,sy+step*yblock,
						cliprect,TRANSPARENCY_PEN,0,
						((size&7)+1)<<13,((size&0x70)+0x10)<<9);
			}
		}
		source -= 8;
	}

}

static void get_bg0_tile_info(int tile_index)
{
	int code = flower_bg0ram[tile_index];

	SET_TILE_INFO(2, code, 0, 0)
}

static void get_bg1_tile_info(int tile_index)
{
	int code = flower_bg1ram[tile_index];

	SET_TILE_INFO(2, code, 0, 0)
}

static void get_text_tile_info(int tile_index)
{
	int code = flower_textram[tile_index];

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START(flower)
{
	flower_bg0_tilemap        = tilemap_create(get_bg0_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,16,16);
	flower_bg1_tilemap        = tilemap_create(get_bg1_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,16,16);
	flower_text_tilemap       = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	flower_text_right_tilemap = tilemap_create(get_text_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT, 8, 8, 2,32);

	if(!flower_bg0_tilemap || !flower_bg1_tilemap || !flower_text_tilemap || !flower_text_right_tilemap)
		return 1;

	tilemap_set_transparent_pen(flower_bg1_tilemap,0);
	tilemap_set_transparent_pen(flower_text_tilemap,0);
	tilemap_set_transparent_pen(flower_text_right_tilemap,0);

	tilemap_set_scrolly(flower_text_tilemap, 0, 16);
	tilemap_set_scrolly(flower_text_right_tilemap, 0, 16);

	return 0;

}

VIDEO_UPDATE( flower )
{
	struct rectangle myclip = *cliprect;

	tilemap_set_scrolly(flower_bg0_tilemap,0, flower_bg0_scroll[0]);
	tilemap_set_scrolly(flower_bg1_tilemap,0, flower_bg1_scroll[0]);

	tilemap_draw(bitmap,cliprect,flower_bg0_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,flower_bg1_tilemap,0,0);

	flower_drawsprites(bitmap,cliprect);

	if(flip_screen)
	{
		myclip.min_x = cliprect->min_x;
		myclip.max_x = cliprect->min_x + 15;
	}
	else
	{
		myclip.min_x = cliprect->max_x - 15;
		myclip.max_x = cliprect->max_x;
	}

	tilemap_draw(bitmap,cliprect,flower_text_tilemap,0,0);
	tilemap_draw(bitmap,&myclip,flower_text_right_tilemap,0,0);
}

WRITE8_HANDLER( flower_textram_w )
{
	flower_textram[offset] = data;
	tilemap_mark_tile_dirty(flower_text_tilemap, offset);
	tilemap_mark_all_tiles_dirty(flower_text_right_tilemap);
}

WRITE8_HANDLER( flower_bg0ram_w )
{
	flower_bg0ram[offset] = data;
	tilemap_mark_tile_dirty(flower_bg0_tilemap, offset & 0x1ff);
}

WRITE8_HANDLER( flower_bg1ram_w )
{
	flower_bg1ram[offset] = data;
	tilemap_mark_tile_dirty(flower_bg1_tilemap, offset & 0x1ff);
}

WRITE8_HANDLER( flower_flipscreen_w )
{
	flip_screen_set(data);
}
