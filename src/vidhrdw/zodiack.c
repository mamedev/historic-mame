/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static struct rectangle spritevisiblearea =
{
	2*8+1, 30*8-1,
	2*8, 30*8-1
};

unsigned char *zodiack_videoram2;

extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;

static int flipscreen;

void zodiack_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* first, the character/sprite palette */
	for (i = 0;i < 32;i++)
	{
		int col,bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		if ((i & 3) == 0)  COLOR(3,i) = 0;
	}
}


void zodiac_flipscreen_w(int offset,int data)
{
	if (flipscreen != (!data))
	{
		flipscreen = !data;

		memset(dirtybuffer, 1, videoram_size);
	}
}


void zodiac_control_w(int offset,int data)
{
	/* Bit 0-1 - coin counters */
	coin_counter_w(0, data & 0x02);
	coin_counter_w(1, data & 0x01);

	/* Bit 2 - ???? */
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void zodiack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* draw the background characters */
	for (offs = 0; offs < videoram_size; offs++)
	{
		int code,sx,sy;


		if (!dirtybuffer[offs])  continue;

		dirtybuffer[offs] = 0;


		sy = offs / 32;
		sx = offs % 32;

		if (flipscreen)
		{
			sx = 31 - sx;
		}

		code = videoram[offs];

		drawgfx(tmpbitmap,Machine->gfx[3],
				code,
				galaxian_attributesram[2 * sx + 1] & 0x07,
				flipscreen, 0,
				8*sx, 8*sy,
				0,TRANSPARENCY_NONE,0);
	}


	/* draw the foreground characters */
	for (offs = 0; offs < videoram_size; offs++)
	{
		int code,sx,sy;


		sy = offs / 32;
		sx = offs % 32;

		if (flipscreen)
		{
			sy = 31 - sy;
			sx = 31 - sx;
		}

		code = zodiack_videoram2[offs];

		drawgfx(bitmap,Machine->gfx[0],
				code,
				0,
				flipscreen, flipscreen,
				8*sx, 8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* copy the temporary bitmap to the screen */
	{
		int i, scroll[32];


		if (flipscreen)
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = galaxian_attributesram[2 * i];
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -galaxian_attributesram[2 * i];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* draw the bullets */
	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		int x,y;
		int color;


		x =       galaxian_bulletsram[offs + 3] + Machine->drv->gfxdecodeinfo[2].gfxlayout->width;
		y = 255 - galaxian_bulletsram[offs + 1];

		drawgfx(bitmap,Machine->gfx[2],
				0,	/* this is just a dot, generated by the hardware */
				0,//color,
				0,0,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy,spritecode;


		sx = 240 - spriteram[offs + 3];
		sy = 240 - spriteram[offs];
		flipx = !(spriteram[offs + 1] & 0x40);
		flipy =   spriteram[offs + 1] & 0x80;
		spritecode = spriteram[offs + 1] & 0x3f;

		drawgfx(bitmap,Machine->gfx[1],
				spritecode,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				//flipscreen[0] ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
				//&spritevisiblearea,TRANSPARENCY_PEN,0);
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}