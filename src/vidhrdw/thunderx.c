#include "driver.h"
#include "vidhrdw/konamiic.h"


#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

int scontra_priority;
static int layer_colorbase[3],sprite_colorbase;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x1f) << 8) | (bank << 13);
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void scontra_sprite_callback(int *code,int *color,int *priority)
{
#if 0
if (keyboard_pressed(KEYCODE_Q) && (*color & 0x40)) *color = rand();
if (keyboard_pressed(KEYCODE_W) && (*color & 0x20)) *color = rand();
if (keyboard_pressed(KEYCODE_E) && (*color & 0x10)) *color = rand();
#endif
	/* bit 4 used as well, meaning not clear. bit 6 not used */
	*priority = (*color & 0x20) >> 5;	/* ??? */
	*color = sprite_colorbase + (*color & 0x0f);
}

static void thunderx_sprite_callback(int *code,int *color,int *priority)
{
#if 0
if (keyboard_pressed(KEYCODE_Q) && (*color & 0x40)) *color = rand();
if (keyboard_pressed(KEYCODE_W) && (*color & 0x20)) *color = rand();
if (keyboard_pressed(KEYCODE_E) && (*color & 0x10)) *color = rand();
#endif
	*priority = (*color & 0x20) >> 5;	/* ??? */
	*color = sprite_colorbase + (*color & 0x0f);
}



/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int scontra_vh_start(void)
{
	layer_colorbase[0] = 48;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 16;
	sprite_colorbase = 32;

	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,scontra_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

int thunderx_vh_start(void)
{
	layer_colorbase[0] = 48;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 16;
	sprite_colorbase = 32;

	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,thunderx_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

void scontra_vh_stop(void)
{
	K052109_vh_stop();
	K051960_vh_stop();
}


void scontra_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	if (scontra_priority)
	{
		K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
		K051960_draw_sprites(bitmap,1,1);
		K052109_tilemap_draw(bitmap,1,0);
		K051960_draw_sprites(bitmap,0,0);
		K052109_tilemap_draw(bitmap,0,0);
	}
	else
	{
		K052109_tilemap_draw(bitmap,1,TILEMAP_IGNORE_TRANSPARENCY);
		K051960_draw_sprites(bitmap,1,1);
		K052109_tilemap_draw(bitmap,2,0);
		K051960_draw_sprites(bitmap,0,0);
		K052109_tilemap_draw(bitmap,0,0);
	}
}

void thunderx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
	K051960_draw_sprites(bitmap,1,1);
	K052109_tilemap_draw(bitmap,1,0);
	K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0);
}
