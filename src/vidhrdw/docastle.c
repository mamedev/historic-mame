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
static void convert_color_prom(unsigned short *colortable,const unsigned char *color_prom,
		int priority)
{
	int i,j;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 5) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		r = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;
		/* green component */
		bit0 = (*color_prom >> 2) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 4) & 0x01;
		g = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 0) & 0x01;
		bit2 = (*color_prom >> 1) & 0x01;
		b = 0x23 * bit0 + 0x4b * bit1 + 0x91 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* reserve one color for the transparent pen (none of the game colors can have */
	/* these RGB components) */
	palette_set_color(256,1,1,1);
	/* and the last color for the sprite covering pen */
	palette_set_color(257,2,2,2);


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



PALETTE_INIT( docastle )
{
	convert_color_prom(colortable,color_prom,0);
}

PALETTE_INIT( dorunrun )
{
	convert_color_prom(colortable,color_prom,1);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( docastle )
{
	if (video_start_generic() != 0)
		return 1;

	if ((tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
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
VIDEO_UPDATE( docastle )
{
	int offs;


	if (get_vh_global_attribute_changed())
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


		if (Machine->gfx[1]->total_elements > 256)
		{
			/* spriteram

			 indoor soccer appears to have a slightly different spriteram
			 format to the other games, allowing a larger number of sprite
			 tiles

			 yyyy yyyy  xxxx xxxx  TX-T pppp  tttt tttt

			 y = ypos
			 x = xpos
			 X = x-flip
			 T = extra tile number bits
			 p = palette
			 t = tile number

			 */

			code = spriteram[offs + 3];
			color = spriteram[offs + 2] & 0x0f;
			sx = ((spriteram[offs + 1] + 8) & 0xff) - 8;
			sy = spriteram[offs];
			flipx = spriteram[offs + 2] & 0x40;
			flipy = 0;
			if (spriteram[offs + 2] & 0x10) code += 0x100;
			if (spriteram[offs + 2] & 0x80) code += 0x200;
		}
		else
		{
			/* spriteram

			this is the standard spriteram layout, used by most games

			 yyyy yyyy  xxxx xxxx  YX-p pppp  tttt tttt

			 y = ypos
			 x = xpos
			 X = x-flip
			 Y = y-flip
			 p = palette
			 t = tile number

			 */

			code = spriteram[offs + 3];
			color = spriteram[offs + 2] & 0x1f;
			sx = ((spriteram[offs + 1] + 8) & 0xff) - 8;
			sy = spriteram[offs];
			flipx = spriteram[offs + 2] & 0x40;
			flipy = spriteram[offs + 2] & 0x80;
		}

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
