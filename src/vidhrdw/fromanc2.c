/******************************************************************************

	Video Hardware for Video System Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/04 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static struct osd_bitmap *fromanc2_tmpbitmapx[2];
static struct osd_bitmap *fromanc2_tmpbitmap[2][4];
static data16_t *fromanc2_videoram[2][4];
static data8_t *fromanc2_dirty[2][4];
static data16_t *fromanc2_paletteram[2];
static int fromanc2_scrollx[2][4], fromanc2_scrolly[2][4];
static int fromanc2_gfxbank[2][4];


void fromanc2_vh_stop(void);
void fromancr_vh_stop(void);


/******************************************************************************


******************************************************************************/
READ16_HANDLER( fromanc2_paletteram_0_r )
{
	return fromanc2_paletteram[0][offset];
}

READ16_HANDLER( fromanc2_paletteram_1_r )
{
	return fromanc2_paletteram[1][offset];
}

WRITE16_HANDLER( fromanc2_paletteram_0_w )
{
	int color;
	int r, g, b;

	COMBINE_DATA(&fromanc2_paletteram[0][offset]);

	// GGGG_GRRR_RRBB_BBBx
	r = ((data >>  6) & 0x1f);
	g = ((data >> 11) & 0x1f);
	b = ((data >>  1) & 0x1f);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	color = (((offset & 0x0700) << 1) + (offset & 0x00ff));
	palette_set_color((0x000 + color), r, g, b);
}

WRITE16_HANDLER( fromanc2_paletteram_1_w )
{
	int color;
	int r, g, b;

	COMBINE_DATA(&fromanc2_paletteram[1][offset]);

	// GGGG_GRRR_RRBB_BBBx
	r = ((data >>  6) & 0x1f);
	g = ((data >> 11) & 0x1f);
	b = ((data >>  1) & 0x1f);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	color = (((offset & 0x0700) << 1) + (offset & 0x00ff));
	palette_set_color((0x100 + color), r, g, b);
}

READ16_HANDLER( fromancr_paletteram_0_r )
{
	return fromanc2_paletteram[0][offset];
}

READ16_HANDLER( fromancr_paletteram_1_r )
{
	return fromanc2_paletteram[1][offset];
}

WRITE16_HANDLER( fromancr_paletteram_0_w )
{
	int color;
	int r, g, b;

	COMBINE_DATA(&fromanc2_paletteram[0][offset]);

	// xGGG_GGRR_RRRB_BBBB
	r = ((data >>  5) & 0x1f);
	g = ((data >> 10) & 0x1f);
	b = ((data >>  0) & 0x1f);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	color = (((offset & 0x0700) << 1) + (offset & 0x00ff));
	palette_set_color((0x000 + color), r, g, b);
}

WRITE16_HANDLER( fromancr_paletteram_1_w )
{
	int color;
	int r, g, b;

	COMBINE_DATA(&fromanc2_paletteram[1][offset]);

	// xGGG_GGRR_RRRB_BBBB
	r = ((data >>  5) & 0x1f);
	g = ((data >> 10) & 0x1f);
	b = ((data >>  0) & 0x1f);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	color = (((offset & 0x0700) << 1) + (offset & 0x00ff));
	palette_set_color((0x100 + color), r, g, b);
}

/******************************************************************************


******************************************************************************/
WRITE16_HANDLER( fromanc2_videoram_0_w )
{
#if 0
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][0][(offset & 0x0fff)]);
		fromanc2_dirty[0][0][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][0][(offset & 0x0fff)]);
		fromanc2_dirty[1][0][(offset & 0x0fff)] = 1;
	}
#else
		COMBINE_DATA(&fromanc2_videoram[0][0][(offset & 0x1fff)]);
		fromanc2_dirty[0][0][(offset & 0x1fff)] = 1;
#endif
}

WRITE16_HANDLER( fromanc2_videoram_1_w )
{
#if 0
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][1][(offset & 0x0fff)]);
		fromanc2_dirty[0][1][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][1][(offset & 0x0fff)]);
		fromanc2_dirty[1][1][(offset & 0x0fff)] = 1;
	}
#else
		COMBINE_DATA(&fromanc2_videoram[0][1][(offset & 0x1fff)]);
		fromanc2_dirty[0][1][(offset & 0x1fff)] = 1;
#endif
}

WRITE16_HANDLER( fromanc2_videoram_2_w )
{
#if 0
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][2][(offset & 0x0fff)]);
		fromanc2_dirty[0][2][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][2][(offset & 0x0fff)]);
		fromanc2_dirty[1][2][(offset & 0x0fff)] = 1;
	}
