/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char dirtycharacter[256];
static int backcolor;

unsigned char *zarzon_character_ram;
unsigned char *zarzon_videoram1;

void zarzon_characterram_w(int offset,int data)
{
    if (zarzon_character_ram[offset] != data)
	{
        dirtycharacter[offset / 8 % 256] = 1;
        zarzon_character_ram[offset] = data;
	}
}


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void zarzon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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


	backcolor = 0;	/* background color can be changed by the game */

	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i % 4 == 0) COLOR(0,i) = 0x10 + backcolor;
		else COLOR(0,i) = 0x10 + 4 * (i % 4) + (i / 4);
	}

	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		if (i % 4 == 0) COLOR(1,i) = 0;
		else COLOR(1,i) = 4 * (i % 4) + (i / 4);
	}
}



void zarzon_backcolor_w(int offset, int data)
{
	if (backcolor != (data & 3))
	{
		int i;


		backcolor = data & 3;

		for (i = 0;i < 16;i += 4)
			Machine->gfx[0]->colortable[i] = Machine->pens[0x10 + backcolor];

		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void zarzon_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
    for (offs = videoram_size; offs >= 0; offs--)
	{
        int sx,sy;

		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32 + 2;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					(colorram[offs] & 0x0C) >> 2,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
    }

	/* copy the temporary bitmap to the screen */
    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	for (offs = videoram_size; offs >= 0; offs--)
	{
		int charcode;
		int sx,sy;

		sx = offs % 32;
		sy = offs / 32 + 2;

		charcode = zarzon_videoram1[offs];

		if (dirtycharacter[charcode] == 1)
		{
			decodechar(Machine->gfx[1],charcode,zarzon_character_ram,
					   Machine->drv->gfxdecodeinfo[1].gfxlayout);
			dirtycharacter[charcode] = 0;
		}

		drawgfx(bitmap,Machine->gfx[1],
				charcode,
				colorram[offs] & 0x03,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	}
}
