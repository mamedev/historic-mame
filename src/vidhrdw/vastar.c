/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *vastar_bg1colorram2;
unsigned char *vastar_fgvideoram,*vastar_fgcolorram1,*vastar_fgcolorram2;
unsigned char *vastar_bg2videoram,*vastar_bg2colorram1,*vastar_bg2colorram2;
unsigned char *vastar_bg1scroll,*vastar_bg2scroll;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



void vastar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (i % 4) colortable[i] = i;
		else colortable[i] = 0;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int vastar_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void vastar_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void vastar_bg1colorram2_w(int offset,int data)
{
	if (vastar_bg1colorram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		vastar_bg1colorram2[offset] = data;
	}
}

void vastar_bg2videoram_w(int offset,int data)
{
	if (vastar_bg2videoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vastar_bg2videoram[offset] = data;
	}
}

void vastar_bg2colorram1_w(int offset,int data)
{
	if (vastar_bg2colorram1[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vastar_bg2colorram1[offset] = data;
	}
}

void vastar_bg2colorram2_w(int offset,int data)
{
	if (vastar_bg2colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vastar_bg2colorram2[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void vastar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


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

			drawgfx(tmpbitmap,Machine->gfx[3],
					videoram[offs] + 256 * (vastar_bg1colorram2[offs] & 0x01),
					colorram[offs],
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[4],
					vastar_bg2videoram[offs] + 256 * (vastar_bg2colorram2[offs] & 0x01),
					vastar_bg2colorram1[offs],
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmaps to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -vastar_bg2scroll[offs];
		copyscrollbitmap(bitmap,tmpbitmap2,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		for (offs = 0;offs < 32;offs++)
			scroll[offs] = -vastar_bg1scroll[offs];
		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 2;offs >= 0;offs -= 2)
	{
		int code;


		code = ((spriteram_3[offs] & 0xfc) >> 2) + ((spriteram_2[offs] & 0x01) << 6)
				+ ((spriteram[offs+1] & 0x04) << 5);

		if (spriteram_2[offs] & 0x08)	/* double width */
		{
			drawgfx(bitmap,Machine->gfx[2],
					code/2,
					(spriteram[offs+1] & 0x03) + ((spriteram[offs+1] & 0x38) >> 1),	/* ?? */
					0,spriteram_3[offs] & 0x01,
					spriteram_3[offs + 1],224-spriteram[offs],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			/* redraw with wraparound */
			drawgfx(bitmap,Machine->gfx[2],
					code/2,
					(spriteram[offs+1] & 0x03) + ((spriteram[offs+1] & 0x38) >> 1),	/* ?? */
					0,spriteram_3[offs] & 0x01,
					spriteram_3[offs + 1],256+224-spriteram[offs],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
		else
			drawgfx(bitmap,Machine->gfx[1],
					code,
					(spriteram[offs+1] & 0x03) + ((spriteram[offs+1] & 0x38) >> 1),	/* ?? */
					0,spriteram_3[offs] & 0x01,
					spriteram_3[offs + 1],240-spriteram[offs],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield - they are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				vastar_fgvideoram[offs] + 256 * (vastar_fgcolorram2[offs] & 0x01),
				vastar_fgcolorram1[offs],
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
