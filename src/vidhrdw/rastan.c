/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void rastan_vh_stop (void);

unsigned char *rastan_paletteram;
unsigned char *rastan_spriteram;
unsigned char *rastan_scrollx;
unsigned char *rastan_scrolly;

static unsigned char *pal_dirty;
static unsigned char spritepalettebank;


/*
 *   video system start; we also initialize the system memory as well here
 */

int rastan_vh_start (void)
{
	if (generic_vh_start() != 0)
		return 1;

	/* Allocate dirty buffers */
	pal_dirty = malloc (0x80);
	if (!pal_dirty)
	{
		rastan_vh_stop ();
		return 1;
	}

	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void rastan_vh_stop (void)
{
	/* Free dirty buffers */
	free (pal_dirty);
	pal_dirty = 0;

	generic_vh_stop();
}



/*
 *   scroll write handlers
 */

void rastan_scrollY_w (int offset, int data)
{
	COMBINE_WORD_MEM (&rastan_scrolly[offset], data);
}

void rastan_scrollX_w (int offset, int data)
{
	COMBINE_WORD_MEM (&rastan_scrollx[offset], data);
}


/*
 *   palette RAM read/write handlers
 */

void rastan_paletteram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&rastan_paletteram[offset], data);
	offset /= 32;
	if (offset < 0x80)
		pal_dirty[offset] = 1;
}

int rastan_paletteram_r (int offset)
{
	return READ_WORD (&rastan_paletteram[offset]);
}



void rastan_updatehook0(int offset)
{
	int data1,data2;

	offset &= ~0x03;
	data1 = READ_WORD(&videoram00[offset]);
	data2 = READ_WORD(&videoram00[offset + 2]);

	set_tile_attributes(0,				/* layer number */
		offset / 4,						/* x/y position */
		0,data2 & 0x3fff,				/* tile bank, code */
		data1 & 0x7f,					/* color */
		data1 & 0x4000, data1 & 0x8000,	/* flip x/y */
		TILE_TRANSPARENCY_PEN);			/* transparency */
}

void rastan_updatehook1(int offset)
{
	int data1,data2;

	offset &= ~0x03;
	data1 = READ_WORD(&videoram10[offset]);
	data2 = READ_WORD(&videoram10[offset + 2]);

	set_tile_attributes(1,				/* layer number */
		offset / 4,						/* x/y position */
		0,data2 & 0x3fff,				/* tile bank, code */
		data1 & 0x7f,					/* color */
		data1 & 0x4000, data1 & 0x8000,	/* flip x/y */
		TILE_TRANSPARENCY_OPAQUE);		/* transparency */
}



void rastan_videocontrol_w (int offset, int data)
{
	if (offset == 0)
	{
		/* bits 2 and 3 are the coin counters */

		/* bits 5-7 look like the sprite palette bank */
		spritepalettebank = (data & 0xe0) >> 5;

		/* other bits unknown */
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,pom;
	int i;
	struct GfxLayer *layer;


	for (pom = 0; pom < 0x80; pom++)
	{
		if (pal_dirty[pom])
		{
			for (i = 0; i < 16; i++)
			{
				int palette = READ_WORD (&rastan_paletteram[pom*32+i*2]);
				int red = palette & 31;
				int green = (palette >> 5) & 31;
				int blue = (palette >> 10) & 31;

				red = (red << 3) + (red >> 2);
				green = (green << 3) + (green >> 2);
				blue = (blue << 3) + (blue >> 2);
				setgfxcolorentry (Machine->gfx[0], pom*16+i, red, green, blue);
			}
		}
	}

	layer = Machine->layer[0];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		if (pal_dirty[TILE_COLOR(layer->tilemap.virtualtiles[i])])
			layer->tilemap.virtualdirty[i] = 1;
	}
	layer = Machine->layer[1];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		if (pal_dirty[TILE_COLOR(layer->tilemap.virtualtiles[i])])
			layer->tilemap.virtualdirty[i] = 1;
	}

	memset (pal_dirty, 0, 0x80);


	set_tile_layer_attributes(1,bitmap,					/* layer number, bitmap */
			READ_WORD(&rastan_scrollx[0]) - 16,READ_WORD(&rastan_scrolly[0]),	/* scroll x/y */
			0,0,										/* flip x/y */
			0,0);										/* global attributes */
	set_tile_layer_attributes(0,bitmap,					/* layer number, bitmap */
			READ_WORD(&rastan_scrollx[2]) - 16,READ_WORD(&rastan_scrolly[2]),	/* scroll x/y */
			0,0,										/* flip x/y */
			0,0);										/* global attributes */

	update_tile_layer(1,bitmap);
	update_tile_layer(0,bitmap);


	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800-8; offs >= 0; offs -= 8)
	{
		int num = READ_WORD (&rastan_spriteram[offs+4]);


		if (num)
		{
			int sx,sy,col,data1;

			sx = READ_WORD(&rastan_spriteram[offs+6]) & 0x1ff;
			if (sx > 400) sx = sx - 512;
			sy = READ_WORD(&rastan_spriteram[offs+2]) & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = READ_WORD (&rastan_spriteram[offs]);

			col = (data1 & 0x0f) + 0x10 * spritepalettebank;

			drawgfx(bitmap,Machine->gfx[0],
					num,
					col,
					data1 & 0x4000, data1 & 0x8000,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+15,sy,sy+15);
		}
	}
}

