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


unsigned char *mrdo_videoram1;
unsigned char *mrdo_colorram1;
unsigned char *mrdo_videoram2;
unsigned char *mrdo_colorram2;
unsigned char *mrdo_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static int scroll_x;


static struct osd_bitmap *tmpbitmap,*tmpbitmap1,*tmpbitmap2;



static struct rectangle visiblearea =
{
	4*8, 28*8-1,
	1*8, 31*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mr. Do! has two 32 bytes palette PROM and a 32 bytes sprite color lookup
  table PROM.
  The palette PROMs are connected to the RGB output this way:

  U2:
  bit 7 -- unused
        -- unused
        -- 100 ohm resistor  -- BLUE
        --  75 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        --  75 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 --  75 ohm resistor  -- RED

  T2:
  bit 7 -- unused
        -- unused
        -- 150 ohm resistor  -- BLUE
        -- 120 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- GREEN
        -- 120 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- RED
  bit 0 -- 120 ohm resistor  -- RED

***************************************************************************/
void mrdo_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i,j;


	for (i = 0;i < 64;i++)
	{
		for (j = 0;j < 4;j++)
		{
			int bit0,bit1,bit2,bit3;


			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 1) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 0) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 1) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 0) & 0x01;
			palette[3*(4*i + j)] = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 3) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 2) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 3) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 2) & 0x01;
			palette[3*(4*i + j) + 1] = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 5) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 4) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 5) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 4) & 0x01;
			palette[3*(4*i + j) + 2] = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
		}
	}

	/* characters with pen 0 = black */
	for (i = 0;i < 4 * 64;i++)
	{
		if (i % 4 == 0) colortable[i] = 0;
		else colortable[i] = i;
	}
	/* characters with colored pen 0 */
	colortable[0 + 4 * 64] = 3;	/* black, but avoid avoid transparency */
	for (i = 1;i < 4 * 64;i++)
		colortable[i + 4 * 64] = i;

	/* sprites */
	for (i = 0;i < 4 * 8;i++)
	{
		int bits1,bits2;


		/* low 4 bits are for sprite n */
		bits1 = (color_prom[i + 64] >> 0) & 3;
		bits2 = (color_prom[i + 64] >> 2) & 3;
		colortable[i + 4 * 128] = bits1 + (bits2 << 2) + (bits2 << 5);

		/* high 4 bits are for sprite n + 8 */
		bits1 = (color_prom[i + 64] >> 4) & 3;
		bits2 = (color_prom[i + 64] >> 6) & 3;
		colortable[i + 4 * 136] = bits1 + (bits2 << 2) + (bits2 << 5);
	}
}



int mrdo_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((tmpbitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		osd_free_bitmap(tmpbitmap1);
		return 1;
	}

	for (i = 0;i < tmpbitmap1->height;i++)
		memset(tmpbitmap1->line[i],Machine->background_pen,tmpbitmap1->width);
	for (i = 0;i < tmpbitmap2->height;i++)
		memset(tmpbitmap2->line[i],Machine->background_pen,tmpbitmap2->width);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mrdo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap1);
	osd_free_bitmap(tmpbitmap2);
}



void mrdo_videoram1_w(int offset,int data)
{
	if (mrdo_videoram1[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_videoram1[offset] = data;
	}
}



void mrdo_colorram1_w(int offset,int data)
{
	if (mrdo_colorram1[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_colorram1[offset] = data;
	}
}



void mrdo_videoram2_w(int offset,int data)
{
	if (mrdo_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_videoram2[offset] = data;
	}
}



void mrdo_colorram2_w(int offset,int data)
{
	if (mrdo_colorram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_colorram2[offset] = data;
	}
}



void mrdo_scrollx_w(int offset,int data)
{
	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - write SCROLLX from mirror address %04x\n",Z80_GetPC(),offset);

	scroll_x = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mrdo_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			/* Mr. Do! has two playfields, one of which can be scrolled. However, */
			/* during gameplay this feature is not used, so I keep the composition */
			/* of the two playfields in a third temporary bitmap, to speed up rendering. */
			drawgfx(tmpbitmap1,Machine->gfx[1],
					mrdo_videoram1[offs] + 2 * (mrdo_colorram1[offs] & 0x80),
					mrdo_colorram1[offs] & 0x7f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap2,Machine->gfx[0],
					mrdo_videoram2[offs] + 2 * (mrdo_colorram2[offs] & 0x80),
					mrdo_colorram2[offs] & 0x7f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);

			drawgfx(tmpbitmap,Machine->gfx[1],
					mrdo_videoram1[offs] + 2 * (mrdo_colorram1[offs] & 0x80),
					mrdo_colorram1[offs] & 0x7f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap,Machine->gfx[0],
					mrdo_videoram2[offs] + 2 * (mrdo_colorram2[offs] & 0x80),
					mrdo_colorram2[offs] & 0x7f,
					0,0,sx,sy,
					0,(mrdo_colorram2[offs] & 0x40) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN,0);
		}
	}


	/* copy the character mapped graphics */
	if (scroll_x)
	{
		copybitmap(bitmap,tmpbitmap1,0,0,256-scroll_x,0,&visiblearea,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap1,0,0,-scroll_x,0,&visiblearea,TRANSPARENCY_NONE,0);

		copybitmap(bitmap,tmpbitmap2,0,0,0,0,&visiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}
	else
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&visiblearea,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (i = 4 * 63;i >= 0;i -= 4)
	{
		if (mrdo_spriteram[i + 1] != 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					mrdo_spriteram[i],mrdo_spriteram[i + 2] & 0x0f,
					mrdo_spriteram[i + 2] & 0x20,mrdo_spriteram[i + 2] & 0x10,
					256 - mrdo_spriteram[i + 1],240 - mrdo_spriteram[i + 3],
					&visiblearea,TRANSPARENCY_PEN, 0);
		}
	}
}