#else
		COMBINE_DATA(&fromanc2_videoram[0][2][(offset & 0x1fff)]);
		fromanc2_dirty[0][2][(offset & 0x1fff)] = 1;
#endif
}

WRITE16_HANDLER( fromanc2_videoram_3_w )
{
#if 0
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][3][(offset & 0x0fff)]);
		fromanc2_dirty[0][3][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][3][(offset & 0x0fff)]);
		fromanc2_dirty[1][3][(offset & 0x0fff)] = 1;
	}
#else
		COMBINE_DATA(&fromanc2_videoram[0][3][(offset & 0x1fff)]);
		fromanc2_dirty[0][3][(offset & 0x1fff)] = 1;
#endif
}

WRITE16_HANDLER( fromancr_videoram_0_w )
{
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][0][(offset & 0x0fff)]);
		fromanc2_dirty[0][0][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][0][(offset & 0x0fff)]);
		fromanc2_dirty[1][0][(offset & 0x0fff)] = 1;
	}
}

WRITE16_HANDLER( fromancr_videoram_1_w )
{
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][1][(offset & 0x0fff)]);
		fromanc2_dirty[0][1][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][1][(offset & 0x0fff)]);
		fromanc2_dirty[1][1][(offset & 0x0fff)] = 1;
	}
}

WRITE16_HANDLER( fromancr_videoram_2_w )
{
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][2][(offset & 0x0fff)]);
		fromanc2_dirty[0][2][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][2][(offset & 0x0fff)]);
		fromanc2_dirty[1][2][(offset & 0x0fff)] = 1;
	}
}

WRITE16_HANDLER( fromancr_videoram_3_w )
{
	if (offset < 0x1000)
	{
		COMBINE_DATA(&fromanc2_videoram[0][3][(offset & 0x0fff)]);
		fromanc2_dirty[0][3][(offset & 0x0fff)] = 1;
	}
	else
	{
		COMBINE_DATA(&fromanc2_videoram[1][3][(offset & 0x0fff)]);
		fromanc2_dirty[1][3][(offset & 0x0fff)] = 1;
	}
}

WRITE16_HANDLER( fromanc2_gfxreg_0_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[0][0] = -(data - 0x000); break;
		case	0x01:	fromanc2_scrolly[0][0] = -(data - 0x000); break;
		case	0x02:	fromanc2_scrollx[0][1] = -(data - 0x004); break;
		case	0x03:	fromanc2_scrolly[0][1] = -(data - 0x000); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromanc2_gfxreg_1_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[1][0] = -(data - 0x1be); break;
		case	0x01:	fromanc2_scrolly[1][0] = -(data - 0x1ef); break;
		case	0x02:	fromanc2_scrollx[1][1] = -(data - 0x1c2); break;
		case	0x03:	fromanc2_scrolly[1][1] = -(data - 0x1ef); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromanc2_gfxreg_2_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[0][2] = -(data - 0x1c0); break;
		case	0x01:	fromanc2_scrolly[0][2] = -(data - 0x1ef); break;
		case	0x02:	fromanc2_scrollx[0][3] = -(data - 0x1c3); break;
		case	0x03:	fromanc2_scrolly[0][3] = -(data - 0x1ef); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromanc2_gfxreg_3_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[1][2] = -(data - 0x1bf); break;
		case	0x01:	fromanc2_scrolly[1][2] = -(data - 0x1ef); break;
		case	0x02:	fromanc2_scrollx[1][3] = -(data - 0x1c3); break;
		case	0x03:	fromanc2_scrolly[1][3] = -(data - 0x1ef); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromancr_gfxreg_0_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[0][0] = -(data - 0x1bf); break;
		case	0x01:	fromanc2_scrolly[0][0] = -(data - 0x1ef); break;
		case	0x02:	fromanc2_scrollx[1][0] = -(data - 0x1c3); break;
		case	0x03:	fromanc2_scrolly[1][0] = -(data - 0x1ef); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromancr_gfxreg_1_w )
{
	switch(offset)
	{
		case	0x00:	fromanc2_scrollx[0][1] = -(data - 0x000); break;
		case	0x01:	fromanc2_scrolly[0][1] = -(data - 0x000); break;
		case	0x02:	fromanc2_scrollx[1][1] = -(data - 0x004); break;
		case	0x03:	fromanc2_scrolly[1][1] = -(data - 0x000); break;
		// offset 0x04 - 0x11 unknown
		default:	break;
	}
}

