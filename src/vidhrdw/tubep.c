/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Bagman has two 32 bytes palette PROMs, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void tubep_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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

	/* sprites and text seem to use the same palette */

	/* text */
	for (i = 0; i < TOTAL_COLORS(0); i++)
	{
		COLOR(0, 2*i + 0) = 0; //transparent "black", however sprites have transparent pen 0x0f
		COLOR(0, 2*i + 1) = i;
	}

	/* sprites */
	for (i = 0; i < TOTAL_COLORS(4); i++)
	{
		COLOR(4, i) = i;
	}

}


data8_t * tubep_textram;

data8_t * dirtybuff;
struct osd_bitmap *tmpbitmap;

int tubep_vh_start(void)
{
	if ((dirtybuff = malloc(0x800/2)) == 0)
		return 1;
	memset(dirtybuff,1,0x800/2);

	if ((tmpbitmap = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuff);
		dirtybuff = 0;
		return 1;
	}

	return 0;
}

void tubep_vh_stop(void)
{
	free(dirtybuff);
	bitmap_free(tmpbitmap);

	dirtybuff = 0;
	tmpbitmap = 0;
}


WRITE_HANDLER( tubep_textram_w )
{
	if (tubep_textram[offset] != data)
	{
		dirtybuff[offset/2] = 1;
		tubep_textram[offset] = data;
	}
}

void tubep_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	for (offs = 0;offs < 0x800; offs+=2)
	{
		if (dirtybuff[offs/2])
		{
			int sx,sy;

			dirtybuff[offs/2] = 0;

			sx = (offs/2) % 32;
			//if (flipscreen[0]) sx = 31 - sx;
			sy = (offs/2) / 32;
			//if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					tubep_textram[offs],
					tubep_textram[offs+1],
					0,0, /*flipscreen[0],flipscreen[1],*/
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] );

}
