/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400


unsigned char *mpatrol_videoram;
unsigned char *mpatrol_colorram;
unsigned char *mpatrol_spriteram1;
unsigned char *mpatrol_spriteram2;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static unsigned char scrollreg[4];


static struct osd_bitmap *tmpbitmap;



int mpatrol_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mpatrol_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void mpatrol_videoram_w(int offset,int data)
{
	if (mpatrol_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mpatrol_videoram[offset] = data;
	}
}



void mpatrol_colorram_w(int offset,int data)
{
	if (mpatrol_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mpatrol_colorram[offset] = data;
	}
}



void mpatrol_scroll_w(int offset,int data)
{
	scrollreg[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mpatrol_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);
			drawgfx(tmpbitmap,Machine->gfx[0],
					mpatrol_videoram[offs] + 2 * (mpatrol_colorram[offs] & 0x80),
					mpatrol_colorram[offs] & 0x7f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		for (i = 0;i < 32 * 8;i += 8)
		{
			int scroll;


			scroll = 0;
			if (i >= (32-8)*8) scroll = scrollreg[0];
			clip.min_y = i;
			clip.max_y = i + 7;
			copybitmap(bitmap,tmpbitmap,0,0,scroll,0,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap,0,0,scroll - 256,0,&clip,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw the sprites. */
	for (offs = 4*23;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				mpatrol_spriteram2[offs + 2],
				mpatrol_spriteram2[offs + 1] & 0x3f,
				mpatrol_spriteram2[offs + 1] & 0x40,mpatrol_spriteram2[offs + 1] & 0x80,
				mpatrol_spriteram2[offs + 3],241 - mpatrol_spriteram2[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
	for (offs = 4*23;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				mpatrol_spriteram1[offs + 2],
				mpatrol_spriteram1[offs + 1] & 0x3f,
				mpatrol_spriteram1[offs + 1] & 0x40,mpatrol_spriteram1[offs + 1] & 0x80,
				mpatrol_spriteram1[offs + 3],241 - mpatrol_spriteram1[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