WRITE16_HANDLER( fromanc2_gfxbank_0_w )
{
	fromanc2_gfxbank[0][0] = ((data & 0x000f) >>  0);	// G (1P)
	fromanc2_gfxbank[0][1] = ((data & 0x00f0) >>  4);	// G (1P)
	fromanc2_gfxbank[0][2] = ((data & 0x0f00) >>  8);	// G (1P)
	fromanc2_gfxbank[0][3] = ((data & 0xf000) >> 12);	// G (1P)
}

WRITE16_HANDLER( fromanc2_gfxbank_1_w )
{
	fromanc2_gfxbank[1][0] = ((data & 0x000f) >>  0);	// G (2P)
	fromanc2_gfxbank[1][1] = ((data & 0x00f0) >>  4);	// G (2P)
	fromanc2_gfxbank[1][2] = ((data & 0x0f00) >>  8);	// G (2P)
	fromanc2_gfxbank[1][3] = ((data & 0xf000) >> 12);	// G (2P)
}

WRITE16_HANDLER( fromancr_gfxbank_w )
{
	fromanc2_gfxbank[0][0] = ((data & 0x00f0) >>  4);	// BG (1P)
	fromanc2_gfxbank[0][1] = ((data & 0xf000) >> 12);	// FG (1P)
//	fromanc2_gfxbank[1][0] = ((data & 0x000f) >>  0);	// BG (2P)
	fromanc2_gfxbank[1][0] = ((data & 0x0008) >>  3); /*?*/	// BG (2P)
	fromanc2_gfxbank[1][1] = ((data & 0x0f00) >>  8);	// FG (2P)
}

/******************************************************************************


******************************************************************************/
int fromanc2_vh_start(void)
{
	fromanc2_videoram[0][0] = malloc(0x2000*2);
	fromanc2_videoram[0][1] = malloc(0x2000*2);
	fromanc2_videoram[0][2] = malloc(0x2000*2);
	fromanc2_videoram[0][3] = malloc(0x2000*2);
	fromanc2_videoram[1][0] = malloc(0x2000*2);
	fromanc2_videoram[1][1] = malloc(0x2000*2);
	fromanc2_videoram[1][2] = malloc(0x2000*2);
	fromanc2_videoram[1][3] = malloc(0x2000*2);

	fromanc2_tmpbitmapx[0] = bitmap_alloc(352, 240);
	fromanc2_tmpbitmapx[1] = bitmap_alloc(352, 240);

	fromanc2_tmpbitmap[0][0] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[0][1] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[0][2] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[0][3] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][0] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][1] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][2] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][3] = bitmap_alloc(512, 512);

	fromanc2_dirty[0][0] = malloc(0x2000*2);
	fromanc2_dirty[0][1] = malloc(0x2000*2);
	fromanc2_dirty[0][2] = malloc(0x2000*2);
	fromanc2_dirty[0][3] = malloc(0x2000*2);
	fromanc2_dirty[1][0] = malloc(0x2000*2);
	fromanc2_dirty[1][1] = malloc(0x2000*2);
	fromanc2_dirty[1][2] = malloc(0x2000*2);
	fromanc2_dirty[1][3] = malloc(0x2000*2);

	fromanc2_paletteram[0] = malloc(0x1000);
	fromanc2_paletteram[1] = malloc(0x1000);

	if ((!fromanc2_videoram[0][0]) ||
	    (!fromanc2_videoram[0][1]) ||
	    (!fromanc2_videoram[0][2]) ||
	    (!fromanc2_videoram[0][3]) ||
	    (!fromanc2_videoram[1][0]) ||
	    (!fromanc2_videoram[1][1]) ||
	    (!fromanc2_videoram[1][2]) ||
	    (!fromanc2_videoram[1][3]) ||
	    (!fromanc2_tmpbitmapx[0]) ||
	    (!fromanc2_tmpbitmapx[1]) ||
	    (!fromanc2_tmpbitmap[0][0]) ||
	    (!fromanc2_tmpbitmap[0][1]) ||
	    (!fromanc2_tmpbitmap[0][2]) ||
	    (!fromanc2_tmpbitmap[0][3]) ||
	    (!fromanc2_tmpbitmap[1][0]) ||
	    (!fromanc2_tmpbitmap[1][1]) ||
	    (!fromanc2_tmpbitmap[1][2]) ||
	    (!fromanc2_tmpbitmap[1][3]) ||
	    (!fromanc2_dirty[0][0]) ||
	    (!fromanc2_dirty[0][1]) ||
	    (!fromanc2_dirty[0][2]) ||
	    (!fromanc2_dirty[0][3]) ||
	    (!fromanc2_dirty[1][0]) ||
	    (!fromanc2_dirty[1][1]) ||
	    (!fromanc2_dirty[1][2]) ||
	    (!fromanc2_dirty[1][3]) ||
	    (!fromanc2_paletteram[0]) ||
	    (!fromanc2_paletteram[1]))
	{
		fromanc2_vh_stop();
		return 1;
	}

	memset(fromanc2_videoram[0][0], 0, 0x2000*2);
	memset(fromanc2_videoram[0][1], 0, 0x2000*2);
	memset(fromanc2_videoram[0][2], 0, 0x2000*2);
	memset(fromanc2_videoram[0][3], 0, 0x2000*2);
	memset(fromanc2_videoram[1][0], 0, 0x2000*2);
	memset(fromanc2_videoram[1][1], 0, 0x2000*2);
	memset(fromanc2_videoram[1][2], 0, 0x2000*2);
	memset(fromanc2_videoram[1][3], 0, 0x2000*2);

	memset(fromanc2_dirty[0][0], 1, 0x2000*2);
	memset(fromanc2_dirty[0][1], 1, 0x2000*2);
	memset(fromanc2_dirty[0][2], 1, 0x2000*2);
	memset(fromanc2_dirty[0][3], 1, 0x2000*2);
	memset(fromanc2_dirty[1][0], 1, 0x2000*2);
	memset(fromanc2_dirty[1][1], 1, 0x2000*2);
	memset(fromanc2_dirty[1][2], 1, 0x2000*2);
	memset(fromanc2_dirty[1][3], 1, 0x2000*2);

	return 0;
}

