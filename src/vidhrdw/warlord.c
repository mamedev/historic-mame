/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg
  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



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
void warlord_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

	colortable[0]=0;
	colortable[1]=1;
	colortable[2]=2;
	colortable[3]=3;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void warlord_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sy = (offs / 32);
			sx = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] & 0x3f , 0 ,
					videoram [ offs ] & 0x40 , videoram [ offs ] & 0x80 ,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


        /* For standup mode use the following line :-
	copybitmap(bitmap,tmpbitmap,1,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        */

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int spritenum  /*,color*/ ;

		spritenum = ( spriteram[offs] & 0x3f ) + 0x40 ;

                /*
		color = spriteram[offs+0x30];
		Machine->gfx[0]->colortable[3] =
				Machine->pens[15 - centiped_spritepalette[(color >> 4) & 3]];
		Machine->gfx[0]->colortable[2] =
				Machine->pens[15 - centiped_spritepalette[(color >> 2) & 3]];
		Machine->gfx[0]->colortable[1] =
				Machine->pens[15 - centiped_spritepalette[(color >> 0) & 3]];
                */
		drawgfx(bitmap,Machine->gfx[0],
				spritenum,0,
				spriteram [ offs ] & 0x40 , spriteram [ offs ] & 0x80 ,
				spriteram[offs + 0x20],248 - spriteram[offs + 0x10],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
