/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/nb1413m3.h"


static int mjsikaku_scrolly;
static int mjsikaku_drawx, mjsikaku_drawy;
static int mjsikaku_sizex, mjsikaku_sizey;
static int mjsikaku_radrx, mjsikaku_radry;
static int mjsikaku_gfxrom;
static int mjsikaku_dispflag;
static int mjsikaku_gfxflag1;
static int mjsikaku_gfxflag2;
static int mjsikaku_gfxflag3;
static int mjsikaku_flipscreen;
static int mjsikaku_flipx, mjsikaku_flipy;
static int mjsikaku_screen_refresh;
static int mjsikaku_gfxmode;

static struct mame_bitmap *mjsikaku_tmpbitmap;
static unsigned short *mjsikaku_videoram;
static unsigned short *mjsikaku_videoworkram;
static unsigned char *mjsikaku_palette;


static void mjsikaku_vramflip(void);
static void mjsikaku_gfxdraw(void);
static void secolove_gfxdraw(void);
static void bijokkoy_gfxdraw(void);
static void seiha_gfxdraw(void);
static void crystal2_gfxdraw(void);


/******************************************************************************


******************************************************************************/
void mjsikaku_init_palette(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* initialize 444 RGB lookup */
	for (i = 0; i < 4096; i++)
	{
		int r,g,b;

		// xxxxbbbb_ggggrrrr
		r = ((i >> 0) & 0x0f);
		g = ((i >> 4) & 0x0f);
		b = ((i >> 8) & 0x0f);

		r = ((r << 4) | r);
		g = ((g << 4) | g);
		b = ((b << 4) | b);

		palette_set_color(i,r,g,b);
	}
}

void seiha_init_palette(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* initialize 655 RGB lookup */
	for (i = 0; i < 65536; i++)
	{
		int r, g, b;

		// bbbbbggg_ggrrrrrr
		r = ((i >>  0) & 0x3f);
		g = ((i >>  6) & 0x1f);
		b = ((i >> 11) & 0x1f);

		r = ((r << 2) | (r >> 4));
		g = ((g << 3) | (g >> 2));
		b = ((b << 3) | (b >> 2));

		palette_set_color(i,r,g,b);
	}
}

void crystal2_init_palette(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* initialize 332 RGB lookup */
	for (i = 0; i < 256; i++)
	{
		int bit0,bit1,bit2,r,g,b;

		// xxxxxxxx_bbgggrrr
		/* red component */
		bit0 = ((i >> 0) & 0x01);
		bit1 = ((i >> 1) & 0x01);
		bit2 = ((i >> 2) & 0x01);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = ((i >> 3) & 0x01);
		bit1 = ((i >> 4) & 0x01);
		bit2 = ((i >> 5) & 0x01);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = ((i >> 6) & 0x01);
		bit2 = ((i >> 7) & 0x01);
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
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

/******************************************************************************


******************************************************************************/
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

	switch (mjsikaku_gfxmode)
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
	}
}

void mjsikaku_gfxflag1_w(int data)
{
	static int mjsikaku_flipscreen_old = -1;

	mjsikaku_gfxflag1 = data;

	mjsikaku_flipx = (data & 0x01) ? 1 : 0;
	mjsikaku_flipy = (data & 0x02) ? 1 : 0;
	mjsikaku_flipscreen = (data & 0x04) ? 0 : 1;
	mjsikaku_dispflag = (data & 0x08) ? 0 : 1;

	if ((nb1413m3_type == NB1413M3_MJSIKAKU) ||
	    (nb1413m3_type == NB1413M3_OTONANO) ||
	    (nb1413m3_type == NB1413M3_SECOLOVE) ||
	    (nb1413m3_type == NB1413M3_CITYLOVE) ||
	    (nb1413m3_type == NB1413M3_SEIHA) ||
	    (nb1413m3_type == NB1413M3_SEIHAM) ||
	    (nb1413m3_type == NB1413M3_IEMOTO) ||
	    (nb1413m3_type == NB1413M3_OJOUSAN) ||
	    (nb1413m3_type == NB1413M3_BIJOKKOY) ||
	    (nb1413m3_type == NB1413M3_HOUSEMNQ) ||
	    (nb1413m3_type == NB1413M3_HOUSEMN2) ||
	    (nb1413m3_type == NB1413M3_CRYSTAL2) ||
	    (nb1413m3_type == NB1413M3_APPAREL))
	{
		mjsikaku_flipscreen ^= 1;
	}

	if (mjsikaku_flipscreen != mjsikaku_flipscreen_old)
	{
		mjsikaku_vramflip();
		mjsikaku_screen_refresh = 1;
		mjsikaku_flipscreen_old = mjsikaku_flipscreen;
	}
}

