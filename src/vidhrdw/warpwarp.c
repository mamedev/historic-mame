/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *warpwarp_bulletsram;

#define VIDEO_RAM_SIZE 0x400


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void warpwarp_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 255;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}
}

void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int mx,my,sx,sy;

	/* Even if Pengo's screen is 28x36, the memory layout is 32x32. We therefore */
	/* have to convert the memory coordinates into screen coordinates. */
	/* Note that 32*32 = 1024, while 28*36 = 1008: therefore 16 bytes of Video RAM */
	/* don't map to a screen position. We don't check that here, however: range */
	/* checking is performed by drawgfx(). */

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
					0 /* videoram[offs+VIDEO_RAM_SIZE] */,
					0,0,8*sx ,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	{
		int x,y;
		int color;
		color = Machine->gfx[0]->colortable[0x01];

		x = warpwarp_bulletsram[1];
		x += 8;
		if (x >= Machine->drv->visible_area.min_x && x <= Machine->drv->visible_area.max_x)
		{
			y = 256 - warpwarp_bulletsram[0];
			y += 5;
			if (y >= 0)
			{
				int j;

				for (j = 0; j < 2; j++)
				{
					bitmap->line[y+j][x+0] = color;
					bitmap->line[y+j][x+1] = color;
					bitmap->line[y+j][x+2] = color;
				}
			}
		}
	}
}
