/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *mario_videoram;
unsigned char *mario_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct osd_bitmap *tmpbitmap;

static int gfx_bank;



int mario_vh_start(void)
{
	gfx_bank = 0;

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mario_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void mario_videoram_w(int offset,int data)
{
	if (mario_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mario_videoram[offset] = data;
	}
}



void mario_gfxbank_w(int offset,int data)
{
	gfx_bank = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mario_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charcode;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs % 32);
			sy = 8 * (31 - offs / 32);

			charcode = mario_videoram[offs] + 256 * gfx_bank;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,charcode >> 4,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (i = 0;i < 4*96;i += 4)
	{
		if (mario_spriteram[i])
		{
			drawgfx(bitmap,Machine->gfx[1],
					mario_spriteram[i+2],mario_spriteram[i+1] & 0x3f,
					mario_spriteram[i+1] & 0x80,mario_spriteram[i+1] & 0x40,
					32*8-mario_spriteram[i+3] - 8,mario_spriteram[i] - 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
