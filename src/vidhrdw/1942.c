/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400
#define BACKGROUND_SIZE 0x200

unsigned char *c1942_backgroundram;
unsigned char *c1942_scroll;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int c1942_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(BACKGROUND_SIZE)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer,0,BACKGROUND_SIZE);

	/* the background area is twice as tall as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_width)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void c1942_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void c1942_background_w(int offset,int data)
{
	if (c1942_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		c1942_backgroundram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void c1942_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,sx,sy;


	for (sy = 0;sy < 32;sy++)
	{
		for (sx = 0;sx < 16;sx++)
		{
			offs = 32 * (31 - sy) + sx;

			if (dirtybuffer2[offs] != 0 || dirtybuffer2[offs + 16] != 0)
			{
				dirtybuffer2[offs] = dirtybuffer2[offs + 16] = 0;

				drawgfx(tmpbitmap2,Machine->gfx[(c1942_backgroundram[offs + 16] & 0x80) ? 2 : 1],
						c1942_backgroundram[offs],
						c1942_backgroundram[offs + 16] & 0x1f,
						c1942_backgroundram[offs + 16] & 0x40,!(c1942_backgroundram[offs + 16] & 0x20),
						16 * sx,16 * sy,
						0,TRANSPARENCY_NONE,0);
			}
		}
	}


	/* copy the background graphics */
	{
		int scroll;


		scroll = c1942_scroll[0] + 256 * c1942_scroll[1] - 256;

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,1,&scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 31*4;offs >= 0;offs -= 4)
	{
		int bank,i,code,col,sx,sy;


		bank = 3;
		if (spriteram[offs] & 0x80) bank++;
		if (spriteram[offs + 1] & 0x20) bank += 2;

		code = spriteram[offs] & 0x7f;
		col = spriteram[offs + 1] & 0x0f;
		sx = spriteram[offs + 2];
		sy = 240 - spriteram[offs + 3] + 0x10 * (spriteram[offs + 1] & 0x10);

		i = (spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

		do
		{
			drawgfx(bitmap,Machine->gfx[bank],
					code + i,col,
					0, 0,
					sx + 16 * i,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,15);

			i--;
		} while (i >= 0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;


		sx = 8 * (offs / 32);
		sy = 8 * (31 - offs % 32);

		if (videoram[offs] != 0x30)	/* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x7f,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
