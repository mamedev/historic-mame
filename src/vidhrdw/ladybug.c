/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *ladybug_videoram;
unsigned char *ladybug_colorram;
unsigned char *ladybug_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Lady Bug has a 32 bytes palette PROM and a 32 bytes sprite color lookup
  table PROM.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- inverter -- 220 ohm resistor  -- BLUE
        -- inverter -- 220 ohm resistor  -- GREEN
        -- inverter -- 220 ohm resistor  -- RED
        -- inverter -- 470 ohm resistor  -- BLUE
        -- unused
        -- inverter -- 470 ohm resistor  -- GREEN
        -- unused
  bit 0 -- inverter -- 470 ohm resistor  -- RED

***************************************************************************/
void ladybug_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit1,bit2;


		bit1 = (~color_prom[i] >> 0) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
		palette[3*i] = 0x47 * bit1 + 0x97 * bit2;
		bit1 = (~color_prom[i] >> 2) & 0x01;
		bit2 = (~color_prom[i] >> 6) & 0x01;
		palette[3*i + 1] = 0x47 * bit1 + 0x97 * bit2;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x47 * bit1 + 0x97 * bit2;
	}

	/* characters */
	for (i = 0;i < 8;i++)
	{
		colortable[4 * i] = 0;
		colortable[4 * i + 1] = i + 0x08;
		colortable[4 * i + 2] = i + 0x10;
		colortable[4 * i + 3] = i + 0x18;
	}

	/* sprites */
	for (i = 0;i < 4 * 8;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* low 4 bits are for sprite n */
		bit0 = (color_prom[i + 32] >> 3) & 0x01;
		bit1 = (color_prom[i + 32] >> 2) & 0x01;
		bit2 = (color_prom[i + 32] >> 1) & 0x01;
		bit3 = (color_prom[i + 32] >> 0) & 0x01;
		colortable[i + 4 * 8] = 1 * bit0 + 2 * bit1 + 4 * bit2 + 8 * bit3;

		/* high 4 bits are for sprite n + 8 */
		bit0 = (color_prom[i + 32] >> 7) & 0x01;
		bit1 = (color_prom[i + 32] >> 6) & 0x01;
		bit2 = (color_prom[i + 32] >> 5) & 0x01;
		bit3 = (color_prom[i + 32] >> 4) & 0x01;
		colortable[i + 4 * 16] = 1 * bit0 + 2 * bit1 + 4 * bit2 + 8 * bit3;
	}
}



int ladybug_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ladybug_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void ladybug_videoram_w(int offset,int data)
{
	if (ladybug_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		ladybug_videoram[offset] = data;
	}
}



void ladybug_colorram_w(int offset,int data)
{
	if (ladybug_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		ladybug_colorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ladybug_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					ladybug_videoram[offs] + 32 * (ladybug_colorram[offs] & 8),
					ladybug_colorram[offs],
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	/* sprites in the columns 15, 1 and 0 are outside of the visible area */
	for (offs = 14 * 0x40;offs >= 2;offs -= 0x40)
	{
		i = 0;
		while (i < 0x40 && ladybug_spriteram[offs + i] != 0)
			i += 4;

		while (i > 0)
		{
/*
 aabbcccc ddddddee fffghhhh iiiiiiii

 aa unknown
 bb flip
 cccc x offset
 dddddd sprite code
 ee unknown
 fff unknown
 g sprite bank
 hhhh color
 iiiiiiii y position
*/
			i -= 4;

			drawgfx(bitmap,Machine->gfx[1],
					(ladybug_spriteram[offs + i + 1] >> 2) + 4 * (ladybug_spriteram[offs + i + 2] & 0x10),
					ladybug_spriteram[offs + i + 2] & 0x0f,
					ladybug_spriteram[offs + i] & 0x10,ladybug_spriteram[offs + i] & 0x20,
					offs / 4 - 8 + (ladybug_spriteram[offs + i] & 0x0f),
					240 - ladybug_spriteram[offs + i + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
