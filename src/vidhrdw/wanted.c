/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int flipscreen_x;
static int flipscreen_y;


void wanted_flipscreen_x_w(int offset, int data)
{
	if (flipscreen_x != data)
	{
		flipscreen_x = data;
		memset(dirtybuffer, 1, videoram_size);
	}
}

void wanted_flipscreen_y_w(int offset, int data)
{
	if (flipscreen_y != data)
	{
		flipscreen_y = data;
		memset(dirtybuffer, 1, videoram_size);
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void wanted_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* draw the background characters */
	for (offs = 0; offs < videoram_size; offs++)
	{
		int code,sx,sy,col,flipx,flipy;


		if (!dirtybuffer[offs])  continue;

		dirtybuffer[offs] = 0;

		sy = offs / 32;
		sx = offs % 32;

		col  = colorram[offs];
		code = videoram[offs] | ((col & 0xc0) << 2);

		flipx = col & 0x20;
		flipy = col & 0x10;

		if (flipscreen_y)
		{
			sy = 31 - sy;
			flipy = !flipy;
		}

		if (flipscreen_x)
		{
			sx = 31 - sx;
			flipx = !flipx;
		}

		drawgfx(tmpbitmap,Machine->gfx[0],
				code,
				col & 0x0f,
				flipx, flipy,
				8*sx, 8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0x0f; offs >= 0; offs --)
	{
		int code,col,sx,sy,flipx,flipy,gfx;


		sy = colorram[0x10+offs];
		sx = videoram[0x30+offs];

		if ((sy == 0xf0) || (sy == 0))  continue;


		code = videoram[0x10+offs];
		col  = colorram[0x30+offs];

		flipx = !(code & 0x02);
		flipy = !(code & 0x01);

		if (offs >= 4)
		{
			/* 16x16 sprites */
			gfx = 1;
			code >>= 2;
		}
		else
		{
			/* 32x32 sprites */
			gfx = 2;
			code = ((code & 0x0c) << 2) | ((code >> 4) & 0x0f);
		}

		sx = 256 - Machine->gfx[gfx]->width  - sx;

		if (!flipscreen_y)
		{
			sy = 256 - Machine->gfx[gfx]->width  - sy;
			flipy = !flipy;
		}

		if (!flipscreen_x)
		{
			sx--;
		}

		drawgfx(bitmap,Machine->gfx[gfx],
				code,
				0,//col,
				flipx, flipy,
				sx, sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

}
