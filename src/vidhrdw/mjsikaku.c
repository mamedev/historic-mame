/***************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 1999/11/05 -

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/mjsikaku.h"
#include "machine/nb1413m3.h"


unsigned char mjsikaku_palette[0x20];

static int mjsikaku_scrolly;
static int mjsikaku_drawx;
static int mjsikaku_drawy;
static int mjsikaku_sizex, mjsikaku_sizey;
static int mjsikaku_radrx, mjsikaku_radry;
static int mjsikaku_dispflag;
static int mjsikaku_dispflag2;
static int mjsikaku_gfxrom;
static int mjsikaku_flipscreen;
static int gfxdraw_mode;

static struct osd_bitmap *tmpbitmap1;
static unsigned short *mjsikaku_videoram;


static void mjsikaku_gfxdraw(void);
static void secolove_gfxdraw(void);
static void bijokkoy_gfxdraw(void);
static void housemnq_gfxdraw(void);
static void seiha_gfxdraw(void);
static void crystal2_gfxdraw(void);


/******************************************************************/


/******************************************************************/
void mjsikaku_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	/* initialize 444 RGB lookup */
	for (i = 0; i < 4096; i++)
	{
		int r, g, b;

		r = (i >> 0) & 0x0f;
		g = (i >> 4) & 0x0f;
		b = (i >> 8) & 0x0f;

		(*palette++) = ((r << 4) | r);
		(*palette++) = ((g << 4) | g);
		(*palette++) = ((b << 4) | b);
	}
}

void secolove_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	/* initialize 444 RGB lookup */
	for (i = 0; i < 4096; i++)
	{
		int r, g, b;

		r = (i >> 0) & 0x0f;
		g = (i >> 4) & 0x0f;
		b = (i >> 8) & 0x0f;

		(*palette++) = ((r << 4) | r);
		(*palette++) = ((g << 4) | g);
		(*palette++) = ((b << 4) | b);
	}
}

void seiha_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r, g, b;

		r = (i >>  0) & 0x1f;
		g = (i >>  5) & 0x1f;
		b = (i >> 10) & 0x1f;

		(*palette++) = ((r << 3) | (r >> 2));
		(*palette++) = ((g << 3) | (g >> 2));
		(*palette++) = ((b << 3) | (b >> 2));
	}
}

void crystal2_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	/* initialize 332 RGB lookup */
	for (i = 0; i < 256; i++)
	{
		int bit0, bit1, bit2;

		/* red component */
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		(*palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		(*palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		(*palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}
}

WRITE_HANDLER( mjsikaku_palette_w )
{
	mjsikaku_palette[(((offset & 0x0f) << 1) | ((offset & 0x10) >> 4))] = (data ^ 0xff);
}

WRITE_HANDLER( secolove_palette_w )
{
	mjsikaku_palette[offset & 0x0f] = (data ^ 0xff);
}

/******************************************************************/


/******************************************************************/
void mjsikaku_radrx_w(int data)
{
	mjsikaku_radrx = data;
}

void mjsikaku_radry_w(int data)
{
	mjsikaku_radry = data;
}

void mjsikaku_sizex_w(int data)
{
	mjsikaku_sizex = data;
}

void mjsikaku_sizey_w(int data)
{
	mjsikaku_sizey = data;

	switch (gfxdraw_mode)
	{
		case	0:
			mjsikaku_gfxdraw();
			break;
		case	1:
			secolove_gfxdraw();
			break;
		case	2:
			seiha_gfxdraw();
			break;
		case	3:
			crystal2_gfxdraw();
			break;
		case	4:
			bijokkoy_gfxdraw();
			break;
		case	5:
			housemnq_gfxdraw();
			break;
	}
}

void mjsikaku_dispflag_w(int data)
{
	mjsikaku_dispflag = data;

	mjsikaku_flipscreen = (data & 0x04) >> 2;
}

void mjsikaku_dispflag2_w(int data)
{
	mjsikaku_dispflag2 = data;

	if (!strcmp(Machine->gamedrv->name, "seiham")) mjsikaku_dispflag2 ^= 0x20;
}

void mjsikaku_drawx_w(int data)
{
	mjsikaku_drawx = data;
}

void mjsikaku_drawy_w(int data)
{
	mjsikaku_drawy = ((data - mjsikaku_scrolly) & 0xff);
}

void mjsikaku_scrolly_w(int data)
{
	mjsikaku_scrolly = ((-data) & 0x1ff);
}

void mjsikaku_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x1f);

	if ((0x20000 * mjsikaku_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		mjsikaku_gfxrom = 0;
	}
}

void secolove_romsel_w(int data)
{
	mjsikaku_gfxrom = ((data & 0xc0) >> 4) + (data & 0x03);

	if ((0x20000 * mjsikaku_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		mjsikaku_gfxrom = 0;
	}
}

void iemoto_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x07);

	if ((0x20000 * mjsikaku_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		mjsikaku_gfxrom = 0;
	}
}

void crystal2_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x03);

	if ((0x20000 * mjsikaku_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
		usrintf_showmessage("GFXROM BANK OVER!!");
		mjsikaku_gfxrom = 0;
	}
}

