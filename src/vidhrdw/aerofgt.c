#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *aerofgt_rasterram;
unsigned char *aerofgt_bg1videoram,*aerofgt_bg2videoram;
int aerofgt_bg1videoram_size,aerofgt_bg2videoram_size;

static unsigned char gfxbank[8];
static unsigned char bg1scrolly[2],bg2scrolly[2];

static struct osd_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int aerofgt_vh_start(void)
{
	int i;

	if ((dirtybuffer = malloc(aerofgt_bg1videoram_size / 2)) == 0)
	{
		return 1;
	}
	memset(dirtybuffer,1,aerofgt_bg1videoram_size / 2);

	if ((tmpbitmap = osd_create_bitmap(512,512)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	if ((dirtybuffer2 = malloc(aerofgt_bg2videoram_size / 2)) == 0)
	{
		free(dirtybuffer);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}
	memset(dirtybuffer2,1,aerofgt_bg2videoram_size / 2);

	if ((tmpbitmap2 = osd_create_bitmap(512,512)) == 0)
	{
		free(dirtybuffer);
		osd_free_bitmap(tmpbitmap);
		free(dirtybuffer2);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void aerofgt_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer2);
	osd_free_bitmap (tmpbitmap2);
}




int aerofgt_bg1videoram_r(int offset)
{
   return READ_WORD(&aerofgt_bg1videoram[offset]);
}

int aerofgt_bg2videoram_r(int offset)
{
   return READ_WORD(&aerofgt_bg2videoram[offset]);
}

void aerofgt_bg1videoram_w(int offset,int data)
{
	int oldword = READ_WORD(&aerofgt_bg1videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&aerofgt_bg1videoram[offset],newword);
		dirtybuffer[offset / 2] = 1;
	}
}

void aerofgt_bg2videoram_w(int offset,int data)
{
	int oldword = READ_WORD(&aerofgt_bg2videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&aerofgt_bg2videoram[offset],newword);
		dirtybuffer2[offset / 2] = 1;
	}
}


void aerofgt_gfxbank_w(int offset,int data)
{
	int oldword = READ_WORD(&gfxbank[offset]);
	int newword;


	/* straighten out the 16-bit word into bytes for conveniency */
	#ifdef LSB_FIRST
	data = ((data & 0x00ff00ff) << 8) | ((data & 0xff00ff00) >> 8);
	#endif

	newword = COMBINE_WORD(oldword,data);
	if (oldword != newword)
	{
		WRITE_WORD(&gfxbank[offset],newword);
		memset(dirtybuffer,1,aerofgt_bg1videoram_size / 2);
		memset(dirtybuffer2,1,aerofgt_bg2videoram_size / 2);
	}
}

void aerofgt_bg1scrolly_w(int offset,int data)
{
	COMBINE_WORD_MEM(bg1scrolly,data);
}

void aerofgt_bg2scrolly_w(int offset,int data)
{
	COMBINE_WORD_MEM(bg2scrolly,data);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void dopalette(void)
{
	int offs;
	int color,code,i;
	int colmask[32];
	int pal_base;


	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 32;color++) colmask[color] = 0;

	for (offs = aerofgt_bg1videoram_size - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&aerofgt_bg1videoram[offs]);
		color = ((code & 0xe000) >> 13);
		code = (code & 0x07ff) + (gfxbank[0 + ((code & 0x1800) >> 11)] << 11);
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (offs = aerofgt_bg2videoram_size - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&aerofgt_bg2videoram[offs]);
		color = ((code & 0xe000) >> 13) + 16;
		code = (code & 0x07ff) + (gfxbank[4 + ((code & 0x1800) >> 11)] << 11),
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 32;color++)
	{
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
		/* bg1 uses colors 0-7 and is not transparent */
		/* bg2 uses colors 16-23 and is transparent */
		if (color >= 16 && (colmask[color] & (1 << 15)))
			palette_used_colors[pal_base + 16 * color + 15] = PALETTE_COLOR_TRANSPARENT;
	}


	for (color = 0;color < 32;color++) colmask[color] = 0;

	offs = 0;
	while (offs < 0x0800 && (READ_WORD(&spriteram_2[offs]) & 0x8000) == 0)
	{
		int attr_start,map_start;

		attr_start = 8 * (READ_WORD(&spriteram_2[offs]) & 0x03ff);

		color = (READ_WORD(&spriteram_2[attr_start + 4]) & 0x0f00) >> 8;
		map_start = 2 * (READ_WORD(&spriteram_2[attr_start + 6]) & 0x3fff);
		if (map_start >= 0x4000) color += 16;

		colmask[color] |= 0xffff;

		offs += 2;
	}

	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}
	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color+16] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	if (palette_recalc())
	{
		memset(dirtybuffer,1,aerofgt_bg1videoram_size / 2);
		memset(dirtybuffer2,1,aerofgt_bg2videoram_size / 2);
	}
}


