/***************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 2000/01/28 -

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/hyhoo.h"
#include "machine/nb1413m3.h"


unsigned char hyhoo_palette[0x10];

static int hyhoo_drawx;
static int hyhoo_drawy;
static int hyhoo_sizex, hyhoo_sizey;
static int hyhoo_radrx, hyhoo_radry;
static int hyhoo_dispflag;
static int hyhoo_dispflag2;
static int hyhoo_gfxrom;
static int hyhoo_gfxrom2;

static struct osd_bitmap *tmpbitmap1;
static unsigned short *hyhoo_videoram;


static void hyhoo_gfxdraw(void);


/******************************************************************/


/******************************************************************/
void hyhoo_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

#if 0
	/* initialize 655 RGB lookup */
	for (i = 0; i < 65536; i++)
	{
		int r, g, b;

		r = (i >>  0) & 0x3f;
		g = (i >>  6) & 0x1f;
		b = (i >> 11) & 0x1f;

		(*palette++) = (r << 3) | (r >> 2);
		(*palette++) = (g << 3) | (g >> 2);
		(*palette++) = (b << 3) | (b >> 2);
	}
#else
	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r, g, b;

		r = (i >>  0) & 0x1f;
		g = (i >>  5) & 0x1f;
		b = (i >> 10) & 0x1f;

		(*palette++) = (r << 3) | (r >> 2);
		(*palette++) = (g << 3) | (g >> 2);
		(*palette++) = (b << 3) | (b >> 2);
	}
#endif
}

WRITE_HANDLER( hyhoo_palette_w )
{
	hyhoo_palette[offset & 0x0f] = (data ^ 0xff);
}

/******************************************************************/


/******************************************************************/
void hyhoo_radrx_w(int data)
{
	hyhoo_radrx = data;
}

void hyhoo_radry_w(int data)
{
	hyhoo_radry = data;
}

void hyhoo_sizex_w(int data)
{
	hyhoo_sizex = data;
}

void hyhoo_sizey_w(int data)
{
	hyhoo_sizey = data;

	hyhoo_gfxdraw();
}

void hyhoo_dispflag_w(int data)
{
	hyhoo_dispflag = data;
}

void hyhoo_dispflag2_w(int data)
{
	hyhoo_dispflag2 = data;
}

void hyhoo_drawx_w(int data)
{
	hyhoo_drawx = data;
}

void hyhoo_drawy_w(int data)
{
	hyhoo_drawy = data;
}

