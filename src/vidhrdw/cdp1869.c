#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/cdp1802/cdp1802.h"
#include "vidhrdw/cdp1869.h"
#include "sound/cdp1869.h"

/*

    RCA CDP1869/70/76 Video Interface System (VIS)

    http://homepage.mac.com/ruske/cosmacelf/cdp1869.pdf


    TODO:

    - color format control
    - PCB

*/

typedef struct
{
	int ntsc_pal;	// 0 = NTSC, 1 = PAL
	int dispoff;
	int fresvert, freshorz;
	int dblpage, line16, line9, cmem, cfc;
	UINT8 col, bkg;
	UINT16 pma, hma;
	UINT8 cram[4096];
	UINT8 pram[4096];
	//UINT8 pcb[4096];
} CDP1869_VIDEO_CONFIG;

static CDP1869_VIDEO_CONFIG cdp1869;

static unsigned short colortable_cdp1869[] =
{
	0x00, 0x00,	0x00, 0x01,	0x00, 0x02,	0x00, 0x03,	0x00, 0x04,	0x00, 0x05,	0x00, 0x06,	0x00, 0x07
};

PALETTE_INIT( cdp1869 )
{
	palette_set_color( machine, 0, 0x00, 0x00, 0x00 ); // black       0 % of max luminance
	palette_set_color( machine, 1, 0x00, 0x97, 0x00 ); // green      59
	palette_set_color( machine, 2, 0x00, 0x00, 0x1c ); // blue       11
	palette_set_color( machine, 3, 0x00, 0xb3, 0xb3 ); // cyan       70
	palette_set_color( machine, 4, 0x4c, 0x00, 0x00 ); // red        30
	palette_set_color( machine, 5, 0xe3, 0xe3, 0x00 ); // yellow     89
	palette_set_color( machine, 6, 0x68, 0x00, 0x68 ); // magenta    41
	palette_set_color( machine, 7, 0xff, 0xff, 0xff ); // white     100

	memcpy(colortable, colortable_cdp1869, sizeof(colortable_cdp1869));
}

static int cdp1869_get_pma(void)
{
	if (cdp1869.dblpage)
	{
		return cdp1869.pma;
	}
	else
	{
		return cdp1869.pma & 0x3ff;
	}
}

WRITE8_HANDLER ( cdp1869_charram_w )
{
	if (cdp1869.cmem)
	{
		int addr = (offset & 0x07) + (cdp1869.pram[cdp1869_get_pma()] * 16);
		cdp1869.cram[addr] = data;
		//cdp1869.pcb[addr] = cpu.q ? 1: 0;
	}
}

READ8_HANDLER ( cdp1869_charram_r )
{
	int addr = (offset & 0x07) + (cdp1869.pram[cdp1869_get_pma()] * 16);
/*
    int cid1;
    if (cdp1869.pcb[addr] == 0)
    {
        cid1 = 1; // IN0 0x80
    }
    else
    {
        cid1 = 0;
    }
*/

	return cdp1869.cram[addr];
}

WRITE8_HANDLER ( cdp1869_pageram_w )
{
	switch (cdp1869.cmem)
	{
	case 0:
		cdp1869.pram[offset] = data;
		break;

	case 1:
		cdp1869.pram[cdp1869_get_pma()] = data;
		break;
	}
}

READ8_HANDLER ( cdp1869_pageram_r )
{
	switch (cdp1869.cmem)
	{
	case 0:
		return cdp1869.pram[offset];

	case 1:
		return cdp1869.pram[cdp1869_get_pma()];

	default:
		return 0xff;
	}
}

static int cdp1869_get_lines(void)
{
	if (cdp1869.line16 && !cdp1869.dblpage)
	{
		return 16;
	}
	else if (!cdp1869.line9)
	{
		return 9;
	}
	else
	{
		return 8;
	}
}

static void cdp1869_draw_line(mame_bitmap *bitmap, int x, int y, int data, int pcb)
{
	int i;
	int color = 0;

	int ccb0 = (data & 0x40) >> 6;
	int ccb1 = (data & 0x80) >> 7;

	switch (cdp1869.col)
	{
	case 0:
		color = (ccb0 << 2) + (ccb1 << 1) + pcb;
		break;

	case 1:
		color = (ccb0 << 2) + (pcb << 1) + ccb1;
		break;

	case 2:
	case 3:
		color = (pcb << 2) + (ccb0 << 1) + ccb1;
		break;
	}

	data <<= 2;

	for (i = 0; i < CDP1870_CHAR_WIDTH; i++)
	{
		if (data & 0x80)
		{
			plot_pixel(bitmap, x, y, Machine->pens[color]);

			if (!cdp1869.fresvert)
			{
				plot_pixel(bitmap, x, y + 1, Machine->pens[color]);
			}

			if (!cdp1869.freshorz)
			{
				x++;

				plot_pixel(bitmap, x, y, Machine->pens[color]);

				if (!cdp1869.fresvert)
				{
					plot_pixel(bitmap, x, y + 1, Machine->pens[color]);
				}
			}
		}

		x++;

		data <<= 1;
	}
}

