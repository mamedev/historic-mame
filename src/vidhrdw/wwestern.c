/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *wwestern_videoram2,*wwestern_videoram3;
unsigned char *wwestern_characterram;
unsigned char *wwestern_scrollx1,*wwestern_scrollx2,*wwestern_scrollx3;
unsigned char *wwestern_scrolly1,*wwestern_scrolly2,*wwestern_scrolly3;
unsigned char *wwestern_colscrolly1,*wwestern_colscrolly2,*wwestern_colscrolly3;
unsigned char *wwestern_gfxpointer,*wwestern_paletteram;
unsigned char *wwestern_colorbank,*wwestern_video_priority;
unsigned char *wwestern_colorbank,*wwestern_video_enable;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static const unsigned char *colors;
static int dirtypalette,dirtycolor;
static unsigned char dirtycharacter1[256],dirtycharacter2[256];
static unsigned char dirtysprite1[64],dirtysprite2[64];



/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void wwestern_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	colors = color_prom;	/* we'll need the colors later to dynamically remap the characters */

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (~color_prom[2*i+1] >> 6) & 0x01;
		bit1 = (~color_prom[2*i+1] >> 7) & 0x01;
		bit2 = (~color_prom[2*i] >> 0) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (~color_prom[2*i+1] >> 3) & 0x01;
		bit1 = (~color_prom[2*i+1] >> 4) & 0x01;
		bit2 = (~color_prom[2*i+1] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (~color_prom[2*i+1] >> 0) & 0x01;
		bit1 = (~color_prom[2*i+1] >> 1) & 0x01;
		bit2 = (~color_prom[2*i+1] >> 2) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int wwestern_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap3 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void wwestern_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}



int wwestern_gfxrom_r(int offset)
{
	int offs;


	offs = wwestern_gfxpointer[0]+wwestern_gfxpointer[1]*256;

	wwestern_gfxpointer[0]++;
	if (wwestern_gfxpointer[0] == 0) wwestern_gfxpointer[1]++;

	if (offs < 0x8000)
		return Machine->memory_region[2][offs];
	else return 0;
}



void wwestern_videoram2_w(int offset,int data)
{
	if (wwestern_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		wwestern_videoram2[offset] = data;
	}
}



void wwestern_videoram3_w(int offset,int data)
{
	if (wwestern_videoram3[offset] != data)
	{
		dirtybuffer[offset] = 1;

		wwestern_videoram3[offset] = data;
	}
}



void wwestern_paletteram_w(int offset,int data)
{
	if (wwestern_paletteram[offset] != data)
	{
		dirtypalette = 1;

		wwestern_paletteram[offset] = data;
	}
}



extern void wwestern_colorbank_w(int offset,int data)
{
	if (wwestern_colorbank[offset] != data)
	{
		dirtycolor = 1;

		wwestern_colorbank[offset] = data;
	}
}



