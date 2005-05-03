/***************************************************************************

 Lethal Enforcers
 (c) 1992 Konami

 Video hardware emulation.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

#define GUNX( a ) ( ( readinputport( a ) * 287 ) / 0xff )
#define GUNY( a ) ( ( readinputport( a ) * 223 ) / 0xff )

static int sprite_colorbase;
static int layer_colorbase[4];
static int layerpri[4];

static void lethalen_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 2;
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;
	*color = sprite_colorbase; // | (*color & 0x001f);
	*code = (*code & 0x3fff); // | spritebanks[(*code >> 12) & 3];
}

static void lethalen_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] + ((*color & 0x3c)<<2);
}

VIDEO_START(lethalen)
{
	K053251_vh_start();

	K056832_vh_start(REGION_GFX1, K056832_BPP_8LE, 1, NULL, lethalen_tile_callback, 0);
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER, lethalen_sprite_callback))
		return 1;

	// this game uses external linescroll RAM
	K056832_SetExtLinescroll();

	// the US and Japanese cabinets apparently use different mirror setups
	if (!strcmp(Machine->gamedrv->name, "lethalen"))
	{
 		K056832_set_LayerOffset(0, -64, 0);
		K056832_set_LayerOffset(1, -64, 0);
		K056832_set_LayerOffset(2, -64, 0);
		K056832_set_LayerOffset(3, -64, 0);
		K053245_set_SpriteOffset(96, -8);
	}
	else
	{
 		K056832_set_LayerOffset(0, 64, 0);
		K056832_set_LayerOffset(1, 64, 0);
		K056832_set_LayerOffset(2, 64, 0);
		K056832_set_LayerOffset(3, 64, 0);
		K053245_set_SpriteOffset(-96, 8);
	}

	layer_colorbase[0] = 0x00;
	layer_colorbase[1] = 0x40;
	layer_colorbase[2] = 0x80;
	layer_colorbase[3] = 0xc0;

	return 0;
}

WRITE8_HANDLER(le_palette_control)
{
	switch (offset)
	{
		case 0:	// 40c8 - PCU1 from schematics
			layer_colorbase[0] = ((data & 0x7)-1) * 0x40;
			layer_colorbase[1] = (((data>>4) & 0x7)-1) * 0x40;
			K056832_mark_plane_dirty(0);
			K056832_mark_plane_dirty(1);
			break;

		case 4: // 40cc - PCU2 from schematics
			layer_colorbase[2] = ((data & 0x7)-1) * 0x40;
			layer_colorbase[3] = (((data>>4) & 0x7)-1) * 0x40;
			K056832_mark_plane_dirty(2);
			K056832_mark_plane_dirty(3);
			break;

		case 8:	// 40d0 - PCU3 from schematics
			sprite_colorbase = ((data & 0x7)-1) * 0x40;
			break;
	}
}

VIDEO_UPDATE(lethalen)
{
	fillbitmap(bitmap, 7168, cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);


	K056832_tilemap_draw(bitmap, cliprect, 3, 0, 1);
	K056832_tilemap_draw(bitmap, cliprect, 2, 0, 2);
	K056832_tilemap_draw(bitmap, cliprect, 1, 0, 4);

	K053245_sprites_draw(bitmap, cliprect);

	// force "A" layer over top of everything
	K056832_tilemap_draw(bitmap, cliprect, 0, 0, 0);

	draw_crosshair(bitmap, GUNX(2)+216, 240-GUNY(3), cliprect);
	draw_crosshair(bitmap, GUNX(4)+216, 240-GUNY(5), cliprect);
}
