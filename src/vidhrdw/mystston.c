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
unsigned char *mystston_videoram3,*mystston_colorram3;
int mystston_videoram3_size;
unsigned char *mystston_scroll;
static struct osd_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int mystston_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((dirtybuffer2 = malloc(mystston_videoram3_size)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}
	memset(dirtybuffer2,1,mystston_videoram3_size);

	/* Mysterious Stones has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		free(dirtybuffer2);
		return 1;
	}

	/* Mysterious Stones has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap2 = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(tmpbitmap);
		free(dirtybuffer);
		free(dirtybuffer2);
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
	free(dirtybuffer2);
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap2);
}


void mystston_videoram3_w(int offset,int data)
{
	if (mystston_videoram3[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		mystston_videoram3[offset] = data;
	}
}



void mystston_colorram3_w(int offset,int data)
{
	if (mystston_colorram3[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		mystston_colorram3[offset] = data;
	}
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
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 16 * (offs % 32);
			sy = 16 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[2],
					videoram[offs] + 256 * (colorram[offs] & 0x01),
					0,
					sx >= 256,0,	/* flip horizontally tiles on the right half of the bitmap */
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx;


		scrollx = -*mystston_scroll;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x10) ? 4 : 3],
					spriteram[offs+1],
					0,
					spriteram[offs] & 0x02,spriteram[offs] & 0x04,
					(240 - spriteram[offs+2]) & 0xff,spriteram[offs+3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = mystston_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = 8 * (offs % 32);
		sy = 8 * (offs / 32);

		drawgfx(bitmap,Machine->gfx[(mystston_colorram2[offs] & 0x04) ? 1 : 0],
				mystston_videoram2[offs] + 256 * (mystston_colorram2[offs] & 0x03),
				0,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

