/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"


static int flipscreen[2];



WRITE_HANDLER( pinbo_flipscreen_w )
{
	if (flipscreen[0] != (data & 1))
	{
		flipscreen[0] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
	if (flipscreen[1] != (data & 2))
	{
		flipscreen[1] = data & 2;
		memset(dirtybuffer,1,videoram_size);
	}
}

VIDEO_UPDATE( pinbo )
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

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0x70) << 5),
					colorram[offs] & 0x0f,
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy;
		int code,color;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs];
		flipx = spriteram[offs+1] & 0x40;
		flipy = spriteram[offs+1] & 0x80;
		if (flipscreen[0])
		{
			sx = 240 - sx;
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}
		code = (spriteram[offs+1] & 0x3f) | 0x40 | ((spriteram[offs+2] & 0x30) << 3);
		color = (spriteram[offs+2] & 0x0f);

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
