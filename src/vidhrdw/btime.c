/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int background_image;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Actually Burger Time uses RAM, not PROMs to store the palette. However the
  game doesn't seem to use this feature, so we don't care to emulate dynamic
  palette adjustments.
  The palette RAM is connected to the RGB output this way:

  bit 7 -- 15 kohm resistor  -- BLUE (inverted)
        -- 33 kohm resistor  -- BLUE (inverted)
        -- 15 kohm resistor  -- GREEN (inverted)
        -- 33 kohm resistor  -- GREEN (inverted)
        -- 47 kohm resistor  -- GREEN (inverted)
        -- 15 kohm resistor  -- RED (inverted)
        -- 33 kohm resistor  -- RED (inverted)
  bit 0 -- 47 kohm resistor  -- RED (inverted)

***************************************************************************/
void btime_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x2e * bit0 + 0x41 * bit1 + 0x90 * bit2;
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x2e * bit0 + 0x41 * bit1 + 0x90 * bit2;
		bit0 = 0;
		bit1 = (~color_prom[i] >> 6) & 0x01;
		bit2 = (~color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x2e * bit0 + 0x41 * bit1 + 0x90 * bit2;
	}

	for (i = 0;i < 2 * 8;i++)
		colortable[i] = i;
}



void btime_background_w(int offset,int data)
{
	if (background_image != data)
	{
		memset(dirtybuffer,1,videoram_size);

		background_image = data;

		/* kludge to make the sprites disappear when the screen is cleared */
		if (data == 0)
		{
			int i;


			for (i = 0;i < spriteram_size;i += 4)
				spriteram[i] = 0;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void btime_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			/* draw the background (this can be handled better) */
			if (background_image & 0x10)
			{
				struct rectangle clip;
				int bx,by;
				int base,bgoffs;
				/* kludge to get the correct background */
				static int mapconvert[8] = { 1,2,3,0,5,6,7,4 };


				clip.min_x = sx;
				clip.max_x = sx+7;
				clip.min_y = sy;
				clip.max_y = sy+7;

				bx = sx & 0xf0;
				by = sy & 0xf0;

				base = 0x100 * mapconvert[(background_image & 0x07)];
				bgoffs = base+16*(by/16)+bx/16;

				drawgfx(tmpbitmap,Machine->gfx[2],
						Machine->memory_region[2][bgoffs],
						0,
						0,0,
						bx,by,
						&clip,TRANSPARENCY_NONE,0);

				drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 2) ? 1 : 0],
						videoram[offs] + 256 * (colorram[offs] & 1),
						0,
						0,0,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
			else
				drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 2) ? 1 : 0],
						videoram[offs] + 256 * (colorram[offs] & 1),
						0,
						0,0,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			drawgfx(bitmap,Machine->gfx[3],
					spriteram[offs + 1],
					0,
					spriteram[offs] & 0x02,0,
					239 - spriteram[offs + 2],spriteram[offs + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