void mjsikaku_gfxflag2_w(int data)
{
	mjsikaku_gfxflag2 = data;

	if (nb1413m3_type == NB1413M3_SEIHAM) mjsikaku_gfxflag2 ^= 0x20;
}

void mjsikaku_gfxflag3_w(int data)
{
	mjsikaku_gfxflag3 = (data & 0xe0);
}

void mjsikaku_drawx_w(int data)
{
	mjsikaku_drawx = (data ^ 0xff);
}

void mjsikaku_drawy_w(int data)
{
	if (mjsikaku_flipscreen) mjsikaku_drawy = ((data - 2) ^ 0xff) & 0xff;
	else mjsikaku_drawy = (data ^ 0xff);
}

void mjsikaku_scrolly_w(int data)
{
	if (mjsikaku_flipscreen) mjsikaku_scrolly = (data ^ 0xff);
	else mjsikaku_scrolly = (data - 1) & 0xff;
}

void mjsikaku_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x07);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

void secolove_romsel_w(int data)
{
	mjsikaku_gfxrom = ((data & 0xc0) >> 4) + (data & 0x03);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

void iemoto_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x07);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

void seiha_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x1f);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

void crystal2_romsel_w(int data)
{
	mjsikaku_gfxrom = (data & 0x03);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

/******************************************************************************


******************************************************************************/
void mjsikaku_vramflip(void)
{
	int x, y;
	unsigned short color1, color2;

	for (y = 0; y < (Machine->drv->screen_height / 2); y++)
	{
		for (x = 0; x < Machine->drv->screen_width; x++)
		{
			color1 = mjsikaku_videoram[(y * Machine->drv->screen_width) + x];
			color2 = mjsikaku_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)];
			mjsikaku_videoram[(y * Machine->drv->screen_width) + x] = color2;
			mjsikaku_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)] = color1;

			color1 = mjsikaku_videoworkram[(y * Machine->drv->screen_width) + x];
			color2 = mjsikaku_videoworkram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)];
			mjsikaku_videoworkram[(y * Machine->drv->screen_width) + x] = color2;
			mjsikaku_videoworkram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)] = color1;
		}
	}
}

