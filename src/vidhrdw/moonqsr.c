/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

#define MAX_STARS 250

unsigned char *moonqsr_attributesram;
unsigned char *moonqsr_bulletsram;
static int stars_on,stars_scroll;

struct star
{
	int x,y,col;
};
static struct star stars[MAX_STARS];
static int total_stars;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Cresta has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

***************************************************************************/
void moonqsr_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}


	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}


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


	for (i = 32;i < 32 + 64;i++)
		colortable[i] = i;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int moonqsr_vh_start(void)
{
	int generator;
	int x,y;


	stars_on = 0;

	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */
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
					stars[total_stars].col = Machine->gfx[2]->colortable[color];

					total_stars++;
				}
			}
		}
	}

	return 0;
}



void moonqsr_attributes_w(int offset,int data)
{
	if ((offset & 1) && moonqsr_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < VIDEO_RAM_SIZE;i += 32)
			dirtybuffer[i] = 1;
	}

	moonqsr_attributesram[offset] = data;
}



void moonqsr_stars_w(int offset,int data)
{
	stars_on = (data & 1);
	stars_scroll = 0;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void moonqsr_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 8 * (moonqsr_attributesram[2 * sy + 1] & 0x20),
					moonqsr_attributesram[2 * sy + 1] & 0x07,
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		for (i = 0;i < 32 * 8;i += 8)
		{
			int scroll;


			scroll = moonqsr_attributesram[2 * (i / 8)];

			clip.min_y = i;
			clip.max_y = i + 7;
			copybitmap(bitmap,tmpbitmap,0,0,scroll,0,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap,0,0,scroll - 256,0,&clip,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw the bullets */
	for (offs = 0; offs < 4*8; offs += 4)
	{
		int x,y;
		int color;


		if (offs == 4*7)
			color = Machine->gfx[2]->colortable[0x0f];	/* yellow */
		else color = Machine->gfx[2]->colortable[0x3f];	/* white */

		x = moonqsr_bulletsram[offs + 1];

		if (x >= Machine->drv->visible_area.min_x && x <= Machine->drv->visible_area.max_x)
		{
			y = 256 - moonqsr_bulletsram[offs + 3] - 4;

			if (y >= 0)
			{
				int j;


				for (j = 0; j < 3; j++)
				{
					bitmap->line[y+j][x] = color;
				}
			}
		}
	}


	/* Draw the sprites */
	for (offs = 0;offs < 4*8;offs += 4)
	{
		if (spriteram[offs + 3] > 8)	/* ???? */
		{
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram[offs + 1] & 0x3f) + 2 * (spriteram[offs + 2] & 0x20),
					spriteram[offs + 2] & 0x07,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					spriteram[offs],spriteram[offs + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the stars */
	if (stars_on)
	{
		int bpen;


		bpen = Machine->background_pen;
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;


			x = stars[offs].x;
			y = (stars[offs].y + stars_scroll/2) % 256;

			if (((x & 1) ^ ((y >> 4) & 1)) &&
					(bitmap->line[y][x] == bpen))
				bitmap->line[y][x] = stars[offs].col;
		}

		stars_scroll++;
	}
}
