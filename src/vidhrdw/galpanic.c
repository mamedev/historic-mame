#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *galpanic_bgvideoram,*galpanic_fgvideoram;
int galpanic_fgvideoram_size;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int galpanic_vh_start(void)
{
	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void galpanic_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



int galpanic_bgvideoram_r(int offset)
{
	return READ_WORD(&galpanic_bgvideoram[offset]);
}

void galpanic_bgvideoram_w(int offset,int data)
{
	int sx,sy,color,r,g,b;
#define rgbpenindex(r,g,b) ((Machine->scrbitmap->depth==16) ? ((((r)>>3)<<10)+(((g)>>3)<<5)+((b)>>3)) : ((((r)>>5)<<5)+(((g)>>5)<<2)+((b)>>6)))
extern unsigned short *shrinked_pens;


	COMBINE_WORD_MEM(&galpanic_bgvideoram[offset],data);


	sy = (offset/2) / 256;
	sx = (offset/2) % 256;
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;


		temp = sx;
		sx = sy;
		sy = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		sx = tmpbitmap->width - 1 - sx;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		sy = tmpbitmap->height - 1 - sy;

	color = READ_WORD(&galpanic_bgvideoram[offset]);

	r = (color >>  6) & 0x1f;
	g = (color >> 11) & 0x1f;
	b = (color >>  1) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	if (tmpbitmap->depth==16)
		((unsigned short *)tmpbitmap->line[sy])[sx] = shrinked_pens[rgbpenindex(r,g,b)];
	else
		tmpbitmap->line[sy][sx] = shrinked_pens[rgbpenindex(r,g,b)];
}

int galpanic_fgvideoram_r(int offset)
{
	return READ_WORD(&galpanic_fgvideoram[offset]);
}

void galpanic_fgvideoram_w(int offset,int data)
{
	COMBINE_WORD_MEM(&galpanic_fgvideoram[offset],data);
}

int galpanic_paletteram_r(int offset)
{
	return READ_WORD(&paletteram[offset]);
}

void galpanic_paletteram_w(int offset,int data)
{
	int r,g,b;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	WRITE_WORD(&paletteram[offset],newword);

	r = (newword >>  6) & 0x1f;
	g = (newword >> 11) & 0x1f;
	b = (newword >>  1) & 0x1f;
	/* bit 0 seems to be a transparency flag for the front bitmap */

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset / 2,r,g,b);
}

int galpanic_spriteram_r(int offset)
{
	return READ_WORD(&spriteram[offset]);
}

void galpanic_spriteram_w(int offset,int data)
{
	COMBINE_WORD_MEM(&spriteram[offset],data);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galpanic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;


	palette_recalc();

	/* copy the temporary bitmap to the screen */
	/* it's raw RGB, so it doesn't have to be recalculated even if palette_recalc() */
	/* returns true */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		for (offs = 0;offs < galpanic_fgvideoram_size;offs+=2)
		{
			int color;


			sx = (offs/2) / 256;
			sy = (offs/2) % 256;
			if (Machine->orientation & ORIENTATION_FLIP_X)
				sx = bitmap->width - 1 - sx;
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				sy = bitmap->height - 1 - sy;

			color = READ_WORD(&galpanic_fgvideoram[offs]);
			if (color)
			{
				if (bitmap->depth==16)
					((unsigned short *)bitmap->line[sy])[sx] = Machine->pens[color];
				else
					bitmap->line[sy][sx] = Machine->pens[color];
			}
		}
	}
	else
	{
		for (offs = 0;offs < galpanic_fgvideoram_size;offs+=2)
		{
			int color;


			sx = (offs/2) % 256;
			sy = (offs/2) / 256;
			if (Machine->orientation & ORIENTATION_FLIP_X)
				sx = bitmap->width - 1 - sx;
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				sy = bitmap->height - 1 - sy;

			color = READ_WORD(&galpanic_fgvideoram[offs]);
			if (color)
			{
				if (bitmap->depth==16)
					((unsigned short *)bitmap->line[sy])[sx] = Machine->pens[color];
				else
					bitmap->line[sy][sx] = Machine->pens[color];
			}
		}
	}


	sx = sy = 0;
	for (offs = 0;offs < spriteram_size;offs += 0x10)
	{
		int x,y,code,color,flipx,flipy;


		x = READ_WORD(&spriteram[offs + 8]) - ((READ_WORD(&spriteram[offs + 6]) & 0x01) << 8);
		y = READ_WORD(&spriteram[offs + 10]) + ((READ_WORD(&spriteram[offs + 6]) & 0x02) << 7);
		if (READ_WORD(&spriteram[offs + 6]) & 0x04)	/* multi sprite */
		{
			sx += x;
			sy += y;
		}
		else
		{
			sx = x;
			sy = y;
		}

		/* bit 0 [offs + 0] is used but I don't know what for */

		code = READ_WORD(&spriteram[offs + 12]) + ((READ_WORD(&spriteram[offs + 14]) & 0x1f) << 8);
		color = (READ_WORD(&spriteram[offs + 6]) & 0xf0) >> 4;
		flipx = READ_WORD(&spriteram[offs + 14]) & 0x80;
		flipy = READ_WORD(&spriteram[offs + 14]) & 0x40;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				sx,sy - 16,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
