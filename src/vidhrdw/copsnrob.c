/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"


static const struct artwork_element copsnrob_overlay[] =
{
	{{  0,  71, 0, 255}, 0x40, 0x40, 0xc0, OVERLAY_DEFAULT_OPACITY},	/* blue */
	{{ 72, 187, 0, 255}, 0xf0, 0xf0, 0x30, OVERLAY_DEFAULT_OPACITY},	/* yellow */
	{{188, 255, 0, 255}, 0xbd, 0x9b, 0x13, OVERLAY_DEFAULT_OPACITY},	/* amber */
	{{-1,-1,-1,-1},0,0,0,0}
};

unsigned char *copsnrob_bulletsram;
unsigned char *copsnrob_carimage;
unsigned char *copsnrob_cary;
unsigned char *copsnrob_trucky;


int copsnrob_vh_start(void)
{
	overlay_create(copsnrob_overlay, 2, Machine->drv->total_colors - 2);

    return 0;
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void copsnrob_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    int offs, x;


	palette_recalc();


    /* redrawing the entire display is faster in this case */

    for (offs = videoram_size;offs >= 0;offs--)
    {
		int sx,sy;

		sx = 31 - (offs % 32);
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] & 0x3f,0,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
    }


    /* Draw the cars. Positioning was based on a screen shot */
    if (copsnrob_cary[0])
    {
        drawgfx(bitmap,Machine->gfx[1],
                copsnrob_carimage[0],0,
                1,0,
                0xe4,256-copsnrob_cary[0],
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }

    if (copsnrob_cary[1])
    {
        drawgfx(bitmap,Machine->gfx[1],
                copsnrob_carimage[1],0,
                1,0,
                0xc4,256-copsnrob_cary[1],
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }

    if (copsnrob_cary[2])
    {
        drawgfx(bitmap,Machine->gfx[1],
                copsnrob_carimage[2],0,
                0,0,
                0x24,256-copsnrob_cary[2],
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }

    if (copsnrob_cary[3])
    {
        drawgfx(bitmap,Machine->gfx[1],
                copsnrob_carimage[3],0,
                0,0,
                0x04,256-copsnrob_cary[3],
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }


    /* Draw the beer truck. Positioning was based on a screen shot.
       Even though the manual says there can be up to 3 beer trucks
       on the screen, after examining the code, I don't think that's the
       case. I also verified this just by playing the game, if there were
       invisible trucks, the bullets would disappear. */

    if (copsnrob_trucky[0])
    {
        drawgfx(bitmap,Machine->gfx[2],
                0,0,
                0,0,
                0x80,256-copsnrob_trucky[0],
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }


    /* Draw the bullets.
       They are flickered on/off every frame by the software, so don't
       play it with frameskip 1 or 3, as they could become invisible */

    for (x = 0; x < 256; x++)
    {
	    int y, bullet, mask1, mask2, val;


        val = copsnrob_bulletsram[x];

        // Check for the most common case
        if (!(val & 0x0f)) continue;

        mask1 = 0x01;
        mask2 = 0x10;

        // Check each bullet
        for (bullet = 0; bullet < 4; bullet++)
        {
            if (val & mask1)
            {
                for (y = 0; y <= Machine->drv->visible_area.max_y; y++)
                {
                    if (copsnrob_bulletsram[y] & mask2)
                    {
                        plot_pixel(bitmap, 256-x, y, Machine->pens[1]);
                    }
                }
            }

            mask1 <<= 1;
            mask2 <<= 1;
        }
    }
}
