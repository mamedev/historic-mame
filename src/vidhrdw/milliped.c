/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

//#define DEBUG_COLOR

#ifdef DEBUG_COLOR
#include <stdio.h>
FILE	*color_log;
#endif

int milliped_vh_start (void)
{
#ifdef DEBUG_COLOR
	color_log = fopen ("color.log","w");
#endif
	return generic_vh_start();
}

void milliped_vh_stop (void)
{
#ifdef DEBUG_COLOR
	fclose (color_log);
#endif
	generic_vh_stop();
}

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
         int color;

			dirtybuffer[offs] = 0;

			sx = (offs / 32);
			sy = (31 - offs % 32);

         if (videoram[offs] & 0x40)	/* not used by Centipede */
            Add = 0xC0;

		if (videoram[offs] & 0x80)
			color = 8;
		else color = 0;
         drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + Add,
					color,
					0,0,8*(sx+1),8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


#ifdef DEBUG_COLOR
	if (color_log) {
		int i;

		for (i = 0; i < 0x1f; i++)
			fprintf (color_log, "%02x ",RAM[0x2480+i]);

		fprintf (color_log,"\n");
		}
#endif
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
					/*spriteram[offs + 0x30],*/0,
					spriteram[offs] & 0x80,0,
					248 - spriteram[offs + 0x10],248 - spriteram[offs + 0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
