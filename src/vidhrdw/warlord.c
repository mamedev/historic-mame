/***************************************************************************
Warlords Driver by Lee Taylor and John Clegg
  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/***************************************************************************

  Convert the color PROM into a more useable format.

  The palette PROM are connected to the RGB output this way:

  bit 2 -- RED
        -- GREEN
  bit 0 -- BLUE

***************************************************************************/
void warlord_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i, j;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((*color_prom >> 2) & 0x01) * 0xff;
		*(palette++) = ((*color_prom >> 1) & 0x01) * 0xff;
		*(palette++) = ((*color_prom >> 0) & 0x01) * 0xff;

		color_prom++;
	}

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			COLOR(0,i*4+j) = i*16+j;
			COLOR(1,i*4+j) = i*16+j*4;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void warlord_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,color;

			dirtybuffer[offs] = 0;

			sy = (offs / 32);
			sx = (offs % 32);

			/* The four quadrants have different colors */
			color = ((sy & 0x10) >> 3) | ((sx & 0x10) >> 4);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram [offs] & 0x3f,
					color,
					videoram [offs] & 0x40, videoram [offs] & 0x80 ,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int sx, sy, spritenum, color;

		sx = spriteram [offs + 0x20];
        sy = 248 - spriteram[offs + 0x10];

		spritenum = (spriteram[offs] & 0x3f);

		/* LBO - borrowed this from Centipede. It really adds a psychedelic touch. */
		/* Warlords is unusual because the sprite color code specifies the */
		/* colors to use one by one, instead of a combination code. */
		/* bit 5-4 = color to use for pen 11 */
		/* bit 3-2 = color to use for pen 10 */
		/* bit 1-0 = color to use for pen 01 */
		/* pen 00 is transparent */
		color = spriteram[offs+0x30];

		/* The four quadrants have different colors. This is not 100% accurate,
		   because right on the middle the sprite could actually have two or more
		   different color, but this is not noticable, as the color that
		   changes between the quadrants is mostly used on the paddle sprites */
		color = ((sy & 0x80) >> 6) | ((sx & 0x80) >> 7);

		drawgfx(bitmap,Machine->gfx[1],
				spritenum, color,
				spriteram [offs] & 0x40, spriteram [offs] & 0x80 ,
				sx, sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
