/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *galaxian_attributesram;

static int sound_enabled = 0;

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
void suprmous_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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
		COLOR(0,i) = i;
//		if (i & 3) COLOR(0,i) = i;
//		else COLOR(0,i) = 0;	/* 00 is always black, regardless of the contents of the PROM */
	}
}



void suprmous_flipx_w(int offset,int data)
{
	if (*flip_screen_x != (data & 1))
	{
		*flip_screen_x = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void suprmous_flipy_w(int offset,int data)
{
	if (*flip_screen_y != (data & 1))
	{
		*flip_screen_y = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void suprmous_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
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

			sx = (offs % 32);
			sy = (offs / 32);

			if (*flip_screen_x) sx = 31 - sx;
			if (*flip_screen_y) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x07,
					*flip_screen_x,*flip_screen_y,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
			int i, scroll[32];


			if (*flip_screen_x)
			{
					for (i = 0;i < 32;i++)
					{
							scroll[31-i] = -galaxian_attributesram[2 * i];
							if (*flip_screen_y) scroll[31-i] = -scroll[31-i];
					}
			}
			else
			{
					for (i = 0;i < 32;i++)
					{
							scroll[i] = -galaxian_attributesram[2 * i];
							if (*flip_screen_y) scroll[i] = -scroll[i];
					}
			}

			copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy;

		if (spriteram[offs + 0] == 0 ||
		    spriteram[offs + 3] == 0)
		{
		    continue;
		}

		sx = (spriteram[offs+3] + 1) & 0xff;
		sy = 240 - spriteram[offs];

		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;

		if (*flip_screen_x)
		{
		    sx = 242 - sx;
		    flipx = !flipx;
		}

		if (*flip_screen_y)
		{
		    sy = 240 - sy;
		    flipy = !flipy;
		}

		/* Sprites 0-3 are drawn one pixel to the left */
		if (offs <= 3*4) sy++;

		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs + 1] & 0x3f) | 0x40,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				*flip_screen_x & 1 ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
