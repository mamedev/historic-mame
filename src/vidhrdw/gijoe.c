#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


static int sprite_colorbase;
static int layer_colorbase[4], layerpri[3];

static void gijoe_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;
	*color = sprite_colorbase | (*color & 0x001f);
}

static void gijoe_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

static int scrolld[2][4][2] = {
 	{{ 20-112, 0 }, { 20-112, 0}, { 20-112, 0}, { 20-112, 0}},
 	{{-76-112, 0 }, {-76-112, 0}, {-76-112, 0}, {-76-112, 0}}
};

VIDEO_START( gijoe )
{
	K054157_vh_start(REGION_GFX1, 0, scrolld, NORMAL_PLANE_ORDER, gijoe_tile_callback);
	if (K053247_vh_start(REGION_GFX2, 48, 23, NORMAL_PLANE_ORDER, gijoe_sprite_callback))
		return 1;
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

VIDEO_UPDATE( gijoe )
{
	int layer[3];
	int new_base;

	new_base = K053251_get_palette_index(K053251_CI1);
	if(layer_colorbase[0] != new_base) {
		layer_colorbase[0] = new_base;
		K054157_mark_plane_dirty(0);
	}

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

	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);

	K054157_tilemap_update();

	layer[0] = 1;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 2;
	layerpri[1] = K053251_get_priority(K053251_CI3);
	layer[2] = 3;
	layerpri[2] = K053251_get_priority(K053251_CI4);

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, cliprect);
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	K054157_tilemap_draw(bitmap,cliprect, layer[0], 0, 1);
	K054157_tilemap_draw(bitmap,cliprect, layer[1], 0, 2);
	K054157_tilemap_draw(bitmap,cliprect, layer[2], 0, 4);

	K053247_sprites_draw(bitmap,cliprect);

	K054157_tilemap_draw(bitmap,cliprect, 0, 0, 0);
}
