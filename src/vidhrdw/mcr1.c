/***************************************************************************

  vidhrdw/mcr1.c

  Functions to emulate the video hardware of an mcr 1 style machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr1_paletteram_bg,*mcr1_paletteram_r;



/***************************************************************************

  Generic MCR/I video routines

***************************************************************************/

void mcr1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
   int offs;
   int mx,my;


   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 1;offs >= 0;offs--)
   {
      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs) % 32;
			my = (offs) / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,
					16*mx,16*my,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
      }
   }

   /* copy the character mapped graphics */
   copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

   /* Draw the sprites. */
   for (offs = 0;offs < spriteram_size;offs += 4)
   {
      int code,flipx,flipy,sx,sy;

      if (spriteram[offs] == 0)
			continue;

      sx = (spriteram[offs+2]+3)*2;
      sy = (241-spriteram[offs])*2;
      code = spriteram[offs+1] & 0x3f;
      flipx = spriteram[offs+1] & 0x40;
      flipy = spriteram[offs+1] & 0x80;

      drawgfx(bitmap,Machine->gfx[1],
			  code,
			  0,
			  flipx,flipy,
			  sx,sy,
			  &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
   }
}
