/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400

unsigned char *nibbler_videoram1;
unsigned char *nibbler_videoram2;
unsigned char *nibbler_colorram;
unsigned char *nibbler_characterram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static unsigned char dirtycharacter[256];

static struct osd_bitmap *tmpbitmap;



int nibbler_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void nibbler_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void nibbler_videoram1_w(int offset,int data)
{
	if (nibbler_videoram1[offset] != data)
	{
		dirtybuffer[offset] = 1;

		nibbler_videoram1[offset] = data;
	}
}



void nibbler_videoram2_w(int offset,int data)
{
	if (nibbler_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		nibbler_videoram2[offset] = data;
	}
}



void nibbler_colorram_w(int offset,int data)
{
	if (nibbler_colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		nibbler_colorram[offset] = data;
	}
}



void nibbler_characterram_w(int offset,int data)
{
	if (nibbler_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		nibbler_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void nibbler_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;
	extern struct GfxLayout nibbler_charlayout;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int charcode;


		charcode = nibbler_videoram1[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


		/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,nibbler_characterram,&nibbler_charlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32) - 2;
			sy = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					nibbler_videoram2[offs],nibbler_colorram[offs] >> 4,
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,nibbler_colorram[offs] & 0x0f,
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	for (i = 0;i < 256;i++)
	{
		if (dirtycharacter[i] == 2) dirtycharacter[i] = 0;
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
