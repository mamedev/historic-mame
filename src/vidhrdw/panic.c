/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Sprite number - Bank conversion */

static const unsigned char Remap[64][2] = {
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, // 00
{0x00,0},{0x26,1},{0x25,1},{0x24,1},{0x23,1},{0x22,1},{0x21,1},{0x20,1}, // 08
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, // 10
{0x00,0},{0x16,1},{0x15,1},{0x14,1},{0x13,1},{0x12,1},{0x11,1},{0x10,1}, // 18
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, // 20
{0x00,0},{0x06,1},{0x05,1},{0x04,1},{0x03,1},{0x02,1},{0x01,1},{0x00,1}, // 28
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, // 30
{0x07,3},{0x06,3},{0x05,3},{0x04,3},{0x03,3},{0x02,3},{0x01,3},{0x00,3}, // 38
};

unsigned char *panic_videoram;

void panic_videoram_w(int offset,int data)
{
	if (panic_videoram[offset] != data)
	{
		int i,x,y;
		int col;

		panic_videoram[offset] = data;

		x = offset / 32 + 16;
		y = 256-8 - 8 * (offset % 32);

		col = Machine->gfx[0]->colortable[7];	/* white */

		for (i = 0;i < 8;i++)
		{

			if (data & 0x01) tmpbitmap->line[y][x] = col;
			else tmpbitmap->line[y][x] = Machine->gfx[0]->colortable[0];	/* black */

			y++;
			data >>= 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void panic_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs, Sprite, Bank, Rotate;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */

	for (offs = 0;offs <= 7*4;offs += 4)
	{
		if (spriteram[offs] != 0)
        {
        	/* Remap sprite number to my layout */

            Sprite = Remap[(spriteram[offs] & 0x3F)][0];
            Bank   = Remap[(spriteram[offs] & 0x3F)][1];
            Rotate = spriteram[offs] & 0x40;

            /* Switch Bank */

            if(spriteram[offs+3] & 0x08) Bank=2;

		    drawgfx(bitmap,Machine->gfx[Bank],
				    Sprite,
				    spriteram[offs+3] & 0x07,
				    Rotate,0,
				    spriteram[offs+1]+16,spriteram[offs+2]-16,
				    &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }
	}
}
