/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *vanguard_videoram2;
unsigned char *vanguard_characterram;
unsigned char *vanguard_scrollx,*vanguard_scrolly;

static unsigned char dirtycharacter[256];



void vanguard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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



void vanguard_characterram_w(int offset,int data)
{
	if (vanguard_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		vanguard_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void vanguard_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 31 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs],
					(colorram[offs] >> 3) & 0x07,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = *vanguard_scrollx - 2;
		scrolly = -(*vanguard_scrolly);
		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;
		int sx,sy;


		charcode = vanguard_videoram2[offs];

		/* decode modified characters */
		if (dirtycharacter[charcode] == 1)
		{
			decodechar(Machine->gfx[0],charcode,vanguard_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter[charcode] = 2;
		}

		if (charcode != 0x30)	/* don't draw spaces */
		{
			sx = (31 - offs / 32) - 2;
			sy = (offs % 32);

			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0x07,
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 2) dirtycharacter[offs] = 0;
	}
}
