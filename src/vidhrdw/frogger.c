/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *frogger_videoram;
unsigned char *frogger_attributesram;
unsigned char *frogger_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static struct osd_bitmap *tmpbitmap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Frogger has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  Additionally, there is a bit which is 1 in the upper half of the display
  (136 lines? I'm not sure of the exact value); it is connected to blue
  through a 470 ohm resistor. It is used to make the river blue instead of
  black.

***************************************************************************/
void frogger_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

	/* use an otherwise unused pen for the river background */
	palette[3*4] = 0;
	palette[3*4 + 1] = 0;
	palette[3*4 + 2] = 0x47;

	/* normal */
	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}
	/* blue background (river) */
	for (i = 4 * 8;i < 4 * 16;i++)
	{
		if (i & 3) colortable[i] = i - 4*8;
		else colortable[i] = 4;
	}
}



int frogger_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void frogger_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void frogger_videoram_w(int offset,int data)
{
	if (frogger_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		frogger_videoram[offset] = data;
	}
}



void frogger_attributes_w(int offset,int data)
{
	if ((offset & 1) && frogger_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < VIDEO_RAM_SIZE;i += 32)
			dirtybuffer[i] = 1;
	}

	frogger_attributesram[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void frogger_vh_screenrefresh(struct osd_bitmap *bitmap)
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
					frogger_videoram[offs],
					frogger_attributesram[2 * sy + 1] + (sy < 17 ? 8 : 0),
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


			scroll = frogger_attributesram[2 * (i / 8)];
			scroll = ((scroll << 4) & 0xf0) | ((scroll >> 4) & 0x0f);

			clip.min_y = i;
			clip.max_y = i + 7;
			copybitmap(bitmap,tmpbitmap,0,0,scroll,0,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap,0,0,scroll - 256,0,&clip,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw the sprites */
	for (offs = 0;offs < 4*8;offs += 4)
	{
		if (frogger_spriteram[offs + 3] != 0)
		{
			int x;


			x = frogger_spriteram[offs];
			x = ((x << 4) & 0xf0) | ((x >> 4) & 0x0f);

			drawgfx(bitmap,Machine->gfx[1],
					frogger_spriteram[offs + 1] & 0x3f,
					frogger_spriteram[offs + 2],
					frogger_spriteram[offs + 1] & 0x80,frogger_spriteram[offs + 1] & 0x40,
					x,frogger_spriteram[offs + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
