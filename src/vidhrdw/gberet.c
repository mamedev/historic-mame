/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x800

unsigned char *gberet_scroll;
static struct osd_bitmap *tmpbitmap1;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int gberet_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	/* Green Beret has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap1 = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void gberet_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap1);
	generic_vh_stop();
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gberet_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap1,Machine->gfx[0],
					videoram[offs] + 4 * (colorram[offs] & 0x40),
					colorram[offs] & 0x0f,
					colorram[offs] & 0x10,colorram[offs] & 0x20,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		for (i = 0;i < 32 * 8;i += 8)
		{
			int scroll;


			scroll = gberet_scroll[i / 8] + 256 * gberet_scroll[i / 8 + 32];

			clip.min_y = i;
			clip.max_y = i + 7;
			copybitmap(bitmap,tmpbitmap1,0,0,-scroll,0,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap1,0,0,512 - scroll,0,&clip,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw the sprites.  */
	if ((RAM[0xe043] & 0x08) == 0)
	{
		for (offs = 0;offs < 48*4;offs += 4)
		{
			if (spriteram[offs+3] && (spriteram[offs+1] & 0x80) == 0)
				drawgfx(bitmap,Machine->gfx[(spriteram[offs+1] & 0x40) ? 2 : 1],
						spriteram[offs],spriteram[offs+1] & 0x0f,
						spriteram[offs+1] & 0x10,spriteram[offs+1] & 0x20,
						spriteram[offs+2],spriteram[offs+3],
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
	else
	{
		for (offs = 64*4;offs < 64*4+48*4;offs += 4)
		{
			if (spriteram[offs+3] && (spriteram[offs+1] & 0x80) == 0)
				drawgfx(bitmap,Machine->gfx[(spriteram[offs+1] & 0x40) ? 2 : 1],
						spriteram[offs],spriteram[offs+1] & 0x0f,
						spriteram[offs+1] & 0x10,spriteram[offs+1] & 0x20,
						spriteram[offs+2],spriteram[offs+3],
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
