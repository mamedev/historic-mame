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
#define BIGSPRITE_SIZE 0x100
#define BIGSPRITE_WIDTH 128
#define BIGSPRITE_HEIGHT 128


unsigned char *cclimber_videoram;
unsigned char *cclimber_colorram;
unsigned char *cclimber_bsvideoram;
unsigned char *cclimber_spriteram;
unsigned char *cclimber_bigspriteram;
unsigned char *cclimber_column_scroll;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static unsigned char bsdirtybuffer[BIGSPRITE_SIZE];

static struct osd_bitmap *tmpbitmap,*bsbitmap;


static struct rectangle visiblearea =
{
	0*8, 32*8-1,
	2*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Crazy Climber has three 32 bytes palette PROM.
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 4 * 24;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 4 * 24;i++)
	{
		if ((i & 3) == 0) colortable[i] = 0;
		else colortable[i] = i;
	}
}



int cclimber_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((bsbitmap = osd_create_bitmap(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	for (i = 0;i < tmpbitmap->height;i++)
		memset(tmpbitmap->line[i],Machine->background_pen,tmpbitmap->width);

	for (i = 0;i < bsbitmap->height;i++)
		memset(bsbitmap->line[i],Machine->background_pen,bsbitmap->width);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cclimber_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(bsbitmap);
}



void cclimber_videoram_w(int offset,int data)
{
	if (cclimber_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		cclimber_videoram[offset] = data;
	}
}



void cclimber_colorram_w(int offset,int data)
{
	if (cclimber_colorram[offset] != data)
	{
		/* bit 5 of the address is not used for color memory. There is just */
		/* 512 bytes of memory; every two consecutive rows share the same memory */
		/* region. */
		offset &= 0xffdf;

		dirtybuffer[offset] = 1;
		dirtybuffer[offset + 0x20] = 1;

		cclimber_colorram[offset] = data;
		cclimber_colorram[offset + 0x20] = data;
	}
}



void cclimber_bigsprite_videoram_w(int offset,int data)
{
	if (cclimber_bsvideoram[offset] != data)
	{
		bsdirtybuffer[offset] = 1;

		cclimber_bsvideoram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[(cclimber_colorram[offs] & 0x10) ? 1 : 0],
					cclimber_videoram[offs],
					cclimber_colorram[offs] & 0x0f,
					cclimber_colorram[offs] & 0x40,cclimber_colorram[offs] & 0x80,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_y = visiblearea.min_y;
		clip.max_y = visiblearea.max_y;

		i = 0;
		while (i < 8 * 32)
		{
			int cons,scroll;


			/* count consecutive columns scrolled by the same amount */
			scroll = cclimber_column_scroll[i / 8];
			cons = 8;
			while (i + cons < 8 * 32 &&	cclimber_column_scroll[(i + cons) / 8] == scroll)
				cons += 8;

			clip.min_x = i;
			clip.max_x = i + cons - 1;
			copybitmap(bitmap,tmpbitmap,0,0,0,-scroll,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap,0,0,0,256 - scroll,&clip,TRANSPARENCY_NONE,0);

			i += cons;
		}
	}


	/* draw the "big sprite" */
	{
		int newcol;
		static int lastcol;


		newcol = cclimber_bigspriteram[1] & 0x07;

		/* first of all, update it. */
		for (offs = 0;offs < BIGSPRITE_SIZE;offs++)
		{
			int sx,sy;


			if (bsdirtybuffer[offs] || newcol != lastcol)
			{
				bsdirtybuffer[offs] = 0;

				sx = 8 * (offs % 16);
				sy = 8 * (offs / 16);

				drawgfx(bsbitmap,Machine->gfx[2],
						cclimber_bsvideoram[offs],newcol,
						0,0,sx,sy,
						0,TRANSPARENCY_NONE,0);
			}

		}

		lastcol = newcol;

		copybitmap(bitmap,bsbitmap,
				cclimber_bigspriteram[1] & 0x10,cclimber_bigspriteram[1] & 0x20,
				136 - cclimber_bigspriteram[3],128 - cclimber_bigspriteram[2],
				&visiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}

	/* draw sprites (must be done after the "big sprite" to obtain the correct priority) */
	for (i = 0;i < 8*4;i += 4)
	{
		drawgfx(bitmap,Machine->gfx[cclimber_spriteram[i + 1] & 0x10 ? 4 : 3],
				cclimber_spriteram[i] & 0x3f,cclimber_spriteram[i + 1] & 0x0f,
				cclimber_spriteram[i] & 0x40,cclimber_spriteram[i] & 0x80,
				cclimber_spriteram[i + 3],240 - cclimber_spriteram[i + 2],
				&visiblearea,TRANSPARENCY_PEN,0);
	}
}
