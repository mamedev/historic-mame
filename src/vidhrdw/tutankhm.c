/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/


/*  Stuff that work only in MS DOS (Color cycling)
 */

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char tutankhm_paletteram[16];
unsigned char *tutankhm_scrollx;
static int flipscreen[2];



/***************************************************************************

  Tutankhm doesn't have a color PROM, it uses RAM palette registers.

***************************************************************************/
void tutankhm_palette_w(int offset,int data)
{
	int r, g, b;
	int bit0,bit1,bit2;


	tutankhm_paletteram[offset] = data;

	/* red component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	osd_modify_pen(Machine->pens[offset], r, g, b);
}



static void videowrite(int offset,int data)
{
	unsigned char x1,x2,y1,y2;


	x1 = ( offset & 0x7f ) << 1;
	y1 = ( offset >> 7 );
	x2 = x1 + 1;
	y2 = y1;

	if (flipscreen[0])
	{
		x1 = 255 - x1;
		x2 = 255 - x2;
	}
	if (flipscreen[1])
	{
		y1 = 255 - y1;
		y2 = 255 - y2;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;


		temp = x1;
		x1 = y1;
		y1 = temp;
		temp = x2;
		x2 = y2;
		y2 = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		x1 = 255 - x1;
		x2 = 255 - x2;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		y1 = 255 - y1;
		y2 = 255 - y2;
	}

	tmpbitmap->line[y1][x1] = Machine->pens[data & 0x0f];
	tmpbitmap->line[y2][x2] = Machine->pens[data >> 4];
}



void tutankhm_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		videowrite(offset,data);
	}
}



void tutankhm_flipscreen_w(int offset,int data)
{
	if (flipscreen[offset] != (data & 1))
	{
		int offs;


		flipscreen[offset] = data & 1;
		/* refresh the display */
		for (offs = 0;offs < videoram_size;offs++)
			videowrite(offs,videoram[offs]);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void tutankhm_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the temporary bitmap to the screen */
	{
		int scroll[32], i;


		if (flipscreen[0])
		{
			for (i = 0;i < 8;i++)
				scroll[i] = 0;
			for (i = 8;i < 32;i++)
			{
				scroll[i] = -*tutankhm_scrollx;
				if (flipscreen[1]) scroll[i] = -scroll[i];
			}
		}
		else
		{
			for (i = 0;i < 24;i++)
			{
				scroll[i] = -*tutankhm_scrollx;
				if (flipscreen[1]) scroll[i] = -scroll[i];
			}
			for (i = 24;i < 32;i++)
				scroll[i] = 0;
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}
