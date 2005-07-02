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
//static int layerpri[4];
struct mame_bitmap * sprite_planes0123;
struct mame_bitmap * sprite_planes45xx;


static void lethalen_sprite_callback(int *code, int *color, int *priority_mask)
{
/*
    int pri = (*color & 0x00e0) >> 2;
    if (pri <= layerpri[2])                             *priority_mask = 0;
    else if (pri > layerpri[2] && pri <= layerpri[1])   *priority_mask = 0xf0;
    else if (pri > layerpri[1] && pri <= layerpri[0])   *priority_mask = 0xf0|0xcc;
    else                                                *priority_mask = 0xf0|0xcc|0xaa;
    *color = sprite_colorbase; // |
*/
	*priority_mask = 0x00;
	*color = *color & 0x000f;

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

	if (K053245_vh_start(0, REGION_GFX3,NORMAL_PLANE_ORDER, lethalen_sprite_callback))
		return 1;

	if (K053245_vh_start(1, REGION_GFX4,NORMAL_PLANE_ORDER, lethalen_sprite_callback))
		return 1;

	sprite_planes0123 = auto_bitmap_alloc_depth(64*8, 32*8, 16);
	sprite_planes45xx = auto_bitmap_alloc_depth(64*8, 32*8, 16);

	// this game uses external linescroll RAM
	K056832_SetExtLinescroll();

	// the US and Japanese cabinets apparently use different mirror setups
	if (!strcmp(Machine->gamedrv->name, "lethalen"))
	{
 		K056832_set_LayerOffset(0, -64, 0);
		K056832_set_LayerOffset(1, -64, 0);
		K056832_set_LayerOffset(2, -64, 0);
		K056832_set_LayerOffset(3, -64, 0);
		K053245_set_SpriteOffset(0, 96, -8);
		K053245_set_SpriteOffset(1, 96, -8);
	}
	else
	{
 		K056832_set_LayerOffset(0, 64, 0);
		K056832_set_LayerOffset(1, 64, 0);
		K056832_set_LayerOffset(2, 64, 0);
		K056832_set_LayerOffset(3, 64, 0);
		K053245_set_SpriteOffset(0, -96, 8);
		K053245_set_SpriteOffset(1, -96, 8);
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
	int x,y;


	fillbitmap(bitmap, 7168, cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);
	fillbitmap(sprite_planes0123, 0, cliprect);
	fillbitmap(sprite_planes45xx, 0, cliprect);


	K056832_tilemap_draw(bitmap, cliprect, 3, 0, 1);
	K056832_tilemap_draw(bitmap, cliprect, 2, 0, 2);
	K056832_tilemap_draw(bitmap, cliprect, 1, 0, 4);

	/* Lethal Enforcers has 2 sprite chips, each rendering a 4bpp layer
      (although 2bpp is unused in one)

      we render each layer to its own bitmap then mix them

      (i'm guessing this is how the HW would work.. as each sprite renderer is only capable of 4bpp)

      this does not seem to work well with priorities ....

    */

	/*
     the priority buffer probably needs handling manually this way...fun fun fun
    */
	K053245_sprites_draw(0, sprite_planes0123, cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);/* hmm how do we stop it using priority bitmap when we don't want it? for the first chip? */
	K053245_sprites_draw(1, sprite_planes45xx, cliprect);

	#if 0
	{
		FILE *fp;
		fp=fopen("planes0123", "w+b");
		if (fp)
		{
			fwrite(sprite_planes0123->base, 256*512, 1, fp);
			fclose(fp);
		}
	}
	{
		FILE *fp;
		fp=fopen("planes45xx", "w+b");
		if (fp)
		{
			fwrite(sprite_planes45xx->base, 256*512, 1, fp);
			fclose(fp);
		}
	}
	#endif

	/* now we mix the layers to make a 6bpp layer .. this isn't perfect.. i think shadow sprites cause problems
      we probably need to handle them manually because of the weird configuration
      */

	for(y=0;y<32*8;y++)
	{
		UINT16* line_dest = (UINT16 *)(bitmap->line[y]);
		UINT16* line_0123 = (UINT16 *)(sprite_planes0123->line[y]);
		UINT16* line_45xx = (UINT16 *)(sprite_planes45xx->line[y]);

		for (x=0;x<64*8;x++)
		{
			int dat;
			/* reorder sprite rom loading / reverse decode instead? */
			dat  = (line_0123[x] & 0x0003)<<2;
			dat |= (line_0123[x] & 0x000c)>>2;
			dat |= (line_45xx[x] & 0x0003)<<4;

			dat |= (line_0123[x] & 0x00f0)<<2; // color
			dat |= (line_45xx[x] & 0x00f0)<<2; // color ( just incaes one layer was transparent so no pixel was drawn )

			if (dat) line_dest[x]=dat+0x400; /* use sprite_colorbase ? */

		}

	}


	// force "A" layer over top of everything
	K056832_tilemap_draw(bitmap, cliprect, 0, 0, 0);

	draw_crosshair(bitmap, GUNX(2)+216, 240-GUNY(3), cliprect, 0);
	draw_crosshair(bitmap, GUNX(4)+216, 240-GUNY(5), cliprect, 1);
}
