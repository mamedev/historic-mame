/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

updated: 02-06-98 HJB moved Astro Invaders specific to z80bw.c
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *invaders_videoram;
static int flipped = 0;

/* palette colors (see drivers/8080bw.c) */
enum { BLACK, RED, GREEN, YELLOW, WHITE, CYAN, PURPLE };


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int invaders_vh_start (void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void invaders_vh_stop (void)
{
	osd_free_bitmap (tmpbitmap);
}

void invaders_vh_flipscreen(int data)
{
	if (input_port_3_r(0) & 0x01)
	{
		if (data != flipped)
		{
			int x,y;
			int x_size = 256, y_size = 224;
			int x_start = 0, y_start = 0;
			int x_last, y_last;
			if (Machine->orientation & ORIENTATION_SWAP_XY)
			{
				int tw = x_size;
				x_size = y_size, y_size = tw;
			}
			if (Machine->orientation & ORIENTATION_FLIP_X)
				x_start = 256-x_size;
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				y_start = 256-y_size;
			x_last = x_start+x_size-1;
			y_last = y_start+y_size-1;
			for (y = 0; y < y_size; y++)
			{
				for (x = 0; x < x_size/2; x++)
				{
					if (tmpbitmap->depth == 16)
					{
						int col = ((unsigned short *)tmpbitmap->line[y_last-y])[x_last-x];
						((unsigned short *)Machine->scrbitmap->line[y_last-y])[x_last-x] =
								((unsigned short *)tmpbitmap->line[y_last-y])[x_last-x] =
								((unsigned short *)tmpbitmap->line[y_start+y])[x_start+x];
						((unsigned short *)Machine->scrbitmap->line[y_start+y])[x_start+x] =
								((unsigned short *)tmpbitmap->line[y_start+y])[x_start+x] = col;
					}
					else
					{
						int col = tmpbitmap->line[y_last-y][x_last-x];
						Machine->scrbitmap->line[y_last-y][x_last-x] = tmpbitmap->line[y_last-y][x_last-x] = tmpbitmap->line[y_start+y][x_start+x];
						Machine->scrbitmap->line[y_start+y][x_start+x] = tmpbitmap->line[y_start+y][x_start+x] = col;
					}
				}
			}
			osd_mark_dirty (x_start,y_start,x_last,y_last,0);
			flipped = data;
		}
	}
}

static void plot_pixel_8080 (int x, int y, int col)
{
	if (flipped)
	{
		x = 255-x;
		y = 223-y;
	}

	plot_pixel2(Machine->scrbitmap,tmpbitmap,x,y,col);
}

void invaders_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		/* Calculate overlay color for this byte */
		col = Machine->pens[WHITE];
		if (x >= 16 && x < 72) col = Machine->pens[GREEN];
		if (x < 16 && y > 16 && y < 134) col = Machine->pens[GREEN];
		if (x >= 192 && x < 224) col = Machine->pens[RED];

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, col);

			x++;
			data >>= 1;
		}
	}
}

void invdpt2m_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		/* Calculate overlay color for this byte */
		col = Machine->pens[WHITE];
		if (x >= 16 && x < 72) col = Machine->pens[GREEN];
		if (x < 16 && y > 16 && y < 134) col = Machine->pens[GREEN];
                if (x >= 72 && x < 192) col = Machine->pens[YELLOW];
		if (x >= 192 && x < 224) col = Machine->pens[RED];

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, col);

			x ++;
			data >>= 1;
		}
	}
}

void invrvnge_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		/* Calculate overlay color for this byte */
		col = Machine->pens[WHITE];
		if (x < 72) col = Machine->pens[GREEN];
		if (x >= 192 && x < 224) col = Machine->pens[RED];

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, col);

			x ++;
			data >>= 1;
		}
	}
}


