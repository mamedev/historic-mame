/***************************************************************************

   Caveman Ninja Video emulation - Bryan McPhail, mish@tendril.force9.net

****************************************************************************

Data East custom chip 55:  Generates two playfields, playfield 1 is underneath
playfield 2.  Caveman Ninja uses two of these chips.

	16 bytes of control registers per chip.

	Word 0:
		Mask 0x0080: Flip screen
		Mask 0x007f: ?
	Word 2:
		Mask 0xffff: Playfield 2 X scroll (top playfield)
	Word 4:
		Mask 0xffff: Playfield 2 Y scroll (top playfield)
	Word 6:
		Mask 0xffff: Playfield 1 X scroll (bottom playfield)
	Word 8:
		Mask 0xffff: Playfield 1 Y scroll (bottom playfield)
	Word 0xa:
		Mask 0xc000: Playfield 1 shape??
		Mask 0x3000: Playfield 1 rowscroll style (maybe mask 0x3800??)
		Mask 0x0300: Playfield 1 colscroll style (maybe mask 0x0700??)?

		Mask 0x00c0: Playfield 2 shape??
		Mask 0x0030: Playfield 2 rowscroll style (maybe mask 0x0038??)
		Mask 0x0003: Playfield 2 colscroll style (maybe mask 0x0007??)?
	Word 0xc:
		Mask 0x8000: Playfield 1 is 8*8 tiles else 16*16
		Mask 0x4000: Playfield 1 rowscroll enabled
		Mask 0x2000: Playfield 1 colscroll enabled
		Mask 0x1f00: ?

		Mask 0x0080: Playfield 2 is 8*8 tiles else 16*16
		Mask 0x0040: Playfield 2 rowscroll enabled
		Mask 0x0020: Playfield 2 colscroll enabled
		Mask 0x001f: ?
	Word 0xe:
		??

Colscroll style:
	0	64 columns across bitmap
	1	32 columns across bitmap

Rowscroll style:
	0	512 rows across bitmap


Locations 0 & 0xe are mostly unknown:

							 0		14
Caveman Ninja (bottom):		0053	1100 (see below)
Caveman Ninja (top):		0010	0081
Two Crude (bottom):			0053	0000
Two Crude (top):			0010	0041
Dark Seal (bottom):			0010	0000
Dark Seal (top):			0053	4101
Tumblepop:					0010	0000
Super Burger Time:			0010	0000

Location 14 in Cninja (bottom):
 1100 = pf2 uses graphics rom 1, pf3 uses graphics rom 2
 0000 = pf2 uses graphics rom 2, pf3 uses graphics rom 2
 1111 = pf2 uses graphics rom 1, pf3 uses graphics rom 1

**************************************************************************

Sprites - Data East custom chip 52

	8 bytes per sprite, unknowns bits seem unused.

	Word 0:
		Mask 0x8000 - ?
		Mask 0x4000 - Y flip
		Mask 0x2000 - X flip
		Mask 0x1000 - Sprite flash
		Mask 0x0800 - ?
		Mask 0x0600 - Sprite height (1x, 2x, 4x, 8x)
		Mask 0x01ff - Y coordinate

	Word 2:
		Mask 0xffff - Sprite number

	Word 4:
		Mask 0x8000 - ?
		Mask 0x4000 - Sprite is drawn beneath top 8 pens of playfield 4
		Mask 0x3e00 - Colour (32 palettes, most games only use 16)
		Mask 0x01ff - X coordinate

	Word 6:
		Always unused.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define PRINT_PF_ATTRIBUTES
#define TILERAM_SIZE	0x1000

unsigned char *cninja_pf1_data,*cninja_pf2_data;
unsigned char *cninja_pf3_data,*cninja_pf4_data;
unsigned char *cninja_pf1_rowscroll,*cninja_pf2_rowscroll;
unsigned char *cninja_pf3_rowscroll,*cninja_pf4_rowscroll;
static unsigned char *cninja_pf1_dirty,*cninja_pf3_dirty;
static unsigned char *cninja_pf2_dirty,*cninja_pf4_dirty;

static struct osd_bitmap *cninja_pf1_bitmap;
static struct osd_bitmap *cninja_pf2_bitmap;
static struct osd_bitmap *cninja_pf3_bitmap;
static struct osd_bitmap *cninja_pf4_bitmap;
static struct osd_bitmap *cninja_tf4_bitmap;

static unsigned char cninja_control_0[16];
static unsigned char cninja_control_1[16];

static int cninja_pf2_bank,cninja_pf3_bank;
static int offsetx[4],offsety[4];
static int bootleg;

/******************************************************************************/

