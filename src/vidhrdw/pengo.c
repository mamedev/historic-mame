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
static int flipscreen;
static int xoffsethack;


static struct rectangle spritevisiblearea =
{
	0*8, 28*8-1,
	2*8, 34*8-1
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
void pengo_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

	if (Machine->gfx[2])	/* only Pengo has the second gfx bank */
	{
		/* second bank character lookup table */
		/* sprites use the same color lookup table as characters */
		for (i = 0;i < TOTAL_COLORS(2);i++)
		{
			if (*color_prom) COLOR(2,i) = *color_prom + 0x10;	/* second palette bank */
			else COLOR(2,i) = 0;	/* preserve transparency */

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
	flipscreen = 0;
	xoffsethack = 0;

	return generic_vh_start();
}

int pacman_vh_start(void)
{
	gfx_bank = 0;
	flipscreen = 0;
	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	xoffsethack = 1;

	return generic_vh_start();
}



void pengo_flipscreen_w(int offset,int data)
{
	if (flipscreen != (data & 1))
	{
		flipscreen = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
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
void pengo_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;


			dirtybuffer[offs] = 0;

	/* Even if Pengo's screen is 28x36, the memory layout is 32x32. We therefore */
	/* have to convert the memory coordinates into screen coordinates. */
	/* Note that 32*32 = 1024, while 28*36 = 1008: therefore 16 bytes of Video RAM */
	/* don't map to a screen position. We don't check that here, however: range */
	/* checking is performed by drawgfx(). */

			mx = offs / 32;
			my = offs % 32;

			if (mx <= 1)
			{
				sx = 29 - my;
				sy = mx + 34;
			}
			else if (mx >= 30)
			{
				sx = 29 - my;
				sy = mx - 30;
			}
			else
			{
				sx = 29 - mx;
				sy = my + 2;
			}

			if (flipscreen)
			{
				sx = 27 - sx;
				sy = 35 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[2 * gfx_bank],
					videoram[offs],
					colorram[offs],
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 2;offs > 2*2;offs -= 2)
	{
		drawgfx(bitmap,Machine->gfx[1 + 2 * gfx_bank],
				spriteram[offs] >> 2,spriteram[offs + 1],
				spriteram[offs] & 2,spriteram[offs] & 1,
				239 - spriteram_2[offs],272 - spriteram_2[offs + 1],
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

		/* also plot the sprite 256 pixels higher for vertical wraparound (tunnel in */
		/* Crush Roller) */
		drawgfx(bitmap,Machine->gfx[1 + 2 * gfx_bank],
				spriteram[offs] >> 2,spriteram[offs + 1],
				spriteram[offs] & 2,spriteram[offs] & 1,
				239 - spriteram_2[offs],272-256 - spriteram_2[offs + 1],
				&spritevisiblearea,TRANSPARENCY_COLOR,0);
	}
	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	for (offs = 2*2;offs >= 0;offs -= 2)
	{
		drawgfx(bitmap,Machine->gfx[1 + 2 * gfx_bank],
				spriteram[offs] >> 2,spriteram[offs + 1],
				spriteram[offs] & 2,spriteram[offs] & 1,
				239 - xoffsethack - spriteram_2[offs],272 - spriteram_2[offs + 1],
				&spritevisiblearea,TRANSPARENCY_COLOR,0);

		/* also plot the sprite 256 pixels higher for vertical wraparound (tunnel in */
		/* Crush Roller) */
		drawgfx(bitmap,Machine->gfx[1 + 2 * gfx_bank],
				spriteram[offs] >> 2,spriteram[offs + 1],
				spriteram[offs] & 2,spriteram[offs] & 1,
				239 - xoffsethack - spriteram_2[offs],272-256 - spriteram_2[offs + 1],
				&spritevisiblearea,TRANSPARENCY_COLOR,0);
	}
}
