/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define VIDEO_RAM_SIZE 0x1000

unsigned char *popeye_videoram,*popeye_colorram;



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void popeye_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64) - 16;

			drawgfx(tmpbitmap,Machine->gfx[3],
					videoram[offs],
					0,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 128*4;offs += 4)
	{
		drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x04) ? 1 : 2],
				0xff - (spriteram[offs + 2] & 0x7f) - 8*(spriteram[offs + 3] & 0x10),
				spriteram[offs + 3] & 0x03,
				spriteram[offs + 2] & 0x80,0,
				2*(spriteram[offs]-3),2*(256-spriteram[offs + 1]) - 16,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;


		sx = 16 * (offs % 32);
		sy = 16 * (offs / 32) - 16;

		if (popeye_videoram[offs] != 0xff)	/* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
					popeye_videoram[offs],popeye_colorram[offs],
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
