/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This file is used by the Crazy Climber and Crazy Kong drivers.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define BIGSPRITE_WIDTH 128
#define BIGSPRITE_HEIGHT 128

unsigned char *cclimber_bsvideoram;
int cclimber_bsvideoram_size;
unsigned char *cclimber_bigspriteram;
unsigned char *cclimber_column_scroll;
static unsigned char *bsdirtybuffer;
static struct osd_bitmap *bsbitmap;
static int flipscreen[2];



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


	/* character and sprite lookup table */
	/* they use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* big sprite lookup table */
	/* it uses colors 64-95 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 4 == 0) COLOR(2,i) = 0;
		else COLOR(2,i) = i + 64;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int cclimber_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((bsdirtybuffer = malloc(cclimber_bsvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(bsdirtybuffer,1,cclimber_bsvideoram_size);

	if ((bsbitmap = osd_create_bitmap(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
	{
		free(bsdirtybuffer);
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
	free(bsdirtybuffer);
	generic_vh_stop();
}



void cclimber_flipscreen_w(int offset,int data)
{
	if (flipscreen[offset] != (data & 1))
	{
		flipscreen[offset] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
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
static void drawbigsprite(struct osd_bitmap *bitmap)
{
	int sx,sy,flipx,flipy;


	sx = 136 - cclimber_bigspriteram[3];
	sy = 128 - cclimber_bigspriteram[2];
	flipx = cclimber_bigspriteram[1] & 0x10;
	flipy = cclimber_bigspriteram[1] & 0x20;
	if (flipscreen[1])	/* only the Y direction has to be flipped */
	{
		sy = 128 - sy;
		flipy = !flipy;
	}

	copybitmap(bitmap,bsbitmap,
			flipx,flipy,
			sx,sy,
			&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
}


void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			flipx = colorram[offs] & 0x40;
			flipy = colorram[offs] & 0x80;
			if (flipscreen[0])
			{
				sx = 31 - sx;
				flipx = !flipx;
			}
			if (flipscreen[1])
			{
				sy = 31 - sy;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 0x10) ? 1 : 0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x0f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -cclimber_column_scroll[offs];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* update the "big sprite" */
	{
		int newcol;
		static int lastcol;


		newcol = cclimber_bigspriteram[1] & 0x07;

		for (offs = cclimber_bsvideoram_size - 1;offs >= 0;offs--)
		{
			int sx,sy;


			if (bsdirtybuffer[offs] || newcol != lastcol)
			{
				bsdirtybuffer[offs] = 0;

				sx = offs % 16;
				sy = offs / 16;

				drawgfx(bsbitmap,Machine->gfx[2],
						cclimber_bsvideoram[offs],newcol,
						0,0,
						8*sx,8*sy,
						0,TRANSPARENCY_NONE,0);
			}

		}

		lastcol = newcol;
	}


	if (cclimber_bigspriteram[0] & 1)
		/* draw the "big sprite" below sprites */
		drawbigsprite(bitmap);


	/* draw sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sx,sy,flipx,flipy;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs + 2];
		flipx = spriteram[offs] & 0x40;
		flipy = spriteram[offs] & 0x80;
		if (flipscreen[0])
		{
			sx = 240 - sx;
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[spriteram[offs + 1] & 0x10 ? 4 : 3],
				(spriteram[offs] & 0x3f) + 2 * (spriteram[offs + 1] & 0x20),
				spriteram[offs + 1] & 0x0f,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	if ((cclimber_bigspriteram[0] & 1) == 0)
		/* draw the "big sprite" over sprites */
		drawbigsprite(bitmap);
}
