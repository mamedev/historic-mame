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

		drawgfx(prebitmap,Machine->gfx[2],
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

		drawgfx(prebitmap,Machine->gfx[2],
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


#if 0
	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					videoram[offs] >> 4,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
#endif


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
	else clearbitmap(bitmap);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs] != 0xff)
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs+1],spriteram[offs+2],
					spriteram[offs+1] & 0x80,spriteram[offs+1] & 0x40,
					spriteram[offs] - 15,((spriteram[offs+3] + 16) & 0xff) - 32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


#if 0
	/* copy the frontmost playfield */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
#endif

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = 8 * (31 - offs / 32);
		sy = 8 * (offs % 32);

		if (videoram[offs] != 0x60)	/* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					videoram[offs] >> 4,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