void wwestern_characterram_w(int offset,int data)
{
	if (wwestern_characterram[offset] != data)
	{
		if (offset < 0x1800)
		{
			dirtycharacter1[(offset / 8) & 0xff] = 1;
			dirtysprite1[(offset / 32) & 0x3f] = 1;
		}
		else
		{
			dirtycharacter2[(offset / 8) & 0xff] = 1;
			dirtysprite2[(offset / 32) & 0x3f] = 1;
		}

		wwestern_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void wwestern_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;


	/* decode modified characters */
	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter1[offs] == 1)
		{
			decodechar(Machine->gfx[0],offs,wwestern_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter1[offs] = 0;
		}
		if (dirtycharacter2[offs] == 1)
		{
			decodechar(Machine->gfx[2],offs,wwestern_characterram + 0x1800,Machine->drv->gfxdecodeinfo[2].gfxlayout);
			dirtycharacter2[offs] = 0;
		}
	}
	/* decode modified sprites */
	for (offs = 0;offs < 64;offs++)
	{
		if (dirtysprite1[offs] == 1)
		{
			decodechar(Machine->gfx[1],offs,wwestern_characterram,Machine->drv->gfxdecodeinfo[1].gfxlayout);
			dirtysprite1[offs] = 0;
		}
		if (dirtysprite2[offs] == 1)
		{
			decodechar(Machine->gfx[3],offs,wwestern_characterram + 0x1800,Machine->drv->gfxdecodeinfo[3].gfxlayout);
			dirtysprite2[offs] = 0;
		}
	}


	/* if the palette has changed, rebuild the color lookup table */
	if (dirtypalette)
	{
		dirtypalette = 0;

		for (i = 0;i < 8*8;i++)
		{
			int col;


			offs = 0;
			while (offs < Machine->drv->total_colors)
			{
				if ((wwestern_paletteram[2*i] & 1) == colors[2*offs] &&
						wwestern_paletteram[2*i+1] == colors[2*offs+1])
					break;

				offs++;
			}

			col = Machine->pens[offs];
			/* avoid undesired transparency */
			if (col == 0 && i % 8 != 0) col = 1;
			Machine->gfx[0]->colortable[i] = col;
			if (i % 8 == 0) col = 0;	/* create also an alternate color code with transparent pen 0 */
			Machine->gfx[0]->colortable[i+8*8] = col;
		}

		/* redraw everything */
		memset(dirtybuffer,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtycolor || dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap3,Machine->gfx[0],
					wwestern_videoram3[offs],
					wwestern_colorbank[1] & 0x0f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap2,Machine->gfx[0],
					wwestern_videoram2[offs],
					((wwestern_colorbank[0] >> 4) & 0x0f) + 8,	/* use transparent pen 0 */
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	dirtycolor = 0;


	/* copy the first playfield */
	{
		int scrollx,scrolly[32];


		scrollx = *wwestern_scrollx3;
		scrollx = -((scrollx & 0xf8) | (7 - ((scrollx-1) & 7))) + 16;
		for (i = 0;i < 32;i++)
			scrolly[i] = -wwestern_colscrolly3[i] - *wwestern_scrolly3;

		if (*wwestern_video_enable & 0x40)
			copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		else
			clearbitmap(bitmap);
	}

	/* copy the second playfield if it has not priority over sprites */
	if ((*wwestern_video_enable & 0x20) && (*wwestern_video_priority & 0x08) == 0)
	{
		int scrollx,scrolly[32];


		scrollx = *wwestern_scrollx2;
		scrollx = -((scrollx & 0xf8) | (7 - ((scrollx+1) & 7))) + 16;
		for (i = 0;i < 32;i++)
			scrolly[i] = -wwestern_colscrolly2[i] - *wwestern_scrolly2;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	if (*wwestern_video_enable & 0x80)
	{
		for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
		{
			drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x40) ? 3 : 1],
					spriteram[offs + 3] & 0x3f,
					2 * ((wwestern_colorbank[1] >> 4) & 0x0f) + ((spriteram[offs + 2] >> 2) & 1),
					spriteram[offs + 2] & 1,0,
					((spriteram[offs]+13)&0xff)-15,240-spriteram[offs + 1],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* copy the second playfield if it has priority over sprites */
	if ((*wwestern_video_enable & 0x20) && (*wwestern_video_priority & 0x08) != 0)
	{
		int scrollx,scrolly[32];


		scrollx = *wwestern_scrollx2;
		scrollx = -((scrollx & 0xf8) | (7 - ((scrollx+1) & 7))) + 16;
		for (i = 0;i < 32;i++)
			scrolly[i] = -wwestern_colscrolly2[i] - *wwestern_scrolly2;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	if (*wwestern_video_enable & 0x10)
	{
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if (videoram[offs] != 0xbb)	/* don't draw spaces */
			{
				int sx,sy;


				sx = offs % 32;
				sy = (8*(offs / 32) - wwestern_colscrolly1[sx] - *wwestern_scrolly1) & 0xff;
				/* horizontal scrolling of the frontmost playfield is not implemented */
				sx = 8*sx;

				drawgfx(bitmap,Machine->gfx[0],
						videoram[offs],
						wwestern_colorbank[0] & 0x0f,
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}
