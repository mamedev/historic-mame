/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Arkanoid has a three 512x4 palette PROMs (one per gun).
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void arkanoid_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i,j,used;
	unsigned char allocated[3*256];


	/* The game has 512 colors, but we are limited to a maximum of 256. */
	/* Luckily, many of the colors are duplicated, so the total number of */
	/* different colors is less than 256. We select the unique colors and */
	/* put them in our palette. */

	memset(palette,0,3 * Machine->drv->total_colors);

	used = 0;
	for (i = 0;i < 512;i++)
	{
		for (j = 0;j < used;j++)
		{
			if (allocated[j] == color_prom[i] &&
					allocated[j+256] == color_prom[i+512] &&
					allocated[j+2*256] == color_prom[i+2*512])
				break;
		}
		if (j == used)
		{
			int bit0,bit1,bit2,bit3;


			used++;

			allocated[j] = color_prom[i];
			allocated[j+256] = color_prom[i+512];
			allocated[j+2*256] = color_prom[i+2*512];

			bit0 = (color_prom[i] >> 0) & 0x01;
			bit1 = (color_prom[i] >> 1) & 0x01;
			bit2 = (color_prom[i] >> 2) & 0x01;
			bit3 = (color_prom[i] >> 3) & 0x01;
			palette[3*j] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[i+512] >> 0) & 0x01;
			bit1 = (color_prom[i+512] >> 1) & 0x01;
			bit2 = (color_prom[i+512] >> 2) & 0x01;
			bit3 = (color_prom[i+512] >> 3) & 0x01;
			palette[3*j + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[i+512*2] >> 0) & 0x01;
			bit1 = (color_prom[i+512*2] >> 1) & 0x01;
			bit2 = (color_prom[i+512*2] >> 2) & 0x01;
			bit3 = (color_prom[i+512*2] >> 3) & 0x01;
			palette[3*j + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		}

		colortable[i] = j;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void arkanoid_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;
static int base;

if (osd_key_pressed(OSD_KEY_SPACE))
{
	while (osd_key_pressed(OSD_KEY_SPACE));
	base ^= 1;
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
			int sx,sy;
			int num;

			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sy = offs2 % 32;
			sx = 31 - (offs2 / 32);

//			if (flipscreen[0]) sx = 31 - sx;
//			if (flipscreen[1]) sy = 31 - sy;

			num = videoram[offs+1] + ((videoram[offs] & 0x07) << 8) + 2048*base;
			drawgfx(tmpbitmap,Machine->gfx[0],
					num,
					((videoram[offs] & 0xf8) >> 3) + 32*base,
//					flipscreen[0],flipscreen[1],
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
