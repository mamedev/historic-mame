/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int charbank;
static int flipx,flipy;


WRITE_HANDLER( hexa_d008_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* bit 0 = flipx (or y?) */
	if (flipx != (data & 0x01))
	{
		flipx = data & 0x01;
		memset(dirtybuffer,1,videoram_size);
	}

	/* bit 1 = flipy (or x?) */
	if (flipy != (data & 0x02))
	{
		flipy = data & 0x02;
		memset(dirtybuffer,1,videoram_size);
	}

	/* bit 2 - 3 unknown */

	/* bit 4 could be the ROM bank selector for 8000-bfff (not sure) */
	bankaddress = 0x10000 + ((data & 0x10) >> 4) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bit 5 = char bank */
	if (charbank != ((data & 0x20) >> 5))
	{
		charbank = (data & 0x20) >> 5;
		memset(dirtybuffer,1,videoram_size);
	}

	/* bit 6 - 7 unknown */
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( hexa )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs + 1])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs + 1] = 0;

			sx = (offs/2) % 32;
			sy = (offs/2) / 32;
			if (flipx) sx = 31 - sx;
			if (flipy) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs + 1] + ((videoram[offs] & 0x07) << 8) + (charbank << 11),
					(videoram[offs] & 0xf8) >> 3,
					flipx,flipy,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
