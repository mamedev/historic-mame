/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *carnival_videoram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;


int carnival_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void carnival_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void carnival_videoram_w(int offset,int data)
{
	if (carnival_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		carnival_videoram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void carnival_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					carnival_videoram[offs]+44,
0,/*					carnival_colorram[offs],*/
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
