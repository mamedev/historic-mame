/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define VIDEO_RAM_SIZE 0x400

unsigned char *venture_characterram;
static unsigned char dirtycharacter[256];



void venture_characterram_w(int offset,int data)
{
	if (venture_characterram[offset] != data)
	{
		dirtycharacter[offset / 8] = 1;

		venture_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void venture_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;
	extern struct GfxLayout venture_charlayout;


	for(offs = 0;offs < 256;offs++)
	{
			if (dirtycharacter[offs] == 1)
			{
				decodechar(Machine->gfx[1],offs,venture_characterram,&venture_charlayout);
				dirtycharacter[offs] = 2;
			}
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


		/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[1],charcode,venture_characterram,&venture_charlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					charcode,0,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	for (i = 0;i < 256;i++)
	{
		if (dirtycharacter[i] == 2) dirtycharacter[i] = 0;
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	{
			int sx,sy;


			sx = 236-RAM[0x5000];
			sy = 244-RAM[0x5040];

			drawgfx(bitmap,Machine->gfx[0],
					0,1,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			sx = 236-RAM[0x5080];
			sy = 244-RAM[0x50c0];

			drawgfx(bitmap,Machine->gfx[0],
					1,2,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
