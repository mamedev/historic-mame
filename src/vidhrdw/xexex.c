#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/konamigx.h"

static int sprite_colorbase;
static int layer_colorbase[4], bg_colorbase, layerpri[4];
static int cur_alpha;

void xexex_set_alpha(int on)
{
	cur_alpha = on;
}

static void xexex_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[3])								*priority_mask = 0;
	else if (pri > layerpri[3] && pri <= layerpri[2])	*priority_mask = 0xff00;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xff00|0xf0f0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xff00|0xf0f0|0xcccc;
	else 												*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static void xexex_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

static int scrolld[2][4][2] = {
 	{{ 42-64, 16 }, {42-64, 16}, {42-64-2, 16}, {42-64-4, 16}},
 	{{ 53-64, 16 }, {53-64, 16}, {53-64-2, 16}, {53-64-4, 16}}
};

VIDEO_START( xexex )
{
	int region = REGION_GFX3;

	cur_alpha = 0;

	K053251_vh_start();
	K054338_vh_start();
	K053250_vh_start(1, &region);
	if (K054157_vh_start(REGION_GFX1, 1, scrolld, NORMAL_PLANE_ORDER, xexex_tile_callback))
		return 1;

	if (K053247_vh_start(REGION_GFX2, -28, 32, NORMAL_PLANE_ORDER, xexex_sprite_callback))
		return 1;

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
int xdump = 0;
VIDEO_UPDATE( xexex )
{
	int layer[4];
	int plane;
	int cur_alpha_level;
	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);
	bg_colorbase       = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = 0x70;
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[3] = K053251_get_palette_index(K053251_CI4);

	K054157_tilemap_update();

	cur_alpha_level = ((K054338_read_register(K338_REG_PBLEND) & 0x1f) << 3) | ((K054338_read_register(K338_REG_PBLEND) & 0x1f) >> 2);

	layer[0] = 1;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 2;
	layerpri[1] = K053251_get_priority(K053251_CI3);
	layer[2] = 3;
	layerpri[2] = K053251_get_priority(K053251_CI4);
	layer[3] = -1;
	layerpri[3] = K053251_get_priority(K053251_CI1);

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, cliprect);

	K054338_fill_backcolor(bitmap, 0); //*

	for(plane=0; plane<4; plane++)
		if(layer[plane] < 0)
			K053250_draw(bitmap,cliprect, 0, bg_colorbase, 1<<plane);
		else if(!cur_alpha || (layer[plane] != 1))
			K054157_tilemap_draw(bitmap,cliprect, layer[plane], 0, 1<<plane);

	K053247_sprites_draw(bitmap,cliprect);

	if(cur_alpha) {
		alpha_set_level(cur_alpha_level);
		K054157_tilemap_draw(bitmap,cliprect, 1, TILEMAP_ALPHA, 0);
	}

	K054157_tilemap_draw(bitmap,cliprect, 0, 0, 0);
}
