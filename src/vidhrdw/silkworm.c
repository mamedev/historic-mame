/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *silkworm_videoram,*silkworm_colorram;
unsigned char *silkworm_videoram2,*silkworm_colorram2;
unsigned char *silkworm_scroll;
unsigned char *silkworm_paletteram;
int silkworm_videoram2_size;

static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static unsigned char dirtycolor[64];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Silkworm doesn't have a color PROM. It uses 1024 bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (4 bits per pixel; the high 4 bits of the first byte are unused).
  Since the graphics use 4 bitplanes, hence 16 colors, this makes for 64
  different color codes, divided in four groups of 16 (sprites, characters,
  bg #1, bg #2).

  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)
  bit 7 -- unused
        -- unused
        -- unused
        -- unused
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
        -- 2.2kohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
  bit 0 -- 2.2kohm resistor  -- GREEN

***************************************************************************/
void silkworm_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	/* we can use only 256 colors, so we'll have to degrade the */
	/* 4x4x4 color space to 3x3x2 */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int silkworm_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,videoram_size);

	/* the background area is twice as wide as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}
	if ((tmpbitmap3 = osd_create_bitmap(2*Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap2);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void silkworm_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void silkworm_videoram_w(int offset,int data)
{
	if (silkworm_videoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		silkworm_videoram[offset] = data;
	}
}



void silkworm_colorram_w(int offset,int data)
{
	if (silkworm_colorram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		silkworm_colorram[offset] = data;
	}
}



void silkworm_paletteram_w(int offset,int data)
{
	if (silkworm_paletteram[offset] != data)
	{
		dirtycolor[offset/2/16] = 1;
		silkworm_paletteram[offset] = data;

		if ((offset & ~1) == 0x200)
		{
			int i;


			/* special case: color 0 of the character set is used for the */
			/* background. To speed up rendering, we set it in color 0 of bg #2 */
			for (i = 0;i < 16;i++)
				dirtycolor[48 + i] = 1;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void silkworm_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* rebuild the color lookup table */
	{
		int i,j;
		int gf[4] = { 1, 0, 3, 4 };
		int off[4] = { 0x000, 0x200, 0x400, 0x600 };


		for (j = 0;j < 4;j++)
		{
			for (i = 0;i < 16*16;i++)
			{
				int col;


				col = ((silkworm_paletteram[2*i+off[j]] & 0x0c) << 4) |
						((silkworm_paletteram[2*i+1+off[j]] & 0xe0) >> 2) |
						((silkworm_paletteram[2*i+1+off[j]] & 0x0e) >> 1);

				/* avoid undesired transparency using dark red instead of black */
				if (j == 2 && col == 0x00 && i % 16 != 0) col = 0x01;

				/* special case: color 0 of the character set is used for the */
				/* background. To speed up rendering, we set it in color 0 of bg #2 */
				if (j == 3 && i % 16 == 0)
					col = ((silkworm_paletteram[0x200] & 0x0c) << 4) |
							((silkworm_paletteram[0x201] & 0xe0) >> 2) |
							((silkworm_paletteram[0x201] & 0x0e) >> 1);

				Machine->gfx[gf[j]]->colortable[i] = Machine->pens[col];
			}
		}
	}



	/* draw the background. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int color;


		color = colorram[offs] >> 4;

		if (dirtybuffer[offs] || dirtycolor[color + 48])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[4],
					videoram[offs] + 256 * (colorram[offs] & 0x07),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}


		color = silkworm_colorram[offs] >> 4;

		if (dirtybuffer2[offs] || dirtycolor[color + 32])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap3,Machine->gfx[3],
					silkworm_videoram[offs] + 256 * (silkworm_colorram[offs] & 0x07),
					color,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	for (offs = 0;offs < 64;offs++)
		dirtycolor[offs] = 0;


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		scrollx = -silkworm_scroll[3] - 256 * (silkworm_scroll[4] & 1) - 48;
		scrolly = -silkworm_scroll[5];
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		scrollx = -silkworm_scroll[0] - 256 * (silkworm_scroll[1] & 1) - 48;
		scrolly = -silkworm_scroll[2];
		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		if (spriteram[offs + 0] & 4)
		{
			int size,code,color,flipx,flipy,sx,sy;


			size = (spriteram[offs + 2] & 3);	/* 1 = single 2 = double */
			code = (spriteram[offs + 1] >> 2) + ((spriteram[offs + 0] & 0xf8) << 3);
			if (size == 2) code >>= 2;
			color = spriteram[offs + 3] & 0x0f;
			flipx = spriteram[offs + 0] & 1;
			flipy = spriteram[offs + 0] & 2;
			sx = spriteram[offs + 5] - ((spriteram[offs + 3] & 0x10) << 4);
			sy = spriteram[offs + 4] - ((spriteram[offs + 3] & 0x20) << 3);

			drawgfx(bitmap,Machine->gfx[size],
					code,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = silkworm_videoram2_size - 1;offs >= 0;offs--)
	{
		if (silkworm_videoram2[offs])	/* don't draw spaces */
		{
			int sx,sy;


			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					silkworm_videoram2[offs] + ((silkworm_colorram2[offs] & 0x03) << 8),
					silkworm_colorram2[offs] >> 4,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
