/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *zaxxon_videoram;
unsigned char *zaxxon_colorram;
unsigned char *zaxxon_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct osd_bitmap *tmpbitmap;



int zaxxon_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void zaxxon_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void zaxxon_videoram_w(int offset,int data)
{
	if (zaxxon_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		zaxxon_videoram[offset] = data;
	}
}



void zaxxon_colorram_w(int offset,int data)
{
	if (zaxxon_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		zaxxon_colorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


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
					zaxxon_videoram[offs],
0,//					zaxxon_colorram[offs],
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (i = 4*63;i >= 0;i -= 4)
	{
		if (zaxxon_spriteram[i])
		{
			drawgfx(bitmap,Machine->gfx[2],
					zaxxon_spriteram[i+1] & 0x7f,zaxxon_spriteram[i+2] & 0x7f,
					zaxxon_spriteram[i+1] & 0x80,zaxxon_spriteram[i+2] & 0x80,
					zaxxon_spriteram[i] - 7,zaxxon_spriteram[i+3] - 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
