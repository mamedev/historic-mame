/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define BITMAP_WIDTH		256
#define BITMAP_HEIGHT		256

static UINT8 *main_bitmap;
static UINT8 *videoram_lookup;
static UINT8 *converted_gfx;


void arabian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	/* this should be very close */
	for (i = 0;i < Machine->drv->total_colors/2;i++)
	{
		int on = (i & 0x08) ? 0x80 : 0xff;

		*(palette++) = (i & 0x04) ? 0xff : 0;
		*(palette++) = (i & 0x02) ? on : 0;
		*(palette++) = (i & 0x01) ? on : 0;
	}


	/* this is handmade to match the screen shot */

	*(palette++) = 0x00;
	*(palette++) = 0x00;	  // 0000
	*(palette++) = 0x00;

	*(palette++) = 0x00;
	*(palette++) = 0xff;  // 0001
	*(palette++) = 0x00;

	*(palette++) = 0x00;
	*(palette++) = 0xff;  // 0010
	*(palette++) = 0x00;

	*(palette++) = 0x00;
	*(palette++) = 0xff;  // 0011
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0x00;  // 0100
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 0101
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 0110
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 0111
	*(palette++) = 0x00;



	*(palette++) = 0x00;
	*(palette++) = 0x00;  // 1000
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 1001
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0x80;  // 1010
	*(palette++) = 0x00;

	*(palette++) = 0x00;
	*(palette++) = 0xff;  // 1011
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0x00;  // 1100
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 1101
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0x80;  // 1110
	*(palette++) = 0x00;

	*(palette++) = 0xff;
	*(palette++) = 0xff;  // 1111
	*(palette++) = 0x00;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