void fromanc2_vh_stop(void)
{
	if (fromanc2_paletteram[1]) free(fromanc2_paletteram[1]);
	fromanc2_paletteram[1] = 0;
	if (fromanc2_paletteram[0]) free(fromanc2_paletteram[0]);
	fromanc2_paletteram[0] = 0;

	if (fromanc2_dirty[1][3]) free(fromanc2_dirty[1][3]);
	fromanc2_dirty[1][3] = 0;
	if (fromanc2_dirty[1][2]) free(fromanc2_dirty[1][2]);
	fromanc2_dirty[1][2] = 0;
	if (fromanc2_dirty[1][1]) free(fromanc2_dirty[1][1]);
	fromanc2_dirty[1][1] = 0;
	if (fromanc2_dirty[1][0]) free(fromanc2_dirty[1][0]);
	fromanc2_dirty[1][0] = 0;
	if (fromanc2_dirty[0][3]) free(fromanc2_dirty[0][3]);
	fromanc2_dirty[0][3] = 0;
	if (fromanc2_dirty[0][2]) free(fromanc2_dirty[0][2]);
	fromanc2_dirty[0][2] = 0;
	if (fromanc2_dirty[0][1]) free(fromanc2_dirty[0][1]);
	fromanc2_dirty[0][1] = 0;
	if (fromanc2_dirty[0][0]) free(fromanc2_dirty[0][0]);
	fromanc2_dirty[0][0] = 0;

	if (fromanc2_tmpbitmap[1][3]) bitmap_free(fromanc2_tmpbitmap[1][3]);
	fromanc2_tmpbitmap[1][3] = 0;
	if (fromanc2_tmpbitmap[1][2]) bitmap_free(fromanc2_tmpbitmap[1][2]);
	fromanc2_tmpbitmap[1][2] = 0;
	if (fromanc2_tmpbitmap[1][1]) bitmap_free(fromanc2_tmpbitmap[1][1]);
	fromanc2_tmpbitmap[1][1] = 0;
	if (fromanc2_tmpbitmap[1][0]) bitmap_free(fromanc2_tmpbitmap[1][0]);
	fromanc2_tmpbitmap[1][0] = 0;
	if (fromanc2_tmpbitmap[0][3]) bitmap_free(fromanc2_tmpbitmap[0][3]);
	fromanc2_tmpbitmap[0][3] = 0;
	if (fromanc2_tmpbitmap[0][2]) bitmap_free(fromanc2_tmpbitmap[0][2]);
	fromanc2_tmpbitmap[0][2] = 0;
	if (fromanc2_tmpbitmap[0][1]) bitmap_free(fromanc2_tmpbitmap[0][1]);
	fromanc2_tmpbitmap[0][1] = 0;
	if (fromanc2_tmpbitmap[0][0]) bitmap_free(fromanc2_tmpbitmap[0][0]);
	fromanc2_tmpbitmap[0][0] = 0;

	if (fromanc2_tmpbitmapx[1]) bitmap_free(fromanc2_tmpbitmapx[1]);
	fromanc2_tmpbitmapx[1] = 0;
	if (fromanc2_tmpbitmapx[0]) bitmap_free(fromanc2_tmpbitmapx[0]);
	fromanc2_tmpbitmapx[0] = 0;

	if (fromanc2_videoram[1][3]) free(fromanc2_videoram[1][3]);
	fromanc2_videoram[1][3] = 0;
	if (fromanc2_videoram[1][2]) free(fromanc2_videoram[1][2]);
	fromanc2_videoram[1][2] = 0;
	if (fromanc2_videoram[1][1]) free(fromanc2_videoram[1][1]);
	fromanc2_videoram[1][1] = 0;
	if (fromanc2_videoram[1][0]) free(fromanc2_videoram[1][0]);
	fromanc2_videoram[1][0] = 0;
	if (fromanc2_videoram[0][3]) free(fromanc2_videoram[0][3]);
	fromanc2_videoram[0][3] = 0;
	if (fromanc2_videoram[0][2]) free(fromanc2_videoram[0][2]);
	fromanc2_videoram[0][2] = 0;
	if (fromanc2_videoram[0][1]) free(fromanc2_videoram[0][1]);
	fromanc2_videoram[0][1] = 0;
	if (fromanc2_videoram[0][0]) free(fromanc2_videoram[0][0]);
	fromanc2_videoram[0][0] = 0;
}

