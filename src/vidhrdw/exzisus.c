/***************************************************************************

Functions to emulate the video hardware of the machine.

 Video hardware of this hardware is almost similar with "mexico86". So,
 most routines are derived from mexico86 driver.

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"


data8_t *exzisus_videoram0;
data8_t *exzisus_videoram1;
data8_t *exzisus_objectram0;
data8_t *exzisus_objectram1;
size_t  exzisus_objectram_size0;
size_t  exzisus_objectram_size1;



/***************************************************************************
  Initialize and destroy video hardware emulation
***************************************************************************/

void exzisus_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0 ; i < Machine->drv->total_colors ; i ++)
	{
		int bit0, bit1, bit2, bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}


/***************************************************************************
  Memory handlers
***************************************************************************/

READ_HANDLER ( exzisus_videoram_0_r )
{
	return exzisus_videoram0[offset];
}


READ_HANDLER ( exzisus_videoram_1_r )
{
	return exzisus_videoram1[offset];
}


READ_HANDLER ( exzisus_objectram_0_r )
{
	return exzisus_objectram0[offset];
}


READ_HANDLER ( exzisus_objectram_1_r )
{
	return exzisus_objectram1[offset];
}


WRITE_HANDLER( exzisus_videoram_0_w )
{
	exzisus_videoram0[offset] = data;
}


WRITE_HANDLER( exzisus_videoram_1_w )
{
	exzisus_videoram1[offset] = data;
}


WRITE_HANDLER( exzisus_objectram_0_w )
{
	exzisus_objectram0[offset] = data;
}


WRITE_HANDLER( exzisus_objectram_1_w )
{
	exzisus_objectram1[offset] = data;
}


/***************************************************************************
  Screen refresh
***************************************************************************/

void exzisus_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int offs;
	int sx, sy, xc, yc;
	int gfx_num, gfx_attr, gfx_offs;

	/* Is this correct ? */
	fillbitmap(bitmap, Machine->pens[1023], &Machine->visible_area);

	/* ---------- 1st TC0010VCU ---------- */
	sx = 0;
	for (offs = 0 ; offs < exzisus_objectram_size0 ; offs += 4)
    {
		int height;

		/* Skip empty sprites. */
		if ( !(*(UINT32 *)(&exzisus_objectram0[offs])) )
		{
			continue;
		}

		gfx_num = exzisus_objectram0[offs + 1];
		gfx_attr = exzisus_objectram0[offs + 3];

		if ((gfx_num & 0x80) == 0)	/* 16x16 sprites */
		{
			gfx_offs = ((gfx_num & 0x7f) << 3);
			height = 2;

			sx = exzisus_objectram0[offs + 2];
			sx |= (gfx_attr & 0x40) << 2;
		}
		else	/* tilemaps (each sprite is a 16x256 column) */
		{
			gfx_offs = ((gfx_num & 0x3f) << 7) + 0x0400;
			height = 32;

			if (gfx_num & 0x40)			/* Next column */
			{
				sx += 16;
			}
			else
			{
				sx = exzisus_objectram0[offs + 2];
				sx |= (gfx_attr & 0x40) << 2;
			}
		}

		sy = 256 - (height << 3) - (exzisus_objectram0[offs]);

		for (xc = 0 ; xc < 2 ; xc++)
		{
			int goffs = gfx_offs;
			for (yc = 0 ; yc < height ; yc++)
			{
				int code, color, x, y;

				code  = (exzisus_videoram0[goffs + 1] << 8) | exzisus_videoram0[goffs];
				color = (exzisus_videoram0[goffs + 1] >> 6) | (gfx_attr & 0x0f);
				x = (sx + (xc << 3)) & 0xff;
				y = (sy + (yc << 3)) & 0xff;

				drawgfx(bitmap, Machine->gfx[0],
						code & 0x3fff,
						color,
						0, 0,
						x, y,
						&Machine->visible_area, TRANSPARENCY_PEN, 15);
				goffs += 2;
			}
			gfx_offs += height << 1;
		}
	}

	/* ---------- 2nd TC0010VCU ---------- */
	sx = 0;
	for (offs = 0 ; offs < exzisus_objectram_size1 ; offs += 4)
    {
		int height;

		/* Skip empty sprites. */
		if ( !(*(UINT32 *)(&exzisus_objectram1[offs])) )
		{
			continue;
		}

		gfx_num = exzisus_objectram1[offs + 1];
		gfx_attr = exzisus_objectram1[offs + 3];

		if ((gfx_num & 0x80) == 0)	/* 16x16 sprites */
		{
			gfx_offs = ((gfx_num & 0x7f) << 3);
			height = 2;

			sx = exzisus_objectram1[offs + 2];
			sx |= (gfx_attr & 0x40) << 2;
		}
		else	/* tilemaps (each sprite is a 16x256 column) */
		{
			gfx_offs = ((gfx_num & 0x3f) << 7) + 0x0400;	///
			height = 32;

			if (gfx_num & 0x40)			/* Next column */
			{
				sx += 16;
			}
			else
			{
				sx = exzisus_objectram1[offs + 2];
				sx |= (gfx_attr & 0x40) << 2;
			}
		}
		sy = 256 - (height << 3) - (exzisus_objectram1[offs]);

		for (xc = 0 ; xc < 2 ; xc++)
		{
			int goffs = gfx_offs;
			for (yc = 0 ; yc < height ; yc++)
			{
				int code, color, x, y;

				code  = (exzisus_videoram1[goffs + 1] << 8) | exzisus_videoram1[goffs];
				color = (exzisus_videoram1[goffs + 1] >> 6) | (gfx_attr & 0x0f);
				x = (sx + (xc << 3)) & 0xff;
				y = (sy + (yc << 3)) & 0xff;

				drawgfx(bitmap, Machine->gfx[1],
						code & 0x3fff,
						color,
						0, 0,
						x, y,
						&Machine->visible_area, TRANSPARENCY_PEN, 15);
				goffs += 2;
			}
			gfx_offs += height << 1;
		}
	}
}


