/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfxbank,palettebank;
extern int arkanoid_paddle_select;



WRITE_HANDLER( arkanoid_d008_w )
{
	/* bits 0 and 1 flip X and Y, I don't know which is which */
	flip_screen_x_set(data & 0x01);
	flip_screen_y_set(data & 0x02);

	/* bit 2 selects the input paddle */
    arkanoid_paddle_select = data & 0x04;

	/* bit 3 is coin lockout (but not the service coin) */
	coin_lockout_w(0, !(data & 0x08));
	coin_lockout_w(1, !(data & 0x08));

	/* bit 4 is unknown */

	/* bits 5 and 6 control gfx bank and palette bank. They are used together */
	/* so I don't know which is which. */
	set_vh_global_attribute(&gfxbank, (data & 0x20) >> 5);
	set_vh_global_attribute(&palettebank, (data & 0x40) >> 6);

	/* bit 7 is unknown */
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( arkanoid )
{
	int offs;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		int offs2;

		offs2 = offs/2;
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy,code;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs + 1] = 0;

			sx = offs2 % 32;
			sy = offs2 / 32;

			if (flip_screen_x) sx = 31 - sx;
			if (flip_screen_y) sy = 31 - sy;

			code = videoram[offs + 1] + ((videoram[offs] & 0x07) << 8) + 2048 * gfxbank;
			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					((videoram[offs] & 0xf8) >> 3) + 32 * palettebank,
					flip_screen_x,flip_screen_y,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sx,sy,code;


		sx = spriteram[offs];
		sy = 248 - spriteram[offs + 1];
		if (flip_screen_x) sx = 248 - sx;
		if (flip_screen_y) sy = 248 - sy;

		code = spriteram[offs + 3] + ((spriteram[offs + 2] & 0x03) << 8) + 1024 * gfxbank;
		drawgfx(bitmap,Machine->gfx[0],
				2 * code,
				((spriteram[offs + 2] & 0xf8) >> 3) + 32 * palettebank,
				flip_screen_x,flip_screen_y,
				sx,sy + (flip_screen_y ? 8 : -8),
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		drawgfx(bitmap,Machine->gfx[0],
				2 * code + 1,
				((spriteram[offs + 2] & 0xf8) >> 3) + 32 * palettebank,
				flip_screen_x,flip_screen_y,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