int fromancr_vh_start(void)
{
	fromanc2_videoram[0][0] = malloc(0x2000);
	fromanc2_videoram[0][1] = malloc(0x2000);
	fromanc2_videoram[0][2] = malloc(0x2000);
	fromanc2_videoram[1][0] = malloc(0x2000);
	fromanc2_videoram[1][1] = malloc(0x2000);
	fromanc2_videoram[1][2] = malloc(0x2000);

	fromanc2_tmpbitmapx[0] = bitmap_alloc(352, 240);
	fromanc2_tmpbitmapx[1] = bitmap_alloc(352, 240);

	fromanc2_tmpbitmap[0][0] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[0][1] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[0][2] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][0] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][1] = bitmap_alloc(512, 512);
	fromanc2_tmpbitmap[1][2] = bitmap_alloc(512, 512);

	fromanc2_dirty[0][0] = malloc(0x2000);
	fromanc2_dirty[0][1] = malloc(0x2000);
	fromanc2_dirty[0][2] = malloc(0x2000);
	fromanc2_dirty[1][0] = malloc(0x2000);
	fromanc2_dirty[1][1] = malloc(0x2000);
	fromanc2_dirty[1][2] = malloc(0x2000);

	fromanc2_paletteram[0] = malloc(0x1000);
	fromanc2_paletteram[1] = malloc(0x1000);

	if ((!fromanc2_videoram[0][0]) ||
	    (!fromanc2_videoram[0][1]) ||
	    (!fromanc2_videoram[0][2]) ||
	    (!fromanc2_videoram[1][0]) ||
	    (!fromanc2_videoram[1][1]) ||
	    (!fromanc2_videoram[1][2]) ||
	    (!fromanc2_tmpbitmapx[0]) ||
	    (!fromanc2_tmpbitmapx[1]) ||
	    (!fromanc2_tmpbitmap[0][0]) ||
	    (!fromanc2_tmpbitmap[0][1]) ||
	    (!fromanc2_tmpbitmap[0][2]) ||
	    (!fromanc2_tmpbitmap[1][0]) ||
	    (!fromanc2_tmpbitmap[1][1]) ||
	    (!fromanc2_tmpbitmap[1][2]) ||
	    (!fromanc2_dirty[0][0]) ||
	    (!fromanc2_dirty[0][1]) ||
	    (!fromanc2_dirty[0][2]) ||
	    (!fromanc2_dirty[1][0]) ||
	    (!fromanc2_dirty[1][1]) ||
	    (!fromanc2_dirty[1][2]) ||
	    (!fromanc2_paletteram[0]) ||
	    (!fromanc2_paletteram[1]))
	{
		fromancr_vh_stop();
		return 1;
	}

	memset(fromanc2_videoram[0][0], 0, 0x2000);
	memset(fromanc2_videoram[0][1], 0, 0x2000);
	memset(fromanc2_videoram[0][2], 0, 0x2000);
	memset(fromanc2_videoram[1][0], 0, 0x2000);
	memset(fromanc2_videoram[1][1], 0, 0x2000);
	memset(fromanc2_videoram[1][2], 0, 0x2000);

	memset(fromanc2_dirty[0][0], 1, 0x2000);
	memset(fromanc2_dirty[0][1], 1, 0x2000);
	memset(fromanc2_dirty[0][2], 1, 0x2000);
	memset(fromanc2_dirty[1][0], 1, 0x2000);
	memset(fromanc2_dirty[1][1], 1, 0x2000);
	memset(fromanc2_dirty[1][2], 1, 0x2000);

	return 0;
}

