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
void milliped_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
         int Add = 0x40;

			dirtybuffer[offs] = 0;

			sx = (offs / 32);
			sy = (31 - offs % 32);

         if (videoram[offs] & 0x40)	/* not used by Centipede */
            Add = 0xC0;

         drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + Add,
0,
					0,0,8*(sx+1),8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		if (spriteram[offs + 0x20] < 0xf8)
		{
			int spritenum;


			spritenum = spriteram[offs] & 0x3f;
			if (spritenum & 1) spritenum = spritenum / 2 + 64;
			else spritenum = spritenum / 2;

			drawgfx(bitmap,Machine->gfx[1],
					spritenum,
0,/*					spriteram[offs + 0x30],*/
					spriteram[offs] & 0x80,0,
					248 - spriteram[offs + 0x10],248 - spriteram[offs + 0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
