/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *marineb_column_scroll;


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void draw_chars(struct osd_bitmap *_tmpbitmap, struct osd_bitmap *bitmap,
                       int scroll_cols)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			/* does Marine Boy use flipx/flipy??? (it might have 64 colors) */
			flipx = colorram[offs] & 0x20;
			flipy = colorram[offs] & 0x10;

			drawgfx(_tmpbitmap,Machine->gfx[0],
					videoram[offs] | ((colorram[offs] & 0x40) << 2),
					colorram[offs] & 0x0f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < scroll_cols;offs++)
			scroll[offs] = -marineb_column_scroll[0];

		for (;offs < 32;offs++)
			scroll[offs] = 0;

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}


void marineb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	draw_chars(tmpbitmap, bitmap, 24);


	/* draw the sprites */
	for (offs = 0x0b; offs >= 0; offs--)
	{
		int gfx,sx,sy,code,col,flipx,flipy,offs2;


		if ((offs == 0) || (offs == 2))  continue;  /* no sprites here */


		if (offs < 8)
		{
			offs2 = 0x0018 + offs;
		}
		else
		{
			offs2 = 0x03d8 - 8 + offs;
		}


		code  = videoram[offs2];
		sx    = videoram[offs2 + 0x20];
		sy    = 240 - colorram[offs2];
		col   = colorram[offs2 + 0x20];
		flipx = code & 0x02;
		flipy = code & 0x01;

		if (offs < 4)
		{
			/* big sprite */
			sy -= 0x10;
			gfx = 2;
			code = (code >> 4) | ((code & 0x0c) << 2);
		}
		else
		{
			/* small sprite */
			gfx = 1;
			code >>= 2;
		}

		drawgfx(bitmap,Machine->gfx[gfx],
				code,
				col,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}


void changes_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,sx,sy,code,col,flipx,flipy;


	draw_chars(tmpbitmap, bitmap, 26);


	/* draw the small sprites */
	for (offs = 0x05; offs >= 0; offs--)
	{
		int offs2;


		offs2 = 0x001a + offs;

		code  = videoram[offs2];
		sx    = videoram[offs2 + 0x20];
		sy    = 240 - colorram[offs2];
		col   = colorram[offs2 + 0x20];
		flipx = code & 0x02;
		flipy = code & 0x01;

		drawgfx(bitmap,Machine->gfx[1],
				code >> 2,
				col,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

	/* draw the big sprite */

	code  = videoram[0x3df];
	sx    = videoram[0x3ff];
	sy    = 224 - colorram[0x3df];
	col   = colorram[0x3ff];
	flipx = code & 0x02;
	flipy = code & 0x01;

	code >>= 4;

	drawgfx(bitmap,Machine->gfx[2],
			code,
			col,
			flipx,flipy,
			sx,sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* draw again for wrap around */

	drawgfx(bitmap,Machine->gfx[2],
			code,
			col,
			flipx,flipy,
			sx-256,sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}


void springer_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	draw_chars(tmpbitmap, bitmap, 0);


	/* draw the sprites */
	for (offs = 0x0f; offs >= 0; offs--)
	{
		int gfx,sx,sy,code,col,flipx,flipy,offs2;


		if ((offs == 0) || (offs == 2))  continue;  /* no sprites here */


		offs2 = 0x0010 + offs;


		code  = videoram[offs2];
		sx    = 240 - videoram[offs2 + 0x20];
		sy    = 240 - colorram[offs2];
		col   = colorram[offs2 + 0x20];
		flipx = ~code & 0x02;
		flipy = code & 0x01;

		if (offs < 4)
		{
			/* big sprite */
			sy -= 0x10;
			sx -= 0x10;
			gfx = 2;
			code = (code >> 4) | ((code & 0x0c) << 2);
		}
		else
		{
			/* small sprite */
			gfx = 1;
			code >>= 2;
		}

		drawgfx(bitmap,Machine->gfx[gfx],
				code,
				col,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}


void hoccer_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	draw_chars(tmpbitmap, bitmap, 0);


	/* draw the sprites */
	for (offs = 0x07; offs >= 0; offs--)
	{
		int sx,sy,code,col,flipx,flipy,offs2;


		offs2 = 0x0018 + offs;


		code  = spriteram[offs2];
		sx    = spriteram[offs2 + 0x20];
		sy    = 240 - colorram[offs2];
		col   = colorram[offs2 + 0x20];
		flipx = code & 0x02;
		flipy = code & 0x01;

		drawgfx(bitmap,Machine->gfx[1],
				code >> 2,
				col,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

