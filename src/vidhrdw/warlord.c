/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg
  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *warlord_paletteram;


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void warlord_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sy = (offs / 32);
			sx = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram [offs] & 0x3f,
					0,
					videoram [offs] & 0x40, videoram [offs] & 0x80 ,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int spritenum, color;

		spritenum = (spriteram[offs] & 0x3f) + 0x40;

		/* LBO - borrowed this from Centipede. It really adds a psychedelic touch. */
		/* Warlords is unusual because the sprite color code specifies the */
		/* colors to use one by one, instead of a combination code. */
		/* bit 5-4 = color to use for pen 11 */
		/* bit 3-2 = color to use for pen 10 */
		/* bit 1-0 = color to use for pen 01 */
		/* pen 00 is transparent */
		color = spriteram[offs+0x30];
#if 0
		Machine->gfx[1]->colortable[3] =
				Machine->pens[12 + ((color >> 4) & 3)];
		Machine->gfx[1]->colortable[2] =
				Machine->pens[12 + ((color >> 2) & 3)];
		Machine->gfx[1]->colortable[1] =
				Machine->pens[12 + ((color >> 0) & 3)];
#endif

		drawgfx(bitmap,Machine->gfx[1],
				spritenum, 0,
				spriteram [offs] & 0x40, spriteram [offs] & 0x80 ,
				spriteram [offs + 0x20], 248 - spriteram[offs + 0x10],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
