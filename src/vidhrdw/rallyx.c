/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *rallyx_videoram2,*rallyx_colorram2;
unsigned char *rallyx_radarcarx,*rallyx_radarcary,*rallyx_radarcarcolor;
unsigned char *rallyx_scrollx,*rallyx_scrolly;
static unsigned char *dirtybuffer2;	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct osd_bitmap *tmpbitmap1;



static struct rectangle visiblearea =
{
	0*8, 28*8-1,
	0*8, 28*8-1
};

static struct rectangle radarvisiblearea =
{
	28*8, 36*8-1,
	0*8, 28*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Rally X has one 32x8 palette PROM and one 256x4 color lookup table PROM.
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
void rallyx_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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
	/* characters use colors 0-15 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0x0f;

	/* radar dots lookup table */
	/* they use colors 16-31 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 2 == 0) COLOR(2,i) = 0;
		else COLOR(2,i) = (i / 2) + 16;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int rallyx_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap1 = osd_create_bitmap(32*8,32*8)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void rallyx_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap1);
	free(dirtybuffer2);
	generic_vh_stop();
}



void rallyx_videoram2_w(int offset,int data)
{
	if (rallyx_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		rallyx_videoram2[offset] = data;
	}
}



void rallyx_colorram2_w(int offset,int data)
{
	if (rallyx_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		rallyx_colorram2[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void rallyx_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,sx,sy;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap1,Machine->gfx[0],
					rallyx_videoram2[offs],
					rallyx_colorram2[offs] & 0x3f,
					!(rallyx_colorram2[offs] & 0x40),rallyx_colorram2[offs] & 0x80,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (sy = 0;sy < 32;sy++)
	{
		for (sx = 0;sx < 8;sx++)
		{
			offs = sy * 32 + sx;

			if (dirtybuffer[offs])
			{
				dirtybuffer[offs] = 0;

				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs],
						colorram[offs] & 0x3f,
						!(colorram[offs] & 0x40),colorram[offs] & 0x80,
						8 * ((sx ^ 4) + 28),8*(sy-2),
						&radarvisiblearea,TRANSPARENCY_NONE,0);
			}
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		scrollx = -(*rallyx_scrollx - 3);
		scrolly = -(*rallyx_scrolly + 16);

		copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* radar */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < 6*2;offs += 2)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs] >> 2,spriteram_2[offs + 1],
				spriteram[offs] & 1,spriteram[offs] & 2,
				spriteram[offs + 1] - 1,224 - spriteram_2[offs],
				&visiblearea,TRANSPARENCY_COLOR,0);
	}


	/* draw the cars on the radar */
	for (offs = 0; offs < 9; offs++)
	{
		int x,y;
		int color;


		/* TODO: map to the correct color */
		color = ((rallyx_radarcarcolor[offs] >> 1) - 1) & 0x03;	/* ?????? */

		x = rallyx_radarcarx[offs] + 256 * (1 - (rallyx_radarcarcolor[offs] & 1));
		y = 237 - rallyx_radarcary[offs];

		drawgfx(bitmap,Machine->gfx[2],
				0,	/* this is just a square, generated by the hardware */
				color,
				0,0,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}