void fromancr_vh_stop(void)
{
	if (fromanc2_paletteram[1]) free(fromanc2_paletteram[1]);
	fromanc2_paletteram[1] = 0;
	if (fromanc2_paletteram[0]) free(fromanc2_paletteram[0]);
	fromanc2_paletteram[0] = 0;

	if (fromanc2_dirty[1][2]) free(fromanc2_dirty[1][2]);
	fromanc2_dirty[1][2] = 0;
	if (fromanc2_dirty[1][1]) free(fromanc2_dirty[1][1]);
	fromanc2_dirty[1][1] = 0;
	if (fromanc2_dirty[1][0]) free(fromanc2_dirty[1][0]);
	fromanc2_dirty[1][0] = 0;
	if (fromanc2_dirty[0][2]) free(fromanc2_dirty[0][2]);
	fromanc2_dirty[0][2] = 0;
	if (fromanc2_dirty[0][1]) free(fromanc2_dirty[0][1]);
	fromanc2_dirty[0][1] = 0;
	if (fromanc2_dirty[0][0]) free(fromanc2_dirty[0][0]);
	fromanc2_dirty[0][0] = 0;

	if (fromanc2_tmpbitmap[1][2]) bitmap_free(fromanc2_tmpbitmap[1][2]);
	fromanc2_tmpbitmap[1][2] = 0;
	if (fromanc2_tmpbitmap[1][1]) bitmap_free(fromanc2_tmpbitmap[1][1]);
	fromanc2_tmpbitmap[1][1] = 0;
	if (fromanc2_tmpbitmap[1][0]) bitmap_free(fromanc2_tmpbitmap[1][0]);
	fromanc2_tmpbitmap[1][0] = 0;
	if (fromanc2_tmpbitmap[0][2]) bitmap_free(fromanc2_tmpbitmap[0][2]);
	fromanc2_tmpbitmap[0][2] = 0;
	if (fromanc2_tmpbitmap[0][1]) bitmap_free(fromanc2_tmpbitmap[0][1]);
	fromanc2_tmpbitmap[0][1] = 0;
	if (fromanc2_tmpbitmap[0][0]) bitmap_free(fromanc2_tmpbitmap[0][0]);
	fromanc2_tmpbitmap[0][0] = 0;

	if (fromanc2_tmpbitmapx[1]) bitmap_free(fromanc2_tmpbitmapx[1]);
	fromanc2_tmpbitmapx[1] = 0;
	if (fromanc2_tmpbitmapx[0]) bitmap_free(fromanc2_tmpbitmapx[0]);
	fromanc2_tmpbitmapx[0] = 0;

	if (fromanc2_videoram[1][2]) free(fromanc2_videoram[1][2]);
	fromanc2_videoram[1][2] = 0;
	if (fromanc2_videoram[1][1]) free(fromanc2_videoram[1][1]);
	fromanc2_videoram[1][1] = 0;
	if (fromanc2_videoram[1][0]) free(fromanc2_videoram[1][0]);
	fromanc2_videoram[1][0] = 0;
	if (fromanc2_videoram[0][2]) free(fromanc2_videoram[0][2]);
	fromanc2_videoram[0][2] = 0;
	if (fromanc2_videoram[0][1]) free(fromanc2_videoram[0][1]);
	fromanc2_videoram[0][1] = 0;
	if (fromanc2_videoram[0][0]) free(fromanc2_videoram[0][0]);
	fromanc2_videoram[0][0] = 0;
}

/******************************************************************************


******************************************************************************/
void fromanc2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int vram;
//	int layer;
	int offs;

	if (full_refresh)
	{
		memset(fromanc2_dirty[0][0], 1, 0x2000);
		memset(fromanc2_dirty[0][1], 1, 0x2000);
		memset(fromanc2_dirty[0][2], 1, 0x2000);
		memset(fromanc2_dirty[0][3], 1, 0x2000);
		memset(fromanc2_dirty[1][0], 1, 0x2000);
		memset(fromanc2_dirty[1][1], 1, 0x2000);
		memset(fromanc2_dirty[1][2], 1, 0x2000);
		memset(fromanc2_dirty[1][3], 1, 0x2000);
	}

