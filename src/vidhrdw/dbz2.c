/*
  Dragonball Z 2 Super Battle
  (c) 1994 Banpresto

  Video hardware emulation.
*/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int scrolld[2][4][2] = {
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};

static int layer_colorbase;
static int sprite_colorbase;

static void dbz2_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);

	*color = layer_colorbase | ((*color & 0xf0) >> 4);
}

static void dbz2_sprite_callback(int *code, int *color, int *priority_mask)
{
	*priority_mask = 0;
	*color = sprite_colorbase | (*color & 0x001f);
}

VIDEO_START(dbz2)
{
	K053251_vh_start();

	if (K054157_vh_start(REGION_GFX1, 0, scrolld, NORMAL_PLANE_ORDER, dbz2_tile_callback))
	{
		return 1;
	}

	if (K053247_vh_start(REGION_GFX2, -28, 32, NORMAL_PLANE_ORDER, dbz2_sprite_callback))
	{
		return 1;
	}

	return 0;
}

VIDEO_UPDATE(dbz2)
{
	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	K054157_tilemap_update();

	K054157_tilemap_draw(bitmap, cliprect, 3, 0, 1<<0);

	K053247_sprites_draw(bitmap, cliprect);
}


