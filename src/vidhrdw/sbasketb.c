/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *sbasketb_scroll;
static struct rectangle scroll_area = { 0*8, 32*8-1, 0*8, 32*8-1 };



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sbasketb_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int offs;
        int sx,sy,attribute;


        /* for every character in the Video RAM, check if it has been modified */
        /* since last time and update it accordingly. */
        for (offs = videoram_size - 1;offs >= 0;offs--)
        {
                if (dirtybuffer[offs])
                {
                        dirtybuffer[offs] = 0;

                        attribute = colorram[offs];

                        sx = 8 * (31 - offs / 32);
                        sy = 8 * (offs % 32);

                        drawgfx(tmpbitmap,Machine->gfx[(attribute & 0x20) >> 5],
                                        videoram[offs],
                                        0,
                                        attribute & 0x80,
                                        attribute & 0x40,
                                        sx,sy,
                                        &scroll_area,TRANSPARENCY_NONE,0);
                }
        }


        /* copy the temporary bitmap to the screen */
        {
                int scroll[32], i;

                for (i = 0;i < 6;i++)
                        scroll[i] = 0;

                for (i = 6;i < 32;i++)
                        scroll[i] = *sbasketb_scroll;

                copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }

        /* Draw the sprites. */
        for (offs = spriteram_size - 16;offs >= 0;offs -= 16)
        {
                if (spriteram[offs + 4] || spriteram[offs + 6])
                        drawgfx(bitmap,Machine->gfx[2],
                                        spriteram[offs + 0xe] | ((spriteram[offs + 0xf] & 0x20) << 3),
                                        0,
                                        spriteram[offs + 0xf] & 0x80,
                                        spriteram[offs + 0xf] & 0x40,
                                        spriteram[offs + 6],spriteram[offs + 4],
                                        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }
}
