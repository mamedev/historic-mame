/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

unsigned char *bombjack_paletteram;
static unsigned char dirtycolor[16];	/* keep track of modified colors */
static int background_image;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Bomb Jack doesn't have a color PROM. It uses 256 bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (4 bits per pixel; the high 4 bits of the second byte are unused).
  Since the graphics use 3 bitplanes, hence 8 colors, this makes for 16
  different color codes.

  MAME currently doesn't support dynamic palette creation. As a temporary
  workaround, we create here a 256 colors (8 bits, vs. the original 12)
  palette which will be static, and dynamically create a lookup table at
  run time which approximates the original palette.

***************************************************************************/
void bombjack_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 0) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}

	for (i = 0;i < 256;i++)
		colortable[i] = i;

	/* provide a default palette so the ROM copyright notice at startup can be read */
	for (i = 256;i < 256+128;i++)
		colortable[i] = (i-256) % 8;
}



void bombjack_paletteram_w(int offset,int data)
{
	if (bombjack_paletteram[offset] != data)
	{
		dirtycolor[offset / 16] = 1;

		bombjack_paletteram[offset] = data;
	}
}



void bombjack_background_w(int offset,int data)
{
	if (background_image != data)
	{
		int i;

		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;

		background_image = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs,col,base;


	/* rebuild the color lookup table */
	for (i = 0;i < 128;i++)
	{
		col = (bombjack_paletteram[2*i] >> 1) & 0x07;	/* red component */
		col |= (bombjack_paletteram[2*i] >> 2) & 0x38;	/* green component */
		col |= (bombjack_paletteram[2*i + 1] << 4) & 0xc0;	/* blue component */
		Machine->gfx[0]->colortable[i] = Machine->gfx[4]->colortable[col];
	}


	base = 0x200 * (background_image & 0x07);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy;
		int bx,by;
		int tilecode,tileattribute;


		sx = 8 * (31 - offs / 32);
		sy = 8 * (offs % 32);

		if (background_image & 0x10)
		{
			int bgoffs;


			bx = sx & 0xf0;
			by = sy & 0xf0;

			bgoffs = base+16*(15-bx/16)+by/16;

			tilecode = Machine->memory_region[2][bgoffs],
			tileattribute = Machine->memory_region[2][bgoffs + 0x100];
		}
		else tilecode = 0xff;

		if (dirtybuffer[offs] || dirtycolor[colorram[offs] & 0x0f] ||
				(tilecode != 0xff && dirtycolor[tileattribute & 0x0f]))
		{
			/* draw the background (this can be handled better) */
			if (tilecode != 0xff)
			{
			/* don't redraw the background if only the foreground color has changed */
				if (dirtybuffer[offs] ||
						dirtycolor[tileattribute & 0x0f])
				{
					struct rectangle clip;


					clip.min_x = sx;
					clip.max_x = sx+7;
					clip.min_y = sy;
					clip.max_y = sy+7;

					drawgfx(tmpbitmap,Machine->gfx[1],
							tilecode,
							tileattribute & 0x0f,
							tileattribute & 0x80,0,
							bx,by,
							&clip,TRANSPARENCY_NONE,0);
				}

				drawgfx(tmpbitmap,Machine->gfx[0],
							videoram[offs] + 16 * (colorram[offs] & 0x10),
							colorram[offs] & 0x0f,
							0,0,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
			else
				drawgfx(tmpbitmap,Machine->gfx[0],
							videoram[offs] + 16 * (colorram[offs] & 0x10),
							colorram[offs] & 0x0f,
							0,0,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


			dirtybuffer[offs] = 0;
		}
	}


	for (i = 0;i < 16;i++)
		dirtycolor[i] = 0;


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 4*23;offs >= 0;offs -= 4)
	{
/*
 abbbbbbb cdefgggg hhhhhhhh iiiiiiii

 a        ? (set when big sprites are selected)
 bbbbbbb  sprite code
 c        x flip
 d        y flip (used only in death sequence?)
 e        use big sprites (32x32 instead of 16x16)
 f        ? (set only when the bonus (B) materializes?)
 gggg     color
 hhhhhhhh x position
 iiiiiiii y position
*/
		drawgfx(bitmap,Machine->gfx[spriteram[offs+1] & 0x20 ? 3 : 2],
				spriteram[offs] & 0x7f,spriteram[offs+1] & 0x0f,
				spriteram[offs+1] & 0x80,spriteram[offs+1] & 0x40,
				spriteram[offs+2]-1,spriteram[offs+3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
