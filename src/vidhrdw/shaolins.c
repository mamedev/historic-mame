/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *shaolins_scroll;


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void shaolins_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;
	int sx,sy;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs <= videoram_size;offs++)
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sx = 8 * (31 - (offs / 32) );
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0x40) << 2),
					colorram[offs] & 0x0f,
					colorram[offs] & 0x20,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


/* remove and use single blit */
	/* copy the temporary bitmap to the screen */
	{
		int scroll[32], i;

		for (i = 0;i < 4;i++)
			scroll[i] = 0;
		for (i = 4;i < 32;i++)
			scroll[i] = *shaolins_scroll+1;
		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	for (offs = spriteram_size-32; offs >= 0; offs-=32 ) /* max 24 sprites */
	{
		if( spriteram[offs] ) /* visibility flag */
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs+8], 		/* sprite index: 0..255 */
					spriteram[offs+9] & 0x0F, 			/* pal? */
					!(spriteram[offs+9] & 0x80),(spriteram[offs+9] & 0x40), /* flip flags */
					spriteram[offs+4]-7,240-spriteram[offs+6],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
