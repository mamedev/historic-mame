/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfx_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong has two 256x4 palette PROMs and maybe one 256x4 lookup table
  PROM (for characters). I haven't figured out how the supposed-to-be lookup
  table works.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- inverter -- 220 ohm resistor  -- RED
        -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
  bit 0 -- inverter -- 220 ohm resistor  -- GREEN
  bit 3 -- inverter -- 470 ohm resistor  -- GREEN
        -- inverter -- 1  kohm resistor  -- GREEN
        -- inverter -- 220 ohm resistor  -- BLUE
  bit 0 -- inverter -- 470 ohm resistor  -- BLUE

***************************************************************************/
void dkong_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	/* provide our own lookup table for characters */
	static unsigned char chartable[64] =
	{
		80,80,80,80,
		16,16,16,16,
		16,16,16,16,
		16,16,16,16,
		16,16,16,16,
		80,80,80,80,
		80,80,144,144,
		144,144,144,144,
		16,16,16,144,
		16,16,16,16,
		0,0,0,0,
		208,208,208,208,
		144,144,144,144,
		144,144,144,144,
		144,144,144,144,
		144,144,16,8
	};


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (~color_prom[i] >> 1) & 0x01;
		bit1 = (~color_prom[i] >> 2) & 0x01;
		bit2 = (~color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (~color_prom[i+256] >> 2) & 0x01;
		bit1 = (~color_prom[i+256] >> 3) & 0x01;
		bit2 = (~color_prom[i] >> 0) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (~color_prom[i+256] >> 0) & 0x01;
		bit2 = (~color_prom[i+256] >> 1) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	/* characters */
	for (i = 0;i < 64*4;i++)
		colortable[i] = chartable[i / 4] + i % 4;

	/* sprites */
	for (i = 0;i < 16*4;i++)
		colortable[64*4 + i] = i + 128;
}



int dkong_vh_start(void)
{
	gfx_bank = 0;

	return generic_vh_start();
}



void dkongjr_gfxbank_w(int offset,int data)
{
	gfx_bank = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dkong_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charcode;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			charcode = videoram[offs] + 256 * gfx_bank;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,charcode >> 2,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 1] & 0x7f,spriteram[offs + 2] & 0x7f,
					spriteram[offs + 1] & 0x80,spriteram[offs + 2] & 0x80,
					spriteram[offs] - 7,spriteram[offs + 3] - 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