static void cdp1869_draw_char(mame_bitmap *bitmap, int x, int y, int code)
{
	int i, addr, pcb, data;

	if (!cdp1869.freshorz)
	{
		x *= 2;
	}

	if (!cdp1869.fresvert)
	{
		y *= 2;
	}

	addr = code * 16;

	for (i = 0; i < cdp1869_get_lines(); i++)
	{
		data = cdp1869.cram[addr];
		//pcb = cdp1869.pcb[addr];
		pcb = 0;

		cdp1869_draw_line(bitmap, x, y, data, pcb);

		addr++;
		y++;
	}
}

VIDEO_START( cdp1869 )
{
	return 0;
}

VIDEO_UPDATE( cdp1869 )
{
	if (cdp1869.dispoff)
	{
		fillbitmap(bitmap, get_black_pen(machine), cliprect);
	}
	else
	{
		int i, width, height, chars, addr;

		fillbitmap(bitmap, cdp1869.bkg, cliprect);

		width = cdp1869.freshorz ? 40 : 20;
		height = cdp1869.fresvert ? 25 : 12;

		chars = width * height;
		addr = cdp1869.hma;

		for (i = 0; i < chars; i++)
		{
			int x = (i % width) * CDP1870_CHAR_WIDTH;
			int y = (i / width) * cdp1869_get_lines();
			int code = cdp1869.pram[addr];

			cdp1869_draw_char(bitmap, x, y, code);

			addr++;

			if (addr > 2048) addr = 0;
		}
	}

	return 0;
}

static void cdp1869_out4_w(UINT16 data)
{
	/*
      bit   description

        0   tone amp 2^0
        1   tone amp 2^1
        2   tone amp 2^2
        3   tone amp 2^3
        4   tone freq sel0
        5   tone freq sel1
        6   tone freq sel2
        7   tone off
        8   tone / 2^0
        9   tone / 2^1
       10   tone / 2^2
       11   tone / 2^3
       12   tone / 2^4
       13   tone / 2^5
       14   tone / 2^6
       15   always 0
    */

	cdp1869_set_toneamp(0, data & 0x0f);
	cdp1869_set_tonefreq(0, (data & 0x70) >> 4);
	cdp1869_set_toneoff(0, (data & 0x80) >> 7);
	cdp1869_set_tonediv(0, (data & 0x7f00) >> 8);
}

static void cdp1869_out5_w(UINT16 data)
{
	/*
      bit   description

        0   cmem access mode
        1   x
        2   x
        3   9-line
        4   x
        5   16 line hi-res
        6   double page
        7   fres vert
        8   wn amp 2^0
        9   wn amp 2^1
       10   wn amp 2^2
       11   wn amp 2^3
       12   wn freq sel0
       13   wn freq sel1
       14   wn freq sel2
       15   wn off
    */

	cdp1869.cmem = (data & 0x01);
	cdp1869.line9 = (data & 0x08) >> 3;
	cdp1869.line16 = (data & 0x20) >> 5;
	cdp1869.dblpage = (data & 0x40) >> 6;
	cdp1869.fresvert = (data & 0x80) >> 7;

	cdp1869_set_wnamp(0, (data & 0x0f00) >> 8);
	cdp1869_set_wnfreq(0, (data & 0x7000) >> 12);
	cdp1869_set_wnoff(0, (data & 0x8000) >> 15);
}

static void cdp1869_out6_w(UINT16 data)
{
	/*
      bit   description

        0   pma0 reg
        1   pma1 reg
        2   pma2 reg
        3   pma3 reg
        4   pma4 reg
        5   pma5 reg
        6   pma6 reg
        7   pma7 reg
        8   pma8 reg
        9   pma9 reg
       10   pma10 reg
       11   x
       12   x
       13   x
       14   x
       15   x
    */

	cdp1869.pma = data & 0x7ff;
}

static void cdp1869_out7_w(UINT16 data)
{
	/*
      bit   description

        0   x
        1   x
        2   hma2 reg
        3   hma3 reg
        4   hma4 reg
        5   hma5 reg
        6   hma6 reg
        7   hma7 reg
        8   hma8 reg
        9   hma9 reg
       10   hma10 reg
       11   x
       12   x
       13   x
       14   x
       15   x
    */

	cdp1869.hma = data & 0x7fc;
}

WRITE8_HANDLER ( cdp1870_out3_w )
{
	/*
      bit   description

        0   bkg green
        1   bkg blue
        2   bkg red
        3   cfc
        4   disp off
        5   colb0
        6   colb1
        7   fres horz
    */

	cdp1869.bkg = (data & 0x07);
	cdp1869.cfc = (data & 0x08) >> 3;
	cdp1869.dispoff = (data & 0x10) >> 4;
	cdp1869.col = (data & 0x60) >> 5;
	cdp1869.freshorz = (data & 0x80) >> 7;
}

WRITE8_HANDLER ( cdp1869_out_w )
{
	UINT16 address = activecpu_get_reg(activecpu_get_reg(CDP1802_X) + CDP1802_R0) - 1; // TODO: why -1?

	switch (offset)
	{
	case 0:
		cdp1869_out4_w(address);
		break;
	case 1:
		cdp1869_out5_w(address);
		break;
	case 2:
		cdp1869_out6_w(address);
		break;
	case 3:
		cdp1869_out7_w(address);
		break;
	}
}
