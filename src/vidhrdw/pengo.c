/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This file is used by the Pengo and Pac Man drivers.
  They are almost identical, the only differences being the extra gfx bank
  in Pengo, and the need to compensate for an hardware sprite positioning
  "bug" in Pac Man.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfx_bank;
static int xoffsethack;

static struct rectangle spritevisiblearea =
{
	2*8, 34*8-1,
	0*8, 28*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Pac Man has a 16 bytes palette PROM and a 128 bytes color lookup table PROM.

  Pengo has a 32 bytes palette PROM and a 256 bytes color lookup table PROM
  (actually that's 512 bytes, but the high address bit is grounded).

  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void pengo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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

	/* color_prom now points to the beginning of the lookup table */

	/* character lookup table */
	/* sprites use the same color lookup table as characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++);

	if (Machine->gfx[1])	/* only Pengo has the second gfx bank */
	{
		/* second bank character lookup table */
		/* sprites use the same color lookup table as characters */
		for (i = 0;i < TOTAL_COLORS(1);i++)
		{
			if (*color_prom) COLOR(1,i) = *color_prom + 0x10;	/* second palette bank */
			else COLOR(1,i) = 0;	/* preserve transparency */

			color_prom++;
		}
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int pengo_vh_start(void)
{
	gfx_bank = 0;
	xoffsethack = 0;

	return generic_vh_start();
}

int pacman_vh_start(void)
{
	gfx_bank = 0;
	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	xoffsethack = 1;

	return generic_vh_start();
}



void pengo_updatehook0(int offset)
{
	int mx,my,sx,sy;


	mx = offset % 32;
	my = offset / 32;

	if (my < 2)
	{
		if (mx < 2 || mx >= 30) return;	/* not visible */
		sx = my + 34;
		sy = mx - 2;
	}
	else if (my >= 30)
	{
		if (mx < 2 || mx >= 30) return;	/* not visible */
		sx = my - 30;
		sy = mx - 2;
	}
	else
	{
		sx = mx + 2;
		sy = my - 2;
	}

	set_tile_attributes(0,				/* layer number */
		sx + sy * 36,					/* x/y position */
		0,videoram00[offset],	/* tile bank, code (global bank is handled later) */
		videoram01[offset] & 0x1f,		/* color */
		0,0,							/* flip x/y */
		TILE_TRANSPARENCY_OPAQUE);		/* transparency */
}



void pengo_gfxbank_w(int offset,int data)
{
	/* the Pengo hardware can set independently the palette bank, color lookup */
	/* table, and chars/sprites. However the game always set them together (and */
	/* the only place where this is used is the intro screen) so I don't bother */
	/* emulating the whole thing. */
	gfx_bank = data & 1;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void pengo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	set_tile_layer_attributes(0,bitmap,			/* layer number, bitmap */
			0,0,								/* scroll x/y */
			*flip_screen & 1,*flip_screen & 1,	/* flip x/y */
			MAKE_TILE_BANK(1),MAKE_TILE_BANK(gfx_bank));	/* global attributes */
	update_tile_layer(0,bitmap);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 2;offs > 2*2;offs -= 2)
	{
		drawgfx(bitmap,Machine->gfx[gfx_bank],
				spriteram[offs] >> 2,
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				272 - spriteram_2[offs + 1],spriteram_2[offs] - 31,
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

layer_mark_rectangle_dirty(Machine->layer[0],
		272 - spriteram_2[offs + 1],272 - spriteram_2[offs + 1]+15,
		spriteram_2[offs] - 31,spriteram_2[offs] - 31+15);

		/* also plot the sprite with wraparound (tunnel in Crush Roller) */
		drawgfx(bitmap,Machine->gfx[gfx_bank],
				spriteram[offs] >> 2,
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				272-256 - spriteram_2[offs + 1],spriteram_2[offs] - 31,
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

layer_mark_rectangle_dirty(Machine->layer[0],
		272-256 - spriteram_2[offs + 1],272-256 - spriteram_2[offs + 1]+15,
		spriteram_2[offs] - 31,spriteram_2[offs] - 31+15);
	}
	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	for (offs = 2*2;offs >= 0;offs -= 2)
	{
		drawgfx(bitmap,Machine->gfx[gfx_bank],
				spriteram[offs] >> 2,
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				272 - spriteram_2[offs + 1],spriteram_2[offs] - 31 + xoffsethack,
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

layer_mark_rectangle_dirty(Machine->layer[0],
		272 - spriteram_2[offs + 1],272 - spriteram_2[offs + 1]+15,
		spriteram_2[offs] - 31 + xoffsethack,spriteram_2[offs] - 31+15 + xoffsethack);

		/* also plot the sprite with wraparound (tunnel in Crush Roller) */
		drawgfx(bitmap,Machine->gfx[gfx_bank],
				spriteram[offs] >> 2,
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 2,spriteram[offs] & 1,
				272-256 - spriteram_2[offs + 1],spriteram_2[offs] - 31 + xoffsethack,
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

layer_mark_rectangle_dirty(Machine->layer[0],
		272-256 - spriteram_2[offs + 1],272-256 - spriteram_2[offs + 1]+15,
		spriteram_2[offs] - 31 + xoffsethack,spriteram_2[offs] - 31+15 + xoffsethack);
	}
}
