/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *bombjack_paletteram;
static int background_image;
static int flipscreen;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Bomb Jack doesn't have a color PROM. It uses 256 bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (4 bits per pixel; the high 4 bits of the second byte are unused).
  Since the graphics use 3 bitplanes, hence 8 colors, this makes for 16
  different color codes.

  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)
  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 7 -- unused
        -- unused
        -- unused
        -- unused
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
void bombjack_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* initialize the color table */
	for (i = 0;i < Machine->drv->total_colors;i++)
		colortable[i] = i;
}



void bombjack_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b,val;


	bombjack_paletteram[offset] = data;

	/* red component */
	val = bombjack_paletteram[offset & ~1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	val = bombjack_paletteram[offset & ~1];
	bit0 = (val >> 4) & 0x01;
	bit1 = (val >> 5) & 0x01;
	bit2 = (val >> 6) & 0x01;
	bit3 = (val >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	val = bombjack_paletteram[offset | 1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	osd_modify_pen(Machine->pens[offset / 2],r,g,b);
}



void bombjack_background_w(int offset,int data)
{
	if (background_image != data)
	{
		memset(dirtybuffer,1,videoram_size);
		background_image = data;
	}
}



void bombjack_flipscreen_w(int offset,int data)
{
	if (flipscreen != (data & 1))
	{
		flipscreen = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,base;


	base = 0x200 * (background_image & 0x07);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;
		int tilecode,tileattribute;


		sx = 31 - offs / 32;
		sy = offs % 32;

		if (background_image & 0x10)
		{
			int bgoffs;


			bgoffs = base+16*(15-sx/2)+sy/2;

			tilecode = Machine->memory_region[2][bgoffs],
			tileattribute = Machine->memory_region[2][bgoffs + 0x100];
		}
		else
		{
			tilecode = 0xff;
			tileattribute = 0;	/* avoid compiler warning */
		}

		if (dirtybuffer[offs])
		{
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			/* draw the background (this can be handled better) */
			if (tilecode != 0xff)
			{
				struct rectangle clip;
				int flipx;


				clip.min_x = 8*sx;
				clip.max_x = 8*sx+7;
				clip.min_y = 8*sy;
				clip.max_y = 8*sy+7;

				flipx = tileattribute & 0x80;
				if (flipscreen) flipx = !flipx;

				drawgfx(tmpbitmap,Machine->gfx[1],
						tilecode,
						tileattribute & 0x0f,
						flipx,flipscreen,
						16*(sx/2),16*(sy/2),
						&clip,TRANSPARENCY_NONE,0);

				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs] + 16 * (colorram[offs] & 0x10),
						colorram[offs] & 0x0f,
						flipscreen,flipscreen,
						8*sx,8*sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
			else
				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs] + 16 * (colorram[offs] & 0x10),
						colorram[offs] & 0x0f,
						flipscreen,flipscreen,
						8*sx,8*sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


			dirtybuffer[offs] = 0;
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{

/*
 abbbbbbb cdefgggg hhhhhhhh iiiiiiii

 a        use big sprites (32x32 instead of 16x16)
 bbbbbbb  sprite code
 c        x flip
 d        y flip (used only in death sequence?)
 e        ? (set when big sprites are selected)
 f        ? (set only when the bonus (B) materializes?)
 gggg     color
 hhhhhhhh x position
 iiiiiiii y position
*/
		int sx,sy,flipx,flipy;


		sx = spriteram[offs+2]-1;
		sy = spriteram[offs+3];
		flipx =	spriteram[offs+1] & 0x80;
		flipy = spriteram[offs+1] & 0x40;
		if (flipscreen)
		{
			if (spriteram[offs+1] & 0x20)
			{
				sx = 224 - sx;
				sy = 224 - sy;
			}
			else
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x80) ? 3 : 2],
				spriteram[offs] & 0x7f,
				spriteram[offs+1] & 0x0f,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