void rainbow_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,pom;
	int i;
	struct GfxLayer *layer;


	for (pom = 0; pom < 0x80; pom++)
	{
		if (pal_dirty[pom])
		{
			for (i = 0; i < 16; i++)
			{
				int palette = READ_WORD (&rastan_paletteram[pom*32+i*2]);
				int red = palette & 31;
				int green = (palette >> 5) & 31;
				int blue = (palette >> 10) & 31;

				red = (red << 3) + (red >> 2);
				green = (green << 3) + (green >> 2);
				blue = (blue << 3) + (blue >> 2);
				setgfxcolorentry (Machine->gfx[0], pom*16+i, red, green, blue);
			}
		}
	}

	layer = Machine->layer[0];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		if (pal_dirty[TILE_COLOR(layer->tilemap.virtualtiles[i])])
			layer->tilemap.virtualdirty[i] = 1;
	}
	layer = Machine->layer[1];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		if (pal_dirty[TILE_COLOR(layer->tilemap.virtualtiles[i])])
			layer->tilemap.virtualdirty[i] = 1;
	}

	memset (pal_dirty, 0, 0x80);


	set_tile_layer_attributes(1,bitmap,					/* layer number, bitmap */
			READ_WORD(&rastan_scrollx[0]) - 16,READ_WORD(&rastan_scrolly[0]),	/* scroll x/y */
			0,0,										/* flip x/y */
			0,0);										/* global attributes */
	set_tile_layer_attributes(0,bitmap,					/* layer number, bitmap */
			READ_WORD(&rastan_scrollx[2]) - 16,READ_WORD(&rastan_scrolly[2]),	/* scroll x/y */
			0,0,										/* flip x/y */
			0,0);										/* global attributes */

	update_tile_layer(1,bitmap);


	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800-8; offs >= 0; offs -= 8)
	{
		int num = READ_WORD (&rastan_spriteram[offs+4]);


		if (num)
		{
			int sx,sy,col,data1;

			sx = READ_WORD(&rastan_spriteram[offs+6]) & 0x1ff;
			if (sx > 400) sx = sx - 512;
			sy = READ_WORD(&rastan_spriteram[offs+2]) & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = READ_WORD (&rastan_spriteram[offs]);

			col = (data1 + 0x10) & 0x7f;

			drawgfx(bitmap,Machine->gfx[0],
					num,
					col,
					data1 & 0x4000, data1 & 0x8000,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
layer_mark_rectangle_dirty(Machine->layer[0],sx,sx+15,sy,sy+15);
		}
	}

	update_tile_layer(0,bitmap);

	for (offs = 0x800-8; offs >= 0; offs -= 8)
	{
		int num = READ_WORD (&rastan_spriteram[offs+4]);


		if (num)
		{
			int sx,sy;

			sx = READ_WORD(&rastan_spriteram[offs+6]) & 0x1ff;
			if (sx > 400) sx = sx - 512;
			sy = READ_WORD(&rastan_spriteram[offs+2]) & 0x1ff;
			if (sy > 400) sy = sy - 512;

layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+15,sy,sy+15);
		}
	}
}



/* CUT HERE ->  YM2151 TEST */
#if 0
void rastan_vhmus_screenrefresh(struct osd_bitmap *bitmap)
{
	int i=1;
	i++;
}
#endif