static void update_24bitcol(int offset)
{
	int r,g,b;

	if (offset%4) offset-=2;

	b = (READ_WORD(&paletteram[offset]) >> 0) & 0xff;
	g = (READ_WORD(&paletteram[offset+2]) >> 8) & 0xff;
	r = (READ_WORD(&paletteram[offset+2]) >> 0) & 0xff;

	palette_change_color(offset / 4,r,g,b);
}

void cninja_palette_24bit_w(int offset,int data)
{
	COMBINE_WORD_MEM(&paletteram[offset],data);
	update_24bitcol(offset);
}

/******************************************************************************/

static void cninja_update_palette(void)
{
	int offs,color,code,i,pal_base;
	int colmask[16];
    unsigned int *pen_usage; /* Save some struct derefs */

	palette_init_used_colors();

	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	pen_usage=Machine->gfx[0]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < TILERAM_SIZE;offs += 2)
	{
		code = READ_WORD(&cninja_pf1_data[offs]);
		color = code >> 12;
		code &= 0x0fff;
		colmask[color] |= pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* If two playfields are using same bank we have to combine palette code */
	if (cninja_pf2_bank==1 && cninja_pf3_bank==1) {
		pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
		pen_usage=Machine->gfx[1]->pen_usage;
		for (color = 0;color < 16;color++) colmask[color] = 0;
		for (offs = 0; offs < TILERAM_SIZE;offs += 2)
		{
			code = READ_WORD(&cninja_pf2_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
			code = READ_WORD(&cninja_pf3_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
		}

		for (color = 0;color < 16;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}
	else if (cninja_pf2_bank==2 && cninja_pf3_bank==2) {
		pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
		pen_usage=Machine->gfx[2]->pen_usage;
		for (color = 0;color < 16;color++) colmask[color] = 0;
		for (offs = 0; offs < TILERAM_SIZE;offs += 2)
		{
			code = READ_WORD(&cninja_pf2_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
			code = READ_WORD(&cninja_pf3_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
		}

		for (color = 0;color < 16;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}
	else {
		pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
		pen_usage=Machine->gfx[1]->pen_usage;
		for (color = 0;color < 16;color++) colmask[color] = 0;
		for (offs = 0; offs < TILERAM_SIZE;offs += 2)
		{
			code = READ_WORD(&cninja_pf2_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
		}

		for (color = 0;color < 16;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
			}
		}

		pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
		pen_usage=Machine->gfx[2]->pen_usage;
		for (color = 0;color < 16;color++) colmask[color] = 0;
		for (offs = 0; offs < TILERAM_SIZE;offs += 2)
		{
			code = READ_WORD(&cninja_pf3_data[offs]);
			color = code >> 12;
			code &= 0x0fff;
			colmask[color] |= pen_usage[code];
		}

		for (color = 0;color < 16;color++)
		{
			if (colmask[color] & (1 << 0))
				palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
			for (i = 1;i < 16;i++)
			{
				if (colmask[color] & (1 << i))
					palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
			}
		}
	}

	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	pen_usage=Machine->gfx[3]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < TILERAM_SIZE;offs += 2)
	{
		code = READ_WORD(&cninja_pf4_data[offs]);
		color = code >> 12;
		code &= 0x0fff;
		colmask[color] |= pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[4].color_codes_start;
	pen_usage=Machine->gfx[4]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,multi;

		sprite = READ_WORD (&spriteram[offs+2]) & 0x3fff;
		if (!sprite) continue;

		x = READ_WORD(&spriteram[offs+4]);
		y = READ_WORD(&spriteram[offs]);

		color = (x >> 9) &0xf;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;
		sprite &= ~multi;

		/* Save palette by missing offscreen sprites */
		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		x = 240 - x;
		if (x>256) continue;

		while (multi >= 0)
		{
			colmask[color] |= pen_usage[sprite + multi];
			multi--;
		}
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		memset(cninja_pf1_dirty,1,TILERAM_SIZE);
		memset(cninja_pf2_dirty,1,TILERAM_SIZE);
		memset(cninja_pf3_dirty,1,TILERAM_SIZE);
		memset(cninja_pf4_dirty,1,TILERAM_SIZE);
	}
}

static void cninja_drawsprites(struct osd_bitmap *bitmap, int pri)
{
	int offs;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash;
		sprite = READ_WORD (&spriteram[offs+2]) & 0x3fff;
		if (!sprite) continue;

		x = READ_WORD(&spriteram[offs+4]);

		/* Sprite/playfield priority */
		if ((x&0x4000) && pri==1) continue;
		if (!(x&0x4000) && pri==0) continue;

		y = READ_WORD(&spriteram[offs]);
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;
		colour = (x >> 9) &0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		if (x>256) continue; /* Speedup */

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[4],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y - 16 * multi,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static void cninja_pf2_update(void)
{
	int offs,mx,my,color,tile,quarter;

	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x400 * quarter; offs < 0x400 * quarter + 0x400;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (cninja_pf2_dirty[offs])
			{
				cninja_pf2_dirty[offs] = 0;
				tile = READ_WORD(&cninja_pf2_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(cninja_pf2_bitmap,Machine->gfx[cninja_pf2_bank],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

static void cninja_pf3_update(void)
{
	int offs,mx,my,color,tile,quarter;

	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x400 * quarter; offs < 0x400 * quarter + 0x400;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (cninja_pf3_dirty[offs])
			{
				cninja_pf3_dirty[offs] = 0;
				tile = READ_WORD(&cninja_pf3_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(cninja_pf3_bitmap,Machine->gfx[cninja_pf3_bank],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

static void cninja_pf4_update(void)
{
	int offs,mx,my,color,tile,quarter;

	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x400 * quarter; offs < 0x400 * quarter + 0x400;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (cninja_pf4_dirty[offs])
			{
				cninja_pf4_dirty[offs] = 0;
				tile = READ_WORD(&cninja_pf4_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(cninja_pf4_bitmap,Machine->gfx[3],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
				drawgfx(cninja_tf4_bitmap,Machine->gfx[3],
						0,
						0,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
				drawgfx(cninja_tf4_bitmap,Machine->gfx[3],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_PENS,0xff);
			}
		}
	}
}

static void cninja_pf1_update(void)
{
	int mx,my,offs,tile,color;

	mx = -1;
	my = 0;
	for (offs = 0; offs < 0x1000 ;offs += 2)
	{
		mx++;
		if (mx == 64)
		{
			mx = 0;
			my++;
		}

		if (cninja_pf1_dirty[offs])
		{
			cninja_pf1_dirty[offs] = 0;
			tile = READ_WORD(&cninja_pf1_data[offs]);
			color = (tile & 0xf000) >> 12;

			drawgfx(cninja_pf1_bitmap,Machine->gfx[0],
					tile & 0x0fff,
					color,
					0,0,
					8*mx,8*my,
					0,TRANSPARENCY_NONE,0);
		}
	}
}

/******************************************************************************/

void cninja_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int scrollx,scrolly,offs;
	int pf23_control,pf1_control;

	/* Handle gfx rom switching */
	pf23_control=READ_WORD (&cninja_control_0[0xe]);
	if ((pf23_control&0xff)==0x00)
		cninja_pf3_bank=2;
	else
		cninja_pf3_bank=1;

	if ((pf23_control&0xff00)==0x00)
		cninja_pf2_bank=2;
	else
		cninja_pf2_bank=1;

	/* Draw playfields */
	cninja_update_palette();
	cninja_pf1_update();
	cninja_pf2_update();
	cninja_pf3_update();
	cninja_pf4_update();

	pf23_control=READ_WORD (&cninja_control_0[0xc]);
	pf1_control=READ_WORD (&cninja_control_1[0xc]);

	/* Background */
	scrollx = -READ_WORD (&cninja_control_0[6]);
	scrolly = -READ_WORD (&cninja_control_0[8]);

	/* Rowscroll enable */
	if (pf23_control&0x4000) {
		int rscrollx[512];

		for (offs = 0;offs < 512;offs++)
			rscrollx[offs]=0;

		/* Several different rowscroll styles! */
		switch ((READ_WORD (&cninja_control_0[0xa])>>12)&7) {

			case 2: /* 16 rows (of 16 pixels) */
				for (offs = 0;offs < 16;offs++)
					rscrollx[offs] = scrollx - READ_WORD(&cninja_pf2_rowscroll[offs<<1]);
				copyscrollbitmap(bitmap,cninja_pf2_bitmap,32,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				break;

			case 0: /* 256 rows (doubled because the bitmap is 512 */
				for (offs = 0;offs < 512; offs++)
					rscrollx[offs] = scrollx - READ_WORD(&cninja_pf2_rowscroll[offs<<1]);
				copyscrollbitmap(bitmap,cninja_pf2_bitmap,512,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				break;
		}

	}
	else
		copyscrollbitmap(bitmap,cninja_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Foreground */
	scrollx = -READ_WORD (&cninja_control_0[2]);
	scrolly = -READ_WORD (&cninja_control_0[4]);

	if (pf23_control&0x40) { /* Rowscroll */
		int rscrollx[512];

		/* We have to map 16 screen tiles to the size of the bitmap... */
		for (offs = 0;offs < 32;offs++)
			rscrollx[offs] = scrollx - READ_WORD(&cninja_pf3_rowscroll[2*offs]);
		copyscrollbitmap(bitmap,cninja_pf3_bitmap,32,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	}
	else if (pf23_control&0x20) { /* Colscroll */
		int cscroll[128];

		for (offs=0 ; offs < 64;offs++)
			cscroll[offs] = scrolly ;

		/* Used in lava level & Level 1 */
		for (offs=0 ; offs < 32;offs++)
			cscroll[offs+32] = scrolly - READ_WORD(&cninja_pf3_rowscroll[(2*offs)+0x400]);
		copyscrollbitmap(bitmap,cninja_pf3_bitmap,1,&scrollx,64,cscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else
		copyscrollbitmap(bitmap,cninja_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Foreground */
	scrollx=-READ_WORD (&cninja_control_1[6]);
	scrolly=-READ_WORD (&cninja_control_1[8]);
	if (pf1_control&0x2000) { /* Colscroll */
		int cscroll[64];

		/* Used in first lava level */
		for (offs=0 ; offs < 64;offs++)
			cscroll[offs] = scrolly - READ_WORD(&cninja_pf4_rowscroll[(2*offs)+0x400]);
		copyscrollbitmap(bitmap,cninja_pf4_bitmap,1,&scrollx,64,cscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else
		copyscrollbitmap(bitmap,cninja_pf4_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Sprites - draw sprites between pf4 */
	cninja_drawsprites(bitmap,0);

	/* Foreground - Top pens only */
	if (pf1_control&0x2000) { /* Colscroll */
		int cscroll[64];

		/* Used in first lava level */
		for (offs=0 ; offs < 64;offs++)
			cscroll[offs] = scrolly - READ_WORD(&cninja_pf4_rowscroll[(2*offs)+0x400]);
		copyscrollbitmap(bitmap,cninja_tf4_bitmap,1,&scrollx,64,cscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else
		copyscrollbitmap(bitmap,cninja_tf4_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Sprites - draw sprites in front of pf4 */
	cninja_drawsprites(bitmap,1);

	/* Playfield 1 - 8 * 8 Text */
	scrollx = -READ_WORD(&cninja_control_1[2]);
	scrolly = -READ_WORD(&cninja_control_1[4]);

	if (pf1_control&0x40) { /* Rowscroll */
		int rscrollx[256];

		/* 256 rows */
		for (offs = 0;offs < 256;offs++)
			rscrollx[offs] = scrollx - READ_WORD(&cninja_pf1_rowscroll[offs<<1]);
		copyscrollbitmap(bitmap,cninja_pf1_bitmap,256,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}
	else
		copyscrollbitmap(bitmap,cninja_pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}

/******************************************************************************/

void cninja_pf1_data_w(int offset,int data)
{
	int oldword = READ_WORD(&cninja_pf1_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&cninja_pf1_data[offset],newword);
		cninja_pf1_dirty[offset] = 1;
	}
}

void cninja_pf2_data_w(int offset,int data)
{
	int oldword = READ_WORD(&cninja_pf2_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&cninja_pf2_data[offset],newword);
		cninja_pf2_dirty[offset] = 1;
	}
}

void cninja_pf3_data_w(int offset,int data)
{
	int oldword = READ_WORD(&cninja_pf3_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&cninja_pf3_data[offset],newword);
		cninja_pf3_dirty[offset] = 1;
	}
}

void cninja_pf4_data_w(int offset,int data)
{
	int oldword = READ_WORD(&cninja_pf4_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&cninja_pf4_data[offset],newword);
		cninja_pf4_dirty[offset] = 1;
	}
}

void cninja_control_0_w(int offset,int data)
{
	if (bootleg && offset==6) {
		COMBINE_WORD_MEM(&cninja_control_0[offset],data+0xa);
		return;
	}
	COMBINE_WORD_MEM(&cninja_control_0[offset],data);
}

void cninja_control_1_w(int offset,int data)
{
	if (bootleg) {
		switch (offset) {
			case 2:
				COMBINE_WORD_MEM(&cninja_control_1[offset],data-2);
				return;
			case 6:
				COMBINE_WORD_MEM(&cninja_control_1[offset],data+0xa);
				return;
		}
	}
	COMBINE_WORD_MEM(&cninja_control_1[offset],data);
}

int cninja_pf1_data_r(int offset)
{
	return READ_WORD(&cninja_pf1_data[offset]);
}

void cninja_pf1_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&cninja_pf1_rowscroll[offset],data);
}

void cninja_pf2_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&cninja_pf2_rowscroll[offset],data);
}

void cninja_pf3_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&cninja_pf3_rowscroll[offset],data);
}

int cninja_pf3_rowscroll_r(int offset)
{
	return READ_WORD(&cninja_pf3_rowscroll[offset]);
}

void cninja_pf4_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&cninja_pf4_rowscroll[offset],data);
}

/******************************************************************************/

void cninja_vh_stop (void)
{
	osd_free_bitmap(cninja_pf1_bitmap);
	osd_free_bitmap(cninja_pf2_bitmap);
	osd_free_bitmap(cninja_pf3_bitmap);
	osd_free_bitmap(cninja_pf4_bitmap);
	osd_free_bitmap(cninja_tf4_bitmap);
	free(cninja_pf4_dirty);
	free(cninja_pf3_dirty);
	free(cninja_pf2_dirty);
	free(cninja_pf1_dirty);
}

int cninja_vh_start(void)
{

	#define ALLOC_ERROR {cninja_vh_stop ();return 1;}

	/* Allocate bitmaps */
	if ((cninja_pf1_bitmap = osd_create_bitmap(512,256)) == 0) ALLOC_ERROR
	if ((cninja_pf2_bitmap = osd_create_bitmap(1024,512)) == 0) ALLOC_ERROR
	if ((cninja_pf3_bitmap = osd_create_bitmap(1024,512)) == 0) ALLOC_ERROR
	if ((cninja_pf4_bitmap = osd_create_bitmap(1024,512)) == 0) ALLOC_ERROR
	if ((cninja_tf4_bitmap = osd_create_bitmap(1024,512)) == 0) ALLOC_ERROR

	cninja_pf1_dirty = malloc(TILERAM_SIZE);
	cninja_pf3_dirty = malloc(TILERAM_SIZE);
	cninja_pf2_dirty = malloc(TILERAM_SIZE);
	cninja_pf4_dirty = malloc(TILERAM_SIZE);

	/* Check for errors */
	if (!cninja_pf1_dirty) ALLOC_ERROR
	if (!cninja_pf2_dirty) ALLOC_ERROR
	if (!cninja_pf3_dirty) ALLOC_ERROR
	if (!cninja_pf4_dirty) ALLOC_ERROR

	offsetx[0] = 0;
	offsetx[1] = 0;
	offsetx[2] = 512;
	offsetx[3] = 512;
	offsety[0] = 0;
	offsety[1] = 256;
	offsety[2] = 0;
	offsety[3] = 256;

	memset(cninja_pf1_dirty,1,TILERAM_SIZE);
	memset(cninja_pf2_dirty,1,TILERAM_SIZE);
	memset(cninja_pf3_dirty,1,TILERAM_SIZE);
	memset(cninja_pf4_dirty,1,TILERAM_SIZE);

	/* The bootleg has some broken scroll registers... */
	if (!strcmp(Machine->gamedrv->name,"stoneage"))
		bootleg=1;
	else
		bootleg=0;

	cninja_pf2_bank=1;
	cninja_pf3_bank=2;

	return 0;
}

/******************************************************************************/
