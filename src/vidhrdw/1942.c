/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *c1942_scroll;
unsigned char *c1942_palette_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  1942 has three 256x4 palette PROMs (one per gun) and three 256x4 lookup
  table PROMs (one for characters, one for sprites, one for background tiles).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void c1942_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* characters use colors 128-143 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 128;

	/* background tiles use colors 0-63 in four banks */
	for (i = 0;i < TOTAL_COLORS(1)/4;i++)
	{
		COLOR(1,i) = *color_prom;
		COLOR(1,i+32*8) = *color_prom + 16;
		COLOR(1,i+2*32*8) = *color_prom + 32;
		COLOR(1,i+3*32*8) = *color_prom + 48;
		color_prom++;
	}

	/* sprites use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
		COLOR(2,i) = *(color_prom++) + 64;
}



void c1942_updatehook0(int offset)
{
	set_tile_attributes(0,											/* layer number */
		offset,														/* x/y position */
		0,videoram00[offset] + ((videoram01[offset] & 0x80) << 1),	/* tile bank, code */
		videoram01[offset] & 0x3f,									/* color */
		0,0,														/* flip x/y */
		TILE_TRANSPARENCY_PEN);										/* transparency */
}

void c1942_updatehook1(int offset)
{
	offset &= ~0x10;

	set_tile_attributes(1,													/* layer number */
		offset / 32 + (offset % 16) * 32,									/* x/y position */
		0,(videoram10[offset] + ((videoram10[offset + 0x10] & 0x80) << 1)),	/* tile bank, code */
		videoram10[offset + 0x10] & 0x1f,	/* color (there's also a palette bank, handled later) */
		videoram10[offset + 0x10] & 0x20,videoram10[offset + 0x10] & 0x40,	/* flip x/y */
		TILE_TRANSPARENCY_OPAQUE);											/* transparency */
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void c1942_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


//osd_clearbitmap(bitmap);
	set_tile_layer_attributes(1,bitmap,					/* layer number, bitmap */
			-(c1942_scroll[0] + 256 * c1942_scroll[1]),0,	/* scroll x/y */
			*flip_screen & 0x80,*flip_screen & 0x80,	/* flip x/y */
			MAKE_TILE_COLOR(0x60),MAKE_TILE_COLOR((*c1942_palette_bank & 0x03) << 5));	/* global attributes */
	set_tile_layer_attributes(0,bitmap,					/* layer number, bitmap */
			0,0,										/* scroll x/y */
			*flip_screen & 0x80,*flip_screen & 0x80,	/* flip x/y */
			0,0);										/* global attributes */

	update_tile_layer(1,bitmap);


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int i,code,col,sx,sy,dir;


		code = (spriteram[offs] & 0x7f) + 4*(spriteram[offs + 1] & 0x20)
				+ 2*(spriteram[offs] & 0x80);
		col = spriteram[offs + 1] & 0x0f;
		sx = spriteram[offs + 3] - 0x10 * (spriteram[offs + 1] & 0x10);
		sy = spriteram[offs + 2];
		dir = 1;
		if (*flip_screen & 0x80)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			dir = -1;
		}

		/* handle double / quadruple height (actually width because this is a rotated game) */
		i = (spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

if (dir == 1)
{
	layer_mark_rectangle_dirty(Machine->layer[0],sx,sx+15,sy,sy+16*i+15);
}
else
{
	layer_mark_rectangle_dirty(Machine->layer[0],sx,sx+15,sy-16*i,sy+15);
}
		do
		{
			drawgfx(bitmap,Machine->gfx[2],
					code + i,col,
					*flip_screen & 0x80,*flip_screen & 0x80,
					sx,sy + 16 * i * dir,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,15);

			i--;
		} while (i >= 0);
	}

	update_tile_layer(0,bitmap);


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int i,sx,sy,dir;


		dir = 1;
		sx = spriteram[offs + 3] - 0x10 * (spriteram[offs + 1] & 0x10);
		sy = spriteram[offs + 2];
		dir = 1;
		if (*flip_screen & 0x80)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			dir = -1;
		}

		/* handle double / quadruple height (actually width because this is a rotated game) */
		i = (spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

if (dir == 1)
{
	layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+15,sy,sy+16*i+15);
}
else
{
	layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+15,sy-16*i,sy+15);
}
	}

//if (!osd_key_pressed(OSD_KEY_N))
//{
//while (!osd_key_pressed(OSD_KEY_SPACE));
//while (osd_key_pressed(OSD_KEY_SPACE));
//}
}
