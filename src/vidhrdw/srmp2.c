/***************************************************************************

Functions to emulate the video hardware of the machine.

  Video hardware is very similar with "seta" hardware except color PROM.

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"




int srmp2_color_bank;


void srmp2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	for (i = 0 ; i < Machine->drv->total_colors ; i++)
	{
		int col, r, g, b, n;

		n = i ^ 0x0f;

		col = (color_prom[i] << 8) + color_prom[i + Machine->drv->total_colors];

		r = (col & 0x7c00) >> 10;
		g = (col & 0x03e0) >> 5;
		b = (col & 0x001f);

		*(palette++) = (r << 3) | (r >> 2);
		*(palette++) = (g << 3) | (g >> 2);
		*(palette++) = (b << 3) | (b >> 2);
	}

	for (i = 0 ; i < Machine->drv->total_colors ; i ++)
	{
		colortable[i] = i ^ 0x0f;
	}
}


static void seta_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;
	int xoffs, yoffs;

	int ctrl	=	spriteram16[ 0x600/2 ];
	int ctrl2	=	spriteram16[ 0x602/2 ];

	int flip	=	ctrl & 0x40;

	/* Sprites Banking and/or Sprites Buffering */
	data16_t *src = spriteram16_2 + ( ((ctrl2 ^ (~ctrl2<<1)) & 0x40) ? 0x2000/2 : 0 );

	int max_y	=	Machine -> drv -> screen_height;

	xoffs	=	flip ? 0x10 : 0x10;
	yoffs	=	flip ? 0x06 : 0x06;

	for ( offs = (0x400-2)/2 ; offs >= 0/2; offs -= 2/2 )
	{
		int	code	=	src[offs + 0x000/2];
		int	x		=	src[offs + 0x400/2];

		int	y		=	spriteram16[offs + 0x000/2] & 0xff;

		int	flipx	=	code & 0x8000;
		int	flipy	=	code & 0x4000;

		int color   = (x >> 11) & 0x1f;

		if (flip)
		{
			y = max_y - y
				+(Machine->drv->screen_height-(Machine->visible_area.max_y + 1));
			flipx = !flipx;
			flipy = !flipy;
		}

		code = code & 0x3fff;

		if (srmp2_color_bank & 0x80)
		{
			color |= 0x20;
		}

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx, flipy,
				(x + xoffs) & 0x1ff,
				max_y - ((y + yoffs) & 0x0ff),
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}

}


void srmp2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	fillbitmap(bitmap, Machine->pens[0x1f0], &Machine->visible_area);
	seta_draw_sprites(bitmap);
}
