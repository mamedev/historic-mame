/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *starforc_scrollx2,*starforc_scrolly2;
unsigned char *starforc_scrollx3,*starforc_scrolly3;
unsigned char *starforc_paletteram;
unsigned char *starforc_tilebackground2;
unsigned char *starforc_tilebackground3;
unsigned char *starforc_tilebackground4;
int starforc_bgvideoram_size;
unsigned char *dirtybuffer2,*dirtybuffer3;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static unsigned char dirtycolor[48];	/* keep track of modified colors */




/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void starforc_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits,intensity;


		intensity = (i >> 6) & 0x03;
		bits = (i >> 0) & 0x03;
		palette[3*i] = 0x44 * bits + 0x11 * intensity;
		bits = (i >> 2) & 0x03;
		palette[3*i + 1] = 0x44 * bits + 0x11 * intensity;
		bits = (i >> 4) & 0x03;
		palette[3*i + 2] = 0x44 * bits + 0x11 * intensity;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int starforc_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(starforc_bgvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,0,starforc_bgvideoram_size);

	if ((dirtybuffer3 = malloc(starforc_bgvideoram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer3,0,starforc_bgvideoram_size);

	/* the background area is twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer3);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap3 = osd_create_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
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
void starforc_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer3);
	free(dirtybuffer2);
	generic_vh_stop();
}



void starforc_tiles2_w(int offset,int data)
{
	if (starforc_tilebackground2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		starforc_tilebackground2[offset] = data;
	}
}
void starforc_tiles3_w(int offset,int data)
{
	if (starforc_tilebackground3[offset] != data)
	{
		dirtybuffer3[offset] = 1;

		starforc_tilebackground3[offset] = data;
	}
}
void starforc_tiles4_w(int offset,int data)
{
	if (starforc_tilebackground4[offset] != data)
	{
		dirtybuffer3[offset] = 1;

		starforc_tilebackground4[offset] = data;
	}
}




void starforc_paletteram_w(int offset,int data)
{
	if (starforc_paletteram[offset] != data)
	{
		dirtycolor[offset / 8] = 1;

		starforc_paletteram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void starforc_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* rebuild the color lookup table */
	for (i = 0;i < 48*8;i++)
	{
		int col;


		col = Machine->pens[starforc_paletteram[i]];
		/* avoid undesired transparency using dark gray instead of black */
		if (col == 0x00 && i % 8 != 0) col = 0x40;
		Machine->gfx[0]->colortable[i] = col;
	}

	for (offs = starforc_bgvideoram_size - 1;offs >= 0;offs--)
	{
		int col,code;
		static int colormap[8] = { 0,2,4,6,1,3,5,7 };


		if (dirtybuffer2[offs])
		{
			int sx,sy;

			dirtybuffer2[offs] = 0;

			sx = 31 - offs / 16;
			sy = offs % 16;

			drawgfx(tmpbitmap2,Machine->gfx[3],
					starforc_tilebackground2[offs],
					starforc_tilebackground2[offs] >> 5,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}


		code = starforc_tilebackground4[offs],
		col = colormap[code >> 5];

		if (dirtybuffer3[offs])
		{
			int sx,sy;


			dirtybuffer3[offs] = 0;

			sx = 16 * (31 - offs / 16);
			sy = 16 * (offs % 16);

			drawgfx(tmpbitmap3,Machine->gfx[1],
					starforc_tilebackground3[offs],
					starforc_tilebackground3[offs] >> 5,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);

			drawgfx(tmpbitmap3,Machine->gfx[2],
					code,
					col,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
		}
		else if (code && dirtycolor[8+col])
		{
			int sx,sy;


			sx = 16 * (31 - offs / 16);
			sy = 16 * (offs % 16);

			drawgfx(tmpbitmap3,Machine->gfx[2],
					code,
					col,
					0,0,
					sx,sy,
					0,TRANSPARENCY_PEN,0);
		}
	}


	for (i = 0;i < 48;i++)
		dirtycolor[i] = 0;


	{
		int scrollx,scrolly;


		scrollx = starforc_scrollx2[0] + 256 * starforc_scrollx2[1] + 256;
		scrolly = -starforc_scrolly2[0];
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		scrollx = starforc_scrollx3[0] + 256 * starforc_scrollx3[1] + 256;
		scrolly = -starforc_scrolly3[0];
		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[((spriteram[offs] & 0xc0) == 0xc0) ? 5 : 4],
				spriteram[offs],
				spriteram[offs + 1] & 0x07,
				spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
				spriteram[offs + 2],spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;
		int charcode;


		charcode = videoram[offs] + 0x10 * (colorram[offs] & 0x10);

		if (charcode)	/* don't draw spaces */
		{
			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0x07,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
