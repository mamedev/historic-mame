/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfx_bank;
unsigned char *mario_sprite_palette;



int mario_vh_start(void)
{
	gfx_bank = 0;

	return generic_vh_start();
}



void mario_gfxbank_w(int offset,int data)
{
	gfx_bank = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mario_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charcode;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs % 32);
			sy = 8 * (31 - offs / 32);

			charcode = videoram[offs] + 256 * gfx_bank;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,charcode >> 4,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 2],
					(spriteram[offs + 1] & 0x0f) + 0x10 * *mario_sprite_palette,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					248 - spriteram[offs + 3],spriteram[offs] - 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
