/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *exedexes_backgroundram;
int exedexes_backgroundram_size;
unsigned char *exedexes_bg_scroll;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Exed Exes has three 256x4 palette PROMs (one per gun) and three 256x4 lookup
  table PROMs (one for characters, one for sprites, one for background tiles).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void exedexes_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256] >> 0) & 0x01;
		bit1 = (color_prom[i+256] >> 1) & 0x01;
		bit2 = (color_prom[i+256] >> 2) & 0x01;
		bit3 = (color_prom[i+256] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256*2] >> 0) & 0x01;
		bit1 = (color_prom[i+256*2] >> 1) & 0x01;
		bit2 = (color_prom[i+256*2] >> 2) & 0x01;
		bit3 = (color_prom[i+256*2] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* characters use colors 192-207 */
	for (i = 0;i < 64*4;i++)
		colortable[i] = color_prom[i + 256*3] + 192;

	/* 8x? tiles use colors 0-15 */
	for (i = 64*4;i < 2*64*4;i++)
		colortable[i] = color_prom[i + 256*3];

	/* 16x16 tiles use colors 64-79 */
	for (i = 2*64*4;i < 2*64*4+16*16;i++)
		colortable[i] = color_prom[i + 256*3] + 64;

	/* sprites use colors 128-192 in four banks */
	for (i = 2*64*4+16*16;i < 2*64*4+16*16+16*16;i++)
		colortable[i] = color_prom[i + 256*3] + 128 + 16 * color_prom[i + 256*3 + 256];
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int exedexes_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(exedexes_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,exedexes_backgroundram_size);

	/* the background area is twice as tall as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
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
void exedexes_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void exedexes_background_w(int offset,int data)
{
	if (exedexes_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		exedexes_backgroundram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void exedexes_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,sx,sy;


	for (sy = 0;sy < 32;sy++)
	{
		for (sx = 0;sx < 16;sx++)
		{
			offs = 32 * (31 - sy) + sx;

			if (dirtybuffer2[offs] != 0 || dirtybuffer2[offs + 16] != 0)
			{
				dirtybuffer2[offs] = dirtybuffer2[offs + 16] = 0;

                                drawgfx(tmpbitmap2,Machine->gfx[(exedexes_backgroundram[offs + 16] & 0x80) ? 2 : 1],
                                                exedexes_backgroundram[offs],
                                                (exedexes_backgroundram[offs + 16] & 0x1f),
                                                0,0,
						16 * sx,16 * sy,
						0,TRANSPARENCY_NONE,0);
                                drawgfx(tmpbitmap2,Machine->gfx[(exedexes_backgroundram[offs + 16] & 0x80) ? 2 : 1],
                                                exedexes_backgroundram[offs],
                                                (exedexes_backgroundram[offs + 16] & 0x1f),
                                                0,1,
						16 * sx,16 * sy+8,
						0,TRANSPARENCY_NONE,0);
			}
		}
        }

	/* copy the background graphics */
	{
		int bg_scroll;


		bg_scroll = exedexes_bg_scroll[0] + 256 * exedexes_bg_scroll[1] - 256;

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,1,&bg_scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
        for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
		drawgfx(bitmap,Machine->gfx[3],
				spriteram[offs],
				spriteram[offs + 1] & 0x0f,
				spriteram[offs + 1] & 0x20, spriteram[offs + 1] & 0x10,
				spriteram[offs + 2],240 - spriteram[offs + 3] + 0x10 * (spriteram[offs + 1] & 0x80),
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (videoram[offs] != 0x24 || colorram[offs] != 0)		/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			/* Should only use trancparency when pen 0 is black */
			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x3f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_COLOR,207);
		}
	}
}
