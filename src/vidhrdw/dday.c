/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *dday_videoram2;
unsigned char *dday_videoram3;
unsigned char *dday_video_control;

/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void dday_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = color_prom[0*Machine->drv->total_colors];
		*(palette++) = color_prom[1*Machine->drv->total_colors];
		*(palette++) = color_prom[2*Machine->drv->total_colors];

		color_prom++;
	}

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		colortable[i] = i;
	}
}


void dday_colorram_w(int offset, int data)
{
    colorram[offset & 0x3e0] = data;
}

int dday_colorram_r(int offset)
{
    return colorram[offset & 0x3e0];
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dday_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the backround RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sy = 8 * (offs / 32);
			sx = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					charcode,
					0,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Copy foreground planes. They are characters, but draw them as sprites
	   Don't know about priorities. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx, sy, attrib, flipx;

		sy = offs / 32;
		sx = 31 - offs % 32;

		drawgfx(bitmap,Machine->gfx[0],
			dday_videoram2[offs],
			0,
			0,0,
			8*sx,8*sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		// Only know what Bit 0 is. (And also Bit 2, but I don't think
		// the hardware uses that)
		attrib = colorram[offs & 0x3e0];
		flipx  = attrib & 0x01;

		if (flipx) sx = 31 - sx;

		drawgfx(bitmap,Machine->gfx[2],
			dday_videoram3[offs],
			0,
			flipx,0,
			8*sx,8*sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

    // Uncomment if you want the video control register displayed
    /*{
	char buf[80];
	int l,i;
	sprintf(buf, "7800=%02X", *dday_video_control);

	l = strlen(buf);
	for (i = 0;i < l;i++)
	    drawgfx(Machine->scrbitmap,Machine->uifont,buf[l-i-1],DT_COLOR_WHITE,0,0,Machine->scrbitmap->width-(l-i)*Machine->uifont->width,20,0,TRANSPARENCY_NONE,0);
    }*/
}
