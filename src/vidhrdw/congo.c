/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *congo_videoram;
unsigned char *congo_colorram;
unsigned char *congo_spriteram;
unsigned char *congo_background_position;
unsigned char *congo_background_enable;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];       /* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct osd_bitmap *tmpbitmap,*backgroundbitmap;



int congo_vh_start(void)
{
	int offs;
	int sx,sy;
	struct osd_bitmap *prebitmap;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	/* large bitmap for the precalculated background */
	if ((backgroundbitmap = osd_create_bitmap(512,2303+32)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}


	/* create a temporary bitmap to prepare the background before converting it */
	if ((prebitmap = osd_create_bitmap(4096,256)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		osd_free_bitmap(backgroundbitmap);
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
			if (offs + sx < 0 || offs + sx >= 4096)
			{
				backgroundbitmap->line[sy][sx] = Machine->background_pen;
				backgroundbitmap->line[sy][sx+1] = Machine->background_pen;
			}
			else
			{
				backgroundbitmap->line[sy][sx] = prebitmap->line[sx/2][offs + sx];
				backgroundbitmap->line[sy][sx+1] = prebitmap->line[sx/2][offs + sx+1];
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
void congo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(backgroundbitmap);
}



void congo_videoram_w(int offset,int data)
{
        if (congo_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

                congo_videoram[offset] = data;
	}
}



void congo_colorram_w(int offset,int data)
{
        if (congo_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

                congo_colorram[offset] = data;
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void congo_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;



	/* copy the background */
	{

                int i,skew,scroll;
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

                scroll = 1024+61 - (congo_background_position[0] + 256*congo_background_position[1]) * congo_background_enable[0];

                skew = 128;

                for (i = 0;i < 256-2*8;i++)
		{
			clip.min_y = i;
			clip.max_y = i;
			copybitmap(bitmap,backgroundbitmap,0,0,skew,-scroll,&clip,TRANSPARENCY_NONE,0);

			skew -= 2;
		}
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
        /* Sprites actually start at 0xff * [0xc031], it seems to be static tho'*/
        /* The number of active sprites is stored at 0xc032 */

        for (offs = 0x1e * 0x20 ;offs >= 0x00 ;offs -= 0x20)
	{
                if (congo_spriteram[offs+2] != 0xff)
		{
			drawgfx(bitmap,Machine->gfx[1],
                                        congo_spriteram[offs+1+2]& 0x7f,congo_spriteram[offs+2+2],
                                        congo_spriteram[offs+1+2] & 0x80,congo_spriteram[offs+2+2] & 0x80,
                                        congo_spriteram[offs+2] - 15,((congo_spriteram[offs+3+2] + 16) & 0xff) - 32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;


		sx = 8 * (31 - offs / 32);
		sy = 8 * (offs % 32);

                if (congo_videoram[offs] != 0x60)      /* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
                                        congo_videoram[offs],
                                    congo_colorram[offs],
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
