/***************************************************************************

    Midway 8080-based black and white hardware

****************************************************************************/

#include "driver.h"
#include "mw8080bw.h"



VIDEO_UPDATE( mw8080bw )
{
	offs_t offs;

	for (offs = 0; offs < videoram_size; offs++)
	{
		UINT8 i;

		UINT8 data = videoram[offs];

		for (i = 0; i < 8; i++)
		{
			UINT8 x, y, bit;

			y = offs >> 5;
			x = ((offs & 0x1f) << 3) | i;

			bit = (data >> i) & 0x01;

			plot_pixel(bitmap, x, y, Machine->pens[bit]);
		}
	}

	return 0;
}



/*************************************
 *
 *  Phantom II
 *
 *************************************/


PALETTE_INIT( phantom2 )
{
	palette_set_color(machine,0,0x00,0x00,0x00); /* black */
	palette_set_color(machine,1,0xff,0xff,0xff); /* white */
	palette_set_color(machine,2,0xc0,0xc0,0xc0); /* grey */
}


VIDEO_UPDATE( phantom2 )
{
	static const int CLOUD_SHIFT[] = { 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08,
									   0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80 };
	offs_t offs;
	UINT8 *cloud_region = memory_region(REGION_PROMS);
	UINT8 cloud_pos = phantom2_get_cloud_pos();

	for (offs = 0; offs < videoram_size; offs++)
	{
		UINT8 i;

		UINT8 data = videoram[offs];

		for (i = 0; i < 8; i++)
		{
			UINT8 x, y, col;

			y = offs >> 5;
			x = ((offs & 0x1f) << 3) | i;

			col = (data >> i) & 0x01;

			if (col == 0)
			{
				/* possible cloud gfx in the background */
				int cloud_x;
				offs_t cloud_offs;

				cloud_x = x - 12;  /* the -12 is based on screen shots */
				cloud_offs = (((y - cloud_pos) & 0xff) >> 1 << 4) | (cloud_x >> 4);

				if (cloud_region[cloud_offs] & (CLOUD_SHIFT[cloud_x & 0x0f]))
				{
					/* col = cpu_getcurrentframe() & 0x01; */
					col = 2;	/* grey cloud */
				}
			}

			plot_pixel(bitmap, x, y, Machine->pens[col]);
		}
	}

	return 0;
}



/*************************************
 *
 *  Space Invaders
 *
 *************************************/


VIDEO_UPDATE( invaders )
{
	offs_t offs;

	for (offs = 0; offs < videoram_size; offs++)
	{
		UINT8 i;

		UINT8 data = videoram[offs];

		for (i = 0; i < 8; i++)
		{
			UINT8 x, y, bit;

			y = offs >> 5;
			x = ((offs & 0x1f) << 3) | i;

			if (flip_screen)
			{
				x = ~x;
				y = ~y;
			}

			bit = (data >> i) & 0x01;

			plot_pixel(bitmap, x, y, Machine->pens[bit]);
		}
	}

	return 0;
}
