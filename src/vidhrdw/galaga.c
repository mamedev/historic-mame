/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define MAX_STARS 250
#define STARS_COLOR_BASE 32

unsigned char *galaga_starcontrol;
static unsigned int stars_scroll;

struct star
{
	int x,y,col;
};
static struct star stars[MAX_STARS];
static int total_stars;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Galaga has one 32x8 palette PROM and two 256x4 color lookup table PROMs
  (one for characters, one for sprites). Only the first 128 bytes of the
  lookup tables seem to be used.
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
void galaga_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[31-i] >> 0) & 0x01;
		bit1 = (color_prom[31-i] >> 1) & 0x01;
		bit2 = (color_prom[31-i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[31-i] >> 3) & 0x01;
		bit1 = (color_prom[31-i] >> 4) & 0x01;
		bit2 = (color_prom[31-i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[31-i] >> 6) & 0x01;
		bit2 = (color_prom[31-i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	/* characters */
	for (i = 0;i < 32*4;i++)
		colortable[i] = 15 - (color_prom[i + 32] & 0x0f);
	/* sprites */
	for (i = 32*4;i < 64*4;i++)
		colortable[i] = (15 - (color_prom[i + 32] & 0x0f)) + 0x10;


	/* now the stars */
	for (i = 32;i < 32 + 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };

		bits = ((i-32) >> 0) & 0x03;
		palette[3*i] = map[bits];
		bits = ((i-32) >> 2) & 0x03;
		palette[3*i + 1] = map[bits];
		bits = ((i-32) >> 4) & 0x03;
		palette[3*i + 2] = map[bits];
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int galaga_vh_start(void)
{
	int generator;
	int x,y;


	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */
	/* this comes from the Galaxian hardware, Galaga is probably different */
	total_stars = 0;
	generator = 0;

	for (x = 255;x >= 0;x--)
	{
		for (y = 511;y >= 0;y--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (x >= Machine->drv->visible_area.min_x &&
					x <= Machine->drv->visible_area.max_x &&
					((~generator >> 16) & 1) &&
					(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].col = Machine->pens[color + STARS_COLOR_BASE];

					total_stars++;
				}
			}
		}
	}

	return 0;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galaga_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	clearbitmap(bitmap);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		if ((spriteram_3[offs + 1] & 2) == 0)
		{
			if (spriteram_3[offs] & 8)	/* double width */
			{
				drawgfx(bitmap,Machine->gfx[1],
						spriteram[offs]+2,spriteram[offs + 1],
						spriteram_3[offs] & 2,spriteram_3[offs] & 1,
						spriteram_2[offs]-16,spriteram_2[offs + 1]-40 + 0x100*(spriteram_3[offs + 1] & 1),
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],
						spriteram[offs],spriteram[offs + 1],
						spriteram_3[offs] & 2,spriteram_3[offs] & 1,
						spriteram_2[offs],spriteram_2[offs + 1]-40 + 0x100*(spriteram_3[offs + 1] & 1),
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

				if (spriteram_3[offs] & 4)	/* double height */
				{
					drawgfx(bitmap,Machine->gfx[1],
							spriteram[offs]+3,spriteram[offs + 1],
							spriteram_3[offs] & 2,spriteram_3[offs] & 1,
							spriteram_2[offs]-16,spriteram_2[offs + 1]-24 + 0x100*(spriteram_3[offs + 1] & 1),
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
					drawgfx(bitmap,Machine->gfx[1],
							spriteram[offs]+1,spriteram[offs + 1],
							spriteram_3[offs] & 2,spriteram_3[offs] & 1,
							spriteram_2[offs],spriteram_2[offs + 1]-24 + 0x100*(spriteram_3[offs + 1] & 1),
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
			else	/* normal */
				drawgfx(bitmap,Machine->gfx[1],
						spriteram[offs],spriteram[offs + 1],
						spriteram_3[offs] & 2,spriteram_3[offs] & 1,
						spriteram_2[offs]-16,spriteram_2[offs + 1]-40 + 0x100*(spriteram_3[offs + 1] & 1),
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (videoram[offs] != 0x24)	/* don't draw spaces */
		{
			int sx,sy,mx,my;


		/* Even if Galaga's screen is 28x36, the memory layout is 32x32. We therefore */
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

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the stars */
	{
		int bpen,speed;


		bpen = Machine->background_pen;
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;


			x = stars[offs].x;
			y = (stars[offs].y + stars_scroll/2) % 256 + 16;

			if (((x & 1) ^ ((y >> 4) & 1)) &&
					(bitmap->line[y][x] == bpen))
				bitmap->line[y][x] = stars[offs].col;
		}

		if (galaga_starcontrol[0] & 1) speed = 0;
		else
		{
			if (galaga_starcontrol[2] & 1) speed = -2;
			else speed = 4;
		}

		stars_scroll -= speed * (frameskip+1);
	}
}
