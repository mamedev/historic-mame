/*************************************************************************

	Atari Super Breakout hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"
#include "sbrkout.h"

unsigned char *sbrkout_horiz_ram;
unsigned char *sbrkout_vert_ram;


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( sbrkout )
{
	int offs;
	int ball;


	if (get_vh_global_attribute_changed())
		memset(dirtybuffer,1,videoram_size);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int code,sx,sy;


			dirtybuffer[offs]=0;

			code = videoram[offs] & 0x3f;

			sx = 8*(offs % 32);
			sy = 8*(offs / 32);

			/* Check the "draw" bit */
			if (videoram[offs] & 0x80)
				drawgfx(tmpbitmap,Machine->gfx[0],
						code, 0,
						0,0,sx,sy,
						&Machine->visible_area,TRANSPARENCY_NONE,0);
			else
			{
				struct rectangle bounds;
				bounds.min_x = sx;
				bounds.min_y = sy;
				bounds.max_x = sx + 7;
				bounds.max_y = sy + 7;
				fillbitmap(tmpbitmap, 0, &bounds);
			}
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Draw each one of our three balls */
	for (ball=2;ball>=0;ball--)
	{
		int sx,sy,code;


		sx = 31*8-sbrkout_horiz_ram[ball*2];
		sy = 30*8-sbrkout_vert_ram[ball*2];

		code = ((sbrkout_vert_ram[ball*2+1] & 0x80) >> 7);

		drawgfx(bitmap,Machine->gfx[1],
				code,0,
				0,0,sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

