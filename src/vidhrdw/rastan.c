/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void rastan_vh_stop (void);

unsigned char *rastan_spriteram;
unsigned char *rastan_scrollx;
unsigned char *rastan_scrolly;

static unsigned char spritepalettebank;



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
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

{
	int color,code,i;
	int colmask[128];
	int pal_base;
	struct GfxLayer *layer;


	pal_base = 0;

	for (color = 0;color < 128;color++) colmask[color] = 0;

	layer = Machine->layer[0];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		int tile;

		tile = layer->tilemap.virtualtiles[i];
		code = TILE_CODE(tile);
		color = TILE_COLOR(tile);
		colmask[color] |= layer->tilemap.gfxtilebank[TILE_BANK(tile)]->tiles[code].pens_used;
	}

	layer = Machine->layer[1];
	for (i = layer->tilemap.virtualwidth * layer->tilemap.virtualheight - 1;i >= 0;i--)
	{
		int tile;

		tile = layer->tilemap.virtualtiles[i];
		code = TILE_CODE(tile);
		color = TILE_COLOR(tile);
		colmask[color] |= layer->tilemap.gfxtilebank[TILE_BANK(tile)]->tiles[code].pens_used;
	}

	for (offs = 0x800-8; offs >= 0; offs -= 8)
	{
		code = READ_WORD (&rastan_spriteram[offs+4]);

		if (code)
		{
			int data1;

			data1 = READ_WORD (&rastan_spriteram[offs]);

			color = (data1 & 0x0f) + 0x10 * spritepalettebank;
			colmask[color] |= Machine->gfx[0]->pen_usage[code];
		}
	}

	for (color = 0;color < 128;color++)
	{
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	if (palette_recalc())
		layer_mark_full_screen_dirty();
}


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

void rainbow_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (palette_recalc())
		layer_mark_full_screen_dirty();


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
