/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "common.h"
#include "driver.h"
#include "machine.h"
#include "osdepend.h"



#define VIDEO_RAM_SIZE 0x400


unsigned char *bagman_videoram;
unsigned char *bagman_colorram;
unsigned char *bagman_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;
int video_enable;


static struct rectangle visiblearea =
{
	2*8, 30*8-1,
	0*8, 32*8-1
};



int bagman_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	for (i = 0;i < tmpbitmap->height;i++)
		memset(tmpbitmap->line[i],Machine->background_pen,tmpbitmap->width);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void bagman_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void bagman_videoram_w(int offset,int data)
{
	if (bagman_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		bagman_videoram[offset] = data;
	}
}



void bagman_colorram_w(int offset,int data)
{
	if (bagman_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		bagman_colorram[offset] = data;
	}
}



void bagman_video_enable_w(int offset,int data)
{
	video_enable = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bagman_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	if (video_enable == 0)
	{
		for (i = 0;i < bitmap->height;i++)
			memset(bitmap->line[i],Machine->background_pen,bitmap->width);

		return;
	}


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

			drawgfx(tmpbitmap,Machine->gfx[bagman_colorram[offs] & 0x10 ? 1 : 0],
					bagman_videoram[offs] + 8 * (bagman_colorram[offs] & 0x20),
					bagman_colorram[offs] & 0x0f,
					0,0,
					sx,sy,
					&visiblearea,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&visiblearea,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (i = 4*7;i >= 0;i -= 4)
	{
		if (bagman_spriteram[i+2] && bagman_spriteram[i+3])
			drawgfx(bitmap,Machine->gfx[2],
					(bagman_spriteram[i] & 0x3f) + 2 * (bagman_spriteram[i+1] & 0x20),
					bagman_spriteram[i+1] & 0x1f,
					bagman_spriteram[i] & 0x80,bagman_spriteram[i] & 0x40,
					bagman_spriteram[i+2] + 1,bagman_spriteram[i+3] - 1,
					&visiblearea,TRANSPARENCY_PEN,0);
	}
}
