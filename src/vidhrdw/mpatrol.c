/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

static unsigned char scrollreg[4];
static unsigned char bg1xpos,bg1ypos,bg2xpos,bg2ypos,bgcontrol;
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



void mpatrol_bg1xpos_w(int offset,int data)
{
	bg1xpos = data;
}



void mpatrol_bg1ypos_w(int offset,int data)
{
	bg1ypos = data;
}



void mpatrol_bg2xpos_w(int offset,int data)
{
	bg2xpos = data;
}



void mpatrol_bg2ypos_w(int offset,int data)
{
	bg2ypos = data;
}



void mpatrol_bgcontrol_w(int offset,int data)
{
	bgcontrol = data;
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


	/* copy the background */
	if (bgcontrol == 0x04)
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

clip.min_y = bg2ypos+64;
clip.max_y = bg2ypos+64+15;
copybitmap(bitmap,bgbitmap,0,0,bg2xpos,bg2ypos-64*2+16,&clip,TRANSPARENCY_NONE,0);
copybitmap(bitmap,bgbitmap,0,0,bg2xpos - 256,bg2ypos-64*2+16,&clip,TRANSPARENCY_NONE,0);
		clip.min_y = bg2ypos;
		clip.max_y = bg2ypos + 63;
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos,bg2ypos-64*2,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos - 256,bg2ypos-64*2,&clip,TRANSPARENCY_NONE,0);

clip.min_y = bg1ypos+64;
clip.max_y = bg1ypos+64+15;
copybitmap(bitmap,bgbitmap,0,0,bg1xpos,bg1ypos-64+16,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
copybitmap(bitmap,bgbitmap,0,0,bg1xpos - 256,bg1ypos-64+16,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		clip.min_y = bg1ypos;
		clip.max_y = bg1ypos + 63;
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos,bg1ypos-64,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos - 256,bg1ypos-64,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
	}
	else if (bgcontrol == 0x03)
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = bg2ypos;
		clip.max_y = bg2ypos + 63;
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos,bg2ypos-64*2,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos - 256,bg2ypos-64*2,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = bg1ypos;
		clip.max_y = bg1ypos + 63;
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos,bg1ypos,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos - 256,bg1ypos,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
	}
	else clearbitmap(bitmap);


	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 0;
		clip.max_y = 13 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = 13 * 8;
		clip.max_y = 24 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);

		clip.min_y = 24 * 8;
		clip.max_y = 32 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0],0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
		copybitmap(bitmap,tmpbitmap,0,0,scrollreg[0] - 256,0,&clip,TRANSPARENCY_COLOR,Machine->background_pen);
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
