/***************************************************************************

  z80bw.c

  Functions to emulate the video hardware of the machine.

modified 02-06-98 HJB copied from 8080bw.c and changed to plot_8_pixel

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int flipscreen;
static int screen_flipped;
static int screen_red;
static int palette_modified;


void z80bw_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	screen_red = 0;
	palette_modified = 1;
}


static void set_palette(void)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r,g,b;

		if (screen_red)
		{
			r = 0xff;
			g = 0x00;
			b = 0x00;
		}
		else
		{
			r = ((i & 4) >> 2) * 0xff;
			g = ((i & 2) >> 1) * 0xff;
			b = ((i & 1) >> 0) * 0xff;
		}

		palette_change_color(i,r,g,b);
	}
}


void z80bw_flipscreen_w(int data)
{
	flipscreen = (readinputport(0) & 0x01) ? data : 0;
	screen_flipped = 1;
}


void z80bw_screen_red_w(int data)
{
	screen_red = data;
	palette_modified = 1;
}


static void z80bw_plot_pixel(int x, int y, int col)
{
	if (flipscreen)
	{
		x = 255-x;
		y = 223-y;
	}

	plot_pixel(Machine->scrbitmap,x,y,Machine->pens[col]);
}


void astinvad_videoram_w (int offset,int data)
{
	int i;
	UINT8 x,y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	for (i = 0; i < 8; i++)
	{
		int col = 0;

		if (data & 0x01)
		{
			if (flipscreen)
				col = memory_region(REGION_PROMS)[((y+32)/8)*32+(x/8)] >> 4;
			else
				col = memory_region(REGION_PROMS)[(31-y/8)*32+(31-x/8)] & 0x0f;
		}

		z80bw_plot_pixel(x, y, col);

		x++;
		data >>= 1;
	}
}

void spaceint_videoram_w (int offset,int data) /* LT 23-12-1998 */ /*--WIP--*/
{
	int i;
	UINT8 x,y;
	videoram[offset] = data;

	y = 8 * (offset / 256);
	x = offset % 256;

	for (i = 0; i < 8; i++)
	{
		int col = 0;

		if (data & 0x01)
		{
			/* this is wrong */
			col = memory_region(REGION_PROMS)[(y/16)+16*((x+16)/32)];
		}

		z80bw_plot_pixel(x, y, col);

		y++;
		data >>= 1;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void common_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh,
								    void (*videoram_w_proc)(int, int))
{
	if (palette_modified)
	{
		set_palette();
		palette_modified = 0;
	}

	if (palette_recalc() || full_refresh || screen_flipped)
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
		{
			videoram_w_proc(offs, videoram[offs]);
		}

		screen_flipped = 0;
	}
}

void astinvad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	common_vh_screenrefresh(bitmap, full_refresh, astinvad_videoram_w);
}

void spaceint_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	common_vh_screenrefresh(bitmap, full_refresh, spaceint_videoram_w);
}
