/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VIDEO_RAM_SIZE 0x1000

unsigned char *popeye_videoram,*popeye_colorram,*popeye_backgroundram;
unsigned char *popeye_background_pos,*popeye_palette_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Popeye has four color PROMS:
  - 32x8 char palette
  - 32x8 background palette
  - two 256x4 sprite palette
  I don't know for sure how the PROMs are connected to the RGB output, but
  it's probably the usual:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void popeye_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 0) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}

	for (i = 0;i < 32;i++)	/* characters */
	{
		colortable[2*i] = 0;
		colortable[2*i + 1] = color_prom[i];
	}
	for (i = 0;i < 32;i++)	/* background */
	{
		colortable[i+32*2] = color_prom[i+32];
	}
	for (i = 0;i < 64*4;i++)	/* sprites */
	{
		colortable[i+32*2+32] = (color_prom[i+64] & 0x0f) | ((color_prom[i+64+64*4] << 4) & 0xf0);
	}
}



void popeye_backgroundram_w(int offset,int data)
{
	if (data & 0x80)	/* write to the upper nibble */
	{
		if ((popeye_backgroundram[offset] & 0xf0) != ((data << 4)&0xf0))
		{
			dirtybuffer[offset] = 1;

			popeye_backgroundram[offset] = (popeye_backgroundram[offset] & 0x0f) | ((data << 4)&0xf0);
		}
	}
	else	/* write to the lower nibble */
	{
		if ((popeye_backgroundram[offset] & 0x0f) != (data & 0x0f))
		{
			dirtybuffer[offset] = 1;

			popeye_backgroundram[offset] = (popeye_backgroundram[offset] & 0xf0) | (data & 0x0f);
		}
	}
}



void popeye_palettebank_w(int offset,int data)
{
	if ((data & 0x08) != (*popeye_palette_bank & 0x08))
	{
		int i;


		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;
	}

	*popeye_palette_bank = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void popeye_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

/* Since the visible area misses 1 char each side,    */
/* skip the first rows, don't seem to be used anyway  */

/* Move colour table calculation out of loop          */

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = 128;offs < VIDEO_RAM_SIZE-128;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,y,colour;

			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64) - 16;

			if (sx >= Machine->drv->visible_area.min_x &&
					sx+7 <= Machine->drv->visible_area.max_x &&
					sy >= Machine->drv->visible_area.min_y &&
					sy+7 <= Machine->drv->visible_area.max_y)
			{
                colour = Machine->gfx[3]->colortable[(popeye_backgroundram[offs] & 0x0f) + 2*(*popeye_palette_bank & 0x08)];
				for (y = 0;y < 4;y++)
					memset(&tmpbitmap->line[sy+y][sx],colour,8);

                colour = Machine->gfx[3]->colortable[(popeye_backgroundram[offs] >> 4) + 2*(*popeye_palette_bank & 0x08)];
				for (y = 4;y < 8;y++)
					memset(&tmpbitmap->line[sy+y][sx],colour,8);
			}
		}
	}


	/* copy the background graphics */

/* Sets X & Y offset to 0 when background not wanted   */
/* otherwise copy the bitmap to where it's needed      */

	if (popeye_background_pos[0] == 0)
    {
        clearbitmap(bitmap);
    }
    else
    {
		copybitmap(bitmap,tmpbitmap,0,0,
			400 - 2 * popeye_background_pos[0],
			2 * (256 - popeye_background_pos[1]),
  			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
    }


/* Don't draw sprites with offset of 0 (off screen anyway) */
/* hopefully skips unused sprites (seems to work)          */

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < 159*4;offs += 4)
	{
		/*
		 * offs+3:
		 * bit 7 ?
		 * bit 6 ?
		 * bit 5 ?
		 * bit 4 MSB of sprite code
		 * bit 3 vertical flip
		 * bit 2 sprite bank
		 * bit 1 \ color (with bit 2 as well)
		 * bit 0 /
	 	 */
        if (spriteram[offs] != 0)
	        drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x04) ? 1 : 2],
			        0xff - (spriteram[offs + 2] & 0x7f) - 8*(spriteram[offs + 3] & 0x10),
			        (spriteram[offs + 3] & 0x07) + 8*(*popeye_palette_bank & 0x07),
			        spriteram[offs + 2] & 0x80,spriteram[offs + 3] & 0x08,
			        2*(spriteram[offs])-7,2*(256-spriteram[offs + 1]) - 16,
			        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


/* move the sx,sy calculation inside the conditional part */

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;

		if (popeye_videoram[offs] != 0xff)	/* don't draw spaces */
        {
			sx = 16 * (offs % 32);
			sy = 16 * (offs / 32) - 16;

			drawgfx(bitmap,Machine->gfx[0],
					popeye_videoram[offs],popeye_colorram[offs],
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }
	}
}
