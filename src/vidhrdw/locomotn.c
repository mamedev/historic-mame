/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void locomotn_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			if (offs < 0x400)
			{
						sx = 8 * (offs / 32);
						sy = 8 * (7 - offs % 32);
			}
			else
			{
						sx = 8 * ((offs - 0x400) / 32);
						sy = 8 * (35 - offs % 32);
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs]&0x7f) + 2*(colorram[offs]&0x40) + 2*(videoram[offs]&0x80),
					colorram[offs] & 0x3f,
				/* not a mistake, one bit selects both  flips */
					~colorram[offs] & 0x80,~colorram[offs] & 0x80,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		if (spriteram_2[offs + 1] > 16)
			drawgfx(bitmap,Machine->gfx[1],
					((spriteram_2[offs] & 0x7c) >> 2) + 0x20*(spriteram_2[offs] & 0x01),
					spriteram[offs + 1],
					0,0,
					spriteram[offs] - 16,spriteram_2[offs + 1] + 32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
