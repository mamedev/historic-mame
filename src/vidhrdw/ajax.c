/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

unsigned char ajax_priority;
static int layer_colorbase[3],sprite_colorbase,zoom_colorbase;
static int zoomdirty;


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
	/* priority bits:
	   4 over zoom (0 = have priority)
	   5 over B    (0 = have priority) - is this used?
	   6 over A    (1 = have priority)
	*/
	*priority = (*color & 0x70) >> 4;
	*color = sprite_colorbase + (*color & 0x0f);
}



/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int ajax_052109_vh_start( void )
{
	layer_colorbase[0] = 64;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 32;
	sprite_colorbase = 16;
	zoom_colorbase = 6;	/* == 48 since it's 7-bit graphics */
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
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

void ajax_052109_vh_stop( void )
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
	K052109_vh_stop();
	K051960_vh_stop();
}



/***************************************************************************

	Display Refresh

***************************************************************************/

unsigned char *ajax_051316_ram;

int ajax_zoomram_r(int offset)
{
	return ajax_051316_ram[offset];
}

void ajax_zoomram_w(int offset,int data)
{
	if (ajax_051316_ram[offset] != data)
	{
		dirtybuffer[offset & 0x3ff] = 1;
		zoomdirty = 1;
		ajax_051316_ram[offset] = data;
	}
}


static int transparent;

static void zoom_mark_colors(void)
{
	static char usedcolors[256];
	int offs;

	if (zoomdirty)
	{
		memset(usedcolors,0,256);
		transparent = 1;

		for (offs = 0;offs < 0x400;offs++)
		{
			int sx,sy,code,color;
			unsigned char *dp;

			code = ajax_051316_ram[offs] + 256 * (ajax_051316_ram[offs + 0x400] & 0x07);
			color = ((ajax_051316_ram[offs + 0x400] & 0x08) >> 3);
			dp = Machine->gfx[0]->gfxdata + code * Machine->gfx[0]->char_modulo;
			for (sy = 0;sy < 16;sy++)
			{
				for (sx = 0;sx < 16;sx++)
				{
					int c = dp[sx];

					if (c)
					{
						usedcolors[c + 128 * color] = 1;
						transparent = 0;
					}
				}
				dp += Machine->gfx[0]->line_modulo;
			}
		}
	}


	for (offs = 0;offs < 256;offs++)
	{
		if (usedcolors[offs])
			palette_used_colors[128*zoom_colorbase + offs] = PALETTE_COLOR_USED;
	}
	palette_used_colors[128*zoom_colorbase] = PALETTE_COLOR_TRANSPARENT;
	palette_used_colors[128*zoom_colorbase+128] = PALETTE_COLOR_TRANSPARENT;
}

/* Note: rotation support doesn't handle asymmetrical visible areas. This doesn't */
/* matter because in the Konami games the visible area is always symmetrical. */
void drawzoom(struct osd_bitmap *bitmap)
{
	int offs;


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

				code = ajax_051316_ram[offs] + 256 * (ajax_051316_ram[offs + 0x400] & 0x07);
				color = zoom_colorbase + ((ajax_051316_ram[offs + 0x400] & 0x08) >> 3);

				drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
			}
		}
	}


	if (transparent == 0)
	{
		int startx,starty,incxx,incxy,incyx,incyy;
		UINT32 cx,cy;
		int x,y,sx,sy,ex,ey;


		startx = 256 * ((INT16)(256 * ajax_051316_ram[0x800] + ajax_051316_ram[0x801]));
		incyy  = (INT16)(256 * ajax_051316_ram[0x802] + ajax_051316_ram[0x803]);
		incyx  = (INT16)(256 * ajax_051316_ram[0x804] + ajax_051316_ram[0x805]);
		starty = 256 * ((INT16)(256 * ajax_051316_ram[0x806] + ajax_051316_ram[0x807]));
		incxy  = (INT16)(256 * ajax_051316_ram[0x808] + ajax_051316_ram[0x809]);
		incxx  = (INT16)(256 * ajax_051316_ram[0x80a] + ajax_051316_ram[0x80b]);

		startx += (Machine->drv->visible_area.min_y - 16) * incyx;
		starty += (Machine->drv->visible_area.min_y - 16) * incyy;

		startx += (Machine->drv->visible_area.min_x - 89) * incxx;
		starty += (Machine->drv->visible_area.min_x - 89) * incxy;

		sx = Machine->drv->visible_area.min_x;
		sy = Machine->drv->visible_area.min_y;
		ex = Machine->drv->visible_area.max_x;
		ey = Machine->drv->visible_area.max_y;

		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int t;

			t = startx; startx = starty; starty = t;
			t = sx; sx = sy; sy = t;
			t = ex; ex = ey; ey = t;
			t = incxx; incxx = incyy; incyy = t;
			t = incxy; incxy = incyx; incyx = t;
		}

		if (Machine->orientation & ORIENTATION_FLIP_X)
		{
			int w = ex - sx;

			incxy = -incxy;
			incyx = -incyx;
			startx = 0xfffff - startx;
			startx -= incxx * w;
			starty -= incxy * w;
		}

		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			int h = ey - sy;

			incxy = -incxy;
			incyx = -incyx;
			starty = 0xfffff - starty;
			startx -= incyx * h;
			starty -= incyy * h;
		}

		y = sy;
		while (y <= ey)
		{
			x = sx;
			cx = startx;
			cy = starty;
			while (x <= ex)
			{
				if ((cx & 0xfff00000) == 0 && (cy & 0xfff00000) == 0 &&
						tmpbitmap->line[cy >> 11][cx >> 11] != palette_transparent_pen)
					bitmap->line[y][x] = tmpbitmap->line[cy >> 11][cx >> 11];

				cx += incxx;
				cy += incxy;
				x++;
			}
			startx += incyx;
			starty += incyy;
			y++;
		}
	}


