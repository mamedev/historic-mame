/* Hyper Pacman */

#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_UPDATE( hyperpac )
{
	int sx=0, sy=0, x=0, y=0, offs;

	fillbitmap(bitmap,get_black_pen(),&Machine->visible_area);

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		int dx = spriteram16[offs+4] & 0xff;
		int dy = spriteram16[offs+5] & 0xff;
		int tilecolour = spriteram16[offs+3];
		int attr = spriteram16[offs+7];
		int flipx =   attr & 0x80;
		int flipy =  (attr & 0x40) << 1;
		int tile  = ((attr & 0x1f) << 8) + (spriteram16[offs+6] & 0xff);

		if (tilecolour & 1) dx = -1 - (dx ^ 0xff);
		if (tilecolour & 2) dy = -1 - (dy ^ 0xff);
		if (tilecolour & 4)
		{
			x += dx;
			y += dy;
		}
		else
		{
			x = dx;
			y = dy;
		}

		if (x > 511) x &= 0x1ff;
		if (y > 511) y &= 0x1ff;

		if (flip_screen)
		{
			sx = 240 - x;
			sy = 240 - y;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sx = x;
			sy = y;
		}

		drawgfx(bitmap,Machine->gfx[0],
				tile,
				(tilecolour & 0xf0) >> 4,
				flipx, flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
