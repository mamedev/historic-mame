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


unsigned char *dkong_videoram;
unsigned char *dkong_colorram;
unsigned char *dkong_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;


static struct rectangle visiblearea =
{
	2*8, 30*8-1,
	0*8, 32*8-1
};



int dkong_vh_start(void)
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
void dkong_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void dkong_videoram_w(int offset,int data)
{
	if (dkong_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		dkong_videoram[offset] = data;
	}
}



void dkong_colorram_w(int offset,int data)
{
	if (dkong_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		dkong_colorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dkong_vh_screenrefresh(struct osd_bitmap *bitmap)
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
					dkong_videoram[offs],
0,//					dkong_colorram[offs],
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
{
	struct GfxElement mygfx =
	{
		tmpbitmap->width,tmpbitmap->height,
		tmpbitmap,
		1,
		1,0,1
	};

	/* copy the temporary bitmap to the screen */
	drawgfx(bitmap,&mygfx,0,0,0,0,0,0,&visiblearea,TRANSPARENCY_NONE,0);
}

	/* Draw the sprites. */
	for (i = 0;i < 4*96;i += 4)
	{
		if (dkong_spriteram[i])
		{
			drawgfx(bitmap,Machine->gfx[1],
					dkong_spriteram[i+1] & 0x7f,dkong_spriteram[i+2] & 0x7f,
					dkong_spriteram[i+1] & 0x80,dkong_spriteram[i+2] & 0x80,
					dkong_spriteram[i] - 7,dkong_spriteram[i+3] - 8,
					&visiblearea,TRANSPARENCY_PEN,0);
		}
	}
}
