/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


unsigned char *invaders_videoram;

static struct osd_bitmap *tmpbitmap;



int invaders_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void invaders_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void invaders_videoram_w(int offset,int data)
{
	if (invaders_videoram[offset] != data)
	{
		int i,x,y;


		invaders_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->gfx[0]->colortable[1];	/* white */
			if (y >= 188) col = Machine->gfx[0]->colortable[3];	/* green */
			if (y > 32 && y < 64) col = Machine->gfx[0]->colortable[5];	/* red */

			if (data & 0x80) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->gfx[0]->colortable[0];	/* black */

			y++;
			data <<= 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
