/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *sidearms_bg_scrollx,*sidearms_bg_scrolly;
unsigned char *sidearms_bg2_scrollx,*sidearms_bg2_scrolly;
unsigned char *sidearms_paletteram;
static int dirtypalette;
static struct osd_bitmap *tmpbitmap2;
static int flipscreen;



/* Sidearms has a 2048 color palette RAM, but it doesn't seem to modify it */
/* dynamically. The color space is 4x4x4, and the number of unique colors is */
/* greater than 256; however, ignoring the least significant bit of the blue */
/* component, we can squeeze them into 250 colors with no appreciable loss. */
void sidearms_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[1] >> 0) & 0x01;
		bit1 = (color_prom[1] >> 1) & 0x01;
		bit2 = (color_prom[1] >> 2) & 0x01;
		bit3 = (color_prom[1] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom += 2;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int sidearms_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	/* create a temporary bitmap slightly larger than the screen for the background */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width + 32,Machine->drv->screen_height + 32)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void sidearms_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}



void sidearms_paletteram_w(int offset,int data)
{
	if (sidearms_paletteram[offset] != data)
	{
		/* the palette is initialized at startup and never touched again, */
		/* so we can mark it all dirty without causing a performance hit */
		dirtypalette = 1;
		sidearms_paletteram[offset] = data;
	}
}



void sidearms_c804_w(int offset,int data)
{
	/* bit 4 probably resets the sound CPU */

	/* don't know about the other bits, they probably turn on characters, */
	/* sprites, background */

	/* bit 7 flips screen */
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
/* TODO: support screen flip */
//		memset(dirtybuffer,1,c1942_backgroundram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sidearms_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs, sx, sy;
	int scrollx,scrolly;
	static int lastoffs;


	if (dirtypalette)
	{
		int j, i;


		/* rebuild the colour lookup table from RAM palette */
		for (j = 0;j < 3;j++)
		{
			/*
				 0000-00ff:  background palette. (16x16 colours)
				 0200-02ff:  sprites palette.    (16x16 colours)
				 0300-03ff:  characters palette  (64x4 colours)
			*/
						/* CHARS  TILES   SPRITES */
			int start[3]={0x0300, 0x0000, 0x0200};
			int count[3]={0x0100, 0x0200, 0x0100};
			int base=start[j];
			int max=count[j];

			for (i = 0;i < max;i++)
			{
				int blue, redgreen;


				redgreen=sidearms_paletteram[base];
				blue=sidearms_paletteram[base + 0x400] & 0x0f;

				for (offs = 0;offs < Machine->drv->total_colors-1;offs++)
				{
					if (Machine->gamedrv->color_prom[2*offs] == redgreen &&
							(Machine->gamedrv->color_prom[2*offs+1] & 0x0e) == (blue & 0x0e))
						break;
				}

				/* pen 15 for the tiles is transparent */
				if (j == 1 && i % 16 == 15) offs = 1;

				Machine->gfx[j]->colortable[i] = Machine->pens[offs];

				base++;
			}
		}
	}


	/* There is a scrolling blinking star background behind the tile */
	/* background, but I have absolutely NO IDEA how to render it. */
	/* The scroll registers have a 64 pixels resolution. */
#if IHAVETHEBACKGROUND
	{
		int x,y;
		for (x = 0;x < 48;x+=8)
		{
			for (y = 0;y < 32;y+=8)
			{
				drawgfx(tmpbitmap,Machine->gfx[0],
						(y%8)*48+(x%8),
						0,
						0,0,
						8*x,8*y,
						0,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* copy the temporary bitmap to the screen */
	scrollx = -(*sidearms_bg2_scrollx & 0x3f);
	scrolly = -(*sidearms_bg2_scrolly & 0x3f);

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
#endif


	scrollx = sidearms_bg_scrollx[0] + 256 * sidearms_bg_scrollx[1] + 64;
	scrolly = sidearms_bg_scrolly[0] + 256 * sidearms_bg_scrolly[1];
	offs = 2 * (scrollx >> 5) + 0x100 * (scrolly >> 5);
	scrollx = -(scrollx & 0x1f);
	scrolly = -(scrolly & 0x1f);

	if (offs != lastoffs || dirtypalette)
	{
        unsigned char *p=Machine->memory_region[3];


		lastoffs = offs;

		/* Draw the entire background scroll */
		for (sy = 0;sy < 9;sy++)
		{
			for (sx = 0; sx < 13; sx++)
			{
				int offset;


				offset = offs + 2 * sx;

				/* swap bits 1-7 and 8-10 of the address to compensate for the */
				/* funny layout of the ROM data */
				offset = (offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3);

				drawgfx(tmpbitmap2,Machine->gfx[1],
						p[offset] + 256 * (p[offset+1] & 0x01),
						(p[offset+1] & 0xf8) >> 3,
						p[offset+1] & 0x02,p[offset+1] & 0x04,
						32*sx,32*sy,
						0,TRANSPARENCY_NONE,0);
			}
			offs += 0x100;
		}
	}

	dirtypalette = 0;


#if IHAVETHEBACKGROUND
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,1);
#else
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
#endif

	/* Draw the sprites. */
	for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs] + 8 * (spriteram[offs + 1] & 0xe0),
				spriteram[offs + 1] & 0x0f,
				0,0,
				spriteram[offs + 3] + ((spriteram[offs + 1] & 0x10) << 4) - 64,
					spriteram[offs + 2],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;

		sy = offs / 64;
		sx = offs % 64 - 8;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 4 * (colorram[offs] & 0xc0),
				colorram[offs] & 0x3f,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
	}
}