void lrescue_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		/* Calculate overlay color for this byte */
		col = Machine->pens[WHITE];

		if (x >= 240 && x < 248)
		{
			if (y < 72) col = Machine->pens[CYAN];
			if (y >= 72 && y < 152) col = Machine->pens[RED];
			if (y >= 152) col = Machine->pens[YELLOW];
		}
		if (x >= 232 && x < 240)
		{
			if (y < 72) col = Machine->pens[CYAN];
			if (y >= 72 && y < 160) col = Machine->pens[GREEN];
			if (y >= 160) col = Machine->pens[YELLOW];
		}
		if (x >= 224 && x < 232)
		{
			if (y >= 72 && y < 160) col = Machine->pens[GREEN];     /* or 152? */
			if (y >= 160) col = Machine->pens[YELLOW];
		}

		if (x >= 216 && x < 224) col = Machine->pens[RED];
		if (x >= 192 && x < 216) col = Machine->pens[PURPLE];
		if (x >= 160 && x < 192) col = Machine->pens[GREEN];
		if (x >= 128 && x < 160) col = Machine->pens[CYAN];
		if (x >= 96 && x < 128) col = Machine->pens[PURPLE];
		if (x >= 64 && x < 96) col = Machine->pens[YELLOW];
		if (x >= 40 && x < 64) col = Machine->pens[RED];
		if (x >= 24 && x < 40) col = Machine->pens[CYAN];
		if (x >= 16 && x < 24) col = Machine->pens[RED];
		if (x < 16)
		{
			if (y < 136) col = Machine->pens[CYAN];
			if (y >= 136 && y < 192) col = Machine->pens[PURPLE];
			if (y >= 192) col = Machine->pens[CYAN];
		}
		if (y == 223) col = Machine->pens[BLACK];

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, col);

			x ++;
			data >>= 1;
		}
	}
}


void rollingc_videoram_w (int offset,int data)
{
	/* TODO: get rid of this */
	unsigned char *RAM = memory_region(REGION_CPU1);


	if (invaders_videoram[offset] != data)
	{
		int i,x,y;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[16]);
			else
				plot_pixel_8080 (x, y, Machine->pens[RAM[0xa400 + offset] & 0x0f]);

			x ++;
			data >>= 1;
		}
	}
}


/* Bandido has an optional colour kit than can be added on  */  /* MJC */
/* this includes a rom image that we do not yet have, so .. */

void boothill_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, Machine->pens[WHITE]);

			x ++;
			data >>= 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	palette_recalc();

	if (full_refresh)
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

void seawolf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y,centre,middle;

	/* Update the Bitmap (and erase old cross) */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw the Sight */

	centre = ((input_port_0_r(0) & 0x1f) * 8) + 4;
    middle = 31;

    if (centre<2)   centre=2;
    if (centre>253) centre=253;

	middle = 31;

	for(y=middle-10;y<middle+11;y++)
		plot_pixel(bitmap,centre,y,Machine->pens[GREEN]);

	for(x=centre-20;x<centre+21;x++)
		if((x>0) && (x<256))
			plot_pixel(bitmap,x,middle,Machine->pens[GREEN]);
}

void blueshrk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y,centre,middle;

	/* Update the Bitmap (and erase old cross) */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw the Sight */

	centre = (((input_port_1_r(0)) & 0x7f) * 2) - 12;
    middle = 31;

    if (centre<2)   centre=2;
    if (centre>253) centre=253;

	middle = 31;

	for(y=middle-10;y<middle+11;y++)
		plot_pixel(bitmap,centre,y,Machine->pens[GREEN]);

	for(x=centre-20;x<centre+21;x++)
		if((x>0) && (x<256))
			plot_pixel(bitmap,x,middle,Machine->pens[GREEN]);
}

/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

static const unsigned char *colourrom;

void ballbomb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	colourrom = color_prom + 128;

    *(palette++) = 0;
    *(palette++) = 0;
    *(palette++) = 0;

	for (i = 1;i < Machine->drv->total_colors-1;i++)
	{
		*(palette++) = 0xdf * (((i >> 0) & 1) == 0) + 0x20;
		*(palette++) = 0xdf * (((i >> 1) & 1) == 0) + 0x20;
		*(palette++) = 0xdf * (((i >> 2) & 1) == 0) + 0x20;
	}

    *(palette++) = 0xff;
    *(palette++) = 0xff;
    *(palette++) = 0xff;
}

void ballbomb_videoram_w (int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		invaders_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

        /* 32 x 32 colourmap */

        col = colourrom[((y / 8) * 32) + (x / 8)] & 7;
        col = Machine->pens[col];

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x01))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, col);

			x ++;
			data >>= 1;
		}
	}
}
