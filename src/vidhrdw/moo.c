/***************************************************************************

 Wild West C.O.W.boys of Moo Mesa
 Bucky O'Hare
 (c) 1992 Konami

 Video hardware emulation.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int bg_colorbase,sprite_colorbase;
static int layer_colorbase[4], layerpri[3];
static int moo_scrolld[2][4][2] = {
 	{{-23, 0 }, {-27, 0}, {-29, 0}, {-31, 0}},
 	{{-23-4, 0 }, {-27, 0}, {-29, 0}, {-31, 0}}
};

static void moo_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x03e0) >> 4;

	if      (pri <= layerpri[2])	*priority_mask = 0;
	else if (pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 							*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static void moo_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

VIDEO_START(moo)
{
	K053251_vh_start();

	K054157_vh_start(REGION_GFX1, 0, moo_scrolld, NORMAL_PLANE_ORDER, moo_tile_callback);
	if (K053247_vh_start(REGION_GFX2, -24, 23, NORMAL_PLANE_ORDER, moo_sprite_callback))
	{
		return 1;
	}

	return 0;
}

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

VIDEO_UPDATE(moo)
{
	int layers[3];

	bg_colorbase       = K053251_get_palette_index(K053251_CI1);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[0] = 0x70;
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[3] = K053251_get_palette_index(K053251_CI4);

	K054157_tilemap_update();

	layers[0] = 1;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layers[1] = 2;
	layerpri[1] = K053251_get_priority(K053251_CI3);
	layers[2] = 3;
	layerpri[2] = K053251_get_priority(K053251_CI4);

	sortlayers(layers, layerpri);

	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],cliprect);
	if (layerpri[0] < K053251_get_priority(K053251_CI1))	/* bucky hides back layer behind background */
		K054157_tilemap_draw(bitmap, cliprect, layers[0], 0, 1);
	K054157_tilemap_draw(bitmap, cliprect, layers[1], 0, 2);
	K054157_tilemap_draw(bitmap, cliprect, layers[2], 0, 4);

	K053247_sprites_draw(bitmap,cliprect);

	K054157_tilemap_draw(bitmap, cliprect, 0, 0, 0);
}
