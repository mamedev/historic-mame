/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE	0x400

unsigned char *phoenix_videoram2;
static unsigned char dirtybuffer2[VIDEO_RAM_SIZE];
static struct osd_bitmap *tmpbitmap2;

static int scrollreg;
static int palette_bank;



static struct rectangle backvisiblearea =
{
	3*8, 29*8-1,
	3*8, 31*8-1
};

static struct rectangle backtmparea =
{
	3*8, 29*8-1,
	0*8, 32*8-1
};



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
int phoenix_vh_start(void)
{
	scrollreg = 0;
	palette_bank = 0;


	if (generic_vh_start() != 0)
		return 1;

	/* small temp bitmap for the score display */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,3*8)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void phoenix_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}



void phoenix_videoram2_w(int offset,int data)
{
	if (phoenix_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		phoenix_videoram2[offset] = data;
	}
}



void phoenix_scrollreg_w (int offset,int data)
{
	scrollreg = data;
}



void phoenix_videoreg_w (int offset,int data)
{
	if (palette_bank != ((data >> 1) & 1))
	{
		int offs;


		palette_bank = (data >> 1) & 1;

		for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
		{
			dirtybuffer[offs] = 1;
			dirtybuffer2[offs] = 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		/* background */
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32) - 3 * 8;
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs],
					(videoram[offs] >> 5) + 8 * palette_bank,
					0,0,sx,sy,
					&backtmparea,TRANSPARENCY_NONE,0);
		}
	}

	/* score */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = 8 * (31 - offs / 32) - 3 * 8;
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap2,Machine->gfx[0],
					phoenix_videoram2[offs],
					phoenix_videoram2[offs] >> 5,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	{
		int scroll;


		scroll = -scrollreg;

		copyscrollbitmap(bitmap,tmpbitmap,0,0,1,&scroll,&backvisiblearea,TRANSPARENCY_NONE,0);
	}
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;


		sx = 8 * (31 - offs / 32) - 3 * 8;
		sy = 8 * (offs % 32);

		if (sy >= 3 * 8 && phoenix_videoram2[offs])	/* don't draw score and spaces */
			drawgfx(bitmap,Machine->gfx[0],
					phoenix_videoram2[offs],
					(phoenix_videoram2[offs] >> 5) + 8 * palette_bank,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
