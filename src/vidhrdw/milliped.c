/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *milliped_paletteram;

static struct rectangle spritevisiblearea =
{
	1*8, 31*8-1,
	0*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Actually, Millipede doesn't have a color PROM, it uses RAM.
  The RAM seems to be conncted to the video output this way:

  bit 7 red
        red
        red
        green
        green
        blue
        blue
  bit 0 blue

***************************************************************************/
void milliped_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* initialize the color table */
	for (i = 0;i < Machine->drv->color_table_len;i++)
		colortable[i] = i;
}



void milliped_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2;
	int r,g,b;


	milliped_paletteram[offset] = data;

	/* red component */
	bit0 = (~data >> 5) & 0x01;
	bit1 = (~data >> 6) & 0x01;
	bit2 = (~data >> 7) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* green component */
	bit0 = 0;
	bit1 = (~data >> 3) & 0x01;
	bit2 = (~data >> 4) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* blue component */
	bit0 = (~data >> 0) & 0x01;
	bit1 = (~data >> 1) & 0x01;
	bit2 = (~data >> 2) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	osd_modify_pen(Machine->pens[offset],r,g,b);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void milliped_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int bank;
			int color;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			if (videoram[offs] & 0x40)
				bank = 1;
			else bank = 0;

			if (videoram[offs] & 0x80)
				color = 2;
			else color = 0;

			drawgfx(tmpbitmap,Machine->gfx[0],
					0x40 + (videoram[offs] & 0x3f) + 0x80 * bank,
					bank + color,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int spritenum,color;


		spritenum = spriteram[offs] & 0x3f;
		if (spritenum & 1) spritenum = spritenum / 2 + 64;
		else spritenum = spritenum / 2;

		/* Millipede is unusual because the sprite color code specifies the */
		/* colors to use one by one, instead of a combination code. */
		/* bit 7-6 = palette bank (there are 4 groups of 4 colors) */
		/* bit 5-4 = color to use for pen 11 */
		/* bit 3-2 = color to use for pen 10 */
		/* bit 1-0 = color to use for pen 01 */
		/* pen 00 is transparent */
		color = spriteram[offs + 0x30];
		Machine->gfx[1]->colortable[3] =
				Machine->pens[16+((color >> 4) & 3)+4*((color >> 6) & 3)];
		Machine->gfx[1]->colortable[2] =
				Machine->pens[16+((color >> 2) & 3)+4*((color >> 6) & 3)];
		Machine->gfx[1]->colortable[1] =
				Machine->pens[16+((color >> 0) & 3)+4*((color >> 6) & 3)];

		drawgfx(bitmap,Machine->gfx[1],
				spritenum,
				0,
				0,spriteram[offs] & 0x80,
				spriteram[offs + 0x20],240 - spriteram[offs + 0x10],
				&spritevisiblearea,TRANSPARENCY_PEN,0);

	}
}
