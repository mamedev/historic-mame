/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Yie Ar Kung-Fu has one 32x8 palette PROM, connected to the RGB output this
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
void yiear_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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


	/* sprites lookup table */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = i;

	/* characters lookup table */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i + 16;
}



void yiear_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset/2] = 1;
		videoram[offset] = data;
	}
}

void yiear_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i;

	/* draw background */
	{
		unsigned char *vr = videoram;
		for (i = 0;i < videoram_size;i++) if ( dirtybuffer[i] )
		{
			unsigned char byte1 = vr[i*2];
			unsigned char byte2 = vr[i*2+1];
			int code = 16*(byte1 & 0x10) + byte2;
			int flipx = byte1 & 0x80;
			int flipy = byte1 & 0x40;
			int sx = 8 * (i % 32);
			int sy = 8 * (i / 32);

			dirtybuffer[i] = 0;

			drawgfx(tmpbitmap,Machine->gfx[0],
				code,0,flipx,flipy,sx,sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw sprites. */
	{
		unsigned char *sr  = spriteram;

		for (i = 23*16; i>=0; i -= 16)
		{
			int sy = 239-sr[i+0x06];
			if( sy<239 ){
				int code = 256*(sr[i+0x0f] & 1) + sr[i+0x0e];
				int flipx = (sr[i+0x0f]&0x40)?0:1;
				int flipy = (sr[i+0x0f]&0x80)?1:0;
				int sx = sr[i+0x04];

				drawgfx(bitmap,Machine->gfx[1],
					code,0,flipx,flipy,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
		}
	}
}
