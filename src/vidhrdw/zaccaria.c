/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *zaccaria_attributesram;




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Just a guess.

***************************************************************************/
void zaccaria_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}
}



void zaccaria_attributes_w(int offset,int data)
{
	if ((offset & 1) && zaccaria_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	zaccaria_attributesram[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void zaccaria_vh_screenrefresh(struct osd_bitmap *bitmap)
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
					videoram[offs] + ((colorram[offs] & 0x03) << 8),
					(zaccaria_attributesram[2 * (offs % 32) + 1] & 0x07)	/* just */
							+ 8 * ((colorram[offs] & 0x1c) >> 2),			/* a guess */
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -zaccaria_attributesram[2 * offs];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw sprites */
	for (offs = 0;offs < spriteram_2_size;offs += 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram_2[offs + 2] & 0x3f) + (spriteram_2[offs + 1] & 0xc0),
				spriteram_2[offs + 1] & 0x3f,	/* guess */
				spriteram_2[offs + 2] & 0x40,spriteram_2[offs + 2] & 0x80,
				spriteram_2[offs + 3] + 1,241 - spriteram_2[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs + 1] & 0x3f) + (spriteram[offs + 2] & 0xc0),
				spriteram[offs + 2] & 0x3f,	/* guess */
				spriteram[offs + 1] & 0x40,spriteram[offs + 1] & 0x80,
				spriteram[offs + 3] + 1,241 - spriteram[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
