/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

unsigned char *amidar_attributesram;



void amidar_attributes_w(int offset,int data)
{
	if ((offset & 1) && amidar_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < VIDEO_RAM_SIZE;i += 32)
			dirtybuffer[i] = 1;
	}

	amidar_attributesram[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void amidar_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = (31 - offs / 32);
			sy = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					amidar_attributesram[2 * sy + 1],
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 4*7;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 2],
				spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
				spriteram[offs],((spriteram[offs + 3] + 8) & 0xff) - 8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
