/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int gfxbank;
static unsigned char *superqix_bitmapram,*superqix_bitmapram2;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int superqix_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	/* palette RAM is accessed thorough I/O ports, so we have to */
	/* allocate it ourselves */
	if ((paletteram = malloc(256 * sizeof(unsigned char))) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((superqix_bitmapram = malloc(0x7000 * sizeof(unsigned char))) == 0)
	{
		free(paletteram);
		generic_vh_stop();
		return 1;
	}

	if ((superqix_bitmapram2 = malloc(0x7000 * sizeof(unsigned char))) == 0)
	{
		free(superqix_bitmapram);
		free(paletteram);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void superqix_vh_stop(void)
{
	free(superqix_bitmapram2);
	free(superqix_bitmapram);
	free(paletteram);
	generic_vh_stop();
}



int superqix_bitmapram_r(int offset)
{
	return superqix_bitmapram[offset];
}

void superqix_bitmapram_w(int offset,int data)
{
	superqix_bitmapram[offset] = data;
}

int superqix_bitmapram2_r(int offset)
{
	return superqix_bitmapram2[offset];
}

void superqix_bitmapram2_w(int offset,int data)
{
	superqix_bitmapram2[offset] = data;
}



void superqix_0410_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* bits 0-1 select the tile bank */
	if (gfxbank != (data & 0x03))
	{
		gfxbank = data & 0x03;
		memset(dirtybuffer,1,videoram_size);
	}

	/* bit 3 enables NMI */
	interrupt_enable_w(offset,data & 0x08);

	/* bits 4-5 control ROM bank */
	bankaddress = 0x10000 + ((data & 0x30) >> 4) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void superqix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* recalc the palette if necessary */
	if (palette_recalc())
		memset(dirtybuffer,1,videoram_size);


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

			drawgfx(tmpbitmap,Machine->gfx[(colorram[offs] & 0x04) ? 0 : (1 + gfxbank)],
					videoram[offs] + 256 * (colorram[offs] & 0x03),
					(colorram[offs] & 0xf0) >> 4,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

{
	int x,y;


	for (y = 0;y < 224;y++)
	{
		for (x = 0;x < 128;x++)
		{
			int sx,sy,d;


			d = superqix_bitmapram[y*128+x];
			/* TODO: bitmapram2 is used for player 2 in cocktail mode */

			if (d & 0xf0)
			{
				sx = 2*x;
				sy = y+16;
				if (Machine->orientation & ORIENTATION_SWAP_XY)
				{
					int temp;


					temp = sx;
					sx = sy;
					sy = temp;
				}
				if (Machine->orientation & ORIENTATION_FLIP_X)
					sx = bitmap->width - 1 - sx;
				if (Machine->orientation & ORIENTATION_FLIP_Y)
					sy = bitmap->height - 1 - sy;

				bitmap->line[sy][sx] = Machine->pens[d >> 4];
			}

			if (d & 0x0f)
			{
				sx = 2*x + 1;
				sy = y+16;
				if (Machine->orientation & ORIENTATION_SWAP_XY)
				{
					int temp;


					temp = sx;
					sx = sy;
					sy = temp;
				}
				if (Machine->orientation & ORIENTATION_FLIP_X)
					sx = bitmap->width - 1 - sx;
				if (Machine->orientation & ORIENTATION_FLIP_Y)
					sy = bitmap->height - 1 - sy;

				bitmap->line[sy][sx] = Machine->pens[d & 0x0f];
			}
		}
	}
}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		/* TODO: I haven't looked for the flip bits, but they are there, and used e.g. */
		/* in the animation at the end of round 5 */
		drawgfx(bitmap,Machine->gfx[5],
				spriteram[offs] + 256 * (spriteram[offs + 3] & 0x01),
				(spriteram[offs + 3] & 0xf0) >> 4,
				0,0,	/* TODO: add flip */
				spriteram[offs + 1],spriteram[offs + 2],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* redraw characters which have priority over the bitmap */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (colorram[offs] & 0x08)
		{
			int sx,sy;


			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[(colorram[offs] & 0x04) ? 0 : 1],
					videoram[offs] + 256 * (colorram[offs] & 0x03),
					(colorram[offs] & 0xf0) >> 4,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