static void drawsprites(struct osd_bitmap *bitmap,int priority)
{
	int offs;


	priority <<= 12;

	offs = 0;
	while (offs < 0x0800 && (READ_WORD(&spriteram_2[offs]) & 0x8000) == 0)
	{
		int attr_start;

		attr_start = 8 * (READ_WORD(&spriteram_2[offs]) & 0x03ff);

		/* is the way I handle priority correct? Or should I just check bit 13? */
		if ((READ_WORD(&spriteram_2[attr_start + 4]) & 0x3000) == priority)
		{
			int map_start;
			int ox,oy,x,y,xsize,ysize,flipx,flipy,color;

			ox = READ_WORD(&spriteram_2[attr_start + 2]) & 0x01ff;
			xsize = (READ_WORD(&spriteram_2[attr_start + 2]) & 0x0e00) >> 9;
			oy = READ_WORD(&spriteram_2[attr_start + 0]) & 0x01ff;
			ysize = (READ_WORD(&spriteram_2[attr_start + 0]) & 0x0e00) >> 9;
			flipx = READ_WORD(&spriteram_2[attr_start + 4]) & 0x4000;
			flipy = READ_WORD(&spriteram_2[attr_start + 4]) & 0x8000;
			color = (READ_WORD(&spriteram_2[attr_start + 4]) & 0x0f00) >> 8;
			map_start = 2 * (READ_WORD(&spriteram_2[attr_start + 6]) & 0x3fff);

			for (y = 0;y <= ysize;y++)
			{
				int sx,sy;

				if (flipy) sy = ((oy + 16 * (ysize - y) + 16) & 0x1ff) - 16;
				else sy = ((oy + 16 * y + 16) & 0x1ff) - 16;

				for (x = 0;x <= xsize;x++)
				{
					int code;

					if (flipx) sx = ((ox + 16 * (xsize - x) + 16) & 0x1ff) - 16;
					else sx = ((ox + 16 * x + 16) & 0x1ff) - 16;

					code = READ_WORD(&spriteram[map_start]) & 0x1fff;
					drawgfx(bitmap,Machine->gfx[map_start >= 0x4000 ? 2 : 1],
							code,
							color,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
					map_start += 2;
				}
			}
		}

		offs += 2;
	}
}


void aerofgt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	dopalette();

	for (offs = aerofgt_bg1videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs/2])
		{
			int sx,sy,code;


			dirtybuffer[offs/2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			code = READ_WORD(&aerofgt_bg1videoram[offs]);
			drawgfx(tmpbitmap,Machine->gfx[0],
					(code & 0x07ff) + (gfxbank[0 + ((code & 0x1800) >> 11)] << 11),
					(code & 0xe000) >> 13,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = aerofgt_bg2videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer2[offs/2])
		{
			int sx,sy,code;


			dirtybuffer2[offs/2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			code = READ_WORD(&aerofgt_bg2videoram[offs]);
			drawgfx(tmpbitmap2,Machine->gfx[0],
					(code & 0x07ff) + (gfxbank[4 + ((code & 0x1800) >> 11)] << 11),
					((code & 0xe000) >> 13) + 16,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;

		scrollx = -READ_WORD(&aerofgt_rasterram[0x0000])+18;
		scrolly = -READ_WORD(bg1scrolly);
		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	drawsprites(bitmap,0);
	drawsprites(bitmap,1);

	{
		int scrollx,scrolly;

		scrollx = -READ_WORD(&aerofgt_rasterram[0x0400])+20;
		scrolly = -READ_WORD(bg2scrolly);
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	drawsprites(bitmap,2);
	drawsprites(bitmap,3);
}
