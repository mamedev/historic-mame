/***************************************************************************

	vidhrdw.c

	Functions to emulate the video hardware of the machine.

	There are only a few differences between the video hardware of Mysterious
	Stones and Mat Mania. The tile bank select bit is different and the sprite
	selection seems to be different as well. Additionally, the palette is stored
	differently. I'm also not sure that the 2nd tile page is really used in
	Mysterious Stones.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *mystston_videoram2,*mystston_colorram2;
int mystston_videoram2_size;
unsigned char *mystston_scroll;
static int flipscreen;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int mystston_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	/* Mysterious Stones has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mystston_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



void mystston_2000_w(int offset,int data)
{
	/* bits 4 and 5 are coin counters */
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);

	/* bit 7 is screen flip */
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
		memset(dirtybuffer,1,videoram_size);
	}

	/* other bits unused? */
if (errorlog) fprintf(errorlog,"PC %04x: 2000 = %02x\n",cpu_get_pc(),data);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mystston_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,flipy;


			dirtybuffer[offs] = 0;

			sx = 15 - offs / 32;
			sy = offs % 32;
			flipy = (sy >= 16) ? 1 : 0;	/* flip horizontally tiles on the right half of the bitmap */
			if (flipscreen)
			{
				sx = 15 - sx;
				sy = 31 - sy;
				flipy = !flipy;
			}
			drawgfx(tmpbitmap,Machine->gfx[2],
					videoram[offs] + 256 * (colorram[offs] & 0x01),
					0,
					flipscreen,flipy,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrolly;


		scrolly = -*mystston_scroll;
		if (flipscreen) scrolly = 256 - scrolly;

		copyscrollbitmap(bitmap,tmpbitmap,0,0,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			int sx,sy,flipx,flipy;


			sx = 240 - spriteram[offs+3];
			sy = (240 - spriteram[offs+2]) & 0xff;
			flipx = spriteram[offs] & 0x04;
			flipy = spriteram[offs] & 0x02;
			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x10) ? 4 : 3],
					spriteram[offs+1],
					0,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = mystston_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = 31 - offs / 32;
		sy = offs % 32;
		if (flipscreen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
		}

		drawgfx(bitmap,Machine->gfx[(mystston_colorram2[offs] & 0x04) ? 1 : 0],
				mystston_videoram2[offs] + 256 * (mystston_colorram2[offs] & 0x03),
				0,
				flipscreen,flipscreen,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
