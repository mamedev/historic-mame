/******************************************************************************

Ikki (c) 1985 Sun Electronics

Video hardware driver by Uki

	20/Jun/2001 -

******************************************************************************/

#include "vidhrdw/generic.h"

static UINT8 ikki_flipscreen, ikki_scroll[2];

void ikki_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	int colors = Machine->drv->total_colors-1;

	for (i = 0; i<colors; i++)
	{
		*(palette++) = color_prom[0]*0x11;
		*(palette++) = color_prom[colors]*0x11;
		*(palette++) = color_prom[2*colors]*0x11;

		color_prom++;
	}

		*(palette++) = 0; /* 256th color is not drawn on screen */
		*(palette++) = 0; /* this is used for special transparent function */
		*(palette++) = 1;

	color_prom += 2*colors;

	/* color_prom now points to the beginning of the lookup table */

	/* sprites lookup table */
	for (i=0; i<512; i++)
	{
		int d = 255-*(color_prom++);
		if ( ((i % 8) == 7) && (d == 0) )
			*(colortable++) = 256; /* special transparent */
		else
			*(colortable++) = d; /* normal color */
	}

	/* bg lookup table */
	for (i=0; i<512; i++)
		*(colortable++) = *(color_prom++);

}

WRITE_HANDLER( ikki_scroll_w )
{
	ikki_scroll[offset] = data;
}

WRITE_HANDLER( ikki_scrn_ctrl_w )
{
	ikki_flipscreen = (data >> 2) & 1;
}

void ikki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{

	int offs,chr,col,px,py,f,bank,d;
	data8_t *VIDEOATTR = memory_region( REGION_USER1 );

	f = ikki_flipscreen;

	/* draw bg layer */

	for (offs=0; offs<(videoram_size/2); offs++)
	{
		int sx,sy;

		sx = offs / 32;
		sy = offs % 32;

		py = sy*8;
		px = sx*8;

		d = VIDEOATTR[ sx ];

		switch (d)
		{
			case 0x02: /* scroll area */
				px = sx*8 - ikki_scroll[1];
				if (px<0)
					px=px+8*22;
				py = (sy*8 + ~ikki_scroll[0]) & 0xff;
				break;

			case 0x03: /* non-scroll area */
				break;

			case 0x00: /* sprite disable? */
				break;

			case 0x0d: /* sprite disable? */
				break;

			case 0x0b: /* non-scroll area (?) */
				break;

			case 0x0e: /* unknown */
				break;
		}

		if (f != 0)
		{
			px = 248-px;
			py = 248-py;
		}

		col = videoram[offs*2];
		bank = (col & 0xe0) << 3;
		col = ((col & 0x1f)<<0) | ((col & 0x80) >> 2);

		drawgfx(bitmap,Machine->gfx[0],
			videoram[offs*2+1] + bank,
			col,
			f,f,
			px,py,
			&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

/* draw sprites */

	fillbitmap(tmpbitmap, Machine->pens[256], 0);

	/* c060 - c0ff */
	for (offs=0x00; offs<0x800; offs +=4)
	{
		chr = spriteram[offs+1] >> 1 ;
		col = spriteram[offs+2];

		px = spriteram[offs+3];
		py = spriteram[offs+0];

		chr += (col & 0x80);
		col = (col & 0x3f) >> 0 ;

		if (f==0)
			py = 224-py;
		else
			px = 240-px;

		px = px & 0xff;
		py = py & 0xff;

		if (px>248)
			px = px-256;
		if (py>240)
			py = py-256;

		drawgfx(tmpbitmap,Machine->gfx[1],
			chr,
			col,
			f,f,
			px,py,
			&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_COLOR,256);


	/* mask sprites */

	for (offs=0; offs<(videoram_size/2); offs++)
	{
		int sx,sy;

		sx = offs / 32;
		sy = offs % 32;

		d = VIDEOATTR[ sx ];

		if ( (d == 0) || (d == 0x0d) )
		{
			py = sy*8;
			px = sx*8;

			if (f != 0)
			{
				px = 248-px;
				py = 248-py;
			}

			col = videoram[offs*2];
			bank = (col & 0xe0) << 3;
			col = ((col & 0x1f)<<0) | ((col & 0x80) >> 2);

			drawgfx(bitmap,Machine->gfx[0],
				videoram[offs*2+1] + bank,
				col,
				f,f,
				px,py,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

}
