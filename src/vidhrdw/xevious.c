/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void xevious_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;


	clearbitmap(bitmap);


	/* Draw the sprites. */
	for (i = 0;i < 64;i++)
	{
		if ((spriteram_3[2*i+1] & 2) == 0)
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[2*i],spriteram[2*i+1],
					spriteram_3[2*i] & 2,spriteram_3[2*i] & 1,
					spriteram_2[2*i]-16,(spriteram_2[2*i+1]-40) + 0x100*(spriteram_3[2*i+1] & 1),
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (videoram[offs] != 0x24)	/* don't draw spaces */
		{
			int sx,sy,mx,my;


		/* Even if xevious's screen is 28x36, the memory layout is 32x32. We therefore */
		/* have to convert the memory coordinates into screen coordinates. */
		/* Note that 32*32 = 1024, while 28*36 = 1008: therefore 16 bytes of Video RAM */
		/* don't map to a screen position. We don't check that here, however: range */
		/* checking is performed by drawgfx(). */

			mx = offs / 32;
			my = offs % 32;

			if (mx <= 1)
			{
				sx = 29 - my;
				sy = mx + 34;
			}
			else if (mx >= 30)
			{
				sx = 29 - my;
				sy = mx - 30;
			}
			else
			{
				sx = 29 - mx;
				sy = my + 2;
			}

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