/******************************************************************/


/******************************************************************/
static void mjsikaku_gfxdraw(void)
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

	if (mjsikaku_dispflag2 & 0x20) return;

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char r, g, b;
			unsigned short drawcolor1, drawcolor2;

			color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
			color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);

			r = ((mjsikaku_palette[(color1 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x01) >> 0);
			g = ((mjsikaku_palette[(color1 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x02) >> 1);
			b = ((mjsikaku_palette[(color1 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x0c) >> 2);
			drawcolor1 = (((b << 8) | (g << 4) | (r << 0)) & 0x0fff);
			tflag1 = ((r == 0x0f) && (g == 0x0f) && (b == 0x0f)) ? 0 : 1;

			r = ((mjsikaku_palette[(color2 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x01) >> 0);
			g = ((mjsikaku_palette[(color2 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x02) >> 1);
			b = ((mjsikaku_palette[(color2 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x0c) >> 2);
			drawcolor2 = (((b << 8) | (g << 4) | (r << 0)) & 0x0fff);
			tflag2 = ((r == 0x0f) && (g == 0x0f) && (b == 0x0f)) ? 0 : 1;

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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;

}

static void secolove_gfxdraw(void)
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

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char r, g, b;
			unsigned short drawcolor1, drawcolor2;

			if (mjsikaku_dispflag2 & 0x04)
			{
				// 65536 colors mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xff) >> 0);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0xff) >> 0);

				{
					// high

					drawcolor1 = color1;	// dammy
					drawcolor2 = color2;	// dammy

					hcflag = 1;

					// src xxxxxxxx_bbgggrrr
					// dst xxxxbbbb_ggggrrrr

					r = ((drawcolor1 & 0x07) >> 0) & 0x07;
					g = ((drawcolor1 & 0x38) >> 3) & 0x07;
					b = ((drawcolor1 & 0xc0) >> 6) & 0x03;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (8 + 2)) | (g << (4 + 1)) | (r << (0 + 1)));

					r = ((drawcolor2 & 0x07) >> 0) & 0x07;
					g = ((drawcolor2 & 0x38) >> 3) & 0x07;
					b = ((drawcolor2 & 0xc0) >> 6) & 0x03;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (8 + 2)) | (g << (4 + 1)) | (r << (0 + 1)));
				}
			}
			else
			{
				// Palettized picture mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);

				if (mjsikaku_dispflag2 & 0x20)
				{
					drawcolor1 = 0;
					drawcolor2 = 0;
					tflag1 = tflag2 = 0;
					hcflag = 0;
				}
				else
				{
					//
					drawcolor1 = mjsikaku_palette[color1];
					drawcolor2 = mjsikaku_palette[color2];

					hcflag = 1;

					// src xxxxxxxx_bbgggrrr
					// dst xxxxbbbb_ggggrrrr

					r = ((drawcolor1 & 0x07) >> 0) & 0x07;
					g = ((drawcolor1 & 0x38) >> 3) & 0x07;
					b = ((drawcolor1 & 0xc0) >> 6) & 0x03;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (8 + 2)) | (g << (4 + 1)) | (r << (0 + 1)));

					r = ((drawcolor2 & 0x07) >> 0) & 0x07;
					g = ((drawcolor2 & 0x38) >> 3) & 0x07;
					b = ((drawcolor2 & 0xc0) >> 6) & 0x03;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (8 + 2)) | (g << (4 + 1)) | (r << (0 + 1)));
				}
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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] |= drawcolor1;
						drawcolor1 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1];
					}
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] |= drawcolor2;
						drawcolor2 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2];
					}
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;

}

