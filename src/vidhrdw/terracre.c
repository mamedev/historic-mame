/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


data16_t *terrac_videoram2;
size_t terrac_videoram2_size;
data16_t *terrac_scrolly;

static struct osd_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;

static const unsigned char *spritepalettebank;


/***************************************************************************
  Convert color prom.
***************************************************************************/

void terrac_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* characters use colors 0-15 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* background tiles use colors 192-255 in four banks */
	/* the bottom two bits of the color code select the palette bank for */
	/* pens 0-7; the top two bits for pens 8-15. */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		if (i & 8) COLOR(1,i) = 192 + (i & 0x0f) + ((i & 0xc0) >> 2);
		else COLOR(1,i) = 192 + (i & 0x0f) + ((i & 0x30) >> 0);
	}

	/* sprites use colors 128-191 in four banks */
	/* The lookup table tells which colors to pick from the selected bank */
	/* the bank is selected by another PROM and depends on the top 8 bits of */
	/* the sprite code. The PROM selects the bank *separately* for pens 0-7 and */
	/* 8-15 (like for tiles). */
	for (i = 0;i < TOTAL_COLORS(2)/16;i++)
	{
		int j;

		for (j = 0;j < 16;j++)
		{
			if (i & 8)
				COLOR(2,i + j * (TOTAL_COLORS(2)/16)) = 128 + ((j & 0x0c) << 2) + (*color_prom & 0x0f);
			else
				COLOR(2,i + j * (TOTAL_COLORS(2)/16)) = 128 + ((j & 0x03) << 4) + (*color_prom & 0x0f);
		}

		color_prom++;
	}

	/* color_prom now points to the beginning of the sprite palette bank table */
	spritepalettebank = color_prom;	/* we'll need it at run time */
}



WRITE16_HANDLER( terrac_videoram2_w )
{
	int oldword = terrac_videoram2[offset];
	COMBINE_DATA(&terrac_videoram2[offset]);
	if (oldword != terrac_videoram2[offset])
	{
		dirtybuffer2[offset] = 1;
	}
}

READ16_HANDLER( terrac_videoram2_r )
{
   return terrac_videoram2[offset];
}


/***************************************************************************
  Stop the video hardware emulation.
***************************************************************************/

void terrac_vh_stop(void)
{
	free(dirtybuffer2);
	bitmap_free(tmpbitmap2);
	generic_vh_stop();
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/


int terrac_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(terrac_videoram2_size/2)) == 0)
	{
		terrac_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,terrac_videoram2_size/2);

	/* the background area is 4 x 1 (90 Rotated!) */
	if ((tmpbitmap2 = bitmap_alloc(4*Machine->drv->screen_width,
			1*Machine->drv->screen_height)) == 0)
	{
		terrac_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void terracre_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,x,y;


	for (y = 0; y < 64; y++)
	{
		for (x = 0; x < 16; x++)
		{
			if (dirtybuffer2[x + y*32])
			{
				int code = terrac_videoram2[x + y*32] & 0x01ff;
				int color = (terrac_videoram2[x + y*32]&0x7800)>>11;

				dirtybuffer2[x + y*32] = 0;

				drawgfx(tmpbitmap2,Machine->gfx[1],
						code,
						color,
						0,0,
						16 * y,16 * x,
						0,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* copy the background graphics */
	if (*terrac_scrolly & 0x2000)	/* background disable */
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	else
	{
		int scrollx;

		scrollx = -*terrac_scrolly;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}



	for (x = 0;x <spriteram_size/2;x += 4)
	{
		int code;
		int attr = spriteram16[x+2] & 0xff;
		int color = (attr & 0xf0) >> 4;
		int flipx = attr & 0x04;
		int flipy = attr & 0x08;
		int sx,sy;

		sx = (spriteram16[x+3] & 0xff) - 0x80 + 256 * (attr & 1);
		sy = 240 - (spriteram16[x] & 0xff);

		code = (spriteram16[x+1] & 0xff) + ((attr & 0x02) << 7);

		drawgfx(bitmap,Machine->gfx[2],
				code,
				color + 16 * (spritepalettebank[code >> 1] & 0x0f),
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}


	for (offs = videoram_size/2 - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs / 32;
		sy = offs % 32;

		drawgfx(bitmap,Machine->gfx[0],
				videoram16[offs] & 0xff,
				0,
				0,0,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}
}
