/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *bombjack_paletteram;



/***************************************************************************

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

	palette_change_color(offset / 2,r,g,b);
}



void bombjack_background_w(int offset,int data)
{
	int base,offs,code,attr;


	base = 0x200 * (data & 0x07);

	for (offs = 0;offs < 0x100;offs++)
	{
		if (data & 0x10)
		{
			code = Machine->memory_region[2][base + offs];
			attr = Machine->memory_region[2][base + offs + 0x100];
		}
		else
		{
			code = 0xff;
			attr = 0;
		}

		set_tile_attributes(1,			/* layer number */
			offs,						/* x/y position */
			0,code,						/* tile bank, code */
			attr & 0x0f,				/* color */
			attr & 0x40,attr & 0x80,	/* flip x/y */
			TILE_TRANSPARENCY_OPAQUE);	/* transparency */
	}
}



void bombjack_updatehook0(int offset)
{
	set_tile_attributes(0,											/* layer number */
		offset,														/* x/y position */
		0,videoram00[offset] + ((videoram01[offset] & 0x10) << 4),	/* tile bank, code */
		videoram01[offset] & 0x0f,									/* color */
		0,0,														/* flip x/y */
		TILE_TRANSPARENCY_PEN);										/* transparency */
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	set_tile_layer_attributes(1,bitmap,			/* layer number, bitmap */
			0,0,								/* scroll x/y */
			*flip_screen & 1,*flip_screen & 1,	/* flip x/y */
			0,0);								/* global attributes */
	set_tile_layer_attributes(0,bitmap,			/* layer number, bitmap */
			0,0,								/* scroll x/y */
			*flip_screen & 1,*flip_screen & 1,	/* flip x/y */
			0,0);								/* global attributes */
	update_tile_layer(1,bitmap);
	update_tile_layer(0,bitmap);


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


		sx = spriteram[offs+3];
		if (spriteram[offs] & 0x80)
			sy = 225-spriteram[offs+2];
		else
			sy = 241-spriteram[offs+2];
		flipx = spriteram[offs+1] & 0x40;
		flipy =	spriteram[offs+1] & 0x80;
		if (*flip_screen & 1)
		{
			if (spriteram[offs] & 0x80)
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

		drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x80) ? 1 : 0],
				spriteram[offs] & 0x7f,
				spriteram[offs+1] & 0x0f,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

if (spriteram[offs] & 0x80)
{
	layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+31,sy,sy+31);
}
else
{
	layer_mark_rectangle_dirty(Machine->layer[1],sx,sx+15,sy,sy+15);
}
	}
}
