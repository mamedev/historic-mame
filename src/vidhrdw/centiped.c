/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *centiped_charpalette,*centiped_spritepalette;



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


	for (i = 0;i < 16; i++)
	{
		if ((i & 0x08) == 0) /* luminance = 0 */
		{
			palette[3*i] = 0xc0 * (i & 1);
			palette[3*i + 1] = 0xc0 * ((i >> 1) & 1);
			palette[3*i + 2] = 0xc0 * ((i >> 2) & 1);
		}
		else	/* luminance = 1 */
		{
			palette[3*i] = 0xff * (i & 1);
			palette[3*i + 1] = 0xff * ((i >> 1) & 1);
			palette[3*i + 2] = 0xff * ((i >> 2) & 1);
		}
	}
}



void centiped_vh_charpalette_w(int offset, int data)
{
	centiped_charpalette[offset] = data;
	Machine->gfx[0]->colortable[offset] = Machine->pens[15 - data];

	memset(dirtybuffer,1,videoram_size);
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

			sx = (offs / 32);
			sy = (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + 0x40,0,
					0,0,
					8*(sx+1),8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		if (spriteram[offs + 0x20] < 0xf8)
		{
			int spritenum,color;


			spritenum = spriteram[offs] & 0x3f;
			if (spritenum & 1) spritenum = spritenum / 2 + 64;
			else spritenum = spritenum / 2;

			color = spriteram[offs+0x30];
			Machine->gfx[1]->colortable[3] =
					Machine->pens[15 - centiped_spritepalette[(color >> 4) & 3]];
			Machine->gfx[1]->colortable[2] =
					Machine->pens[15 - centiped_spritepalette[(color >> 2) & 3]];
			Machine->gfx[1]->colortable[1] =
					Machine->pens[15 - centiped_spritepalette[(color >> 0) & 3]];
			drawgfx(bitmap,Machine->gfx[1],
					spritenum,0,
					spriteram[offs] & 0x80,0,
					248 - spriteram[offs + 0x10],248 - spriteram[offs + 0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
