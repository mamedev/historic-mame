/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

static unsigned char scrollreg[4];
static unsigned char bgscroll1,bgscroll2;
static struct osd_bitmap *bgbitmap;



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
int mpatrol_vh_start(void)
{
	int i,j;


	if (generic_vh_start() != 0)
		return 1;

	/* temp bitmap for the three background images */
	if ((bgbitmap = osd_create_bitmap(256,64*3)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	/* prepare the background graphics */
	for (i = 0;i < 3;i++)
	{
		for (j = 0;j < 4;j++)
			drawgfx(bgbitmap,Machine->gfx[2 + i],
					j,0,
					0,0,
					64 * j,64 * i,
					0,TRANSPARENCY_NONE,0);
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mpatrol_vh_stop(void)
{
	osd_free_bitmap(bgbitmap);
	generic_vh_stop();
}



void mpatrol_scroll_w(int offset,int data)
{
	scrollreg[offset] = data;
}



void mpatrol_bgscroll1_w(int offset,int data)
{
	bgscroll1 = data;
}



void mpatrol_bgscroll2_w(int offset,int data)
{
	bgscroll2 = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mpatrol_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


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
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x7f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


#if 0
	/* copy the background */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 16 * 8;
		clip.max_y = 24 * 8 - 1;
		copybitmap(bitmap,bgbitmap,0,0,bgscroll2,0,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,bgbitmap,0,0,bgscroll2 - 256,0,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = 20 * 8;
		clip.max_y = 28 * 8 - 1;
		copybitmap(bitmap,bgbitmap,0,0,bgscroll1,96,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		copybitmap(bitmap,bgbitmap,0,0,bgscroll1 - 256,96,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
	}
#endif

	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 0;
		clip.max_y = 16 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = 16 * 8;
		clip.max_y = 24 * 8 - 1;
#if 0
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
#else
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_NONE,0);
#endif

		clip.min_y = 24 * 8;
		clip.max_y = 28 * 8 - 1;
#if 0
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0],0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0] - 256,0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
#else
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0],0,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0] - 256,0,&clip,TRANSPARENCY_NONE,0);
#endif
		clip.min_y = 28 * 8;
		clip.max_y = 32 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0],0,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0] - 256,0,&clip,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 4*23;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram_2[offs + 2],
				spriteram_2[offs + 1] & 0x3f,
				spriteram_2[offs + 1] & 0x40,spriteram_2[offs + 1] & 0x80,
				spriteram_2[offs + 3],241 - spriteram_2[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
	for (offs = 4*23;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 2],
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 1] & 0x40,spriteram[offs + 1] & 0x80,
				spriteram[offs + 3],241 - spriteram[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