#if 0
{
	char baf[80];
	sprintf(baf,"%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x",
			ajax_051316_ram[0x800],
			ajax_051316_ram[0x801],
			ajax_051316_ram[0x802],
			ajax_051316_ram[0x803],
			ajax_051316_ram[0x804],
			ajax_051316_ram[0x805],
			ajax_051316_ram[0x806],
			ajax_051316_ram[0x807],
			ajax_051316_ram[0x808],
			ajax_051316_ram[0x809],
			ajax_051316_ram[0x80a],
			ajax_051316_ram[0x80b],
			ajax_051316_ram[0x80c],
			ajax_051316_ram[0x80d],
			ajax_051316_ram[0x80e],
			ajax_051316_ram[0x80f]);
	usrintf_showmessage(baf);
}
#endif
}

void ajax_052109_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	zoom_mark_colors();
	if (palette_recalc())
	{
		memset(dirtybuffer,1,0x400);
		zoomdirty = 1;
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	}

	tilemap_render(ALL_TILEMAPS);

	/* sprite priority bits:
	   0 over zoom (0 = have priority)
	   1 over B    (0 = have priority)
	   2 over A    (1 = have priority)
	*/
	if (ajax_priority)
	{
		/* basic layer order is B, zoom, A, F */

		/* pri = 2 have priority over zoom, not over A and B - is this used? */
		/* pri = 3 have priority over nothing - is this used? */
//		K051960_draw_sprites(bitmap,2,3);
		K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
		/* pri = 1 have priority over B, not over zoom and A - is this used? */
//		K051960_draw_sprites(bitmap,1,1);
		drawzoom(bitmap);
		/* pri = 0 have priority over zoom and B, not over A */
		/* the game seems to just use pri 0. */
		K051960_draw_sprites(bitmap,0,0);
		K052109_tilemap_draw(bitmap,1,0);
		/* pri = 4 have priority over zoom, A and B */
		/* pri = 5 have priority over A and B, not over zoom - OPPOSITE TO BASIC ORDER! (stage 6 boss) */
		K051960_draw_sprites(bitmap,4,5);
		/* pri = 6 have priority over zoom and A, not over B - is this used? */
		/* pri = 7 have priority over A, not over zoom and B - is this used? */
//		K051960_draw_sprites(bitmap,5,7);
		K052109_tilemap_draw(bitmap,0,0);
	}
	else
	{
		/* basic layer order is B, A, zoom, F */

		/* pri = 2 have priority over zoom, not over A and B - is this used? */
		/* pri = 3 have priority over nothing - is this used? */
//		K051960_draw_sprites(bitmap,2,3);
		K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY);
		/* pri = 0 have priority over zoom and B, not over A - OPPOSITE TO BASIC ORDER! */
		/* pri = 1 have priority over B, not over zoom and A */
		/* the game seems to just use pri 0. */
		K051960_draw_sprites(bitmap,0,1);
		K052109_tilemap_draw(bitmap,1,0);
		drawzoom(bitmap);
		/* pri = 4 have priority over zoom, A and B */
		K051960_draw_sprites(bitmap,4,4);
		/* pri = 5 have priority over A and B, not over zoom - is this used? */
		/* pri = 6 have priority over zoom and A, not over B - is this used? */
		/* pri = 7 have priority over A, not over zoom and B - is this used? */
//		K051960_draw_sprites(bitmap,5,7);
		K052109_tilemap_draw(bitmap,0,0);
	}
}
