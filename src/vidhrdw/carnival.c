/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *carnival_characterram;
static unsigned char dirtycharacter[256];

int	ColourBank = 0;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Carnival has one 32x8 palette PROM. The color code is taken from the three
  most significant bits of the character code, plus two additional palette
  bank bits which however are not used by Carnival.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  foreground
        -- 22 ohm resistor  -- GREEN /
        -- Unused
        -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  background
        -- 22 ohm resistor  -- GREEN /
  bit 0 -- Unused

***************************************************************************/
void carnival_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit;


		bit = (color_prom[i] >> 3) & 0x01;
		palette[3*2*i] = 0xff * bit;
		bit = (color_prom[i] >> 1) & 0x01;
		palette[3*2*i + 1] = 0xff * bit;
		bit = (color_prom[i] >> 2) & 0x01;
		palette[3*2*i + 2] = 0xff * bit;

		bit = (color_prom[i] >> 7) & 0x01;
		palette[3*(2*i+1)] = 0xff * bit;
		bit = (color_prom[i] >> 5) & 0x01;
		palette[3*(2*i+1) + 1] = 0xff * bit;
		bit = (color_prom[i] >> 6) & 0x01;
		palette[3*(2*i+1) + 2] = 0xff * bit;
	}

	/* characters use colors 128-143 */
	for (i = 0;i < 32*2;i++)
		colortable[i] = i;
}



void carnival_characterram_w(int offset,int data)
{
	if (carnival_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		carnival_characterram[offset] = data;
	}
}

void carnival_colour_bank_w(int offset, int data)
{
	/* Offset to first colour */

	ColourBank = data * 8;

    if (errorlog) fprintf(errorlog,"Colour Bank %d\n",data);
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void carnival_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


		/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,carnival_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					(charcode >> 5) + ColourBank,
					0,0,
					sx + 16,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		}
	}


	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 2) dirtycharacter[offs] = 0;
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
