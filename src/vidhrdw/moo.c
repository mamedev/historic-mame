/***************************************************************************

 Wild West C.O.W.boys of Moo Mesa
 Bucky O'Hare
 (c) 1992 Konami

 Video hardware emulation.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int sprite_colorbase;
static int layer_colorbase[4], layerpri[4];
static int moo_scrolld[2][4][2] = {
 	{{ 4, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};

static void moo_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x01e0) >> 4;

	if (pri <= layerpri[3])					*priority_mask = 0;
	else if (pri > layerpri[3] && pri <= layerpri[2])	*priority_mask = 0xff00;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xff00|0xf0f0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xff00|0xf0f0|0xcccc;
	else 							*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static void bucky_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x01e0) >> 3;

	if (pri <= layerpri[2])					*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
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
	if (K053247_vh_start(REGION_GFX2, -52, 24, NORMAL_PLANE_ORDER, moo_sprite_callback))
	{
		return 1;
	}

	return 0;
}

VIDEO_START(bucky)
{
	K053251_vh_start();

	K054157_vh_start(REGION_GFX1, 0, moo_scrolld, NORMAL_PLANE_ORDER, moo_tile_callback);
	if (K053247_vh_start(REGION_GFX2, -52, 24, NORMAL_PLANE_ORDER, bucky_sprite_callback))
	{
		return 1;
	}

	return 0;
}

/* useful function to sort the four tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayers(int *layer, int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(2, 3)
}

VIDEO_UPDATE(moo)
{
	int layers[4];
	int i;
	int new_base;

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);

	layer_colorbase[0] = 0x70;

	new_base = K053251_get_palette_index(K053251_CI2);
	if(layer_colorbase[1] != new_base) {
		layer_colorbase[1] = new_base;
		K054157_mark_plane_dirty(1);
	}

	new_base = K053251_get_palette_index(K053251_CI3);
	if(layer_colorbase[2] != new_base) {
		layer_colorbase[2] = new_base;
		K054157_mark_plane_dirty(2);
	}

	new_base = K053251_get_palette_index(K053251_CI4);
	if(layer_colorbase[3] != new_base) {
		layer_colorbase[3] = new_base;
		K054157_mark_plane_dirty(3);
	}


	K054157_tilemap_update();

	for (i = 0; i < 4; i++)
	{
		layers[i] = i;
	}
	layerpri[0] = K053251_get_priority(K053251_CI0);
	layerpri[1] = K053251_get_priority(K053251_CI2);
	layerpri[2] = K053251_get_priority(K053251_CI3);
	layerpri[3] = K053251_get_priority(K053251_CI4);
	sortlayers(layers, layerpri);

	for (i = 0; i < 4; i++)
	{
		K054157_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
	}

	K053247_sprites_draw(bitmap,cliprect);
}

/* useful function to sort the four tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayersbucky(int *layer, int *pri)
{
#undef SWAP
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(1, 2)
}

VIDEO_UPDATE(bucky)
{
	int new_base;
	int layers[3];
	int i;

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	K054157_tilemap_update();

	for (i = 0; i < 4; i++)
	{
		layers[i] = i;
	}

	layerpri[0] = K053251_get_priority(K053251_CI0);
	layerpri[1] = K053251_get_priority(K053251_CI2);
	layerpri[2] = K053251_get_priority(K053251_CI4);

	layer_colorbase[0] = 0x70;

	new_base = K053251_get_palette_index(K053251_CI2);
	if(layer_colorbase[1] != new_base) {
		layer_colorbase[1] = new_base;
		K054157_mark_plane_dirty(1);
	}

	new_base = K053251_get_palette_index(K053251_CI3);
	if(layer_colorbase[2] != new_base) {
		layer_colorbase[2] = new_base;
		K054157_mark_plane_dirty(2);
	}

	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);

	sortlayersbucky(layers, layerpri);

	for (i = 0; i < 3; i++)
	{
		K054157_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
	}

	K053247_sprites_draw(bitmap,cliprect);
}