static void bijokkoy_gfxdraw(void)
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

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char r, g, b;
			unsigned short drawcolor1, drawcolor2;

			if (mjsikaku_dispflag2 & 0x04)
			{
				// 65536 colors mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xff) >> 0);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0xff) >> 0);

				if (mjsikaku_dispflag2 & 0x20)
				{
					// low
					hcflag = 1;
					drawcolor1 = color1;	// dammy
					drawcolor2 = color2;	// dammy

					// src xxxxxxxx_bbbggrrr
					// dst xbbbbbgg_gggrrrrr

					r = ((drawcolor1 & 0x07) >> 1) & 0x03;
					g = ((drawcolor1 & 0x18) >> 3) & 0x03;
					b = ((drawcolor1 & 0xe0) >> 5) & 0x07;
					tflag1 = ((r == 0x03) && (g == 0x03) && (b == 0x07)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));

					r = ((drawcolor2 & 0x07) >> 1) & 0x03;
					g = ((drawcolor2 & 0x18) >> 3) & 0x03;
					b = ((drawcolor2 & 0xe0) >> 5) & 0x07;
					tflag2 = ((r == 0x03) && (g == 0x03) && (b == 0x07)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));
				}
				else
				{
					// high
					hcflag = 0;
					drawcolor1 = color1;	// dammy
					drawcolor2 = color2;	// dammy

					// src xxxxxxxx_bbgggrrr
					// dst xbbbbbgg_gggrrrrr

					r = (drawcolor1 & 0x07) >> 0;
					g = (drawcolor1 & 0x38) >> 3;
					b = (drawcolor1 & 0xc0) >> 6;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));

					r = (drawcolor2 & 0x07) >> 0;
					g = (drawcolor2 & 0x38) >> 3;
					b = (drawcolor2 & 0xc0) >> 6;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));
				}
			}
			else
			{
				// Palettized picture mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);

				if (mjsikaku_dispflag2 & 0x20)
				{
					//
					drawcolor1 = 0;		// dammy
					drawcolor2 = 0;		// dammy

					hcflag = 0;

					tflag1 = tflag2 = 0;	// dammy
				}
				else
				{
					// Palettized picture mode
					drawcolor1 = mjsikaku_palette[color1];
					drawcolor2 = mjsikaku_palette[color2];

					hcflag = 1;

					// src xxxxxxxx_bbgggrrr
					// dst xbbbbbgg_gggrrrrr

					r = (drawcolor1 & 0x07) >> 0;
					g = (drawcolor1 & 0x38) >> 3;
					b = (drawcolor1 & 0xc0) >> 6;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));

					r = (drawcolor2 & 0x07) >> 0;
					g = (drawcolor2 & 0x38) >> 3;
					b = (drawcolor2 & 0xc0) >> 6;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));
				}
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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] |= drawcolor1;
						drawcolor1 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1];
					}
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] |= drawcolor2;
						drawcolor2 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2];
					}
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;

}

