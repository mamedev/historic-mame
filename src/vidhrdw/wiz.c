/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static struct rectangle spritevisiblearea =
{
	2*8, 32*8-1,
	2*8, 30*8-1
};

static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-1,
	2*8, 30*8-1
};

unsigned char *wiz_videoram2;
unsigned char *wiz_colorram2;
unsigned char *wiz_attributesram;
unsigned char *wiz_attributesram2;

static int flipx, flipy;

unsigned char *wiz_sprite_bank;
static unsigned char background_bank[2];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  I'm assuming 1942 resistor values

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
	-- 470 ohm resistor  -- RED/GREEN/BLUE
	-- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void wiz_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}


	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		COLOR(0,i) = i;
	}
}


void wiz_background_bank_select_w (int offset, int data)
{
    if (background_bank[offset] != data)
    {
	background_bank[offset] = data;

	memset(dirtybuffer, 1, videoram_size);
    }
}


void wiz_flipx_w (int offset, int data)
{
    if (flipx != data)
    {
	flipx = data;

	memset(dirtybuffer, 1, videoram_size);
    }
}


void wiz_flipy_w (int offset, int data)
{
    if (flipy != data)
    {
	flipy = data;

	memset(dirtybuffer, 1, videoram_size);
    }
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void wiz_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs,bank;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	bank = 3 + ((background_bank[0] << 1) | background_bank[1]);

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			if (flipx) sx = 31 - sx;
			if (flipy) sy = 31 - sy;

			// Background seem to be a single color
			drawgfx(tmpbitmap,Machine->gfx[bank],
				videoram[offs],
				0,
				flipx,flipy,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		if (flipx)
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = -wiz_attributesram[2 * i];
				if (flipy) scroll[31-i] = -scroll[31-i];
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -wiz_attributesram[2 * i];
				if (flipy) scroll[i] = -scroll[i];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites. */
	/* This playfield scrolls, so I tried using copyscrollbitmap, but couldn't make */
	/* the characters transparent, even when using TRANSPARENCY_PEN. Maybe someone  */
	/* could enlighten me as to what I'm missing.  */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int scroll,sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		scroll = (8*sy + 256 - wiz_attributesram2[2 * sx]) % 256;
		if (flipy)
		{
		   scroll = (248 - scroll) % 256;
		}
		if (flipx) sx = 31 - sx;


		drawgfx(bitmap,Machine->gfx[background_bank[1]],
			wiz_videoram2[offs],
			wiz_colorram2[offs] & 0x07,
			flipx,flipy,
			8*sx,scroll,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int spriteflipy,sx,sy;

		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs];

		// The flipy bit is backwards compared to Galaxian,
		// either that, or the 2 sprite ROMS use a different
		// layout
		spriteflipy = ~spriteram[offs + 1] & 0x80 ;

		if (flipx)
		{
			sx = 240 - sx;
		}
		if (flipy)
		{
			sy = 240 - sy;
			spriteflipy = !spriteflipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs + 1] & 0x7f,
				spriteram[offs + 2] & 0x07,
				flipx,spriteflipy,
				sx,sy,
				flipx ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}

	for (offs = spriteram_2_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,spritecode;

		sx = spriteram_2[offs + 3];
		sy = 240 - spriteram_2[offs];

		if (flipx)
		{
			sx = 240 - sx;
		}
		if (flipy)
		{
			sy = 240 - sy;
		}

		spritecode = spriteram_2[offs + 1];
		if (spritecode & 0x80)
		{
		    bank = 7 + *wiz_sprite_bank;
		    spritecode &= 0x7f;
		}
		else
		{
		    bank = 9;   // Dragon boss
		}

		drawgfx(bitmap,Machine->gfx[bank],
				spritecode,
				spriteram_2[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				flipx ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
