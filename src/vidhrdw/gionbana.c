/***************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 1999/11/05 -

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/gionbana.h"
#include "machine/nb1413m3.h"


unsigned char gionbana_palette[0x200];
unsigned char gionbana_paltbl[0x8000];

static int gionbana_scrolly;
static int gionbana_drawx;
static int gionbana_drawy1;
static int gionbana_drawy2;
static int gionbana_sizex, gionbana_sizey;
static int gionbana_radrx, gionbana_radry;
static int gionbana_dispflag;
static int gionbana_vram;
static int gionbana_gfxrom;
static int gionbana_paltblnum;
static int gionbana_flipscreen;
static int gfxdraw_mode;

static struct osd_bitmap *tmpbitmap1, *tmpbitmap2;
static unsigned char *gionbana_videoram1, *gionbana_videoram2;


static void gionbana_gfxdraw(void);


/******************************************************************/


/******************************************************************/
READ_HANDLER( gionbana_palette_r )
{
	return gionbana_palette[offset];
}

WRITE_HANDLER( gionbana_palette_w )
{
	int r, g, b;

	gionbana_palette[offset] = data;

	offset &= 0x1fe;

	r = ((gionbana_palette[offset + 0] & 0x0f) << 4);
	g = ((gionbana_palette[offset + 1] & 0xf0) << 0);
	b = ((gionbana_palette[offset + 1] & 0x0f) << 4);

	palette_change_color((offset / 2), r, g, b);
}

READ_HANDLER( maiko_palette_r )
{
	return gionbana_palette[offset];
}

WRITE_HANDLER( maiko_palette_w )
{
	int r, g, b;

	gionbana_palette[offset] = data;

	offset &= 0x0ff;

	r = ((gionbana_palette[offset + 0x000] & 0x0f) << 4);
	g = ((gionbana_palette[offset + 0x000] & 0xf0) << 0);
	b = ((gionbana_palette[offset + 0x100] & 0x0f) << 4);

	palette_change_color((offset & 0x0ff), r, g, b);
}

/******************************************************************/


/******************************************************************/
void gionbana_radrx_w(int data)
{
	gionbana_radrx = data;
}

void gionbana_radry_w(int data)
{
	gionbana_radry = data;
}

void gionbana_sizex_w(int data)
{
	gionbana_sizex = data;
}

void gionbana_sizey_w(int data)
{
	gionbana_sizey = data;

	gionbana_gfxdraw();
}

void gionbana_dispflag_w(int data)
{
	gionbana_dispflag = data;
}

void gionbana_drawx_w(int data)
{
	gionbana_drawx = data;
}

void gionbana_drawy_w(int data)
{
	gionbana_drawy1 = data;
	gionbana_drawy2 = ((data - gionbana_scrolly) & 0xff);
}

void gionbana_scrolly_w(int data)
{
	gionbana_scrolly = ((-data) & 0xff);
}

void gionbana_vramsel_w(int data)
{
	gionbana_vram = data;

	gionbana_flipscreen = (data & 0x80) >> 7;
}

