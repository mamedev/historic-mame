/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of Bump'n'Jump.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/*
 *  Variables used to control the background.
 */
       unsigned char     *bnj_bgram;
       int                bnj_bgram_size;
       unsigned char     *bnj_scroll1;
       unsigned char     *bnj_scroll2;
static unsigned char     *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int bnj_vh_start (void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(bnj_bgram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,bnj_bgram_size);

	/* the background area is twice as tall as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
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
void bnj_vh_stop (void)
{
	osd_free_bitmap (tmpbitmap2);
	free (dirtybuffer2);
	generic_vh_stop ();
}



void bnj_bg_w (int offset, int data)
{
	if (bnj_bgram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		bnj_bgram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bnj_vh_screenrefresh (struct osd_bitmap *bitmap)
{
    int offs;


    /*
     *  For each character in the background RAM, check if it has been
     *  modified since last time and update it accordingly.
     */
    for (offs = bnj_bgram_size-1; offs >=0; offs--)
    {
        if (dirtybuffer2[offs])
        {
            int sx,sy;


            dirtybuffer2[offs] = 0;

            sx = 16 * (((offs % 0x100) < 0x80) ? offs % 8 : (offs % 8) + 8);
            sy = 16 * ((offs < 0x100) ? ((offs % 0x80) / 8) : ((offs % 0x80) / 8) + 16);

            drawgfx (tmpbitmap2, Machine->gfx[2],
                    (bnj_bgram[offs] >> 4) + ((offs & 0x80) >> 3) + 32,
                    0,
                    0, 0,
                    sx, sy,
                    0, TRANSPARENCY_NONE, 0);
        }
    }

    /*
     *  Copy the background graphics.
     */
    if (*bnj_scroll1)
    {
        int scroll;

        scroll = (*bnj_scroll1 & 0x02) * 128 + 511 - *bnj_scroll2;
        copyscrollbitmap (bitmap, tmpbitmap2, 0, 0, 1, &scroll, &Machine->drv->visible_area,TRANSPARENCY_NONE, 0);
    }
    else
    {
        fillbitmap(bitmap, Machine->pens[0], &Machine->drv->visible_area);
    }


	/* Draw the sprites */
	for (offs = 0;offs < videoram_size;offs += 4*0x20)
	{
		if (videoram[offs + 0] & 0x01)
			drawgfx(bitmap,Machine->gfx[1],
					videoram[offs + 0x20],
					0,
					videoram[offs + 0] & 0x02,videoram[offs + 0] & 0x04,
					240 - videoram[offs + 2*0x20],videoram[offs + 3*0x20],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int code;


		code = videoram[offs] + 256 * (colorram[offs] & 0x03);

		if (code != 0)		/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(bitmap,Machine->gfx[0],
					code,
					0,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
