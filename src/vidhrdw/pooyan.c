/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *pooyan_videoram;
unsigned char *pooyan_colorram;
unsigned char *pooyan_spriteram1;
unsigned char *pooyan_spriteram2;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;



int pooyan_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void pooyan_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void pooyan_videoram_w(int offset,int data)
{
	if (pooyan_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		pooyan_videoram[offset] = data;
	}
}



void pooyan_colorram_w(int offset,int data)
{
	if (pooyan_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		pooyan_colorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void pooyan_vh_screenrefresh(struct osd_bitmap *bitmap)
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
					pooyan_videoram[offs],
					pooyan_colorram[offs] & 0x3f,
					pooyan_colorram[offs] & 0x80,pooyan_colorram[offs] & 0x40,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < 24*2;offs += 2)
	{
		drawgfx(bitmap,Machine->gfx[1],
				pooyan_spriteram1[offs + 1] & 0x3f,
				pooyan_spriteram2[offs] & 0x3f,
				pooyan_spriteram2[offs] & 0x80,!(pooyan_spriteram2[offs] & 0x40),
				pooyan_spriteram2[offs + 1],pooyan_spriteram1[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
