/*************************************************************************
 Universal Cheeky Mouse Driver
 (c)Lee Taylor May 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distrabuted as part of any commerical work.
***************************************************************************
Functions to emulate the video hardware of the machine.
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int sprites[0x40];

void cheekymouse_sprite_w(int offset, int data)
{
	sprites[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void cheekymouse_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	static int x_directions[] = {16, 16, 0, 0};
	static int y_directions[] = {0, 0, -16, -16};

	int offs;

	int i, x, y;


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

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	for (offs = 0; offs < 0x40; offs += 4)
	{
		int v1, v2, v3, v4;

		v1 = sprites[offs + 0];
		v2 = sprites[offs + 1];
		v3 = 256 - sprites[offs + 2];
		v4 = sprites[offs + 3];

		if (v4 == 0) continue;

		if (v4 == 9)
		{
			int offset;

			drawgfx(bitmap,Machine->gfx[1],
					63-1 - 2 * (v1 & 0x0f),
					0,
					0,0,
					v3,v2,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			drawgfx(bitmap,Machine->gfx[1],
					63 - 2 * (v1 & 0x0f),
					0,
					0,0,
					v3 + 8*(v1 & 2),v2 + 8*(~v1 & 2),
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
		else
		{
#define HEX(x) ((x < 10) ? (x + 28) : (x - 9))
			int high, low;

			high = v1 >> 4; high = HEX(high); low = HEX(v1%15);
			drawgfx(bitmap,Machine->gfx[0],
					high,0,0,0,v3,v2,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			drawgfx(bitmap,Machine->gfx[0],
					low,0,0,0,v3,v2+8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			high = v4 >> 4; high = HEX(high); low = HEX(v4%15);
			drawgfx(bitmap,Machine->gfx[0],
					high,0,0,0,v3-8,v2,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			drawgfx(bitmap,Machine->gfx[0],
					low,0,0,0,v3-8,v2+8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
