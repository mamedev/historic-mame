/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *warpwarp_bulletsram;



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int mx,my,sx,sy;

			mx = (offs / 32);
			my = (offs % 32);

			if (mx == 0)
			{
				sx = my;
				sy = 33;
			}
			else if (mx == 1)
			{
				sx = my;
				sy = 0;
			}
			else
			{
				sx = mx;
				sy = my+1;
			}
			sx = 32-sx;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs]&0xF,
					0,0,8*sx ,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	{
		int x,y;
		int colour;
		colour = Machine->gfx[0]->colortable[0x09];

		x = warpwarp_bulletsram[1];
		x += 8;
		if (x >= Machine->drv->visible_area.min_x && x <= Machine->drv->visible_area.max_x)
		{
			y = 256 - warpwarp_bulletsram[0];
			if (y<256-16) {
				y += 5;
				if (y >= 0)
				{
					int j;

					for (j = 0; j < 2; j++)
					{
						bitmap->line[y+j][x+0] = colour;
						bitmap->line[y+j][x+1] = colour;
	/*					bitmap->line[y+j][x+2] = colour; */
					}
				}
			}
		}
	}
}
