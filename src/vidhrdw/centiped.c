/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *centiped_paletteram;
static int flipscreen;

static struct rectangle spritevisiblearea =
{
	1*8, 31*8-1,
	0*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Actually, Centipede doesn't have a color PROM. Eight RAM locations control
  the color of characters and sprites. The meanings of the four bits are
  (all bits are inverted):

  bit 3 luminance
        blue
        green
  bit 0 red

***************************************************************************/
void centiped_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* characters use colors 4-7 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i + 4;

	/* sprites use colors 12-15 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = i + 12;
}



void centiped_paletteram_w(int offset,int data)
{
	int r,g,b;


	centiped_paletteram[offset] = data;

	if ((~data & 0x08) == 0) /* luminance = 0 */
	{
		r = 0xc0 * ((~data >> 0) & 1);
		g = 0xc0 * ((~data >> 1) & 1);
		b = 0xc0 * ((~data >> 2) & 1);
	}
	else	/* luminance = 1 */
	{
		r = 0xff * ((~data >> 0) & 1);
		g = 0xff * ((~data >> 1) & 1);
		b = 0xff * ((~data >> 2) & 1);
	}

	osd_modify_pen(Machine->pens[offset],r,g,b);
}



void centiped_vh_flipscreen_w (int offset,int data)
{
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void centiped_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + 0x40,
					0,
					flipscreen,flipscreen,
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
		int flipx;


		spritenum = spriteram[offs] & 0x3f;
		if (spritenum & 1) spritenum = spritenum / 2 + 64;
		else spritenum = spritenum / 2;

		flipx = (spriteram[offs] & 0x80) ^ flipscreen;

		/* Centipede is unusual because the sprite color code specifies the */
		/* colors to use one by one, instead of a combination code. */
		/* bit 5-4 = color to use for pen 11 */
		/* bit 3-2 = color to use for pen 10 */
		/* bit 1-0 = color to use for pen 01 */
		/* pen 00 is transparent */
		color = spriteram[offs+0x30];
		Machine->gfx[1]->colortable[3] =
				Machine->pens[12 + ((color >> 4) & 3)];
		Machine->gfx[1]->colortable[2] =
				Machine->pens[12 + ((color >> 2) & 3)];
		Machine->gfx[1]->colortable[1] =
				Machine->pens[12 + ((color >> 0) & 3)];

		drawgfx(bitmap,Machine->gfx[1],
				spritenum,0,
				flipscreen,flipx,
				spriteram[offs + 0x20],240 - spriteram[offs + 0x10],
				&spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
