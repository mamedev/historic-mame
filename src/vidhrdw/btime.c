/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int background_image;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Burger Time doesn't have a color PROM. It uses RAM to dynamically
  create the palette.
  The palette RAM is connected to the RGB output this way:

  bit 7 -- 15 kohm resistor  -- BLUE (inverted)
        -- 33 kohm resistor  -- BLUE (inverted)
        -- 15 kohm resistor  -- GREEN (inverted)
        -- 33 kohm resistor  -- GREEN (inverted)
        -- 47 kohm resistor  -- GREEN (inverted)
        -- 15 kohm resistor  -- RED (inverted)
        -- 33 kohm resistor  -- RED (inverted)
  bit 0 -- 47 kohm resistor  -- RED (inverted)

***************************************************************************/
void btime_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}


	/* characters and sprites use colors 0-7 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* background tiles use colors 8-15 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
		COLOR(2,i) = i + 8;
}



void btime_paletteram_w(int offset,int data)
{
	int r,g,b;
	int bit0,bit1,bit2;


	r = (~data & 0x07);
	g = (~data & 0x38) >> 3;
	b = (~data & 0xC0) >> 6;

	bit0 = (r >> 0) & 0x01;
	bit1 = (r >> 1) & 0x01;
	bit2 = (r >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (g >> 0) & 0x01;
	bit1 = (g >> 1) & 0x01;
	bit2 = (g >> 2) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = 0;
	bit1 = (b >> 0) & 0x01;
	bit2 = (b >> 1) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	osd_modify_pen(Machine->pens[offset],r,g,b);
}



int btime_mirrorvideoram_r(int offset)
{
	int x,y;


	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	return videoram_r(offset);
}

int btime_mirrorcolorram_r(int offset)
{
	int x,y;


	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	return colorram_r(offset);
}

void btime_mirrorvideoram_w(int offset,int data)
{
	int x,y;


	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	videoram_w(offset,data);
}

void btime_mirrorcolorram_w(int offset,int data)
{
	int x,y;


	/* swap x and y coordinates */
	x = offset / 32;
	y = offset % 32;
	offset = 32 * y + x;

	colorram_w(offset,data);
}



void btime_background_w(int offset,int data)
{
	if (background_image != data)
	{
		memset(dirtybuffer,1,videoram_size);

		background_image = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void btime_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			/* draw the background (this can be handled better) */
			if (background_image & 0x10)
			{
				struct rectangle clip;
				int bx,by;
				int base,bgoffs;
				/* kludge to get the correct background */
				static int mapconvert[8] = { 1,2,3,0,5,6,7,4 };


				clip.min_x = sx;
				clip.max_x = sx+7;
				clip.min_y = sy;
				clip.max_y = sy+7;

				bx = sx & 0xf0;
				by = sy & 0xf0;

				base = 0x100 * mapconvert[(background_image & 0x07)];
				bgoffs = base+16*(by/16)+bx/16;

				drawgfx(tmpbitmap,Machine->gfx[2],
						Machine->memory_region[2][bgoffs],
						0,
						0,0,
						bx,by,
						&clip,TRANSPARENCY_NONE,0);

				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs] + 256 * (colorram[offs] & 3),
						0,
						0,0,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
			else
				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs] + 256 * (colorram[offs] & 3),
						0,
						0,0,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < videoram_size;offs += 4*0x20)
	{
		if (videoram[offs + 0] & 0x01)
			drawgfx(bitmap,Machine->gfx[1],
					videoram[offs + 0x20],
					0,
					videoram[offs + 0] & 0x02,videoram[offs + 0] & 0x04,
					239 - videoram[offs + 2*0x20],videoram[offs + 3*0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
