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
void punchout_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;
static int base;
if (osd_key_pressed(OSD_KEY_Z))
{
	while (osd_key_pressed(OSD_KEY_Z));
	base -= 0x400;
}
if (osd_key_pressed(OSD_KEY_X))
{
	while (osd_key_pressed(OSD_KEY_X));
	base += 0x400;
}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (1||dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs+base],
0,//					colorram[offs] & 0x7f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
