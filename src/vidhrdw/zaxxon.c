/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *zaxxon_background_position;
unsigned char *zaxxon_background_color;
unsigned char *zaxxon_background_enable;
static struct osd_bitmap *backgroundbitmap1,*backgroundbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Zaxxon has one 256x8 palette PROM and one 256x4 PROM which contains the
  color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same row).
  Congo Bongo has similar hardware, but it has color RAM instead of the
  lookup PROM.

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
void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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


	/* all gfx elements use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int zaxxon_vh_start(void)
{
	int offs;
	int sx,sy;
	struct osd_bitmap *prebitmap;


	if (generic_vh_start() != 0)
		return 1;

	/* large bitmap for the precalculated background */
	if ((backgroundbitmap1 = osd_create_bitmap(512,2303+32)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((backgroundbitmap2 = osd_create_bitmap(512,2303+32)) == 0)
	{
		osd_free_bitmap(backgroundbitmap1);
		generic_vh_stop();
		return 1;
	}

	/* create a temporary bitmap to prepare the background before converting it */
	if ((prebitmap = osd_create_bitmap(4096,256)) == 0)
	{
		osd_free_bitmap(backgroundbitmap2);
		osd_free_bitmap(backgroundbitmap1);
		generic_vh_stop();
		return 1;
	}

	/* prepare the background */
	for (offs = 0;offs < 0x4000;offs++)
	{
		sx = 8 * (511 - offs / 32);
		sy = 8 * (offs % 32);

		drawgfx(prebitmap,Machine->gfx[1],
				Machine->memory_region[2][offs] + 256 * (Machine->memory_region[2][0x4000 + offs] & 3),
				Machine->memory_region[2][0x4000 + offs] >> 4,
				0,0,
				sx,sy,
				0,TRANSPARENCY_NONE,0);
	}

	/* the background is stored as a rectangle, but is drawn by the hardware skewed: */
	/* go right two pixels, then up one pixel. Doing the conversion at run time would */
	/* be extremely expensive, so we do it now. To save memory, we squash the image */
	/* horizontally (doing line shifts at run time is much less expensive than doing */
	/* column shifts) */
	for (offs = -510;offs < 4096;offs += 2)
	{
		sy = (2302-510/2) - offs/2;

		for (sx = 0;sx < 512;sx += 2)
		{
			if (offs + sx >= 0 && offs + sx < 4096)
			{
				backgroundbitmap1->line[sy][sx] = prebitmap->line[sx/2][offs + sx];
				backgroundbitmap1->line[sy][sx+1] = prebitmap->line[sx/2][offs + sx+1];
			}
		}
	}


	/* prepare a second background with different colors, used in the death sequence */
	for (offs = 0;offs < 0x4000;offs++)
	{
		sx = 8 * (511 - offs / 32);
		sy = 8 * (offs % 32);

		drawgfx(prebitmap,Machine->gfx[1],
				Machine->memory_region[2][offs] + 256 * (Machine->memory_region[2][0x4000 + offs] & 3),
				16 + (Machine->memory_region[2][0x4000 + offs] >> 4),
				0,0,
				sx,sy,
				0,TRANSPARENCY_NONE,0);
	}

	/* the background is stored as a rectangle, but is drawn by the hardware skewed: */
	/* go right two pixels, then up one pixel. Doing the conversion at run time would */
	/* be extremely expensive, so we do it now. To save memory, we squash the image */
	/* horizontally (doing line shifts at run time is much less expensive than doing */
	/* column shifts) */
	for (offs = -510;offs < 4096;offs += 2)
	{
		sy = (2302-510/2) - offs/2;

		for (sx = 0;sx < 512;sx += 2)
		{
			if (offs + sx >= 0 && offs + sx < 4096)
			{
				backgroundbitmap2->line[sy][sx] = prebitmap->line[sx/2][offs + sx];
				backgroundbitmap2->line[sy][sx+1] = prebitmap->line[sx/2][offs + sx+1];
			}
		}
	}

	osd_free_bitmap(prebitmap);


	/* free the graphics ROMs, they are no longer needed */
	free(Machine->memory_region[2]);
	Machine->memory_region[2] = 0;


	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void zaxxon_vh_stop(void)
{
	osd_free_bitmap(backgroundbitmap2);
	osd_free_bitmap(backgroundbitmap1);
	generic_vh_stop();
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* copy the background */
	if (*zaxxon_background_enable)
	{
		int i,skew,scroll;
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		scroll = 2048+63 - (zaxxon_background_position[0] + 256*zaxxon_background_position[1]);

		skew = 128;

		for (i = 0;i < 256-4*8;i++)
		{
			clip.min_y = i;
			clip.max_y = i;

			if (*zaxxon_background_color)
				copybitmap(bitmap,backgroundbitmap2,0,0,skew,-scroll,&clip,TRANSPARENCY_NONE,0);
			else copybitmap(bitmap,backgroundbitmap1,0,0,skew,-scroll,&clip,TRANSPARENCY_NONE,0);

			skew -= 2;
		}
	}
	else fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs] != 0xff)
		{
				drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs+1] & 0x3f,
					spriteram[offs+2] & 0x3f,
					spriteram[offs+1] & 0x80,spriteram[offs+1] & 0x40,
					spriteram[offs] - 15,((spriteram[offs+3] + 16) & 0xff) - 32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;
		static unsigned char color_prom[] =
		{
			/* U72 - character color lookup table */
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x06,0x06,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x06,0x06,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x01,0x01,0x01,0x03,0x06,0x06,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x05,0x05,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x05,0x05,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x05,0x05,0x06,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x05,0x05,0x04,0x01,
			0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x00,0x04,0x00,0x03,0x03,0x02,0x01,
			0x04,0x01,0x04,0x01,0x04,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x05,0x05,0x04,0x01
		};


		sx = 31 - offs / 32;
		sy = offs % 32;

		if (videoram[offs] != 0x60)	/* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					color_prom[sy + 32 * (7-sx/4)],
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
