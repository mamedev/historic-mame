/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#define JEDI_PLAYFIELD

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *jedi_backgroundram;
size_t jedi_backgroundram_size;
unsigned char *jedi_PIXIRAM;
static unsigned int jedi_vscroll;
static unsigned int jedi_hscroll;
static unsigned int jedi_alpha_bank;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;

WRITE_HANDLER( jedi_vscroll_w ) {
    jedi_vscroll = data | (offset << 8);
}

WRITE_HANDLER( jedi_hscroll_w ) {
    jedi_hscroll = data | (offset << 8);
}

WRITE_HANDLER( jedi_alpha_banksel_w )
{
	if (jedi_alpha_bank != 2*(data & 0x80))
	{
		jedi_alpha_bank = 2*(data & 0x80);
		memset(dirtybuffer,1,videoram_size);
	}
}



/* Color RAM format
   Color RAM is 1024x12
   RAM address: A0..A3 = Playfield color code
                A4..A7 = Motion object color code
                A8..A9 = Alphanumeric color code
   RAM data:
                0..2 = Blue
                3..5 = Green
                6..8 = Blue
                9..11 = Intesnsity
    Output resistor values:
               bit 0 = 22K
               bit 1 = 10K
               bit 2 = 4.7K
*/


WRITE_HANDLER( jedi_paletteram_w )
{
    int r,g,b;
	int bits,intensity;
    unsigned int color;


	paletteram[offset] = data;
	color = paletteram[offset & 0x3FF] | (paletteram[offset | 0x400] << 8);
	intensity = (color >> 9) & 0x07;
	bits = (color >> 6) & 0x07;
	r = 5 * bits * intensity;
	bits = (color >> 3) & 0x07;
	g = 5 * bits * intensity;
	bits = (color >> 0) & 0x07;
	b = 5 * bits * intensity;

	palette_change_color (offset & 0x3ff,r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int jedi_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
	{
		return 1;
	}
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	if ((dirtybuffer2 = malloc(jedi_backgroundram_size)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		free(dirtybuffer);
		return 1;
	}
	memset(dirtybuffer2,1,jedi_backgroundram_size);

	if ((tmpbitmap2 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		free(dirtybuffer);
		free(dirtybuffer2);
		return 1;
	}

	/* the background area is 512x512 */
	if ((tmpbitmap3 = osd_new_bitmap(512,512,8)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		osd_free_bitmap(tmpbitmap2);
		free(dirtybuffer);
		free(dirtybuffer2);
		return 1;
	}

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void jedi_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap3);
	free(dirtybuffer);
	free(dirtybuffer2);
}



WRITE_HANDLER( jedi_backgroundram_w )
{
	if (jedi_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		jedi_backgroundram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void jedi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (palette_recalc())
	{
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,jedi_backgroundram_size);
	}


	/* Return of the Jedi has a peculiar playfield/sprite priority system. That */
	/* is, there is no priority system ;-) The color of the pixel which appears on */
	/* screen depends on all three of the foreground, background and sprites. The */
	/* 1024 colors palette is appropriately set up by the program to "emulate" a */
	/* priority system, but it can also be used to display completely different */
	/* colors (see the palette test in service mode) */

    /* foreground */
    for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 64;
			sy = offs / 64;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + jedi_alpha_bank,
					0,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE_RAW,0);
		}
    }


	/* background */
	for (offs = jedi_backgroundram_size/2 - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs] != 0 || dirtybuffer2[offs + 0x400] != 0)
		{
			int sx,sy,b,c;


			dirtybuffer2[offs] = dirtybuffer2[offs + 0x400] = 0;

			sx = offs % 32;
			sy = offs / 32;
			c = (jedi_backgroundram[offs] & 0xFF);
			b = (jedi_backgroundram[offs + 0x400] & 0x0F);
			c = c | ((b & 0x01) << 8);
			c = c | ((b & 0x08) << 6);
			c = c | ((b & 0x02) << 9);

			drawgfx(tmpbitmap3,Machine->gfx[1],
					c,
					0,
					b & 0x04,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE_RAW,0);
		}
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	fillbitmap(tmpbitmap2,0,&Machine->drv->visible_area);

    for (offs = 0;offs < 0x30;offs++)
	{
		int b,c;


		b = ((spriteram[offs+0x40] & 0x02) >> 1);
		b = b | ((spriteram[offs+0x40] & 0x40) >> 5);
		b = b | (spriteram[offs+0x40] & 0x04);

		c = spriteram[offs] + (b * 256);
		if (spriteram[offs+0x40] & 0x08) c |= 1;	/* double height */

		b = spriteram[offs+0x40] & 0x01;

		drawgfx(tmpbitmap2,Machine->gfx[2],
				c,
				0,
				(spriteram[offs+0x40] & 0x10),(spriteram[offs+0x40] & 0x20),
				(spriteram[offs+0x100]) | (b << 8),240-spriteram[offs+0x80],
				&Machine->drv->visible_area,TRANSPARENCY_PEN_RAW,0);

		if (spriteram[offs+0x40] & 0x08)	/* double height */
			drawgfx(tmpbitmap2,Machine->gfx[2],
					c-1,
					0,
					(spriteram[offs+0x40] & 0x10),(spriteram[offs+0x40] & 0x20),
					(spriteram[offs+0x100]) | (b << 8),240-spriteram[offs+0x80]-16,
					&Machine->drv->visible_area,TRANSPARENCY_PEN_RAW,0);
    }


	/* compose the three layers */
	{
		int x,y;
		UINT8 *s1,*s2,*s3;


		if (bitmap->depth == 16)
		{
			for (y = 0;y < bitmap->height;y++)
			{
				UINT16 *d;

				d = (UINT16 *)bitmap->line[y];
				s1 = tmpbitmap->line[y];
				s2 = tmpbitmap2->line[y];
				s3 = tmpbitmap3->line[(y + jedi_vscroll) & 0x1ff];

				for (x = 0;x < bitmap->width;x++)
				{
					*(d++) = Machine->pens[
							(*(s1++) << 8) |
							(*(s2++) << 4) |
							s3[(x + jedi_hscroll) & 0x1ff]];
				}
			}
		}
		else
		{
			for (y = 0;y < bitmap->height;y++)
			{
				UINT8 *d;

				d = bitmap->line[y];
				s1 = tmpbitmap->line[y];
				s2 = tmpbitmap2->line[y];
				s3 = tmpbitmap3->line[(y + jedi_vscroll) & 0x1ff];

				for (x = 0;x < bitmap->width;x++)
				{
					*(d++) = Machine->pens[
							(*(s1++) << 8) |
							(*(s2++) << 4) |
							s3[(x + jedi_hscroll) & 0x1ff]];
				}
			}
		}
	}
}
