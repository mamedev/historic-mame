#include "driver.h"
#include "vidhrdw/generic.h"


VIDEO_UPDATE( blockade )
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

			sx = offs % 32;
			sy = offs / 32;

			charcode = videoram[offs];

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					0,
					0,0,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
