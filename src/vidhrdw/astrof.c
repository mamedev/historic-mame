/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of Astro Fighter.

  Lee Taylor 28/11/1997

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *astrof_color;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int astrof_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void astrof_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}




void astrof_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		int i,x,y;


		videoram[offset] = data;


		x = (offset % 256);
		y = 8 *(1 + (offset / 256));
		for (i = 0;i < 8 ;i++)
		{
			int col;


			col = Machine->pens[(*astrof_color / 2) + 1];
			if (y < 256)
			{
				if (data & 0x80) Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = col;
				else Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = Machine->pens[0];

				osd_mark_dirty(x,y,x,y,0);
			}
			y--;
			data <<= 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void astrof_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (full_refresh)
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
