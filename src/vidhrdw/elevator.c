/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

unsigned char *elevator_videoram2,*elevator_videoram3;
unsigned char *elevator_scroll1,*elevator_scroll2;
static struct osd_bitmap *tmpbitmap1,*tmpbitmap2;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int elevator_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap1);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void elevator_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap1);
	generic_vh_stop();
}



int elevator_background_r(int offset)
{
	int offs;


	offs = RAM[0xd509]+RAM[0xd50a]*256 - 0x6000;
	RAM[0xd509]++;


	if (offs >= 0 && offs < 0x2000)
		return Machine->memory_region[2][offs];
	else return 0;
}



void elevator_videoram2_w(int offset,int data)
{
	if (elevator_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		elevator_videoram2[offset] = data;
	}
}



void elevator_videoram3_w(int offset,int data)
{
	if (elevator_videoram3[offset] != data)
	{
		dirtybuffer[offset] = 1;

		elevator_videoram3[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void elevator_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;
static int bose=1;
if (osd_key_pressed(OSD_KEY_C))
{
	while (osd_key_pressed(OSD_KEY_C));
	if (bose > 0)
	{
		bose--;
		for (offs = 0;offs < VIDEO_RAM_SIZE;offs++) dirtybuffer[offs] = 1;
	}
}
if (osd_key_pressed(OSD_KEY_V))
{
	while (osd_key_pressed(OSD_KEY_V));
	if (bose < 1)
	{
		bose++;
		for (offs = 0;offs < VIDEO_RAM_SIZE;offs++) dirtybuffer[offs] = 1;
	}
}

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

			drawgfx(tmpbitmap1,Machine->gfx[bose],
					elevator_videoram3[offs],
1,/*                    elevator_attributesram[2 * sy + 1],*/
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap2,Machine->gfx[bose],
					elevator_videoram2[offs],
0,/*                    elevator_attributesram[2 * sy + 1],*/
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the first playfield */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll1[i];

		copyscrollbitmap(bitmap,tmpbitmap1,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* copy the second playfield */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll2[i];

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 31*4;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x40) ? 3 : 2],
				spriteram[offs + 3] & 0x3f,
				(spriteram[offs + 2] >> 2) & 1,
				spriteram[offs + 2] & 1,0,
				((spriteram[offs]+13)&0xff)-15,240-spriteram[offs + 1],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;


		sx = 8*(offs % 32);
		sy = 8*(offs / 32);

		if (videoram[offs])
			drawgfx(bitmap,Machine->gfx[bose],
					videoram[offs],
2,/*                    elevator_attributesram[2 * sy + 1],*/
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
