/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

unsigned char *elevator_videoram2,*elevator_videoram3;
unsigned char *elevator_characterram;
unsigned char *elevator_scroll1,*elevator_scroll2,*elevator_scroll3;
unsigned char *elevator_gfxpointer,*elevator_paletteram;
unsigned char *elevator_colorbank,*elevator_video_enable;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static const unsigned char *colors;
static int dirtypalette,dirtycolor;
static unsigned char dirtycharacter[256];
static unsigned char dirtysprite1[64],dirtysprite2[64];



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


	for (i = 0;i < Machine->drv->total_colors;i++)
		colortable[11*8 + i] = i;


	/* set up colors for the dip switch menu */
	for (i = 0;i < 8*3;i++)
		colortable[8*8 + i] = 0;
	colortable[8*8 + 2] = 36;	/* white */
	colortable[9*8 + 2] = 34;	/* yellow */
	colortable[10*8 + 2] = 27;	/* red */
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int elevator_vh_start(void)
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
void elevator_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
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
		dirtycolor = 1;

		elevator_colorbank[offset] = data;
	}
}



void elevator_characterram_w(int offset,int data)
{
	if (elevator_characterram[offset] != data)
	{
		if (offset < 0x1800)
		{
			dirtycharacter[(offset / 8) & 0xff] = 1;
			dirtysprite1[(offset / 32) & 0x3f] = 1;
		}
		else
			dirtysprite2[(offset / 32) & 0x3f] = 1;

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
	extern struct GfxLayout elevator_charlayout,elevator_spritelayout;


	if (*elevator_video_enable == 0)
	{
		clearbitmap(bitmap);
		return;
	}


	/* decode modified characters */
	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 1)
		{
			decodechar(Machine->gfx[1],offs,elevator_characterram,&elevator_charlayout);
			dirtycharacter[offs] = 0;
		}
	}
	/* decode modified sprites */
	for (offs = 0;offs < 64;offs++)
	{
		if (dirtysprite1[offs] == 1)
		{
			decodechar(Machine->gfx[2],offs,elevator_characterram,&elevator_spritelayout);
			dirtysprite1[offs] = 0;
		}
		if (dirtysprite2[offs] == 1)
		{
			decodechar(Machine->gfx[3],offs,elevator_characterram + 0x1800,&elevator_spritelayout);
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

			col = Machine->gfx[4]->colortable[offs];
			/* avoid undesired transparency */
			if (col == 0 && i % 8 != 0) col = 1;
			Machine->gfx[1]->colortable[i] = col;
		}

		/* redraw everything */
		for (offs = 0;offs < VIDEO_RAM_SIZE;offs++) dirtybuffer[offs] = 1;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtycolor || dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap3,Machine->gfx[1],
					elevator_videoram3[offs],
					elevator_colorbank[1] & 0x0f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap2,Machine->gfx[1],
					elevator_videoram2[offs],
					(elevator_colorbank[0] >> 4) & 0x0f,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	dirtycolor = 0;


	/* copy the first playfield */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = -elevator_scroll3[i];

		copyscrollbitmap(bitmap,tmpbitmap3,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
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
				2 * ((elevator_colorbank[1] >> 4) & 0x0f) + ((spriteram[offs + 2] >> 2) & 1),
				spriteram[offs + 2] & 1,0,
				((spriteram[offs]+13)&0xff)-15,240-spriteram[offs + 1],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (videoram[offs])	/* don't draw spaces */
		{
			int sx,sy;


			sx = (offs % 32);
			sy = (8*(offs / 32) + elevator_scroll1[sx]) & 0xff;

			drawgfx(bitmap,Machine->gfx[1],
					videoram[offs],
					elevator_colorbank[0] & 0x0f,
					0,0,8*sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
