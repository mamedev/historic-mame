/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *sharkatt_videoram;

enum { BLACK, RED, GREEN, YELLOW, WHITE, CYAN, PURPLE };



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int sharkatt_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void sharkatt_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void sharkatt_videoram_w(int offset,int data)
{
	if (sharkatt_videoram[offset] != data)
	{
		int i,x,y;


		sharkatt_videoram[offset] = data;

		y = offset / 32;
		x = 256+16 + 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[WHITE];

			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->pens[BLACK];

			osd_mark_dirty(x,y,x+1,y+1,0);

			x++;
			data <<= 1;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sharkatt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
