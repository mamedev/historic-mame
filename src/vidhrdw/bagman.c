/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *bagman_video_enable;
static int flipscreen[2];



void bagman_flipscreen_w(int offset,int data)
{
	if ((data & 1) != flipscreen[offset])
	{
		flipscreen[offset] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



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

			sx = offs % 32;
			if (flipscreen[0]) sx = 31 - sx;
			sy = offs / 32;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[colorram[offs] & 0x10 ? 1 : 0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x0f,
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs + 2];
		flipx = spriteram[offs] & 0x40;
		flipy = spriteram[offs] & 0x80;
		if (flipscreen[0])
		{
			sx = 240 - sx +1;	/* compensate misplacement */
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		if (spriteram[offs + 2] && spriteram[offs + 3])
			drawgfx(bitmap,Machine->gfx[2],
					(spriteram[offs] & 0x3f) + 2 * (spriteram[offs + 1] & 0x20),
					spriteram[offs + 1] & 0x1f,
					flipx,flipy,
					sx,sy+1,	/* compensate misplacement */
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
