/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

static int layer_colorbase[3],sprite_colorbase;



/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void mainevt_tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*flags = (*color & 0x02) ? TILE_FLIPX : 0;
	/* TODO: My understanding would be that HALF priority sprites only have priority */
	/* over tiles with color bit 5 set. However, that bit is set everywhere *except* */
	/* for where it would be needed, that is behind the score display. So I must */
	/* be doing someting wrong. */
	*code |= ((*color & 0x01) << 8) | ((*color & 0x1c) << 7);
	*color = ((*color & 0xc0) >> 6);
	*color += layer_colorbase[layer];
}

static void dv_tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*flags = (*color & 0x02) ? TILE_FLIPX : 0;
	*code |= ((*color & 0x01) << 8) | ((*color & 0x3c) << 7);
	*color = ((*color & 0xc0) >> 6);
	*color += layer_colorbase[layer];
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void mainevt_sprite_callback(int *code,int *color,int *priority)
{
	/* bit 6 = priority over layer B */
	/* bit 5 = HALF priority over layer B (used for top part of the display) */
	*priority = (*color & 0x60) >> 5;

/* kludge to fix ropes until sprite/sprite priority is supported correctly */
	if (*code == 0x3f8 || *code == 0x3f9) *priority = 2;

	*color = sprite_colorbase + (*color & 0x03);
}

static void dv_sprite_callback(int *code,int *color,int *priority)
{
	*color = sprite_colorbase + (*color & 0x07);
}


/*****************************************************************************/

int mainevt_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 8;
	layer_colorbase[2] = 4;
	sprite_colorbase = 12;

	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,mainevt_tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,mainevt_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

int dv_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 4;
	sprite_colorbase = 8;

	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,dv_tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,dv_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

void mainevt_vh_stop(void)
{
	K052109_vh_stop();
	K051960_vh_stop();
}

/*****************************************************************************/

void mainevt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,1,TILEMAP_IGNORE_TRANSPARENCY);
	K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,2,0);
	K051960_draw_sprites(bitmap,1,3);
	K052109_tilemap_draw(bitmap,0,0);
}