void hyhoo_romsel_w(int data)
{
	hyhoo_gfxrom = (((data & 0xc0) >> 4) + (data & 0x03));
	hyhoo_gfxrom2 = data;

	if ((0x20000 * hyhoo_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		hyhoo_gfxrom = 0;
	}
}

/******************************************************************/


/******************************************************************/
void hyhoo_gfxdraw(void)
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
	int hcflag;

	flipx = (hyhoo_dispflag & 0x01) ? 1 : 0;
	flipy = (hyhoo_dispflag & 0x02) ? 1 : 0;

	hyhoo_gfxrom |= ((nb1413m3_sndrombank1 & 0x02) << 3);

	if (flipx)
	{
		startx = 0;
		sizex = (((hyhoo_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		hyhoo_drawx = ((hyhoo_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (hyhoo_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((hyhoo_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		hyhoo_drawy = ((hyhoo_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (hyhoo_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned short drawcolor1, drawcolor2;

			if (hyhoo_gfxrom2 & 0x04)
			{
				// 65536 colors mode
				color1 = (((GFX[(0x20000 * hyhoo_gfxrom) + ((0x0200 * hyhoo_radry) + (0x0002 * hyhoo_radrx)) + (i++)]) & 0xff) >> 0);
				color2 = (((GFX[(0x20000 * hyhoo_gfxrom) + ((0x0200 * hyhoo_radry) + (0x0002 * hyhoo_radrx)) + (j++)]) & 0xff) >> 0);

				if (hyhoo_gfxrom2 & 0x20)
				{
					// 65536 colors (lower)
					drawcolor1 = color1;
					drawcolor2 = color2;
					tflag1 = 1;
					tflag2 = 1;
					hcflag = 1;
#if 1
					{
						int tmp_r, tmp_g, tmp_b;

						// src xxxxxxxx_bbbggrrr
						// dst bbbbbggg_ggrrrrrr

					//	tmp_r = (((drawcolor1 & 0x07) >> 0) & 0x07);
						tmp_r = (((drawcolor1 & 0x07) >> 1) & 0x03);
						tmp_g = (((drawcolor1 & 0x18) >> 3) & 0x03);
						tmp_b = (((drawcolor1 & 0xe0) >> 5) & 0x07);
					//	drawcolor1 = ((tmp_b << (11 + 0)) | (tmp_g << (6 + 0)) | (tmp_r << (0 + 0)));
						drawcolor1 = ((tmp_b << (10 + 0)) | (tmp_g << (5 + 0)) | (tmp_r << (0 + 0)));

					//	tmp_r = (((drawcolor2 & 0x07) >> 0) & 0x07);
						tmp_r = (((drawcolor2 & 0x07) >> 0) & 0x07);
						tmp_g = (((drawcolor2 & 0x18) >> 3) & 0x03);
						tmp_b = (((drawcolor2 & 0xe0) >> 5) & 0x07);
					//	drawcolor2 = ((tmp_b << (11 + 0)) | (tmp_g << (6 + 0)) | (tmp_r << (0 + 0)));
						drawcolor2 = ((tmp_b << (10 + 0)) | (tmp_g << (5 + 0)) | (tmp_r << (0 + 0)));
					}
#endif
				}
				else
				{
					// 65536 colors(higher)
					drawcolor1 = color1;
					drawcolor2 = color2;
					tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
					tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
					hcflag = 0;

#if 1
					{
						int tmp_r, tmp_g, tmp_b;

						// src xxxxxxxx_bbgggrrr
						// dst bbbbbggg_ggrrrrrr

						tmp_r = (drawcolor1 & 0x07) >> 0;
						tmp_g = (drawcolor1 & 0x38) >> 3;
						tmp_b = (drawcolor1 & 0xc0) >> 6;
					//	drawcolor1 = ((tmp_b << (11 + 3)) | (tmp_g << (6 + 2)) | (tmp_r << (0 + 3)));
						drawcolor1 = ((tmp_b << (10 + 3)) | (tmp_g << (5 + 2)) | (tmp_r << (0 + 2)));

						tmp_r = (drawcolor2 & 0x07) >> 0;
						tmp_g = (drawcolor2 & 0x38) >> 3;
						tmp_b = (drawcolor2 & 0xc0) >> 6;
					//	drawcolor2 = ((tmp_b << (11 + 3)) | (tmp_g << (6 + 2)) | (tmp_r << (0 + 3)));
						drawcolor2 = ((tmp_b << (10 + 3)) | (tmp_g << (5 + 2)) | (tmp_r << (0 + 2)));
					}
#endif
				}
			}
			else
			{
				// Palettized picture
				color1 = (((GFX[(0x20000 * hyhoo_gfxrom) + ((0x0200 * hyhoo_radry) + (0x0002 * hyhoo_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * hyhoo_gfxrom) + ((0x0200 * hyhoo_radry) + (0x0002 * hyhoo_radrx)) + (j++)]) & 0x0f) >> 0);

				drawcolor1 = hyhoo_palette[color1];
				drawcolor2 = hyhoo_palette[color2];
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
				hcflag = 0;

#if 1
				{
					int tmp_r, tmp_g, tmp_b;

					// src xxxxxxxx_bbgggrrr
					// dst bbbbbggg_ggrrrrrr

					tmp_r = (drawcolor1 & 0x07) >> 0;
					tmp_g = (drawcolor1 & 0x38) >> 3;
					tmp_b = (drawcolor1 & 0xc0) >> 6;
				//	drawcolor1 = ((tmp_b << (11 + 3)) | (tmp_g << (6 + 2)) | (tmp_r << (0 + 3)));
					drawcolor1 = ((tmp_b << (10 + 3)) | (tmp_g << (5 + 2)) | (tmp_r << (0 + 2)));

					tmp_r = (drawcolor2 & 0x07) >> 0;
					tmp_g = (drawcolor2 & 0x38) >> 3;
					tmp_b = (drawcolor2 & 0xc0) >> 6;
				//	drawcolor2 = ((tmp_b << (11 + 3)) | (tmp_g << (6 + 2)) | (tmp_r << (0 + 3)));
					drawcolor2 = ((tmp_b << (10 + 3)) | (tmp_g << (5 + 2)) | (tmp_r << (0 + 2)));
				}
#endif
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
				int x1, x2, yy;

				x1 = (((hyhoo_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((hyhoo_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((hyhoo_drawy + y) & 0xff);

				if (tflag1)
				{
					if (hcflag)
					{
						hyhoo_videoram[(yy * Machine->drv->screen_width) + x1] |= drawcolor1;
						drawcolor1 = hyhoo_videoram[(yy * Machine->drv->screen_width) + x1];
					}
					else
					{
						hyhoo_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					}
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					if (hcflag)
					{
						hyhoo_videoram[(yy * Machine->drv->screen_width) + x2] |= drawcolor2;
						drawcolor2 = hyhoo_videoram[(yy * Machine->drv->screen_width) + x2];
					}
					else
					{
						hyhoo_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					}
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 8000) ? 0 : 1;

}

/******************************************************************/


/******************************************************************/
int hyhoo_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((hyhoo_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(hyhoo_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	return 0;
}

void hyhoo_vh_stop(void)
{
	free(hyhoo_videoram);
	bitmap_free(tmpbitmap1);
	hyhoo_videoram = 0;
	tmpbitmap1 = 0;
}

void hyhoo_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y;
	unsigned short color;

	if (full_refresh)
	{
		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = hyhoo_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(tmpbitmap1, x, y, Machine->pens[color]);
			}
		}
	}

	if (!(hyhoo_dispflag & 0x08))	// display enable ?
	{
		copybitmap(bitmap, tmpbitmap1, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0], 0);
	}
}
