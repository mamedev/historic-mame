#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

unsigned char *k88games_zoomram,*k88games_zoomctrl;
static int zoomdirty;
int k88games_priority;

static int layer_colorbase[3],sprite_colorbase,zoom_colorbase;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x0f) << 8) | (bank << 12);
	*color = layer_colorbase[layer] + ((*color & 0xf0) >> 4);
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority)
{
	*priority = (*color & 0x20) >> 5;	/* ??? */
	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int k88games_vh_start(void)
{
	layer_colorbase[0] = 64;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 16;
	sprite_colorbase = 32;
	zoom_colorbase = 48;
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
	{
		return 1;
	}
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}

	if (!(tmpbitmap = osd_create_bitmap(512,512)))
	{
		K052109_vh_stop();
		K051960_vh_stop();
		return 1;
	}
	if (!(dirtybuffer = malloc(0x400)))
	{
		osd_free_bitmap(tmpbitmap);
		K052109_vh_stop();
		K051960_vh_stop();
		return 1;
	}
	memset(dirtybuffer,1,0x400);
	zoomdirty = 1;

	return 0;
}

void k88games_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
	K052109_vh_stop();
	K051960_vh_stop();
}



int k88games_zoomram_r(int offset)
{
	return k88games_zoomram[offset];
}

void k88games_zoomram_w(int offset,int data)
{
	if (k88games_zoomram[offset] != data)
	{
		dirtybuffer[offset & 0x3ff] = 1;
		zoomdirty = 1;
		k88games_zoomram[offset] = data;
	}
}

void k88games_zoomctrl_w(int offset,int data)
{
	k88games_zoomctrl[offset] = data;
}

void k88games_drawzoom(struct osd_bitmap *bitmap)
{
	int offs;
	int x,y,zoomx,zoomy;


	if (zoomdirty)
	{
		zoomdirty = 0;
		for (offs = 0;offs < 0x400;offs++)
		{
			if (dirtybuffer[offs])
			{
				int sx,sy,code,color;


				dirtybuffer[offs] = 0;

				sx = offs % 32;
				sy = offs / 32;

				code = k88games_zoomram[offs] + 256 * (k88games_zoomram[offs + 0x400] & 0x07);
				color =	zoom_colorbase + ((k88games_zoomram[offs + 0x400] & 0x38) >> 3)
						 + ((k88games_zoomram[offs + 0x400] & 0x80) >> 4);

				drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					color,
					k88games_zoomram[offs + 0x400] & 0x40,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
			}
		}
	}


	x = (INT16)(256 * k88games_zoomctrl[0x00] + k88games_zoomctrl[0x01]);
	y = (INT16)(256 * k88games_zoomctrl[0x06] + k88games_zoomctrl[0x07]);
	zoomx = (256 * k88games_zoomctrl[0x02] + k88games_zoomctrl[0x03]);
	if (!zoomx) return;
	zoomx = 0x10000 * 0x800 / zoomx;
	zoomy = (256 * k88games_zoomctrl[0x0a] + k88games_zoomctrl[0x0b]);
	if (!zoomy) return;
	zoomy = 0x10000 * 0x800 / zoomy;

	x = -((x * zoomx) >> 19) + 89;
	y = -((y * zoomy) >> 19) + 16;

	copybitmapzoom(bitmap,tmpbitmap,
			0,0,
			x,y,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen,
			zoomx,zoomy);

#if 0
{
	char baf[80];
	sprintf(baf,"%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x",
			k88games_zoomctrl[0x00],
			k88games_zoomctrl[0x01],
			k88games_zoomctrl[0x02],
			k88games_zoomctrl[0x03],
			k88games_zoomctrl[0x04],
			k88games_zoomctrl[0x05],
			k88games_zoomctrl[0x06],
			k88games_zoomctrl[0x07],
			k88games_zoomctrl[0x08],
			k88games_zoomctrl[0x09],
			k88games_zoomctrl[0x0a],
			k88games_zoomctrl[0x0b],
			k88games_zoomctrl[0x0c],
			k88games_zoomctrl[0x0d],
			k88games_zoomctrl[0x0e],
			k88games_zoomctrl[0x0f]);
	usrintf_showmessage(baf);
}
#endif
}

void k88games_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;

	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	/* palette remapping first */
	{
		unsigned short palette_map[128];
		int color;

		memset (palette_map, 0, sizeof (palette_map));

		for (offs = 0;offs < 0x400;offs++)
		{
			color =	zoom_colorbase + ((k88games_zoomram[offs + 0x400] & 0x38) >> 3)
					 + ((k88games_zoomram[offs + 0x400] & 0x80) >> 4);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				palette_used_colors[i * 16] = PALETTE_COLOR_TRANSPARENT;
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] |= PALETTE_COLOR_USED;
			}
		}
	}
	if (palette_recalc())
	{
		memset(dirtybuffer,1,0x400);
		zoomdirty = 1;
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	}

	tilemap_render(ALL_TILEMAPS);

	if (k88games_priority)
	{
		K052109_tilemap_draw(bitmap,0,TILEMAP_IGNORE_TRANSPARENCY);
		K051960_draw_sprites(bitmap,1,1);
		K052109_tilemap_draw(bitmap,2,0);
		K052109_tilemap_draw(bitmap,1,0);
		K051960_draw_sprites(bitmap,0,0);
		k88games_drawzoom(bitmap);
	}
	else
	{
		K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
		k88games_drawzoom(bitmap);
		K051960_draw_sprites(bitmap,0,0);
		K052109_tilemap_draw(bitmap,1,0);
		K051960_draw_sprites(bitmap,1,1);
		K052109_tilemap_draw(bitmap,0,0);
	}
}
