/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400



static struct rectangle spritevisiblearea =
{
	2*8, 30*8-1,
	4*8, 31*8-1
};



void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 5) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 2) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 0) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}

	for (i = 0;i < 384;i++)
		colortable[i] = color_prom[i];
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x3f,
					colorram[offs] & 0x80,colorram[offs] & 0x40,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 23*2;offs >= 0;offs -= 2)
	{
		if ((spriteram[offs + 1] >= 0x30 && spriteram[offs + 1] <= 0x37) ||
				(spriteram[offs + 1] >= 0x5c && spriteram[offs + 1] <= 0x7c) ||
				(spriteram[offs + 1] >= 0x85 && spriteram[offs + 1] <= 0x87))
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs + 1],
					spriteram_2[offs] & 0x3f,
					spriteram_2[offs] & 0x80,!(spriteram_2[offs] & 0x40),
					2 * spriteram_2[offs + 1] - 16,spriteram[offs],
					&spritevisiblearea,TRANSPARENCY_PEN,0);
		else
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 1],
					spriteram_2[offs] & 0x3f,
					spriteram_2[offs] & 0x80,!(spriteram_2[offs] & 0x40),
					spriteram_2[offs + 1] - 1,spriteram[offs],
					&spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
