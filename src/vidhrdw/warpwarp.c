/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define VIDEO_RAM_SIZE 0x400


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8*(offs % 32);
			sy = 8*(offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	/* sprites #0 and #7 are not used */
	for (i = 6;i > 0;i--)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[2*i] >> 2,0,
				0,0,
				239 - spriteram[2*i],272 - spriteram[2*i + 1],
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}
