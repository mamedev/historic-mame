/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *carnival_characterram;
static unsigned char dirtycharacter[256];



void carnival_characterram_w(int offset,int data)
{
	if (carnival_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		carnival_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void carnival_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


		/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,carnival_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,charcode >> 5,
					0,0,sx + 16,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		}
	}


	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 2) dirtycharacter[offs] = 0;
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