#if 1
	for (vram = 0; vram < 2; vram++)
	{
			for (offs = 0; offs < (0x2000 / sizeof(short)); offs++)
			{
				// 4TH (BG) Layer
				if (fromanc2_dirty[0][1 + (vram * 2)][offs])
				{
					int tile;
					int color;
					int sx, sy;
					int flipx, flipy;

					fromanc2_dirty[0][1 + (vram * 2)][offs] = 0;

					tile  = ((fromanc2_videoram[0][1 + (vram * 2)][offs] & 0x3fff) | (fromanc2_gfxbank[vram][0] << 14));
					color = ((fromanc2_videoram[0][1 + (vram * 2)][offs] & 0xc000) >> 14);
					color |= (0x10 * vram);
					flipx = 0;
					flipy = 0;
					sx = (offs % 64);
					sy = (offs / 64);

					drawgfx(fromanc2_tmpbitmap[vram][0], Machine->gfx[0],
						tile,
						color,
						flipx, flipy,
						(8 * sx), (8 * sy),
						0, TRANSPARENCY_NONE, 0);
				}
			}
	}
#endif

#if 1
	for (vram = 0; vram < 2; vram++)
	{
			for (offs = 0; offs < (0x2000 / sizeof(short)); offs++)
			{
				// 3RD Layer
				if (fromanc2_dirty[0][1 + (vram * 2)][offs + 0x1000])
				{
					int tile;
					int color;
					int sx, sy;
					int flipx, flipy;

					fromanc2_dirty[0][1 + (vram * 2)][offs + 0x1000] = 0;

					tile  = ((fromanc2_videoram[0][1 + (vram * 2)][offs + 0x1000] & 0x3fff) | (fromanc2_gfxbank[vram][1] << 14));
					color = ((fromanc2_videoram[0][1 + (vram * 2)][offs + 0x1000] & 0xc000) >> 14);
					color |= (0x10 * vram);
					flipx = 0;
					flipy = 0;
					sx = (offs % 64);
					sy = (offs / 64);

					drawgfx(fromanc2_tmpbitmap[vram][1], Machine->gfx[1],
						tile,
						color,
						flipx, flipy,
						(8 * sx), (8 * sy),
						0, TRANSPARENCY_NONE, 0);
				}
			}
	}
#endif

#if 1
	for (vram = 0; vram < 2; vram++)
	{
			for (offs = 0; offs < (0x2000 / sizeof(short)); offs++)
			{
				// 2ND Layer
				if (fromanc2_dirty[0][vram * 2][offs])
				{
					int tile;
					int color;
					int sx, sy;
					int flipx, flipy;

					fromanc2_dirty[0][vram * 2][offs] = 0;

					tile  = ((fromanc2_videoram[0][vram * 2][offs] & 0x3fff) | (fromanc2_gfxbank[vram][2] << 14));;
					color = ((fromanc2_videoram[0][vram * 2][offs] & 0xc000) >> 14);
					color |= (0x10 * vram);
					flipx = 0;
					flipy = 0;
					sx = (offs % 64);
					sy = (offs / 64);

					drawgfx(fromanc2_tmpbitmap[vram][2], Machine->gfx[2],
						tile,
						color,
						flipx, flipy,
						(8 * sx), (8 * sy),
						0, TRANSPARENCY_NONE, 0);
				}
			}
	}
#endif

#if 1
	for (vram = 0; vram < 2; vram++)
	{
			for (offs = 0; offs < (0x2000 / sizeof(short)); offs++)
			{
				// 1ST (TOP) Layer
				if (fromanc2_dirty[0][vram * 2][offs + 0x1000])
				{
					int tile;
					int color;
					int sx, sy;
					int flipx, flipy;

					fromanc2_dirty[0][vram * 2][offs + 0x1000] = 0;

					tile  = ((fromanc2_videoram[0][vram * 2][offs + 0x1000] & 0x3fff) | (fromanc2_gfxbank[vram][3] << 14));
					color = ((fromanc2_videoram[0][vram * 2][offs + 0x1000] & 0xc000) >> 14);
					color |= (0x10 * vram);
					flipx = 0;
					flipy = 0;
					sx = (offs % 64);
					sy = (offs / 64);

					drawgfx(fromanc2_tmpbitmap[vram][3], Machine->gfx[3],
						tile,
						color,
						flipx, flipy,
						(8 * sx), (8 * sy),
						0, TRANSPARENCY_NONE, 0);
				}
			}
	}
