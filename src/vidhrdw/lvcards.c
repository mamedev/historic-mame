/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/***************************************************************************

  Convert the color PROMs into a more useable format.

  I don't know the exact resistor values, I'm using the 1942 ones.

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE
  Color Fixed by Boochip 10/sep/03

***************************************************************************/
PALETTE_INIT( lvcards )
{

	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;

				/* red component */
				bit0 = (color_prom[0] >> 0) & 0x11;
				bit1 = (color_prom[0] >> 1) & 0x11;
				bit2 = (color_prom[0] >> 2) & 0x11;
				bit3 = (color_prom[0] >> 3) & 0x11;
				r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

				/* green component */
				bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x11;
				bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x11;
				bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x11;
				bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x11;
				g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

				/* blue component */
				bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x11;
				bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x11;
				bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x11;
				bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x11;
				b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( lvcards )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size; offs >= 0; offs--)
	{
		if (dirtybuffer[offs])
		{
			int col,sx,sy,flipx;

			dirtybuffer[offs] = 0;

			sx    = offs % 32;
			sy    = offs / 32;
			col   =  colorram[offs] & 0x0f;
			flipx =  colorram[offs] & 0x40;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0x30) << 4),
					col,
					flipx,0,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
