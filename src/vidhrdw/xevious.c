/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *xevious_videoram2,*xevious_colorram2;

static struct osd_bitmap *tmpbitmap1,*tmpbitmap2;
static unsigned char *dirtybuffer2;

/* scroll position controller write (CUSTOM 13-1J on seet 7B) */
static int bg_y_pos , bg_x_pos;
static int fo_y_pos , fo_x_pos;
static int flip;




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Xevious has three 256x4 palette PROMs (one per gun) and four 512x4 lookup
  table PROMs (two for sprites, two for background tiles; foreground
  characters map directly to a palette color without using a PROM).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void xevious_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 128;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[256] >> 0) & 0x01;
		bit1 = (color_prom[256] >> 1) & 0x01;
		bit2 = (color_prom[256] >> 2) & 0x01;
		bit3 = (color_prom[256] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*256] >> 0) & 0x01;
		bit1 = (color_prom[2*256] >> 1) & 0x01;
		bit2 = (color_prom[2*256] >> 2) & 0x01;
		bit3 = (color_prom[2*256] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	/* color 0x80 is used by sprites to mark transparency */
	*(palette++) = 1;
	*(palette++) = 1;
	*(palette++) = 1;

	color_prom += 128;	/* the bottom part of the PROM is unused */
	color_prom += 2*256;
	/* color_prom now points to the beginning of the lookup table */

	/* background tiles */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = *(color_prom++);

	/* sprites */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 8 == 0) COLOR(2,i) = 0x80; 	/* transparent */
		else COLOR(2,i) = *color_prom;	/* this can be transparent too */

		color_prom++;
	}

	/* foreground characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i % 2 == 0) COLOR(0,i) = 0x80;	/* transparent */
		else COLOR(0,i) = i / 2;
	}
}



void xevious_vh_latch_w(int offset, int data)
{
	int reg;

	data = data + ((offset&0x01)<<8);	/* A0 -> D8 */
	reg = (offset&0xf0)>>4;

	switch (reg)
	{
	case 0:		/* BG Y scroll position */
		bg_y_pos = data;
		break;
	case 2:		/* BG X scroll position ?? */
		bg_x_pos = data;
		break;
	case 1:		/* FONT Y scroll position ??*/
		fo_y_pos = data;
		break;
	case 3:		/* FONT X scroll position ?? */
		fo_x_pos = data;
		break;
	case 7:		/* DISPLAY XY FLIP ?? */
		flip = data&1;
		break;
   default:
		   if (errorlog)
			   fprintf(errorlog,"CRTC WRITE REG: %x  Data: %03x\n",reg, data);
		   break;
	}
}




/*
background pattern data

colorram mapping
b000-bfff background attribute
          bit 0-1 COL:palette set select
          bit 2-5 AN :color select
          bit 6   AFF:Y flip
          bit 7   PFF:X flip
c000-cfff background pattern name
          bit 0-7 PP0-7

seet 8A
                                        2      +-------+
COL0,1 --------------------------------------->|backg. |
                                        1      |color  |
PP7------------------------------------------->|replace|
                                        4      | ROM   |  6
AN0-3 ---------------------------------------->|  4H   |-----> color code 6 bit
        1  +-----------+      +--------+       |  4F   |
COL0  ---->|B8   ROM 3C| 16   |custom  |  2    |       |
        8  |           |----->|shifter |------>|       |
PP0-7 ---->|B0-7 ROM 3D|      |16->2*8 |       |       |
           +-----------+      +--------+       +-------+

font rom controller
       1  +--------+     +--------+
ANF   --->| ROM    |  8  |shift   |  1
       8  | 3B     |---->|reg     |-----> font data
PP0-7 --->|        |     |8->1*8  |
          +--------+     +--------+

font color ( not use color map )
        2  |
COL0-1 --->|  color code 6 bit
        4  |
AN0-3  --->|

sprite

ROM 3M,3L color reprace table for sprite



*/

