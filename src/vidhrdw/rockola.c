/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *rockola_videoram2;
unsigned char *rockola_characterram;
unsigned char *rockola_scrollx,*rockola_scrolly;
static unsigned char dirtycharacter[256];
static int flipscreen;
static int charbank;


void rockola_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 16*4;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 16*4;i++)
	{
		if (i % 4 == 0) colortable[i] = 0;
		else colortable[i] = i;
	}
}



void rockola_characterram_w(int offset,int data)
{
	if (rockola_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		rockola_characterram[offset] = data;
	}
}



void rockola_flipscreen_w(int offset,int data)
{
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
		memset(dirtybuffer,1,videoram_size);
	}

	if (charbank != ((~data & 0x08) >> 3))
	{
		charbank = (~data & 0x08) >> 3;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void rockola_vh_screenrefresh(struct osd_bitmap *bitmap)
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
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs] + 256 * charbank,
					(colorram[offs] & 0x38) >> 3,
					flipscreen,flipscreen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -*rockola_scrolly;
		scrolly = -*rockola_scrollx + 16;
		if (flipscreen)
		{
			scrollx = -scrollx;
			scrolly = -scrolly;
		}
		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;
		int sx,sy;


		charcode = rockola_videoram2[offs];

		/* decode modified characters */
		if (dirtycharacter[charcode] != 0)
		{
			decodechar(Machine->gfx[0],charcode,rockola_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter[charcode] = 0;
		}

		if (charcode != 0x30)	/* don't draw spaces */
		{
			sx = offs % 32;
			sy = offs / 32 + 2;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0x07,
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
