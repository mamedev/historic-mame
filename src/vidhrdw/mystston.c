/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *mystston_videoram2,*mystston_colorram2;
unsigned char *mystston_scroll;
unsigned char *mystston_paletteram;

#define VIDEO_RAM_SIZE 0x400
#define BACKGROUND_RAM_SIZE 0x200



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Actually Mysterious Stones uses RAM, not PROMs to store the palette.
  I don't know for sure how the palette RAM is connected to the RGB output,
  but it's probably the usual:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void mystston_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
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



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int mystston_vh_start(void)
{
	if ((dirtybuffer = malloc(BACKGROUND_RAM_SIZE)) == 0)
		return 1;
	memset(dirtybuffer,0,BACKGROUND_RAM_SIZE);

	/* Mysterious Stones has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mystston_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



void mystston_videoram2_w(int offset,int data)
{
	if (mystston_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mystston_videoram2[offset] = data;
	}
}



void mystston_colorram2_w(int offset,int data)
{
	if (mystston_colorram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mystston_colorram2[offset] = data;
	}
}



void mystston_paletteram_w(int offset,int data)
{
	if (mystston_paletteram[offset] != data)
	{
		if (offset > 16) memset(dirtybuffer,1,BACKGROUND_RAM_SIZE);

		mystston_paletteram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mystston_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* rebuild the color lookup table */
	for (offs = 0;offs < 8;offs++)
	{
		int col;


		col = Machine->pens[mystston_paletteram[offs]];
		Machine->gfx[2]->colortable[offs] = col;
	}
	for (offs = 0;offs < 8;offs++)
	{
		int col;


		col = Machine->pens[mystston_paletteram[offs + 8]];
		Machine->gfx[0]->colortable[offs] = col;
	}
	for (offs = 0;offs < 8;offs++)
	{
		int col;


		col = Machine->pens[mystston_paletteram[offs + 2*8]];
		Machine->gfx[4]->colortable[offs] = col;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < BACKGROUND_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 16 * (offs % 32);
			sy = 16 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[4 + (mystston_colorram2[offs] & 0x01)],
					mystston_videoram2[offs],
					0,
					sx >= 256,0,	/* flip horizontally tiles on the right half of the bitmap */
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx;


		scrollx = -*mystston_scroll;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < 24*4;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
	/* the meaning of bit 4 of spriteram[offs] is unknown */
			drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x10) ? 3 : 2],
					spriteram[offs+1],
					0,
					spriteram[offs] & 0x02,0,
					(240 - spriteram[offs+2]) & 0xff,spriteram[offs+3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if ((colorram[offs] & 0x07) != 0x07 || videoram[offs] != 0x40)	/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(bitmap,Machine->gfx[(colorram[offs] & 0x04) ? 1 : 0],
					videoram[offs] + 256 * (colorram[offs] & 0x03),
					0,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