static void housemnq_gfxdraw(void)
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

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char r, g, b;
			unsigned short drawcolor1, drawcolor2;

			if (mjsikaku_dispflag2 & 0x04)
			{
				// 65536 colors mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xff) >> 0);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0xff) >> 0);

				if (mjsikaku_dispflag2 & 0x20)
				{
					// low
					hcflag = 1;
					drawcolor1 = color1;	// dammy
					drawcolor2 = color2;	// dammy

					// src xxxxxxxx_bbbggrrr
					// dst xbbbbbgg_gggrrrrr

					r = ((drawcolor1 & 0x07) >> 1) & 0x03;
					g = ((drawcolor1 & 0x18) >> 3) & 0x03;
					b = ((drawcolor1 & 0xe0) >> 5) & 0x07;
					tflag1 = ((r == 0x03) && (g == 0x03) && (b == 0x07)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));

					r = ((drawcolor2 & 0x07) >> 1) & 0x03;
					g = ((drawcolor2 & 0x18) >> 3) & 0x03;
					b = ((drawcolor2 & 0xe0) >> 5) & 0x07;
					tflag2 = ((r == 0x03) && (g == 0x03) && (b == 0x07)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));
				}
				else
				{
					// high
					hcflag = 0;
					drawcolor1 = color1;	// dammy
					drawcolor2 = color2;	// dammy

					// src xxxxxxxx_bbgggrrr
					// dst xbbbbbgg_gggrrrrr

					r = (drawcolor1 & 0x07) >> 0;
					g = (drawcolor1 & 0x38) >> 3;
					b = (drawcolor1 & 0xc0) >> 6;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));

					r = (drawcolor2 & 0x07) >> 0;
					g = (drawcolor2 & 0x38) >> 3;
					b = (drawcolor2 & 0xc0) >> 6;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));
				}
			}
			else
			{
				// Palettized picture mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);


				if (mjsikaku_dispflag2 & 0x20)
				{
					//
					drawcolor1 = 0;		// dammy
					drawcolor2 = 0;		// dammy

					hcflag = 1;

					tflag1 = tflag2 = 0;	// dammy
				}
				else
				{
					// Palettized picture mode
					drawcolor1 = mjsikaku_palette[color1];
					drawcolor2 = mjsikaku_palette[color2];

					hcflag = 0;

					// src xxxxxxxx_bbgggrrr
					// dst xbbbbbgg_gggrrrrr

					r = (drawcolor1 & 0x07) >> 0;
					g = (drawcolor1 & 0x38) >> 3;
					b = (drawcolor1 & 0xc0) >> 6;
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));

					r = (drawcolor2 & 0x07) >> 0;
					g = (drawcolor2 & 0x38) >> 3;
					b = (drawcolor2 & 0xc0) >> 6;
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 = ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));
				}
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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] |= drawcolor1;
						drawcolor1 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1];
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					}
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					if (hcflag)
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] |= drawcolor2;
						drawcolor2 = mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2];
					}
					else
					{
						mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					}
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;

}

