/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"
#include "vidhrdw/konamiic.h"

#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

int layer_colorbase[3], sprite_colorbase;


/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*code |= ((*color & 0x0f) << 8) | (bank << 12);
	*color = ((*color & 0xf0) >> 4);
	*color += layer_colorbase[layer];
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority)
{
	*color = sprite_colorbase + (*color & 0x0f);
}



/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

void ajax_052109_vh_stop( void )
{
	K052109_vh_stop();
	K051960_vh_stop();
}

int ajax_052109_vh_start( void )
{
	layer_colorbase[0] = 64;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 32;
	sprite_colorbase = 16;
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

/***************************************************************************

	Display Refresh

***************************************************************************/

void ajax_052109_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);	/* background */
	K052109_tilemap_draw(bitmap,1,0);								/* foreground */
	K051960_draw_sprites(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0);								/* text layer */
}
