/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void snowbros_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x=0,y=0,offs;


	/*
	 * Sprite Tile Format
	 * ------------------
	 *
	 * Byte(s) | Bit(s)   | Use
	 * --------+-76543210-+----------------
	 *  0-5	| -------- | ?
	 *	6	| -------- | ?
	 *	7	| xxxx.... | Palette Bank
	 *	7	| .......x | XPos - Sign Bit
	 *	9	| xxxxxxxx | XPos
	 *	7	| ......x. | YPos - Sign Bit
	 *	B	| xxxxxxxx | YPos
	 *	7	| .....x.. | Use Relative offsets
	 *	C	| -------- | ?
	 *	D	| xxxxxxxx | Sprite Number (low 8 bits)
	 *	E	| -------- | ?
	 *	F	| ....xxxx | Sprite Number (high 4 bits)
	 *	F	| x....... | Flip Sprite Y-Axis
	 *	F	| .x...... | Flip Sprite X-Axis
	 */

	/* This clears & redraws the entire screen each pass */

  	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		int sx = spriteram16[offs+4] & 0xff;
		int sy = spriteram16[offs+5] & 0xff;
		int tilecolour = spriteram16[offs+3];

		if (tilecolour & 1) sx = -1 - (sx ^ 0xff);

		if (tilecolour & 2) sy = -1 - (sy ^ 0xff);

		if (tilecolour & 4)
		{
			x += sx;
			y += sy;
		}
		else
		{
			x = sx;
			y = sy;
		}

		if (x > 511) x &= 0x1ff;
		if (y > 511) y &= 0x1ff;

		if ((x>-16) && (y>0) && (x<256) && (y<240))
		{
			int attr = spriteram16[offs+7];
			int tile = ((attr & 0x0f) << 8) + (spriteram16[offs+6] & 0xff);

			drawgfx(bitmap,Machine->gfx[0],
					tile,
					(tilecolour & 0xf0) >> 4,
					attr & 0x80, attr & 0x40,
					x,y,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

void wintbob_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		int xpos  = spriteram16[offs] & 0xff;
		int ypos  = spriteram16[offs+4] & 0xff;
/*		int unk1  = spriteram16[offs+1] & 0x01;*/  /* Unknown .. Set for the Bottom Left part of Sprites */
		int disbl = spriteram16[offs+1] & 0x02;
/*		int unk2  = spriteram16[offs+1] & 0x04;*/  /* Unknown .. Set for most things */
		int wrapr = spriteram16[offs+1] & 0x08;
		int colr  = (spriteram16[offs+1] & 0xf0) >> 4;
		int tilen = (spriteram16[offs+2] << 8) + (spriteram16[offs+3] & 0xff);
		int flipy = spriteram16[offs+2] & 0x80;
		int flipx = spriteram16[offs+2] & 0x40;

		if (wrapr == 8) xpos -= 256;

		if ((xpos > -16) && (ypos > 0) && (xpos < 256) && (ypos < 240) && (disbl !=2))
		{
			drawgfx(bitmap,Machine->gfx[0],
					tilen,
					colr,
					flipy, flipx,
					xpos,ypos,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