static void seiha_gfxdraw(void)
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

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
		sizey = (starty + 1);
		skipy = -1;
	}

	i = j = 0;

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			unsigned char color1, color2;
			unsigned char r, g, b;
			unsigned short drawcolor1, drawcolor2;

			if (!(mjsikaku_dispflag2 & 0x20))
			{
				// 65536 colors mode
				//   To prevent illegal access
				if (((0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) - (sizey * sizex)) < 0) continue;

				{
					// 65536 colors (lower)

					color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + i]) & 0xff) >> 0);
					color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + j]) & 0xff) >> 0);

					// src xxxxxxxx_bbbggrrr
					// dst xbbbbbgg_gggrrrrr

				//	r = ((color1 & 0x07) >> 0);
					r = ((color1 & 0x06) >> 1);
					g = ((color1 & 0x18) >> 3);
					b = ((color1 & 0xe0) >> 5);
					drawcolor1 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));

				//	r = ((color2 & 0x07) >> 0);
					r = ((color2 & 0x06) >> 1);
					g = ((color2 & 0x18) >> 2);
					b = ((color2 & 0xe0) >> 5);
					drawcolor2 = ((b << (10 + 0)) | (g << (5 + 0)) | (r << (0 + 0)));
				}
				{
					// 65536 colors (higher)

					color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) - (sizey * sizex) + (i++)]) & 0xff) >> 0);
					color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) - (sizey * sizex) + (j++)]) & 0xff) >> 0);

					// src xxxxxxxx_bbgggrrr
					// dst xbbbbbgg_gggrrrrr

					r = ((color1 & 0x07) >> 0);
					g = ((color1 & 0x38) >> 3);
					b = ((color1 & 0xc0) >> 6);
					tflag1 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor1 |= ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));

					r = ((color2 & 0x07) >> 0);
					g = ((color2 & 0x38) >> 3);
					b = ((color2 & 0xc0) >> 6);
					tflag2 = ((r == 0x07) && (g == 0x07) && (b == 0x03)) ? 0 : 1;
					drawcolor2 |= ((b << (10 + 3)) | (g << (5 + 2)) | (r << (0 + 2)));
				}
			}
			else
			{
				// Palettized picture mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);

				// src xxxxbbbb_ggggrrrr
				// dst xbbbbbgg_gggrrrrr

				r = ((mjsikaku_palette[(color1 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x01) >> 0);
				g = ((mjsikaku_palette[(color1 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x02) >> 1);
				b = ((mjsikaku_palette[(color1 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x0c) >> 2);
				tflag1 = ((r == 0x0f) && (g == 0x0f) && (b == 0x0f)) ? 0 : 1;
				drawcolor1 = ((b << (10 + 1)) | (g << (5 + 1)) | (r << (0 + 1)));

				r = ((mjsikaku_palette[(color2 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x01) >> 0);
				g = ((mjsikaku_palette[(color2 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x02) >> 1);
				b = ((mjsikaku_palette[(color2 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x0c) >> 2);
				tflag2 = ((r == 0x0f) && (g == 0x0f) && (b == 0x0f)) ? 0 : 1;
				drawcolor2 = ((b << (10 + 1)) | (g << (5 + 1)) | (r << (0 + 1)));
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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;

}

static void crystal2_gfxdraw(void)
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

	flipx = (mjsikaku_dispflag & 0x01) ? 1 : 0;
	flipy = (mjsikaku_dispflag & 0x02) ? 1 : 0;

	if (flipx)
	{
		startx = 0;
		sizex = (((mjsikaku_sizex - 1) ^ 0xff) & 0xff);
		skipx = 1;
		mjsikaku_drawx = ((mjsikaku_drawx - sizex) & 0xff);
	}
	else
	{
		startx = (mjsikaku_sizex & 0xff);
		sizex = (startx + 1);
		skipx = -1;
	}

	if (flipy)
	{
		starty = 0;
		sizey = (((mjsikaku_sizey - 1) ^ 0xff) & 0xff);
		skipy = 1;
		mjsikaku_drawy = ((mjsikaku_drawy - sizey) & 0xff);
	}
	else
	{
		starty = (mjsikaku_sizey & 0xff);
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

			if (mjsikaku_dispflag2 & 0x04)
			{
				// 65536 colors mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xff) >> 0);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0xff) >> 0);

				drawcolor1 = color1;
				drawcolor2 = color2;
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
			}
			else
			{
				// Palettized picture mode
				color1 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (i++)]) & 0xf0) >> 4);
				color2 = (((GFX[(0x20000 * mjsikaku_gfxrom) + ((0x0200 * mjsikaku_radry) + (0x0002 * mjsikaku_radrx)) + (j++)]) & 0x0f) >> 0);

				drawcolor1 = mjsikaku_palette[color1];
				drawcolor2 = mjsikaku_palette[color2];
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
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

				x1 = (((mjsikaku_drawx * 2) + (x * 2)) & 0x1ff);
				x2 = ((((mjsikaku_drawx * 2) + (x * 2)) + 1) & 0x1ff);
				yy = ((mjsikaku_drawy + y) & 0xff);

				if (tflag1)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x1] = drawcolor1;
					plot_pixel(tmpbitmap1, x1, yy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					mjsikaku_videoram[(yy * Machine->drv->screen_width) + x2] = drawcolor2;
					plot_pixel(tmpbitmap1, x2, yy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 600) ? 0 : 1;

}

/******************************************************************/


/******************************************************************/
int mjsikaku_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 0;
	return 0;
}

int secolove_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 1;
	return 0;
}

int bijokkoy_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 4;
	return 0;
}

int housemnq_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 5;
	return 0;
}

int seiha_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 2;
	return 0;
}

int crystal2_vh_start(void)
{
	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 3;
	return 0;
}

void mjsikaku_vh_stop(void)
{
	free(mjsikaku_videoram);
	bitmap_free(tmpbitmap1);
	mjsikaku_videoram = 0;
	tmpbitmap1 = 0;
}

void mjsikaku_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y;
	unsigned short color;

	if (full_refresh)
	{
		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = mjsikaku_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(tmpbitmap1, x, y, Machine->pens[color]);
			}
		}
	}

	if (!(mjsikaku_dispflag & 0x08))	// display enable ?
	{
		copyscrollbitmap(bitmap, tmpbitmap1, 0, 0, 1, &mjsikaku_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0], 0);
	}
}
