/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *seicross_row_scroll;




void seicross_colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		/* bit 5 of the address is not used for color memory. There is just */
		/* 512k of memory; every two consecutive rows share the same memory */
		/* region. */
		offset &= 0xffdf;

		dirtybuffer[offset] = 1;
		dirtybuffer[offset + 0x20] = 1;

		colorram[offset] = data;
		colorram[offset + 0x20] = data;
	}
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void seicross_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 0x10) ? 1 : 0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x0f,
					colorram[offs] & 0x40,colorram[offs] & 0x80,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = seicross_row_scroll[offs];

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* draw sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[spriteram[offs + 1] & 0x10 ? 4 : 3],
				(spriteram[offs] & 0x3f),
				spriteram[offs + 1] & 0x0f,
				spriteram[offs] & 0x80,spriteram[offs] & 0x40,
				spriteram[offs + 2] + 1,spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

}