#endif

	copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][0], 1, &fromanc2_scrollx[0][0],  1, &fromanc2_scrolly[0][0], 0, TRANSPARENCY_NONE, 0);
	if (!keyboard_pressed(KEYCODE_DEL))  copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][1], 1, &fromanc2_scrollx[0][1],  1, &fromanc2_scrolly[0][1], 0, TRANSPARENCY_COLOR, 0x200);
	if (!keyboard_pressed(KEYCODE_END))  copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][2], 1, &fromanc2_scrollx[0][2],  1, &fromanc2_scrolly[0][2], 0, TRANSPARENCY_COLOR, 0x400);
	if (!keyboard_pressed(KEYCODE_PGDN)) copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][3], 1, &fromanc2_scrollx[0][3],  1, &fromanc2_scrolly[0][3], 0, TRANSPARENCY_COLOR, 0x600);

	copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][0], 1, &fromanc2_scrollx[1][0],  1, &fromanc2_scrolly[1][0], 0, TRANSPARENCY_NONE, 0);
	if (!keyboard_pressed(KEYCODE_DEL))  copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][1], 1, &fromanc2_scrollx[1][1],  1, &fromanc2_scrolly[1][1], 0, TRANSPARENCY_COLOR, 0x300);
	if (!keyboard_pressed(KEYCODE_END))  copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][2], 1, &fromanc2_scrollx[1][2],  1, &fromanc2_scrolly[1][2], 0, TRANSPARENCY_COLOR, 0x500);
	if (!keyboard_pressed(KEYCODE_PGDN)) copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][3], 1, &fromanc2_scrollx[1][3],  1, &fromanc2_scrolly[1][3], 0, TRANSPARENCY_COLOR, 0x700);

	copybitmap(bitmap, fromanc2_tmpbitmapx[0], 0, 0,   0, 0, 0, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, fromanc2_tmpbitmapx[1], 0, 0, 352, 0, 0, TRANSPARENCY_NONE, 0);
}

void fromancr_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int vram;
	int layer;
	int offs;

	if (full_refresh)
	{
		memset(fromanc2_dirty[0][0], 1, 0x2000);
		memset(fromanc2_dirty[0][1], 1, 0x2000);
		memset(fromanc2_dirty[0][2], 1, 0x2000);
		memset(fromanc2_dirty[1][0], 1, 0x2000);
		memset(fromanc2_dirty[1][1], 1, 0x2000);
		memset(fromanc2_dirty[1][2], 1, 0x2000);
	}

	for (vram = 0; vram < 2; vram++)
	{
		for (layer = 0; layer < 3; layer++)
		{
			for (offs = 0; offs < (0x2000 / sizeof(short)); offs++)
			{
				if (fromanc2_dirty[vram][layer][offs])
				{
					int tile;
					int color;
					int sx, sy;
					int flipx, flipy;

					fromanc2_dirty[vram][layer][offs] = 0;

					tile  = (fromanc2_videoram[vram][layer][offs] | (fromanc2_gfxbank[vram][layer] << 16));
					color = vram;
					flipx = 0;
					flipy = 0;
					sx = (offs % 64);
					sy = (offs / 64);

					drawgfx(fromanc2_tmpbitmap[vram][layer], Machine->gfx[layer],
						tile,
						color,
						flipx, flipy,
						(8 * sx), (8 * sy),
						0, TRANSPARENCY_NONE, 0);
				}
			}
		}
	}

	copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][0], 1, &fromanc2_scrollx[0][0],  1, &fromanc2_scrolly[0][0], 0, TRANSPARENCY_NONE, 0);
	copyscrollbitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][1], 1, &fromanc2_scrollx[0][1],  1, &fromanc2_scrolly[0][1], 0, TRANSPARENCY_COLOR, 0x2ff);
	copybitmap(fromanc2_tmpbitmapx[0], fromanc2_tmpbitmap[0][2], 0, 0, 0, 0, 0, TRANSPARENCY_COLOR, 0x0ff);

	copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][0], 1, &fromanc2_scrollx[1][0],  1, &fromanc2_scrolly[1][0], 0, TRANSPARENCY_NONE, 0);
	copyscrollbitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][1], 1, &fromanc2_scrollx[1][1],  1, &fromanc2_scrolly[1][1], 0, TRANSPARENCY_COLOR, 0x3ff);
	copybitmap(fromanc2_tmpbitmapx[1], fromanc2_tmpbitmap[1][2], 0, 0, 0, 0, 0, TRANSPARENCY_COLOR, 0x1ff);

	copybitmap(bitmap, fromanc2_tmpbitmapx[0], 0, 0,   0, 0, 0, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, fromanc2_tmpbitmapx[1], 0, 0, 352, 0, 0, TRANSPARENCY_NONE, 0);
}
