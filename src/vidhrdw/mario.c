/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

static int gfx_bank;



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
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
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
					charcode,charcode >> 2,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (i = 0;i < 4*96;i += 4)
	{
		if (spriteram[i])
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[i+2],spriteram[i+1] & 0x3f,
					spriteram[i+1] & 0x80,spriteram[i+1] & 0x40,
					248 - spriteram[i+3],spriteram[i] - 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
