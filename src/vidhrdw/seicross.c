/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *seicross_row_scroll;




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Seicross has two 32x8 palette PROMs, connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void seicross_vh_convert_color_prom(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}



WRITE_HANDLER( seicross_colorram_w )
{
	if (colorram[offset] != data)
	{
		/* bit 5 of the address is not used for color memory. There is just */
		/* 512k of memory; every two consecutive rows share the same memory */
		/* region. */
		offset &= 0xffdf;

		dirtybuffer[offset] = 1;
		dirtybuffer[offset + 0x20] = 1;

		colorram[offset] = data;
		colorram[offset + 0x20] = data;
	}
}




/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void seicross_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs,x;

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
					videoram[offs] + ((colorram[offs] & 0x10) << 4),
					colorram[offs] & 0x0f,
					colorram[offs] & 0x40,colorram[offs] & 0x80,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -seicross_row_scroll[offs];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	/* draw sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		x=spriteram[offs + 3];
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] & 0x3f) + ((spriteram[offs + 1] & 0x10) << 2) + 128,
				spriteram[offs + 1] & 0x0f,
				spriteram[offs] & 0x40,spriteram[offs] & 0x80,
				x,240-spriteram[offs + 2],
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		if(x>0xf0)
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram[offs] & 0x3f) + ((spriteram[offs + 1] & 0x10) << 2) + 128,
					spriteram[offs + 1] & 0x0f,
					spriteram[offs] & 0x40,spriteram[offs] & 0x80,
					x-256,240-spriteram[offs + 2],
					&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

	for (offs = spriteram_2_size - 4;offs >= 0;offs -= 4)
	{
		x=spriteram_2[offs + 3];
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram_2[offs] & 0x3f) + ((spriteram_2[offs + 1] & 0x10) << 2),
				spriteram_2[offs + 1] & 0x0f,
				spriteram_2[offs] & 0x40,spriteram_2[offs] & 0x80,
				x,240-spriteram_2[offs + 2],
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		if(x>0xf0)
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram_2[offs] & 0x3f) + ((spriteram_2[offs + 1] & 0x10) << 2),
					spriteram_2[offs + 1] & 0x0f,
					spriteram_2[offs] & 0x40,spriteram_2[offs] & 0x80,
					x-256,240-spriteram_2[offs + 2],
					&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