int arabian_vh_start(void)
{
	UINT8 *gfxbase = memory_region(REGION_GFX1);
	int offs;

	/* allocate a common bitmap to use for both planes */
	/* plane 0 (top plane with sprites) is in the upper 4 bits */
	/* plane 1 (bottom plane with background) is in the lower 4 bits */
	if ((main_bitmap = malloc(BITMAP_WIDTH * BITMAP_HEIGHT)) == 0)
		return 1;

	/* allocate memory for the videoram lookup table */
	if ((videoram_lookup = malloc(256*4)) == 0)
	{
		free(main_bitmap);
		return 1;
	}

	/* allocate memory for the converted graphics data */
	if ((converted_gfx = malloc(0x8000 * 2)) == 0)
	{
		free(videoram_lookup);
		free(main_bitmap);
		return 1;
	}


	/*--------------------------------------------------
		transform graphics data into more usable format
		which is coded like this:

		  byte adr+0x4000  byte adr
		  DCBA DCBA        DCBA DCBA

		D-bits of pixel 4
		C-bits of pixel 3
		B-bits of pixel 2
		A-bits of pixel 1

		after conversion :

		  byte adr+0x4000  byte adr
		  DDDD CCCC        BBBB AAAA
	--------------------------------------------------*/

	for (offs = 0; offs < 0x4000; offs++)
	{
		int p1,p2,p3,p4,v1,v2;

		v1 = gfxbase[offs + 0x0000];
		v2 = gfxbase[offs + 0x4000];

		p1 = (v1 & 0x01) | ((v1 & 0x10) >> 3) | ((v2 & 0x01) << 2) | ((v2 & 0x10) >> 1);
		v1 = v1 >> 1;
		v2 = v2 >> 1;
		p2 = (v1 & 0x01) | ((v1 & 0x10) >> 3) | ((v2 & 0x01) << 2) | ((v2 & 0x10) >> 1);
		v1 = v1 >> 1;
		v2 = v2 >> 1;
		p3 = (v1 & 0x01) | ((v1 & 0x10) >> 3) | ((v2 & 0x01) << 2) | ((v2 & 0x10) >> 1);
		v1 = v1 >> 1;
		v2 = v2 >> 1;
		p4 = (v1 & 0x01) | ((v1 & 0x10) >> 3) | ((v2 & 0x01) << 2) | ((v2 & 0x10) >> 1);

		converted_gfx[offs * 4 + 3] = p1;
		converted_gfx[offs * 4 + 2] = p2;
		converted_gfx[offs * 4 + 1] = p3;
		converted_gfx[offs * 4 + 0] = p4;
	}

	/* generate the videoram lookup table */
	/* this takes a single byte written as DCBAdcba and converts it into 4 bytes */
	/* that look like this: */
	/* 		DdDdDdDd CcCcCcCc BbBbBbBb AaAaAaAa */
	for (offs = 0; offs < 256; offs++)
	{
		int temp;

		temp = ((offs & 0x10) >> 3) | ((offs & 0x01) >> 0);
		videoram_lookup[offs * 4 + 0] = (temp << 6) | (temp << 4) | (temp << 2) | temp;

		temp = ((offs & 0x20) >> 4) | ((offs & 0x02) >> 1);
		videoram_lookup[offs * 4 + 1] = (temp << 6) | (temp << 4) | (temp << 2) | temp;

		temp = ((offs & 0x40) >> 5) | ((offs & 0x04) >> 2);
		videoram_lookup[offs * 4 + 2] = (temp << 6) | (temp << 4) | (temp << 2) | temp;

		temp = ((offs & 0x80) >> 6) | ((offs & 0x08) >> 3);
		videoram_lookup[offs * 4 + 3] = (temp << 6) | (temp << 4) | (temp << 2) | temp;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void arabian_vh_stop(void)
{
	free(converted_gfx);
	free(videoram_lookup);
	free(main_bitmap);
}



static void blit_area(UINT8 plane, UINT16 src, UINT8 x, UINT8 y, UINT8 sx, UINT8 sy)
{
	UINT8 *srcdata = &converted_gfx[src * 4];
	int i,j;

	/* mark the whole area dirty */
	mark_dirty(x, y, x + sx * 4 + 3, y + sy);

	/* loop over X, then Y */
	for (i = 0; i <= sx; i++, x += 4)
		for (j = 0; j <= sy; j++)
		{
			UINT8 p1 = *srcdata++;
			UINT8 p2 = *srcdata++;
			UINT8 p3 = *srcdata++;
			UINT8 p4 = *srcdata++;
			UINT8 *base;

			/* get a pointer to the bitmap */
			base = &main_bitmap[(y + j) * BITMAP_WIDTH + x];

			/* bit 0 means write to upper plane (upper 4 bits of our bitmap) */
			if (plane & 0x01)
			{
				if (p4 != 8) base[0] = (base[0] & ~0xf0) | (p4 << 4);
				if (p3 != 8) base[1] = (base[1] & ~0xf0) | (p3 << 4);
				if (p2 != 8) base[2] = (base[2] & ~0xf0) | (p2 << 4);
				if (p1 != 8) base[3] = (base[3] & ~0xf0) | (p1 << 4);
			}

			/* bit 2 means write to lower plane (lower 4 bits of our bitmap) */
			if (plane & 0x04)
			{
				if (p4 != 8) base[0] = (base[0] & ~0x0f) | p4;
				if (p3 != 8) base[1] = (base[1] & ~0x0f) | p3;
				if (p2 != 8) base[2] = (base[2] & ~0x0f) | p2;
				if (p1 != 8) base[3] = (base[3] & ~0x0f) | p1;
			}
		}
}


WRITE_HANDLER( arabian_blitter_w )
{
	/* write the data */
	spriteram[offset] = data;

	/* watch for a write to offset 6 -- that triggers the blit */
	if ((offset & 0x07) == 6)
	{
		UINT8 plane, x, y, sx, sy;
		UINT16 src;

		/* extract the data */
		plane = spriteram[offset-6];
		src   = spriteram[offset-5] | (spriteram[offset-4] << 8);
		x     = spriteram[offset-2] << 2;
		y     = spriteram[offset-3];
		sx    = spriteram[offset-0];
		sy    = spriteram[offset-1];

		/* blit it */
		blit_area(plane,src,x,y,sx,sy);
	}
}



WRITE_HANDLER( arabian_videoram_w )
{
	static const UINT32 mask_table[16] =
	{
		0x00000000, 0xc0c0c0c0, 0x30303030, 0xf0f0f0f0,
		0x0c0c0c0c, 0xcccccccc, 0x3c3c3c3c, 0xfcfcfcfc,
		0x03030303, 0xc3c3c3c3, 0x33333333, 0xf3f3f3f3,
		0x0f0f0f0f, 0xcfcfcfcf, 0x3f3f3f3f, 0xffffffff
	};
	UINT32 *base, lookup, mask;
	UINT8 x,y;

	/* determine X/Y and mark the area dirty */
	x = (offset >> 8) << 2;
	y = offset & 0xff;
	mark_dirty(x, y, x + 3, y);

	/* the data is written as DCBAdcba; the lookup expands it into 4 entries: */
	/* 		DdDdDdDd CcCcCcCc BbBbBbBb AaAaAaAa */
	/* we fetch these 32 bits at a time to operate on the entire 4 pixels at once */
	lookup = *(UINT32 *)&videoram_lookup[data * 4];

	/* the mask comes from the blitter RAM and describes which planes have data */
	/* written into them; endianness doesn't matter here */
	mask = mask_table[spriteram[0] & 0x0f];

	/* get a pointer to our bitmap as a 32-bit pointer*/
	base = (UINT32 *)&main_bitmap[y * BITMAP_WIDTH + x];

	/* now blend */
	*base = (*base & ~mask) | (lookup & mask);
}


void arabian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	UINT32 pens[256];
	int y;

	/* build the combined pens table; this handles transparency of the upper plane */
	for (y = 0; y < 256; y++)
		pens[y] = Machine->pens[(y & 0xf0) ? (y >> 4) : 16 + (y & 0x0f)];

	/* render the screen from the bitmap */
	for (y = 0; y < BITMAP_HEIGHT; y++)
		draw_scanline8(bitmap, 0, y, 256, &main_bitmap[y * BITMAP_WIDTH], pens, -1);
}
