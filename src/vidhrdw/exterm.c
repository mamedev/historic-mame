/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/tms34010/tms34010.h"


data16_t *exterm_master_videoram, *exterm_slave_videoram;



/*************************************
 *
 *	Palette setup
 *
 *************************************/

void exterm_init_palette(UINT8 *palette, UINT16 *colortable,const UINT8 *color_prom)
{
	int i;

	palette += 3*4096;	/* first 4096 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r,g,b;

		r = (i >> 10) & 0x1f;
		g = (i >>  5) & 0x1f;
		b = (i >>  0) & 0x1f;

		(*palette++) = (r << 3) | (r >> 2);
		(*palette++) = (g << 3) | (g >> 2);
		(*palette++) = (b << 3) | (b >> 2);
	}
}



/*************************************
 *
 *	Master shift register
 *
 *************************************/

void exterm_to_shiftreg_master(unsigned int address, UINT16* shiftreg)
{
	memcpy(shiftreg, &exterm_master_videoram[TOWORD(address)], 256 * sizeof(UINT16));
}

void exterm_from_shiftreg_master(unsigned int address, UINT16* shiftreg)
{
	memcpy(&exterm_master_videoram[TOWORD(address)], shiftreg, 256 * sizeof(UINT16));
}

void exterm_to_shiftreg_slave(unsigned int address, UINT16* shiftreg)
{
	memcpy(shiftreg, &exterm_slave_videoram[TOWORD(address)], 256 * 2 * sizeof(UINT8));
}

void exterm_from_shiftreg_slave(unsigned int address, UINT16* shiftreg)
{
	memcpy(&exterm_slave_videoram[TOWORD(address)], shiftreg, 256 * 2 * sizeof(UINT8));
}



/*************************************
 *
 *	Video startup/shutdown
 *
 *************************************/

int exterm_vh_start(void)
{
	palette_used_colors[0] = PALETTE_COLOR_TRANSPARENT;
	return 0;
}

void exterm_vh_stop(void)
{
}



/*************************************
 *
 *	Main video refresh
 *
 *************************************/

void exterm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	data16_t *bgsrc, *fgsrc, *pens = Machine->pens;
	int x, y;

	/* if the display is blanked, fill with black */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, palette_transparent_pen, &Machine->visible_area);
		return;
	}

	/* recalc the palette */
	palette_recalc();

	/* redraw screen */
	bgsrc = &exterm_master_videoram[0];
	fgsrc = (tms34010_get_DPYSTRT(1) & 0x800) ? &exterm_slave_videoram[40*128] : &exterm_slave_videoram[296*128];

	/* 16-bit case */
	if (bitmap->depth == 16)
	{
		for (y = 0; y < 256; y++)
		{
			UINT16 *dst = (UINT16 *)bitmap->line[y];

			if (y < 40 || y > 238)
			{
				for (x = 0; x < 256; x++)
				{
					UINT16 bgdata = *bgsrc++;;
					*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
				}
			}
			else
			{
				for (x = 0; x < 256 / 2; x++)
				{
					UINT16 fgdata = *fgsrc++;
					UINT16 bgdata;

					if (fgdata & 0x00ff)
						*dst++ = pens[fgdata & 0x00ff];
					else
					{
						bgdata = bgsrc[0];
						*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
					}

					if (fgdata & 0xff00)
						*dst++ = pens[fgdata >> 8];
					else
					{
						bgdata = bgsrc[1];
						*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
					}

					bgsrc += 2;
				}
			}
		}
	}

	/* 8-bit case */
	else
	{
		for (y = 0; y < 256; y++)
		{
			UINT8 *dst = bitmap->line[y];

			if (y < 40 || y > 238)
			{
				for (x = 0; x < 256; x++)
				{
					UINT16 bgdata = *bgsrc++;;
					*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
				}
			}
			else
			{
				for (x = 0; x < 256 / 2; x++)
				{
					UINT16 fgdata = *fgsrc++;
					UINT16 bgdata;

					if (fgdata & 0x00ff)
						*dst++ = pens[fgdata & 0x00ff];
					else
					{
						bgdata = bgsrc[0];
						*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
					}

					if (fgdata & 0xff00)
						*dst++ = pens[fgdata >> 8];
					else
					{
						bgdata = bgsrc[1];
						*dst++ = pens[(bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000)];
					}

					bgsrc += 2;
				}
			}
		}
	}
}
