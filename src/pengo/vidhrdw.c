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
static int gfx_bank;


struct sprite
{
	int code;
	int color;
	int xflip,yflip;
	int xpos,ypos;
};

static struct sprite sprites[8];



static struct osd_bitmap *tmpbitmap;



static struct rectangle spritevisiblearea =
{
	0, 28*8-1,
	2*8, 34*8-1
};



int pengo_vh_start(void)
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
void pengo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



int pengo_videoram_r(int address,int offset)
{
	return videoram[offset];
}



int pengo_colorram_r(int address,int offset)
{
	return colorram[offset];
}



void pengo_videoram_w(int address,int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;
	}
}



void pengo_colorram_w(int address,int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}



void pengo_spritecode_w(int address,int offset,int data)
{
	if (offset & 1)
		sprites[offset / 2].color = data;
	else
	{
		offset /= 2;
		sprites[offset].code = data >> 2;
		sprites[offset].xflip = data & 2;
		sprites[offset].yflip = data & 1;
	}
}



void pengo_spritepos_w(int address,int offset,int data)
{
	if (offset & 1)
		sprites[offset / 2].ypos = 272 - data;
	else
		sprites[offset / 2].xpos = 239 - data;
}



void pengo_gfxbank_w(int address,int offset,int data)
{
	/* the Pengo hardware can set independently the palette bank, color lookup */
	/* table, and chars/sprites. However the game always set them together (and */
	/* the only place where this is used is the intro screen) so I don't bother */
	/* emulating the whole thing. */
	gfx_bank = data;
}



/***************************************************************************

  Redraw the screen.

***************************************************************************/
void pengo_vh_screenrefresh(void)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;


			dirtybuffer[offs] = 0;

	/* Even if Pengo's screen is 28x36, the memory layout is 32x32. We therefore */
	/* have to convert the memory coordinates into screen coordinates. */
	/* Note that 32*32 = 1024, while 28*36 = 1008: therefore 16 bytes of Video RAM */
	/* don't map to a screen position. We don't check that here, however: range */
	/* checking is performed by drawgfx(). */

			mx = offs / 32;
			my = offs % 32;

			if (mx <= 1)
			{
				sx = 29 - my;
				sy = mx + 34;
			}
			else if (mx >= 30)
			{
				sx = 29 - my;
				sy = mx - 30;
			}
			else
			{
				sx = 29 - mx;
				sy = my + 2;
			}

			drawgfx(tmpbitmap,Machine->gfx[gfx_bank ? 2 : 0],
					videoram[offs],
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
	drawgfx(Machine->scrbitmap,&mygfx,0,0,0,0,0,0,0,TRANSPARENCY_NONE,0);
}

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	/* sprites #0 and #7 are not used */
	for (i = 6;i > 0;i--)
	{
		drawgfx(Machine->scrbitmap,Machine->gfx[gfx_bank ? 3 : 1],
				sprites[i].code,sprites[i].color,
				sprites[i].xflip,sprites[i].yflip,
				sprites[i].xpos,sprites[i].ypos,
				&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}
}
