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


static unsigned char videoram[VIDEO_RAM_SIZE];
static unsigned char colorram[VIDEO_RAM_SIZE];
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */


struct sprite
{
	int active;
	int code,bank;
	int color;
	int xflip,yflip;
	int xpos,ypos;
};

static struct sprite sprites[256];



static struct osd_bitmap *tmpbitmap;



static struct rectangle visiblearea =
{
	4*8, 28*8-1,
	1*8, 31*8-1
};



int ladybug_vh_start(void)
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
void ladybug_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



int ladybug_videoram_r(int address,int offset)
{
	return videoram[offset];
}



int ladybug_colorram_r(int address,int offset)
{
	return colorram[offset];
}



void ladybug_videoram_w(int address,int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;
	}
}



void ladybug_colorram_w(int address,int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}



void ladybug_sprite_w(int address,int offset,int data)
{
/*
 aabbcccc ddddddee fffghhhh iiiiiiii

 aa unknown (activation?)
 bb flip
 cccc x offset
 dddddd sprite code
 ee unknown
 fff unknown
 g sprite bank
 hhhh color
 iiiiiiii y position
*/
	switch (offset & 3)
	{
		case 0:
			offset /= 4;
			if (data & 0xc0)
			{
				sprites[offset].active = 1;
				sprites[offset].xpos = (offset & 0xf0) + (data & 0x0f) - 8;
				sprites[offset].xflip = data & 0x10;
				sprites[offset].yflip = data & 0x20;
			}
			else sprites[offset].active = 0;
			break;
		case 1:
			sprites[offset / 4].code = data >> 2;
			break;
		case 2:
			sprites[offset / 4].color = data & 0x0f;
			sprites[offset / 4].bank = (data & 0x10) ? 1 : 0;
			break;
		case 3:
			sprites[offset / 4].ypos = 240 - data;
			break;
	}
}



/***************************************************************************

  Redraw the screen.

***************************************************************************/
void ladybug_vh_screenrefresh(void)
{
	int i,j,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs / 32;
			sy = 31 - offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 32 * (colorram[offs] & 8),
					colorram[offs],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

{
	struct GfxElement mygfx =
	{
		tmpbitmap->width,tmpbitmap->height,
		tmpbitmap,
		1,
		1,0,1
	};

	/* copy the temporary bitmap to the screen */
	drawgfx(Machine->scrbitmap,&mygfx,0,0,0,0,0,0,&visiblearea,TRANSPARENCY_NONE,0);
}

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	/* sprites in the columns 16, 15, 1 and 0 are outside of the visible area */
	for (i = 14;i >= 2;i--)
	{
		j = 0;
		while (sprites[16 * i + j].active) j++;

		while (--j >= 0)
		{
			drawgfx(Machine->scrbitmap,Machine->gfx[1],
					sprites[16 * i + j].code + 64 * sprites[16 * i + j].bank,sprites[16 * i + j].color,
					sprites[16 * i + j].xflip,sprites[16 * i + j].yflip,
					sprites[16 * i + j].xpos,sprites[16 * i + j].ypos,
					&visiblearea,TRANSPARENCY_PEN,0);
		}
	}
}