int xevious_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap1 = osd_create_bitmap(32*8,64*8)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap2 = osd_create_bitmap(32*8,64*8)) == 0)
	{
		osd_free_bitmap(tmpbitmap1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}

void xevious_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap1);
	free(dirtybuffer2);
	generic_vh_stop();
}



void xevious_videoram2_w(int offset,int data)
{
	if (xevious_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		xevious_videoram2[offset] = data;
	}
}



void xevious_colorram2_w(int offset,int data)
{
	if (xevious_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		xevious_colorram2[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void xevious_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, sx,sy;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size ;offs >= 0;offs--)
	{
		/* foreground */
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sx = 31 - (offs / 64);
			sy = offs % 64;

			drawgfx(tmpbitmap1,Machine->gfx[0],
					videoram[offs],
					((colorram[offs] & 0x03) << 4) + ((colorram[offs] >> 2) & 0x0f),
					colorram[offs] & 0x80,colorram[offs] & 0x40,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		/* background */
		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			sx = 31 - (offs / 64);
			sy = offs % 64;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					xevious_videoram2[offs] + 256*(xevious_colorram2[offs] & 1),
					((xevious_colorram2[offs] >> 2) & 0xf) +
							((xevious_colorram2[offs] & 0x3) << 5) +
							((xevious_videoram2[offs] & 0x80) >> 3),
					xevious_colorram2[offs] & 0x80,xevious_colorram2[offs] & 0x40,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background */
	{
		int scrollx,scrolly;


		scrollx = bg_x_pos - 16;
		scrolly = -bg_y_pos - 20;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		if ((spriteram[offs + 1] & 0x40) == 0)	/* I'm not sure about this one */
		{
			int bank,code,color,flipx,flipy;


			bank = 2 + ((spriteram[offs] & 0x80) >> 7) + ((spriteram_3[offs] & 0x80) >> 6);
			code = spriteram[offs] & 0x7f;
			color = spriteram[offs + 1] & 0x7f;
			flipx = spriteram_3[offs] & 4;
			flipy = spriteram_3[offs + 1] & 4;
			sx = spriteram_2[offs] - 15;
			sy = spriteram_2[offs + 1] - 40 + 0x100*(spriteram_3[offs + 1] & 1);
			if (spriteram_3[offs] & 2)	/* double width (?) */
			{
				if (spriteram_3[offs] & 1)	/* double width, double height */
				{
					code &= 0x7c;
					drawgfx(bitmap,Machine->gfx[bank],
							code+3,color,flipx,flipy,
							flipx ? sx+16 : sx,flipy ? sy : sy+16,
							&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
					drawgfx(bitmap,Machine->gfx[bank],
							code+1,color,flipx,flipy,
							flipx ? sx : sx+16,flipy ? sy : sy+16,
							&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
				}
				code &= 0x7d;
				drawgfx(bitmap,Machine->gfx[bank],
						code+2,color,flipx,flipy,
						flipx ? sx+16 : sx,flipy ? sy+16 : sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
				drawgfx(bitmap,Machine->gfx[bank],
						code,color,flipx,flipy,
						flipx ? sx : sx+16,flipy ? sy+16 : sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
			}
			else if (spriteram_3[offs] & 1)	/* double height */
			{
				code &= 0x7e;
				drawgfx(bitmap,Machine->gfx[bank],
						code,color,flipx,flipy,
						flipx ? sx+16 : sx,flipy ? sy+16 : sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
				drawgfx(bitmap,Machine->gfx[bank],
						code+1,color,flipx,flipy,
						flipx ? sx+16 : sx,flipy ? sy : sy+16,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
			}
			else	/* normal */
			{
				drawgfx(bitmap,Machine->gfx[bank],
						code,color,flipx,flipy,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
			}
		}
	}


	/* copy the foreground  */
	{
		int scrollx,scrolly;


		scrollx = fo_x_pos - 14;
		scrolly = -fo_y_pos - 32;

		copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0x80);
	}
}
