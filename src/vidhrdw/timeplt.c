/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400



static struct rectangle spritevisiblearea =
{
	2*8, 30*8-1,
	4*8, 31*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Pilot has two 32x8 palette PROMs and two 256x4 lookup table PROMs
  (one for characters, one for sprites).
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 390 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 560 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 1.2kohm resistor  -- BLUE
        -- 390 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
  bit 0 -- 560 ohm resistor  -- GREEN

  bit 7 -- 820 ohm resistor  -- GREEN
        -- 1.2kohm resistor  -- GREEN
        -- 390 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 560 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
        -- 1.2kohm resistor  -- RED
  bit 0 -- not connected

***************************************************************************/
void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,bit3,bit4;


		bit0 = (color_prom[i+32] >> 1) & 0x01;
		bit1 = (color_prom[i+32] >> 2) & 0x01;
		bit2 = (color_prom[i+32] >> 3) & 0x01;
		bit3 = (color_prom[i+32] >> 4) & 0x01;
		bit4 = (color_prom[i+32] >> 5) & 0x01;
		palette[3*i] = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[i+32] >> 6) & 0x01;
		bit1 = (color_prom[i+32] >> 7) & 0x01;
		bit2 = (color_prom[i] >> 0) & 0x01;
		bit3 = (color_prom[i] >> 1) & 0x01;
		bit4 = (color_prom[i] >> 2) & 0x01;
		palette[3*i + 1] = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		bit3 = (color_prom[i] >> 6) & 0x01;
		bit4 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
	}


	/* characters */
	colortable[0] = 0x10;	/* according to the PROM this should be white, but it isn't */
	for (i = 1;i < 32*4;i++)
		colortable[i] = (color_prom[i + 64] & 0x0f) + 0x10;

	/* sprites */
	for (i = 32*4;i < 32*4+64*4;i++)
		colortable[i] = color_prom[i + 64] & 0x0f;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x1f,
					colorram[offs] & 0x80,colorram[offs] & 0x40,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* In Time Pilot, the characters can appear either behind or in front of the */
	/* sprites. The priority is selected by bit 4 of the color attribute of the */
	/* character. This feature is used to limit the sprite visibility area, and */
	/* as a sort of copyright notice protection ("KONAMI" on the title screen */
	/* alternates between characters and sprites, but they are both white so you */
	/* can't see it). To speed up video refresh, we clip the sprites for ourselves. */

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 23*2;offs >= 0;offs -= 2)
	{
		/* handle double width sprites (clouds) */
		if (offs <= 2*2 || offs >= 19*2)
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs + 1],
					spriteram_2[offs] & 0x3f,
					spriteram_2[offs] & 0x80,!(spriteram_2[offs] & 0x40),
					2 * spriteram_2[offs + 1] - 16,spriteram[offs],
					&spritevisiblearea,TRANSPARENCY_PEN,0);
		else
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 1],
					spriteram_2[offs] & 0x3f,
					spriteram_2[offs] & 0x80,!(spriteram_2[offs] & 0x40),
					spriteram_2[offs + 1] - 1,spriteram[offs],
					&spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