void gionbana_romsel_w(int data)
{
	gionbana_gfxrom = (data & 0x0f);

	if ((0x20000 * gionbana_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		gionbana_gfxrom = 0;
	}
}

void gionbana_paltblnum_w(int data)
{
	gionbana_paltblnum = data;
}

READ_HANDLER( gionbana_paltbl_r )
{
	return gionbana_paltbl[offset];
}

WRITE_HANDLER( gionbana_paltbl_w )
{
	gionbana_paltbl[((gionbana_paltblnum & 0x7f) * 0x10) + (offset & 0x0f)] = data;
}

/******************************************************************/


/******************************************************************/
static void gionbana_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int i, j;
	int x, y;
	int flipx, flipy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;

	flipx = (gionbana_dispflag & 0x01) ? 1 : 0;
	flipy = (gionbana_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((gionbana_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		gionbana_drawx = ((gionbana_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (gionbana_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((gionbana_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		gionbana_drawy1 = ((gionbana_drawy1 - sizey) & 0xff);
		gionbana_drawy2 = gionbana_drawy1;
	}
	else
	{
		starty = (gionbana_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char drawcolor1, drawcolor2;

			color1 = (((GFX[(0x20000 * gionbana_gfxrom) + ((0x0200 * gionbana_radry) + (0x0002 * gionbana_radrx)) + (i++)]) & 0xf0) >> 4);
			color2 = (((GFX[(0x20000 * gionbana_gfxrom) + ((0x0200 * gionbana_radry) + (0x0002 * gionbana_radrx)) + (j++)]) & 0x0f) >> 0);

			drawcolor1 = gionbana_paltbl[((gionbana_paltblnum & 0x7f) * 0x10) + color1];
			drawcolor2 = gionbana_paltbl[((gionbana_paltblnum & 0x7f) * 0x10) + color2];

			if (gfxdraw_mode)
			{
				if (gionbana_vram & 0x01)
				{
					tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
					tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
				}
				else
				{
					if (gionbana_vram & 0x08)
					{
						tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
						tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
					}
					else
					{
						tflag1 = tflag2 = 1;
					}

					if (drawcolor1 == 0x7f) drawcolor1 = 0xff;
					if (drawcolor2 == 0x7f) drawcolor2 = 0xff;
				}
			}
			else
			{
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
				gionbana_vram = 0x02;
			}

			if (flipx)
			{
				int tmp;

				tmp = drawcolor1;
				drawcolor1 = drawcolor2;
				drawcolor2 = tmp;

				tmp = tflag1;
				tflag1 = tflag2;
				tflag2 = tmp;
			}

			nb1413m3_busyctr++;

			{
				int x1, x2, y1, y2;

				x1 = (((gionbana_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((gionbana_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				y1 = ((gionbana_drawy1 + y) & 0xff);
				y2 = ((gionbana_drawy2 + y) & 0xff);

				if (gfxdraw_mode)
				{
					if (gionbana_vram & 0x01)
					{
						if (tflag1)
						{
							gionbana_videoram1[(y1 * Machine->drv->screen_width) + x1] = drawcolor1;
							plot_pixel(tmpbitmap1, x1, y1, Machine->pens[drawcolor1]);
						}
						if (tflag2)
						{
							gionbana_videoram1[(y1 * Machine->drv->screen_width) + x2] = drawcolor2;
							plot_pixel(tmpbitmap1, x2, y1, Machine->pens[drawcolor2]);
						}
					}
					if (gionbana_vram & 0x02)
					{
						if (tflag1)
						{
							gionbana_videoram2[(y2 * Machine->drv->screen_width) + x1] = drawcolor1;
							plot_pixel(tmpbitmap2, x1, y2, Machine->pens[drawcolor1]);
						}
						if (tflag2)
						{
							gionbana_videoram2[(y2 * Machine->drv->screen_width) + x2] = drawcolor2;
							plot_pixel(tmpbitmap2, x2, y2, Machine->pens[drawcolor2]);
						}
					}
				}
				else
				{
					if (tflag1)
					{
						gionbana_videoram1[(y2 * Machine->drv->screen_width) + x1] = drawcolor1;
						plot_pixel(tmpbitmap1, x1, y2, Machine->pens[drawcolor1]);
					}
					if (tflag2)
					{
						gionbana_videoram1[(y2 * Machine->drv->screen_width) + x2] = drawcolor2;
						plot_pixel(tmpbitmap1, x2, y2, Machine->pens[drawcolor2]);
					}
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4650) ? 0 : 1;

}

/******************************************************************/


/******************************************************************/
int gionbana_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((tmpbitmap2 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((gionbana_videoram1 = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char))) == 0) return 1;
	if ((gionbana_videoram2 = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char))) == 0) return 1;
	memset(gionbana_videoram1, 0xff, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char)));
	memset(gionbana_videoram2, 0xff, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char)));
	gfxdraw_mode = 1;
	return 0;
}

void gionbana_vh_stop(void)
{
	free(gionbana_videoram2);
	free(gionbana_videoram1);
	bitmap_free(tmpbitmap2);
	bitmap_free(tmpbitmap1);
	gionbana_videoram2 = 0;
	gionbana_videoram1 = 0;
	tmpbitmap2 = 0;
	tmpbitmap1 = 0;
}

int hanamomo_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((gionbana_videoram1 = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char))) == 0) return 1;
	memset(gionbana_videoram1, 0xff, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char)));
	gfxdraw_mode = 0;
	return 0;
}

void hanamomo_vh_stop(void)
{
	free(gionbana_videoram1);
	bitmap_free(tmpbitmap1);
	gionbana_videoram1 = 0;
	tmpbitmap1 = 0;
}

void gionbana_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y;
	int color;

	if (palette_recalc() || full_refresh)
	{
		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = gionbana_videoram1[(y * Machine->drv->screen_width) + x];
				plot_pixel(tmpbitmap1, x, y, Machine->pens[color]);
			}
		}
		if (gfxdraw_mode)
		{
			for (y = 0; y < Machine->drv->screen_height; y++)
			{
				for (x = 0; x < Machine->drv->screen_width; x++)
				{
					color = gionbana_videoram2[(y * Machine->drv->screen_width) + x];
					plot_pixel(tmpbitmap2, x, y, Machine->pens[color]);
				}
			}
		}
	}

	if (!(gionbana_dispflag & 0x08))
	{
		if (gfxdraw_mode)
		{
			copybitmap(bitmap, tmpbitmap1, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
			copyscrollbitmap(bitmap, tmpbitmap2, 0, 0, 1, &gionbana_scrolly, &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0xff]);
		}
		else
		{
			copyscrollbitmap(bitmap, tmpbitmap1, 0, 0, 1, &gionbana_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		}
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0], 0);
	}
}
