/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400

unsigned char *centiped_videoram;
unsigned char *centiped_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;



int centiped_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void centiped_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void centiped_videoram_w(int offset,int data)
{
	if (centiped_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		centiped_videoram[offset] = data;
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


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = (offs / 32);
			sy = (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					centiped_videoram[offs] + 0x40,
0,
					0,0,8*(sx+1),8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		if (centiped_spriteram[offs + 0x20] < 0xf8)
		{
			int spritenum;


			spritenum = centiped_spriteram[offs] & 0x3f;
			if (spritenum & 1) spritenum = spritenum / 2 + 64;
			else spritenum = spritenum / 2;

			drawgfx(bitmap,Machine->gfx[1],
					spritenum,
0,/*					centiped_spriteram[offs + 0x30],*/
					centiped_spriteram[offs] & 0x80,0,
					248 - centiped_spriteram[offs + 0x10],248 - centiped_spriteram[offs + 0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
