/***************************************************************************

 Lethal Enforcers
 (c) 1992 Konami

 Video hardware emulation.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

//static int bg_colorbase;
static int sprite_colorbase;
//static int layer_colorbase[4];
static int layerpri[3];
static int lethalen_scrolld[2][4][2] = {
 	{{-23, 0 }, {-27, 0}, {-29, 0}, {-31, 0}},
 	{{-23-4, 0 }, {-27, 0}, {-29, 0}, {-31, 0}}
};

static void lethalen_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 2;
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;
	*color = sprite_colorbase | (*color & 0x001f);
	*code = (*code & 0xfff); // | spritebanks[(*code >> 12) & 3];
}

static void lethalen_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = 0; //TILE_FLIPYX((*color) & 3);
//	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
	*color = 0;
}

VIDEO_START(lethalen)
{
	K053251_vh_start();

	K054157_vh_start(REGION_GFX1, 0, lethalen_scrolld, NORMAL_PLANE_ORDER, lethalen_tile_callback);
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER, lethalen_sprite_callback))
		return 1;

	return 0;
}

VIDEO_UPDATE(lethalen)
{
//	int layers[3];

	K054157_tilemap_update();

	K054157_tilemap_draw(bitmap, cliprect, 0, 0, 0);
	K054157_tilemap_draw(bitmap, cliprect, 1, 0, 0);
	K054157_tilemap_draw(bitmap, cliprect, 2, 0, 0);
	K054157_tilemap_draw(bitmap, cliprect, 3, 0, 0);
}
