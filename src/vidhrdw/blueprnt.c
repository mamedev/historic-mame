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
void blueprnt_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x7f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 1],
				0,
				spriteram[offs + 2 - 4] & 0x80,0,	/* -4? Awkward, isn't it? */
				239-spriteram[offs + 0],246-spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* redraw the characters which have priority over sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (colorram[offs] & 0x80)
		{
			int sx,sy;


			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x7f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
