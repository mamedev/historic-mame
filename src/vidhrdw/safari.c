/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char dirtycharacter[256];

unsigned char *safari_characterram;

void safari_characterram_w(int offset,int data)
{
	if (safari_characterram[offset] != data)
	{
		safari_characterram[offset] = data;
		dirtycharacter[offset / 8] = 1;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void safari_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
    for (offs = videoram_size; offs >= 0; offs--)
	{
        int sx,sy,ch;

		ch = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[ch])
		{
			if (dirtycharacter[ch] == 1)
			{
				decodechar(Machine->gfx[0],ch,safari_characterram,
						   Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtycharacter[ch] = 2;
			}

			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					ch,
					0,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
    }

	for (offs = 0; offs < 256; offs++)
	{
		if (dirtycharacter[offs] == 2)
		{
			dirtycharacter[offs] = 0;
		}
	}

    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
