/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *bagman_video_enable;



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bagman_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	if (*bagman_video_enable == 0)
	{
		clearbitmap(bitmap);

		return;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[colorram[offs] & 0x10 ? 1 : 0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x0f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs + 2] && spriteram[offs + 3])
			drawgfx(bitmap,Machine->gfx[2],
					(spriteram[offs] & 0x3f) + 2 * (spriteram[offs + 1] & 0x20),
					spriteram[offs + 1] & 0x1f,
					spriteram[offs] & 0x80,spriteram[offs] & 0x40,
					spriteram[offs + 2] + 1,spriteram[offs + 3] - 1,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
