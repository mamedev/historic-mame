/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *espial_attributeram;
unsigned char *espial_column_scroll;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Amidar has one 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void espial_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i + 0] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}


	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}
}



void espial_attributeram_w(int offset,int data)
{
	if (espial_attributeram[offset] != data)
	{
		espial_attributeram[offset] = data;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void espial_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 256*(espial_attributeram[offs] & 0x03),
					colorram[offs],
					espial_attributeram[offs] & 0x04,espial_attributeram[offs] & 0x08,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -espial_column_scroll[offs];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < spriteram_size/2;offs++)
	{
		int sx,sy,code,color,flipx,flipy;


		sx = spriteram[offs + 16];
		sy = 240 - spriteram_2[offs];
		code = spriteram[offs] >> 1;
		color = spriteram_2[offs + 16];
		flipx = spriteram_3[offs] & 0x04;
		flipy = spriteram_3[offs] & 0x08;

		if (spriteram[offs] & 1)	/* double height */
		{
			drawgfx(bitmap,Machine->gfx[1],
					code,
					color,
					flipx,flipy,
					sx,sy - 16,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			drawgfx(bitmap,Machine->gfx[1],
					code + 1,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
		else
			drawgfx(bitmap,Machine->gfx[1],
					code,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
