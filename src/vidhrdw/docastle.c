/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  (Cocktail mode implemented by Chad Hendrickson Aug 1, 1999)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static struct mame_bitmap *tmpbitmap1;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mr. Do's Castle / Wild Ride / Run Run have a 256 bytes palette PROM which
  is connected to the RGB output this way:

  bit 7 -- 200 ohm resistor  -- RED
        -- 390 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
        -- 200 ohm resistor  -- GREEN
        -- 390 ohm resistor  -- GREEN
        -- 820 ohm resistor  -- GREEN
        -- 200 ohm resistor  -- BLUE
  bit 0 -- 390 ohm resistor  -- BLUE

***************************************************************************/
static void convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom,
		int priority)
{
	int i,j;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 5) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;
		/* green component */
		bit0 = (*color_prom >> 2) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 4) & 0x01;
		*(palette++) = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 0) & 0x01;
		bit2 = (*color_prom >> 1) & 0x01;
		*(palette++) = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;

		color_prom++;
	}

	/* reserve one color for the transparent pen (none of the game colors can have */
	/* these RGB components) */
	*(palette++) = 1;
	*(palette++) = 1;
	*(palette++) = 1;
	/* and the last color for the sprite covering pen */
	*(palette++) = 2;
	*(palette++) = 2;
	*(palette++) = 2;


	/* characters */
	/* characters have 4 bitplanes, but they actually have only 8 colors. The fourth */
	/* plane is used to select priority over sprites. The meaning of the high bit is */
	/* reversed in Do's Castle wrt the other games. */

	/* first create a table with all colors, used to draw the background */
	for (i = 0;i < 32;i++)
	{
		for (j = 0;j < 8;j++)
		{
			colortable[16*i+j] = 8*i+j;
			colortable[16*i+j+8] = 8*i+j;
		}
	}
	/* now create a table with only the colors which have priority over sprites, used */
	/* to draw the foreground. */
	for (i = 0;i < 32;i++)
	{
		for (j = 0;j < 8;j++)
		{
			if (priority == 0)	/* Do's Castle */
			{
				colortable[32*16+16*i+j] = 256;	/* high bit clear means less priority than sprites */
				colortable[32*16+16*i+j+8] = 8*i+j;
			}
			else	/* Do Wild Ride, Do Run Run, Kick Rider */
			{
				colortable[32*16+16*i+j] = 8*i+j;
				colortable[32*16+16*i+j+8] = 256;	/* high bit set means less priority than sprites */
			}
		}
	}

	/* sprites */
	/* sprites have 4 bitplanes, but they actually have only 8 colors. The fourth */
	/* plane is used for transparency. */
	for (i = 0;i < 32;i++)
	{
		/* build two versions of the colortable, one with the covering color
		   mapped to transparent, and one with all colors but the covering one
		   mapped to transparent. */
		for (j = 0;j < 16;j++)
		{
			if (j < 8)
				colortable[64*16+16*i+j] = 256;	/* high bit clear means transparent */
			else if (j == 15)
				colortable[64*16+16*i+j] = 256;	/* sprite covering color */
			else
				colortable[64*16+16*i+j] = 8*i+(j&7);
		}
		for (j = 0;j < 16;j++)
		{
			if (j == 15)
				colortable[64*16+32*16+16*i+j] = 257;	/* sprite covering color */
			else
				colortable[64*16+32*16+16*i+j] = 256;
		}
	}
}



void docastle_vh_convert_color_prom(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom)
{
	convert_color_prom(palette,colortable,color_prom,0);
}

void dorunrun_vh_convert_color_prom(unsigned char *palette,unsigned short *colortable,const unsigned char *color_prom)
{
	convert_color_prom(palette,colortable,color_prom,1);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int docastle_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap1 = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void docastle_vh_stop(void)
{
	bitmap_free(tmpbitmap1);
	generic_vh_stop();
}


READ_HANDLER( docastle_flipscreen_off_r )
{
	flip_screen_set(0);
	return 0;
}

READ_HANDLER( docastle_flipscreen_on_r )
{
	flip_screen_set(1);
	return 0;
}

WRITE_HANDLER( docastle_flipscreen_off_w )
{
	flip_screen_set(0);
}

WRITE_HANDLER( docastle_flipscreen_on_w )
{
	flip_screen_set(1);
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void docastle_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (full_refresh)
	{
		memset(dirtybuffer,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 8*(colorram[offs] & 0x20),
					colorram[offs] & 0x1f,
					flip_screen,flip_screen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);

			/* also draw the part of the character which has priority over the */
			/* sprites in another bitmap */
			drawgfx(tmpbitmap1,Machine->gfx[0],
					videoram[offs] + 8*(colorram[offs] & 0x20),
					32 + (colorram[offs] & 0x1f),
					flip_screen,flip_screen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	fillbitmap(priority_bitmap,1,NULL);


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy,code,color;


		code = spriteram[offs + 3];
		color = spriteram[offs + 2] & 0x1f;
		sx = spriteram[offs + 1];
		sy = spriteram[offs];
		flipx = spriteram[offs + 2] & 0x40;
		flipy = spriteram[offs + 2] & 0x80;


		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		/* first draw the sprite, visible */
		pdrawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,256,
				0x00);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx(bitmap,Machine->gfx[1],
				code,
				color + 32,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,256,
				0x02);
	}


	/* now redraw the portions of the background which have priority over sprites */
	copybitmap(bitmap,tmpbitmap1,0,0,0,0,&Machine->visible_area,TRANSPARENCY_COLOR,256);
}
