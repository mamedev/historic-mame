/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400
#define BIGSPRITE_SIZE 0x100
#define BIGSPRITE_WIDTH 128
#define BIGSPRITE_HEIGHT 128

unsigned char *cclimber_bsvideoram;
unsigned char *cclimber_bigspriteram;
unsigned char *cclimber_column_scroll;
static unsigned char bsdirtybuffer[BIGSPRITE_SIZE];
static struct osd_bitmap *bsbitmap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Crazy Climber has three 32 bytes palette PROM.
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 4 * 24;i++)
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

	for (i = 0;i < 4 * 24;i++)
	{
		if ((i & 3) == 0) colortable[i] = 0;
		else colortable[i] = i;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int cclimber_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((bsbitmap = osd_create_bitmap(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cclimber_vh_stop(void)
{
	osd_free_bitmap(bsbitmap);
	generic_vh_stop();
}



void cclimber_colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		/* bit 5 of the address is not used for color memory. There is just */
		/* 512 bytes of memory; every two consecutive rows share the same memory */
		/* region. */
		offset &= 0xffdf;

		dirtybuffer[offset] = 1;
		dirtybuffer[offset + 0x20] = 1;

		colorram[offset] = data;
		colorram[offset + 0x20] = data;
	}
}



void cclimber_bigsprite_videoram_w(int offset,int data)
{
	if (cclimber_bsvideoram[offset] != data)
	{
		bsdirtybuffer[offset] = 1;

		cclimber_bsvideoram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap)
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

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 0x10) ? 1 : 0],
					videoram[offs],
					colorram[offs] & 0x0f,
					colorram[offs] & 0x40,colorram[offs] & 0x80,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -cclimber_column_scroll[i];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the "big sprite" */
	{
		int newcol;
		static int lastcol;


		newcol = cclimber_bigspriteram[1] & 0x07;

		/* first of all, update it. */
		for (offs = 0;offs < BIGSPRITE_SIZE;offs++)
		{
			int sx,sy;


			if (bsdirtybuffer[offs] || newcol != lastcol)
			{
				bsdirtybuffer[offs] = 0;

				sx = 8 * (offs % 16);
				sy = 8 * (offs / 16);

				drawgfx(bsbitmap,Machine->gfx[2],
						cclimber_bsvideoram[offs],newcol,
						0,0,sx,sy,
						0,TRANSPARENCY_NONE,0);
			}

		}

		lastcol = newcol;

		copybitmap(bitmap,bsbitmap,
				cclimber_bigspriteram[1] & 0x10,cclimber_bigspriteram[1] & 0x20,
				136 - cclimber_bigspriteram[3],128 - cclimber_bigspriteram[2],
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
	}

	/* draw sprites (must be done after the "big sprite" to obtain the correct priority) */
	for (i = 0;i < 8*4;i += 4)
	{
		drawgfx(bitmap,Machine->gfx[spriteram[i + 1] & 0x10 ? 4 : 3],
				spriteram[i] & 0x3f,spriteram[i + 1] & 0x0f,
				spriteram[i] & 0x40,spriteram[i] & 0x80,
				spriteram[i + 3],240 - spriteram[i + 2],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
