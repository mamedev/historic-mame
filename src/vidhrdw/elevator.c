/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *elevator_videoram2,*elevator_videoram3;
unsigned char *elevator_characterram;
unsigned char *elevator_scroll1,*elevator_scroll2,*elevator_scroll3;
unsigned char *elevator_gfxpointer,*elevator_paletteram;
unsigned char *elevator_colorbank,*elevator_video_priority;
unsigned char *elevator_colorbank,*elevator_video_enable;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static unsigned char *dirtybuffer2,*dirtybuffer3;
static const unsigned char *colors;
static int dirtypalette;
static unsigned char dirtycharacter1[256],dirtycharacter2[256];
static unsigned char dirtysprite1[64],dirtysprite2[64];



/* Elevator Action uses the frontmost playfield to mask a portion the play area. */
/* To speed up the emulation, we clip it ourselves. */
static struct rectangle spritevisiblearea =
{
	0*8, 32*8-1,
	4*8, 26*8-1
};
static struct rectangle topvisiblearea =
{
	0*8, 32*8-1,
	2*8, 4*8-1
};
static struct rectangle bottomvisiblearea =
{
	0*8, 32*8-1,
	26*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void elevator_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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
int elevator_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,0,videoram_size);

	if ((dirtybuffer3 = malloc(videoram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer3,0,videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer3);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap3 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap2);
		free(dirtybuffer3);
		free(dirtybuffer2);
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
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer3);
	free(dirtybuffer2);
	generic_vh_stop();
}



int elevator_gfxrom_r(int offset)
{
	int offs;


	offs = elevator_gfxpointer[0]+elevator_gfxpointer[1]*256;

	elevator_gfxpointer[0]++;
	if (elevator_gfxpointer[0] == 0) elevator_gfxpointer[1]++;

	if (offs < 0x8000)
		return Machine->memory_region[2][offs];
	else return 0;
}



void elevator_videoram2_w(int offset,int data)
{
	if (elevator_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		elevator_videoram2[offset] = data;
	}
}



void elevator_videoram3_w(int offset,int data)
{
	if (elevator_videoram3[offset] != data)
	{
		dirtybuffer3[offset] = 1;

		elevator_videoram3[offset] = data;
	}
}



void elevator_paletteram_w(int offset,int data)
{
	if (elevator_paletteram[offset] != data)
	{
		dirtypalette = 1;

		elevator_paletteram[offset] = data;
	}
}



extern void elevator_colorbank_w(int offset,int data)
{
	if (elevator_colorbank[offset] != data)
	{
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
		memset(dirtybuffer3,1,videoram_size);

		elevator_colorbank[offset] = data;
	}
}



void elevator_characterram_w(int offset,int data)
{
	if (elevator_characterram[offset] != data)
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

		elevator_characterram[offset] = data;
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


	/* decode modified characters */
	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter1[offs] == 1)
		{
			decodechar(Machine->gfx[0],offs,elevator_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter1[offs] = 0;
		}
		if (dirtycharacter2[offs] == 1)
		{
			decodechar(Machine->gfx[2],offs,elevator_characterram + 0x1800,Machine->drv->gfxdecodeinfo[2].gfxlayout);
			dirtycharacter2[offs] = 0;
		}
	}
	/* decode modified sprites */
	for (offs = 0;offs < 64;offs++)
	{
		if (dirtysprite1[offs] == 1)
		{
			decodechar(Machine->gfx[1],offs,elevator_characterram,Machine->drv->gfxdecodeinfo[1].gfxlayout);
			dirtysprite1[offs] = 0;
		}
		if (dirtysprite2[offs] == 1)
		{
			decodechar(Machine->gfx[3],offs,elevator_characterram + 0x1800,Machine->drv->gfxdecodeinfo[3].gfxlayout);
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
				if ((elevator_paletteram[2*i] & 1) == colors[2*offs] &&
						elevator_paletteram[2*i+1] == colors[2*offs+1])
					break;

				offs++;
			}

			col = Machine->pens[offs];
			/* avoid undesired transparency */
			if (col == 0 && i % 8 != 0) col = 1;
			Machine->gfx[0]->colortable[i] = col;
		}

		/* redraw everything */
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
		memset(dirtybuffer3,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					elevator_colorbank[0] & 0x0f,
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap2,Machine->gfx[0],
					elevator_videoram2[offs],
					(elevator_colorbank[0] >> 4) & 0x0f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer3[offs])
		{
			int sx,sy;


			dirtybuffer3[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap3,Machine->gfx[0],
					elevator_videoram3[offs],
					elevator_colorbank[1] & 0x0f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the first playfield */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll3[i];

		if (*elevator_video_enable & 0x40)
			copyscrollbitmap(bitmap,tmpbitmap3,0,0,32,scroll,&spritevisiblearea,TRANSPARENCY_NONE,0);
		else
			clearbitmap(bitmap);
	}

	/* copy the second playfield if it has not priority over sprites */
	if ((*elevator_video_enable & 0x20) && (*elevator_video_priority & 0x08) == 0)
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll2[i];

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,32,scroll,&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	if (*elevator_video_enable & 0x80)
	{
		for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
		{
			drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x40) ? 3 : 1],
					spriteram[offs + 3] & 0x3f,
					2 * ((elevator_colorbank[1] >> 4) & 0x0f) + ((spriteram[offs + 2] >> 2) & 1),
					spriteram[offs + 2] & 1,0,
					((spriteram[offs]+13)&0xff)-15,240-spriteram[offs + 1],
					&spritevisiblearea,TRANSPARENCY_PEN,0);
		}
	}


	/* copy the second playfield if it has priority over sprites */
	if ((*elevator_video_enable & 0x20) && (*elevator_video_priority & 0x08) != 0)
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll2[i];

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,32,scroll,&spritevisiblearea,TRANSPARENCY_COLOR,Machine->background_pen);
	}


	if (*elevator_video_enable & 0x10)
	{
		/* draw the opaque part of the frontmost playfield */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&topvisiblearea,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&bottomvisiblearea,TRANSPARENCY_NONE,0);


		/* draw the remaining part of the frontmost playfield. */
		/* They are characters, but draw them as sprites */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if (videoram[offs])	/* don't draw spaces */
			{
				int sx,sy;


				sx = offs % 32;
				sy = offs / 32;

				drawgfx(bitmap,Machine->gfx[0],
						videoram[offs],
						elevator_colorbank[0] & 0x0f,
						0,0,8*sx,8*sy,
						&spritevisiblearea,TRANSPARENCY_PEN,0);
			}
		}
	}
}
