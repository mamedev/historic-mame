/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *stinger_videoram2;
unsigned char *stinger_fg_attributesram,*stinger_bg_attributesram;

static unsigned char charbank[2];
static unsigned char palbank[2];
static int palette_bank;
static int flipscreen[2];



void stinger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	if (!color_prom) return;	/* Scion PROMs are missing */

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}



void stinger_attributes_w(int offset,int data)
{
	if ((offset & 1) && stinger_bg_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	stinger_bg_attributesram[offset] = data;
}

void stinger_palettebank_w(int offset,int data)
{
	if (palbank[offset] != (data & 1))
	{
		palbank[offset] = data & 1;
		palette_bank = palbank[0] + 2 * palbank[1];

		memset(dirtybuffer,1,videoram_size);
	}
}

void stinger_charbank_w(int offset,int data)
{
	if (offset == 0 && charbank[offset] != (data & 1))
		memset(dirtybuffer,1,videoram_size);

	charbank[offset] = data & 1;
}

void stinger_flipscreen_w(int offset,int data)
{
	if (flipscreen[offset] != (data & 1))
	{
		flipscreen[offset] = (data & 1);
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void stinger_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


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
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs] + 256 * charbank[0],
					(stinger_bg_attributesram[2 * (offs % 32) + 1] & 0x07) + 8 * palette_bank,
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		if (flipscreen[0])
		{
			for (offs = 0;offs < 32;offs++)
			{
				scroll[31-offs] = (stinger_bg_attributesram[2 * offs]);
				if (flipscreen[1]) scroll[31-offs] = -scroll[31-offs];
			}
		}
		else
		{
			for (offs = 0;offs < 32;offs++)
			{
				scroll[offs] = -(stinger_bg_attributesram[2 * offs]);
				if (flipscreen[1]) scroll[offs] = -scroll[offs];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;
		if (flipscreen[0]) sx = 31 - sx;
		if (flipscreen[1]) sy = 31 - sy;

		drawgfx(bitmap,Machine->gfx[0],
				stinger_videoram2[offs] + 256 * charbank[1],
				(stinger_fg_attributesram[2 * (offs % 32) + 1] & 0x07) + 8 * palette_bank,
				flipscreen[0],flipscreen[1],
				8*sx,(8*sy - stinger_fg_attributesram[2 * (offs % 32)]) & 0xff,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs];
		if (flipscreen[0]) sx = 240 - sx;
		if (flipscreen[1]) sy = 240 - sy;

		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs + 1],
				(spriteram[offs + 2] & 0x07) + 8 * palette_bank,
				flipscreen[0],flipscreen[1],
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy;


		sx = spriteram_2[offs + 3];
		sy = 240 - spriteram_2[offs];
		if (flipscreen[0]) sx = 240 - sx;
		if (flipscreen[1]) sy = 240 - sy;

		drawgfx(bitmap,Machine->gfx[3],
				spriteram_2[offs + 1],
				(spriteram_2[offs + 2] & 0x07) + 8 * palette_bank,
				flipscreen[0],flipscreen[1],
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