static void mjsikaku_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short r, g, b;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (mjsikaku_gfxflag2 & 0x20) return;

	if (mjsikaku_flipx)
	{
		mjsikaku_drawx -= (mjsikaku_sizex << 1);
		startx = mjsikaku_sizex;
		sizex = ((mjsikaku_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		mjsikaku_drawx = (mjsikaku_drawx - mjsikaku_sizex);
		startx = 0;
		sizex = (mjsikaku_sizex + 1);
		skipx = 1;
	}

	if (mjsikaku_flipy)
	{
		mjsikaku_drawy -= ((mjsikaku_sizey << 1) + 1);
		starty = mjsikaku_sizey;
		sizey = ((mjsikaku_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		mjsikaku_drawy = (mjsikaku_drawy - mjsikaku_sizey - 1);
		starty = 0;
		sizey = (mjsikaku_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((mjsikaku_gfxrom << 17) + (mjsikaku_radry << 9) + (mjsikaku_radrx << 1));

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (mjsikaku_flipscreen)
			{
				dx1 = (((((mjsikaku_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((mjsikaku_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((mjsikaku_drawy + y + mjsikaku_scrolly) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((mjsikaku_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((mjsikaku_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((mjsikaku_drawy + y + (-mjsikaku_scrolly & 0xff)) & 0xff);
			}

			if (mjsikaku_flipx)
			{
				// flip
				color1 = (color & 0xf0) >> 4;
				color2 = (color & 0x0f) >> 0;
			}
			else
			{
				// normal
				color1 = (color & 0x0f) >> 0;
				color2 = (color & 0xf0) >> 4;
			}

			r = ((mjsikaku_palette[(color1 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x01) >> 0);
			g = ((mjsikaku_palette[(color1 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x02) >> 1);
			b = ((mjsikaku_palette[(color1 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color1 << 1) + 1] & 0x0c) >> 2);

			drawcolor1 = (((b << 8) | (g << 4) | (r << 0)) & 0x0fff);

			r = ((mjsikaku_palette[(color2 << 1) + 0] & 0x07) << 1) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x01) >> 0);
			g = ((mjsikaku_palette[(color2 << 1) + 0] & 0x38) >> 2) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x02) >> 1);
			b = ((mjsikaku_palette[(color2 << 1) + 0] & 0xc0) >> 4) | ((mjsikaku_palette[(color2 << 1) + 1] & 0x0c) >> 2);

			drawcolor2 = (((b << 8) | (g << 4) | (r << 0)) & 0x0fff);

			tflag1 = tflag2 = 0;

			if (drawcolor1 != 0x0fff) tflag1 = 1;
			if (drawcolor2 != 0x0fff) tflag2 = 1;

			if (tflag1)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(mjsikaku_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(mjsikaku_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;
}

static void secolove_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (mjsikaku_flipx)
	{
		mjsikaku_drawx -= (mjsikaku_sizex << 1);
		startx = mjsikaku_sizex;
		sizex = ((mjsikaku_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		mjsikaku_drawx = (mjsikaku_drawx - mjsikaku_sizex);
		startx = 0;
		sizex = (mjsikaku_sizex + 1);
		skipx = 1;
	}

	if (mjsikaku_flipy)
	{
		mjsikaku_drawy -= ((mjsikaku_sizey << 1) + 1);
		starty = mjsikaku_sizey;
		sizey = ((mjsikaku_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		mjsikaku_drawy = (mjsikaku_drawy - mjsikaku_sizey - 1);
		starty = 0;
		sizey = (mjsikaku_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((mjsikaku_gfxrom << 17) + (mjsikaku_radry << 9) + (mjsikaku_radrx << 1));

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (mjsikaku_flipscreen)
			{
				dx1 = (((((mjsikaku_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((mjsikaku_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((mjsikaku_drawy + y + mjsikaku_scrolly) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((mjsikaku_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((mjsikaku_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((mjsikaku_drawy + y + (-mjsikaku_scrolly & 0xff)) & 0xff);
			}

			if (mjsikaku_gfxflag2 & 0x04)
			{
				// direct mode

				color1 = color;
				color2 = color;
			}
			else
			{
				// palette mode

				if (mjsikaku_flipx)
				{
					// flip
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}
				else
				{
					// normal
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}

				color1 = mjsikaku_palette[color1];
				color2 = mjsikaku_palette[color2];
			}

			if (mjsikaku_gfxflag2 & 0x20)
			{
				// vram lower draw

				if (mjsikaku_gfxflag2 & 0x10)
				{
					// 4096 colors low mode (2nd draw upper)
					color1 = color2 = mjsikaku_palette[((color & 0xf0) >> 4)];
				}
				else
				{
					// 4096 colors low mode (1st draw lower)
					color1 = color2 = mjsikaku_palette[((color & 0x0f) >> 0)];
				}

				tflag1 = tflag2 = 0;

				if (color1 != 0xff)
				{
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0xff00;
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= (color1 & 0x00ff);
					tflag1 = 1;
				}

				if (color2 != 0xff)
				{
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0xff00;
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= (color2 & 0x00ff);
					tflag1 = 2;
				}
			}
			else
			{
				// vram higher draw

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0x00ff;
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0x00ff;

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= ((color1 << 8) & 0xff00);
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= ((color2 << 8) & 0xff00);
			}

			color1 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1];
			color2 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2];

			tflag1 = tflag2 = 0;

			if ((color1 & 0xff00) != 0xff00) tflag1 = 1;
			if ((color2 & 0xff00) != 0xff00) tflag2 = 1;

			drawcolor1  = ((((color1 & 0x0001) >> 0) & 0x0001) | (((color1 & 0x0700) >> 7) & 0x000e));	// R 4bit
			drawcolor1 |= ((((color1 & 0x0002) << 3) & 0x0010) | (((color1 & 0x3800) >> 6) & 0x00e0));	// G 4bit
			drawcolor1 |= ((((color1 & 0x000c) << 6) & 0x0300) | (((color1 & 0xc000) >> 4) & 0x0c00));	// B 4bit

			drawcolor2  = ((((color2 & 0x0001) >> 0) & 0x0001) | (((color2 & 0x0700) >> 7) & 0x000e));	// R 4bit
			drawcolor2 |= ((((color2 & 0x0002) << 3) & 0x0010) | (((color2 & 0x3800) >> 6) & 0x00e0));	// G 4bit
			drawcolor2 |= ((((color2 & 0x000c) << 6) & 0x0300) | (((color2 & 0xc000) >> 4) & 0x0c00));	// B 4bit

			if (tflag1)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(mjsikaku_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(mjsikaku_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;
}

static void bijokkoy_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (mjsikaku_flipx)
	{
		mjsikaku_drawx -= (mjsikaku_sizex << 1);
		startx = mjsikaku_sizex;
		sizex = ((mjsikaku_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		mjsikaku_drawx = (mjsikaku_drawx - mjsikaku_sizex);
		startx = 0;
		sizex = (mjsikaku_sizex + 1);
		skipx = 1;
	}

	if (mjsikaku_flipy)
	{
		mjsikaku_drawy -= ((mjsikaku_sizey << 1) + 1);
		starty = mjsikaku_sizey;
		sizey = ((mjsikaku_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		mjsikaku_drawy = (mjsikaku_drawy - mjsikaku_sizey - 1);
		starty = 0;
		sizey = (mjsikaku_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((mjsikaku_gfxrom << 17) + (mjsikaku_radry << 9) + (mjsikaku_radrx << 1));

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (mjsikaku_flipscreen)
			{
				dx1 = (((((mjsikaku_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((mjsikaku_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((mjsikaku_drawy + y + mjsikaku_scrolly) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((mjsikaku_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((mjsikaku_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((mjsikaku_drawy + y + (-mjsikaku_scrolly & 0xff)) & 0xff);
			}

			if (mjsikaku_gfxflag2 & 0x04)
			{
				// direct mode

				color1 = color;
				color2 = color;
			}
			else
			{
				// palette mode

				if (mjsikaku_flipx)
				{
					// flip
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}
				else
				{
					// normal
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}

				color1 = mjsikaku_palette[color1];
				color2 = mjsikaku_palette[color2];
			}

			if (mjsikaku_gfxflag2 & 0x20)
			{
				// vram lower draw

				if (color1 == 0xff) color1 = 0x0000;
				if (color2 == 0xff) color2 = 0x0000;

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0xff00;
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0xff00;

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= (color1 & 0x00ff);
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= (color2 & 0x00ff);
			}
			else
			{
				// vram higher draw

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0x00ff;
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0x00ff;

				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= ((color1 << 8) & 0xff00);
				mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= ((color2 << 8) & 0xff00);
			}

			color1 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1];
			color2 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2];

			tflag1 = tflag2 = 0;

			if ((color1 & 0xff00) != 0xff00) tflag1 = 1;
			if ((color2 & 0xff00) != 0xff00) tflag2 = 1;

			drawcolor1  = ((((color1 & 0x0007) >>  0) & 0x0007) | (((color1 & 0x0700) >>  5) & 0x0038));	// R 6bit
			drawcolor1 |= ((((color1 & 0x0018) <<  3) & 0x00c0) | (((color1 & 0x3800) >>  3) & 0x0700));	// G 5bit
			drawcolor1 |= ((((color1 & 0x00e0) <<  6) & 0x3800) | (((color1 & 0xc000) >>  0) & 0xc000));	// B 5bit

			drawcolor2  = ((((color2 & 0x0007) >>  0) & 0x0007) | (((color2 & 0x0700) >>  5) & 0x0038));	// R 6bit
			drawcolor2 |= ((((color2 & 0x0018) <<  3) & 0x00c0) | (((color2 & 0x3800) >>  3) & 0x0700));	// G 5bit
			drawcolor2 |= ((((color2 & 0x00e0) <<  6) & 0x3800) | (((color2 & 0xc000) >>  0) & 0xc000));	// B 5bit

			if (tflag1)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(mjsikaku_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(mjsikaku_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;
}

static void seiha_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short r, g, b;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (mjsikaku_flipx)
	{
		mjsikaku_drawx -= (mjsikaku_sizex << 1);
		startx = mjsikaku_sizex;
		sizex = ((mjsikaku_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		mjsikaku_drawx = (mjsikaku_drawx - mjsikaku_sizex);
		startx = 0;
		sizex = (mjsikaku_sizex + 1);
		skipx = 1;
	}

	if (mjsikaku_flipy)
	{
		mjsikaku_drawy -= ((mjsikaku_sizey << 1) + 1);
		starty = mjsikaku_sizey;
		sizey = ((mjsikaku_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		mjsikaku_drawy = (mjsikaku_drawy - mjsikaku_sizey - 1);
		starty = 0;
		sizey = (mjsikaku_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((mjsikaku_gfxrom << 17) + (mjsikaku_radry << 9) + (mjsikaku_radrx << 1));

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (mjsikaku_flipscreen)
			{
				dx1 = (((((mjsikaku_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((mjsikaku_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((mjsikaku_drawy + y + mjsikaku_scrolly) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((mjsikaku_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((mjsikaku_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((mjsikaku_drawy + y + (-mjsikaku_scrolly & 0xff)) & 0xff);
			}

			if (mjsikaku_gfxflag3 & 0x40)
			{
				// direct mode

				if (mjsikaku_gfxflag3 & 0x80)
				{
					// for direct lower (3-2-3 mode)

					// src xxxxxxxx_bbbggrrr
					// dst xxxxxxxx_bbbggrrr

					if (color == 0xff) color = 0x00;

					tflag1 = tflag2 = 1;

					color1 = color2 = color;

					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0xff00;
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0xff00;

					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= (color1 & 0x00ff);
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= (color2 & 0x00ff);
				}
				else
				{
					// for direct higher (3-3-2 mode)

					// src xxxxxxxx_bbgggrrr
					// dst bbgggrrr_xxxxxxxx

					tflag1 = tflag2 = 1;

					if (color == 0xff) color = 0x00;

					color1 = color2 = (color << 8);

					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] &= 0x00ff;
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] &= 0x00ff;

					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] |= (color1 & 0xff00);
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] |= (color2 & 0xff00);
				}
			}
			else
			{
				// palette mode

				// unknown flag (seiha, seiham)
			//	if (mjsikaku_gfxflag3 & 0x80) return;

				// unknown (seiha, seiham, iemoto, ojousan)
				if (!(mjsikaku_gfxflag2 & 0x20)) return;

				if (mjsikaku_flipx)
				{
					// flip
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}
				else
				{
					// normal
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}

				// for palette (4-4-4 mode)

				r = ((mjsikaku_palette[(color1 << 1) + 1] & 0x01) << 2);
				g = ((mjsikaku_palette[(color1 << 1) + 1] & 0x02) << 3);
				b = ((mjsikaku_palette[(color1 << 1) + 1] & 0x0c) << 4);

				color1 = (((mjsikaku_palette[(color1 << 1) + 0] & 0xff) << 8) | b | g | r);

				r = ((mjsikaku_palette[(color2 << 1) + 1] & 0x01) << 2);
				g = ((mjsikaku_palette[(color2 << 1) + 1] & 0x02) << 3);
				b = ((mjsikaku_palette[(color2 << 1) + 1] & 0x0c) << 4);

				color2 = (((mjsikaku_palette[(color2 << 1) + 0] & 0xff) << 8) | b | g | r);

				tflag1 = tflag2 = 0;

				if ((color1 & 0xffd4) != 0xffd4)
				{
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1] = color1;
					tflag1 = 1;
				}

				if ((color2 & 0xffd4) != 0xffd4)
				{
					mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2] = color2;
					tflag2 = 1;
				}
			}

			color1 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx1];
			color2 = mjsikaku_videoworkram[(dy * Machine->drv->screen_width) + dx2];

			// RGB = higher, rgb = lower
			// src BBGGGRRR_bbbggrrr
			// dst BBbbbGGG_ggRRRrrr

			drawcolor1  = (((color1 & 0x0007) >>  0) | ((color1 & 0x0700) >>  5));	// R 6bit
			drawcolor1 |= (((color1 & 0x0018) <<  3) | ((color1 & 0x3800) >>  3));	// G 5bit
			drawcolor1 |= (((color1 & 0x00e0) <<  6) | ((color1 & 0xc000) >>  0));	// B 5bit

			drawcolor2  = (((color2 & 0x0007) >>  0) | ((color2 & 0x0700) >>  5));	// R 6bit
			drawcolor2 |= (((color2 & 0x0018) <<  3) | ((color2 & 0x3800) >>  3));	// G 5bit
			drawcolor2 |= (((color2 & 0x00e0) <<  6) | ((color2 & 0xc000) >>  0));	// B 5bit

			if (tflag1)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(mjsikaku_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(mjsikaku_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;
}

static void crystal2_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (mjsikaku_flipx)
	{
		mjsikaku_drawx -= (mjsikaku_sizex << 1);
		startx = mjsikaku_sizex;
		sizex = ((mjsikaku_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		mjsikaku_drawx = (mjsikaku_drawx - mjsikaku_sizex);
		startx = 0;
		sizex = (mjsikaku_sizex + 1);
		skipx = 1;
	}

	if (mjsikaku_flipy)
	{
		mjsikaku_drawy -= ((mjsikaku_sizey << 1) + 1);
		starty = mjsikaku_sizey;
		sizey = ((mjsikaku_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		mjsikaku_drawy = (mjsikaku_drawy - mjsikaku_sizey - 1);
		starty = 0;
		sizey = (mjsikaku_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((mjsikaku_gfxrom << 17) + (mjsikaku_radry << 9) + (mjsikaku_radrx << 1));

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (mjsikaku_flipscreen)
			{
				dx1 = (((((mjsikaku_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((mjsikaku_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((mjsikaku_drawy + y + mjsikaku_scrolly) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((mjsikaku_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((mjsikaku_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((mjsikaku_drawy + y + (-mjsikaku_scrolly & 0xff)) & 0xff);
			}

			if (mjsikaku_gfxflag2 & 0x04)
			{
				// 65536 colors mode

				drawcolor1 = drawcolor2 = color;

				tflag1 = tflag2 = 0;

				if (drawcolor1 != 0xff) tflag1 = 1;
				if (drawcolor2 != 0xff) tflag2 = 1;
			}
			else
			{
				// Palettized picture mode

				if (mjsikaku_flipx)
				{
					// flip
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}
				else
				{
					// normal
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}

				drawcolor1 = mjsikaku_palette[color1];
				drawcolor2 = mjsikaku_palette[color2];

				tflag1 = tflag2 = 0;

				if (drawcolor1 != 0xff) tflag1 = 1;
				if (drawcolor2 != 0xff) tflag2 = 1;
			}

			if (tflag1)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(mjsikaku_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				mjsikaku_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(mjsikaku_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 600) ? 0 : 1;
}

/******************************************************************************


******************************************************************************/
int mjsikaku_vh_start(void)
{
	if ((mjsikaku_tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_videoworkram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_palette = malloc(0x20 * sizeof(char))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(mjsikaku_videoworkram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	mjsikaku_gfxmode = 0;
	return 0;
}

int secolove_vh_start(void)
{
	if ((mjsikaku_tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_videoworkram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_palette = malloc(0x20 * sizeof(char))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(mjsikaku_videoworkram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	mjsikaku_gfxmode = 1;
	return 0;
}

int bijokkoy_vh_start(void)
{
	if ((mjsikaku_tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_videoworkram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_palette = malloc(0x20 * sizeof(char))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(mjsikaku_videoworkram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	mjsikaku_gfxmode = 4;
	return 0;
}

int seiha_vh_start(void)
{
	if ((mjsikaku_tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_videoworkram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_palette = malloc(0x20 * sizeof(char))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(mjsikaku_videoworkram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	mjsikaku_gfxmode = 2;
	return 0;
}

int crystal2_vh_start(void)
{
	if ((mjsikaku_tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((mjsikaku_videoram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_videoworkram = malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mjsikaku_palette = malloc(0x20 * sizeof(char))) == 0) return 1;
	memset(mjsikaku_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(mjsikaku_videoworkram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	mjsikaku_gfxmode = 3;
	return 0;
}

void mjsikaku_vh_stop(void)
{
	free(mjsikaku_palette);
	free(mjsikaku_videoworkram);
	free(mjsikaku_videoram);
	bitmap_free(mjsikaku_tmpbitmap);
	mjsikaku_palette = 0;
	mjsikaku_videoworkram = 0;
	mjsikaku_videoram = 0;
	mjsikaku_tmpbitmap = 0;
}

/******************************************************************************


******************************************************************************/
void mjsikaku_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int x, y;
	unsigned short color;

	if (full_refresh || mjsikaku_screen_refresh)
	{
		mjsikaku_screen_refresh = 0;
		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = mjsikaku_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(mjsikaku_tmpbitmap, x, y, Machine->pens[color]);
			}
		}
	}

	if (mjsikaku_dispflag)
	{
		copyscrollbitmap(bitmap, mjsikaku_tmpbitmap, 0, 0, 1, &mjsikaku_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x0000], 0);
	}
}
