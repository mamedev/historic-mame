/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static struct rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Amidar has one 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void amidar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


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


	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i & 3) COLOR(0,i) = i;
		else COLOR(0,i) = 0;	/* 00 is always black, regardless of the contents of the PROM */
	}
}



static void updatetile(int x,int y)
{
	set_tile_attributes(0,			/* layer number */
		x + y * 32, 				/* x/y position */
		0,videoram00[32*y+x],		/* tile bank, code */
		videoram01[2*x + 1] & 0x07,	/* color */
		0,0,						/* flip x/y */
		TILE_TRANSPARENCY_OPAQUE);	/* transparency */
}

void amidar_updatehook00(int offset)
{
	updatetile(offset % 32,offset / 32);
}

void amidar_updatehook01(int offset)
{
	if (offset & 1)
	{
		int x,y;


		x = offset / 2;
		for (y = 0;y < 32;y++)
			updatetile(x,y);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void amidar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	set_tile_layer_attributes(0,bitmap,				/* layer number, bitmap */
			0,0,									/* scroll x/y */
			*flip_screen_x & 1,*flip_screen_y & 1,	/* flip x/y */
			0,0);									/* global attributes */
	update_tile_layer(0,bitmap);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy;


		sx = (spriteram[offs + 3] + 1) & 0xff;	/* ??? */
		sy = 240 - spriteram[offs];
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;

		if (*flip_screen_x & 1)
		{
			sx = 241 - sx;	/* note: 241, not 240 */
			flipx = !flipx;
		}
		if (*flip_screen_y & 1)
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		/* Sprites #0, #1 and #2 need to be offset one pixel to be correctly */
		/* centered on the ladders in Turtles (we move them down, but since this */
		/* is a rotated game, we actually move them left). */
		/* Note that the adjustement must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */
		if (offs <= 2*4) sy++;

		drawgfx(bitmap,Machine->gfx[0],
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				*flip_screen_x & 1 ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);

layer_mark_rectangle_dirty(Machine->layer[0],sx,sx+15,sy,sy+15);
	}
}
