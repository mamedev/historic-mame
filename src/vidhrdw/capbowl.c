/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms34061.h"

unsigned char *capbowl_scanline;

static unsigned char *raw_video_ram;
static unsigned int  color_count[4096];
static unsigned char dirty_line[256];

static int max_x, max_y, max_x_offset;

static void capbowl_xyaddress_setpixel(int x, int y, int pixel);
static int  capbowl_xyaddress_getpixel(int x, int y);

#define PAL_SIZE  0x20

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int capbowl_vh_start(void)
{
	int i;

	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	if ((raw_video_ram = malloc(256 * 256)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	// Initialize TMS34061 emulation
    if (tms34061_start(TMS34061_REG_ADDRESS_PACKED,
					   0,
		               capbowl_xyaddress_getpixel,
					   capbowl_xyaddress_setpixel))
	{
		free(raw_video_ram);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	max_y = Machine->drv->visible_area.max_y;
	max_x = Machine->drv->visible_area.max_x;
	max_x_offset = (max_x + 1) / 2 + PAL_SIZE;

	// Initialize color areas. The screen is blank
	memset(raw_video_ram, 0, 256*256);
	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));
	memset(color_count, 0, sizeof(color_count));
	memset(dirty_line, 1, sizeof(dirty_line));

	for (i = 0; i < max_y * 16; i+=16)
	{
		palette_used_colors[i] = PALETTE_COLOR_USED;
		color_count[i] = max_x + 1;  // All the pixels are pen 0
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void capbowl_vh_stop(void)
{
	free(raw_video_ram);
	osd_free_bitmap(tmpbitmap);

	tms34061_stop();
}


INLINE void capbowl_videoram_common_w(int x, int y, int data)
{
	int off = ((y << 8) | x);

	int olddata = raw_video_ram[off];
	if (olddata != data)
	{
		int penstart = y << 4;

		raw_video_ram[off] = data;

		if (y > max_y || x >= max_x_offset) return;

		if (x >= PAL_SIZE)
		{
			int x1 = max_x - ((x-PAL_SIZE) << 1);

			int oldpen1 = penstart | (olddata >> 4);
			int oldpen2 = penstart | (olddata & 0x0f);
			int newpen1 = penstart | (data >> 4);
			int newpen2 = penstart | (data & 0x0f);

			if (oldpen1 != newpen1)
			{
				dirty_line[y] = 1;

				color_count[oldpen1]--;
				if (!color_count[oldpen1]) palette_used_colors[oldpen1] = PALETTE_COLOR_UNUSED;

				color_count[newpen1]++;
				palette_used_colors[newpen1] = PALETTE_COLOR_USED;
			}

			if (oldpen2 != newpen2)
			{
				dirty_line[y] = 1;

				color_count[oldpen2]--;
				if (!color_count[oldpen2]) palette_used_colors[oldpen2] = PALETTE_COLOR_UNUSED;

				color_count[newpen2]++;
				palette_used_colors[newpen2] = PALETTE_COLOR_USED;
			}
		}
		else
		{
			/* Offsets 0-1f are the palette */

			int r = (raw_video_ram[off & ~1] & 0x0f);
			int g = (raw_video_ram[off |  1] >> 4);
			int b = (raw_video_ram[off |  1] & 0x0f);
			r = (r << 4) + r;
			g = (g << 4) + g;
			b = (b << 4) + b;

			palette_change_color(penstart | (x >> 1),r,g,b);
		}
	}
}

void capbowl_videoram_w(int offset, int data)
{
    capbowl_videoram_common_w(offset, *capbowl_scanline, data);
}

static void capbowl_xyaddress_setpixel(int x, int y, int pixel)
{
    capbowl_videoram_common_w(x, y, pixel);
}



INLINE int capbowl_videoram_common_r(int x, int y)
{
	return raw_video_ram[y << 8 | x];
}

int capbowl_videoram_r(int offset)
{
	return capbowl_videoram_common_r(offset, *capbowl_scanline);
}

static int capbowl_xyaddress_getpixel(int x, int y)
{
	return capbowl_videoram_common_r(x, y);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
INLINE void plotpixel(int x,int y,int pen)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = x;
		x = y;
		y = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = tmpbitmap->width - x - 1;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = tmpbitmap->height - y - 1;

	tmpbitmap->line[y][x] = pen;
}

void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int x, y;


	if (tms34061_display_blanked())
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

		return;
	}

    if (palette_recalc())
		memset(dirty_line, 1, sizeof(dirty_line));

	for (y = 0; y <= max_y; y++)
	{
		if (dirty_line[y])
		{
			int x1 = 0;
			int y1 = (y << 8 | PAL_SIZE);
			int y2 =  y << 4;

			dirty_line[y] = 0;

			for (x = PAL_SIZE; x < max_x_offset; x++)
			{
				int data = raw_video_ram[y1++];

				plotpixel(x1++,y,Machine->pens[y2 | (data >> 4)  ]);
				plotpixel(x1++,y,Machine->pens[y2 | (data & 0x0f)]);
			}
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
