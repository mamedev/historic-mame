/***************************************************************************

  Dec0 Video emulation - Bryan McPhail, mish@tendril.force9.net

  Although all the games are similar there are enough special cases
  to make it worthwhile to have separate screen draw functions for
  each game.

*********************************************************************
	Sprite data:  The unknown bits seem to be unused.

	Byte 0:
		Bit 0,1,2 : Y co-ord hi bits
		Bit 3,4 : Sprite height (1x, 2x, 4x, 8x)
		Bit 5  - X flip
		Bit 6  - Y flip
		Bit 7  - Only display Sprite if set
	Byte 1: Y-coords
	Byte 2:
		Bit 0,1,2,3: Hi bits of sprite number
		Bit 4,5,6,7: ?????
	Byte 3: Low bits of sprite number
	Byte 4:
		Bit 0,1,2 - X co-ords hi bits
		Bit 3: ????
		Bit 4,5,6,7:  - Colour
	Byte 5: X-coords

**********************************************************************
  Palette data

    0x000 - character palettes (Sprites on Midnight R)
    0x200 - sprite palettes (Characters on Midnight R)
    0x400 - tiles 1
  	0x600 - tiles 2

	  Bad Dudes, Robocop, Heavy Barrel, Hippodrome - 24 bit rgb
 		Sly Spy, Midnight Resistance - 12 bit rgb

  Tile data

  	4 bit palette select, 12 bit tile select

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

//#define DE_DEBUG
//#define PRINT_PF_ATTRIBUTES

#define NUM_COLORS		0x100	/* Number of total colors we use */
#define TEXTRAM_SIZE	0x2000	/* Size of text layer */
#define TILERAM_SIZE	0x800	/* Size of background and foreground */

/* Palette stuff */
static int Colours_Allocated,Overflow;
static unsigned char *palette_ram_rg,*palette_ram_b;

/* Video */
unsigned char *dec0_sprite;
static unsigned char *dec0_pf1_data,*dec0_pf2_data,*dec0_pf3_data;
static unsigned char *dirty_pal_r,*dirty_pal_g,*dirty_pal_b;
static unsigned char *dec0_pf1_dirty,*dec0_pf3_dirty,*dec0_pf2_dirty;
static struct osd_bitmap *dec0_pf1_bitmap;
static int dec0_pf1_current_shape;
static struct osd_bitmap *dec0_pf2_bitmap;
static int dec0_pf2_current_shape;
static struct osd_bitmap *dec0_pf3_bitmap;
static int dec0_pf3_current_shape;

static int palette_dirty;


/* playfield control registers:
   bank 0:
   0: unknown (82, 86, 8e)
   2: unknown (00 in fg, 03 in bg)
   4: unknown (always 00)
   6: playfield shape: 00 = 4x1, 01 = 2x2, 02 = 1x4

   bank 1:
   0: horizontal scroll
   2: vertical scroll
   4: unknown (08 in Hippodrome, 00 in the others)
   6: 04 = enable row scroll registers; other values unknown (05 in Hippodrome)
*/
static unsigned char dec0_pf1_control_0[8];
static unsigned char dec0_pf1_control_1[8];
static unsigned char dec0_pf1_rowscroll[32];
static unsigned char dec0_pf2_control_0[8];
static unsigned char dec0_pf2_control_1[8];
static unsigned char dec0_pf2_rowscroll[32];
static unsigned char dec0_pf3_control_0[8];
static unsigned char dec0_pf3_control_1[8];
static unsigned char dec0_pf3_rowscroll[32];


static int dec0_pri;

/* Prototypes for this file */
void dec0_recalc_palette(int offset);
void dec1_recalc_palette(int offset);
void dec0_vh_stop (void);
int Palette_Allocated(int r, int g, int b);
void dec0_palette_24bit_remap(void);
void dec0_palette_12bit_remap(void);



static void printpfattributes(void)
{
#ifdef PRINT_PF_ATTRIBUTES
{
	int i,j;
	char buf[20];
	int trueorientation;
	struct osd_bitmap *bitmap = Machine->scrbitmap;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf1_control_0[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*0,0,TRANSPARENCY_NONE,0);
}
for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf1_control_1[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*1,0,TRANSPARENCY_NONE,0);
}
for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf2_control_0[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*3,0,TRANSPARENCY_NONE,0);
}
for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf2_control_1[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*4,0,TRANSPARENCY_NONE,0);
}
for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf3_control_0[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*6,0,TRANSPARENCY_NONE,0);
}
for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&dec0_pf3_control_1[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*7,0,TRANSPARENCY_NONE,0);
}
{
	sprintf(buf,"%04X",dec0_pri);
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,8*j,8*9,0,TRANSPARENCY_NONE,0);
}

	Machine->orientation = trueorientation;
}
#endif
}


static void dec0_drawsprites(struct osd_bitmap *bitmap,int basecolor,int pri_mask,int pri_val)
{
	int offs;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,colour,multi,fx,fy,inc;


		y = READ_WORD(&dec0_sprite[offs]);
		if ((y&0x8000) == 0) continue;

		x = READ_WORD(&dec0_sprite[offs+4]);
		colour = (x & 0xf000) >> 12;
		if ((colour & pri_mask) != pri_val) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */
											/* multi = 0   1   3   7 */

		sprite = READ_WORD (&dec0_sprite[offs+2]) & 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

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
			drawgfx(bitmap,Machine->gfx[3],
					sprite - multi * inc,
					colour + basecolor,
					fx,fy,
					x,y - 16 * multi,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}



static void dec0_pf1_update(int basecolor)
{
	int offs,mx,my,color,tile;
	int offsetx[4];
	int offsety[4];
	int quarter;


	switch (READ_WORD(&dec0_pf1_control_0[6]))
	{
		case 0:	/* 4x1 */
			offsetx[0] = 0;
			offsetx[1] = 256;
			offsetx[2] = 512;
			offsetx[3] = 768;
			offsety[0] = 0;
			offsety[1] = 0;
			offsety[2] = 0;
			offsety[3] = 0;
			if (dec0_pf1_current_shape != 0)
			{
				osd_free_bitmap(dec0_pf1_bitmap);
				dec0_pf1_bitmap = osd_create_bitmap(1024,256);
				dec0_pf1_current_shape = 0;
				memset(dec0_pf1_dirty,1,TILERAM_SIZE);
			}
			break;
		case 1:	/* 2x2 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 256;
			offsetx[3] = 256;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 0;
			offsety[3] = 256;
			if (dec0_pf1_current_shape != 1)
			{
				osd_free_bitmap(dec0_pf1_bitmap);
				dec0_pf1_bitmap = osd_create_bitmap(512,512);
				dec0_pf1_current_shape = 1;
				memset(dec0_pf1_dirty,1,TILERAM_SIZE);
			}
			break;
		case 2:	/* 1x4 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 0;
			offsetx[3] = 0;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 512;
			offsety[3] = 768;
			if (dec0_pf1_current_shape != 2)
			{
				osd_free_bitmap(dec0_pf1_bitmap);
				dec0_pf1_bitmap = osd_create_bitmap(256,1024);
				dec0_pf1_current_shape = 2;
				memset(dec0_pf1_dirty,1,TILERAM_SIZE);
			}
			break;
		default:
if (errorlog) fprintf(errorlog,"error: pf1_update with unknown shape %04x\n",READ_WORD(&dec0_pf1_control_0[6]));
			return;
	}


	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x800 * quarter; offs < 0x800 * quarter + 0x800;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (dec0_pf1_dirty[offs])
			{
				dec0_pf1_dirty[offs] = 0;
				tile = READ_WORD(&dec0_pf1_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(dec0_pf1_bitmap,Machine->gfx[0],
						tile & 0x0fff,
						color + basecolor + 64,	/* always transparent */
						0,0,
						8*mx + offsetx[quarter],8*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

static void dec0_pf2_update(int transparent)
{
	int offs,mx,my,color,tile;
	int offsetx[4];
	int offsety[4];
	int quarter;
	static int last_transparent;


	if (transparent != last_transparent)
	{
		last_transparent = transparent;
		memset(dec0_pf2_dirty,1,TILERAM_SIZE);
	}

	switch (READ_WORD(&dec0_pf2_control_0[6]))
	{
		case 0:	/* 4x1 */
			offsetx[0] = 0;
			offsetx[1] = 256;
			offsetx[2] = 512;
			offsetx[3] = 768;
			offsety[0] = 0;
			offsety[1] = 0;
			offsety[2] = 0;
			offsety[3] = 0;
			if (dec0_pf2_current_shape != 0)
			{
				osd_free_bitmap(dec0_pf2_bitmap);
				dec0_pf2_bitmap = osd_create_bitmap(1024,256);
				dec0_pf2_current_shape = 0;
				memset(dec0_pf2_dirty,1,TILERAM_SIZE);
			}
			break;
		case 1:	/* 2x2 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 256;
			offsetx[3] = 256;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 0;
			offsety[3] = 256;
			if (dec0_pf2_current_shape != 1)
			{
				osd_free_bitmap(dec0_pf2_bitmap);
				dec0_pf2_bitmap = osd_create_bitmap(512,512);
				dec0_pf2_current_shape = 1;
				memset(dec0_pf2_dirty,1,TILERAM_SIZE);
			}
			break;
		case 2:	/* 1x4 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 0;
			offsetx[3] = 0;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 512;
			offsety[3] = 768;
			if (dec0_pf2_current_shape != 2)
			{
				osd_free_bitmap(dec0_pf2_bitmap);
				dec0_pf2_bitmap = osd_create_bitmap(256,1024);
				dec0_pf2_current_shape = 2;
				memset(dec0_pf2_dirty,1,TILERAM_SIZE);
			}
			break;
		default:
if (errorlog) fprintf(errorlog,"error: pf2_update with unknown shape %04x\n",READ_WORD(&dec0_pf2_control_0[6]));
			return;
	}


	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x200 * quarter; offs < 0x200 * quarter + 0x200;offs += 2)
		{
			mx++;
			if (mx == 16)
			{
				mx = 0;
				my++;
			}

			if (dec0_pf2_dirty[offs])
			{
				dec0_pf2_dirty[offs] = 0;
				tile = READ_WORD(&dec0_pf2_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(dec0_pf2_bitmap,Machine->gfx[1],
						tile & 0x0fff,
						color + 32 + 64 * transparent,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

static void dec0_pf3_update(int transparent)
{
	int offs,mx,my,color,tile;
	int offsetx[4];
	int offsety[4];
	int quarter;
	static int last_transparent;

	if (transparent != last_transparent)
	{
		last_transparent = transparent;
		memset(dec0_pf3_dirty,1,TILERAM_SIZE);
	}

	switch (READ_WORD(&dec0_pf3_control_0[6]))
	{
		case 0:	/* 4x1 */
			offsetx[0] = 0;
			offsetx[1] = 256;
			offsetx[2] = 512;
			offsetx[3] = 768;
			offsety[0] = 0;
			offsety[1] = 0;
			offsety[2] = 0;
			offsety[3] = 0;
			if (dec0_pf3_current_shape != 0)
			{
				osd_free_bitmap(dec0_pf3_bitmap);
				dec0_pf3_bitmap = osd_create_bitmap(1024,256);
				dec0_pf3_current_shape = 0;
				memset(dec0_pf3_dirty,1,TILERAM_SIZE);
			}
			break;
		case 1:	/* 2x2 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 256;
			offsetx[3] = 256;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 0;
			offsety[3] = 256;
			if (dec0_pf3_current_shape != 1)
			{
				osd_free_bitmap(dec0_pf3_bitmap);
				dec0_pf3_bitmap = osd_create_bitmap(512,512);
				dec0_pf3_current_shape = 1;
				memset(dec0_pf3_dirty,1,TILERAM_SIZE);
			}
			break;
		case 2:	/* 1x4 */
			offsetx[0] = 0;
			offsetx[1] = 0;
			offsetx[2] = 0;
			offsetx[3] = 0;
			offsety[0] = 0;
			offsety[1] = 256;
			offsety[2] = 512;
			offsety[3] = 768;
			if (dec0_pf3_current_shape != 2)
			{
				osd_free_bitmap(dec0_pf3_bitmap);
				dec0_pf3_bitmap = osd_create_bitmap(256,1024);
				dec0_pf3_current_shape = 2;
				memset(dec0_pf3_dirty,1,TILERAM_SIZE);
			}
			break;
		default:
if (errorlog) fprintf(errorlog,"error: pf3_update with unknown shape %04x\n",READ_WORD(&dec0_pf3_control_0[6]));
			return;
	}


	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x200 * quarter; offs < 0x200 * quarter + 0x200;offs += 2)
		{
			mx++;
			if (mx == 16)
			{
				mx = 0;
				my++;
			}

			if (dec0_pf3_dirty[offs])
			{
				dec0_pf3_dirty[offs] = 0;
				tile = READ_WORD(&dec0_pf3_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(dec0_pf3_bitmap,Machine->gfx[2],
						tile & 0x0fff,
						color + 48 + 64 * transparent,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}


/******************************************************************************/

void dec0_pf1_draw(struct osd_bitmap *bitmap)
{
	int offs,scrolly;


	scrolly = -READ_WORD(&dec0_pf1_control_1[2]);

	if (READ_WORD(&dec0_pf1_control_1[6]) == 0x04)
	{
		int rscrollx[16];

		for (offs = 0;offs < 16;offs++)
			rscrollx[offs] = -READ_WORD(&dec0_pf1_control_1[0]) - READ_WORD(&dec0_pf1_rowscroll[2*offs]);
		copyscrollbitmap(bitmap,dec0_pf1_bitmap,16,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
	else
	{
		int scrollx;

		scrollx = -READ_WORD(&dec0_pf1_control_1[0]);
		copyscrollbitmap(bitmap,dec0_pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}

/******************************************************************************/

void dec0_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,xscroll_f,yscroll_f,xscroll_b,yscroll_b;


	if (palette_dirty)
	{
		dec0_palette_24bit_remap();
		palette_dirty = 0;
	}

	/* Scroll positions */
	xscroll_b=READ_WORD(&dec0_pf3_control_1[0]);
	yscroll_b=READ_WORD(&dec0_pf3_control_1[2]);
	xscroll_f=READ_WORD(&dec0_pf2_control_1[0]);
	yscroll_f=READ_WORD(&dec0_pf2_control_1[2]);


	dec0_pf1_update(0);


/* WARNING: inverted wrt Midnight Resistance */
if ((dec0_pri & 0x01) == 0)
{
	int scrollx[32];
	int scrolly;


	dec0_pf2_update(0);
	dec0_pf3_update(1);

    /* Foreground */
	for (offs = 0;offs < 32;offs++)
		scrollx[offs] = -xscroll_f;
	if (READ_WORD(&dec0_pf2_control_1[6]) == 0x04)
	{
		for (offs = 0;offs < 16;offs++)
			scrollx[offs] -= READ_WORD(&dec0_pf2_rowscroll[2*offs]);
	}
   	scrolly=-yscroll_f;
	copyscrollbitmap(bitmap,dec0_pf2_bitmap,32,scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Background */
	for (offs = 0;offs < 32;offs++)
		scrollx[offs] = -xscroll_b;
	if (READ_WORD(&dec0_pf3_control_1[6]) == 0x04)
	{
		for (offs = 0;offs < 16;offs++)
			scrollx[offs] -= READ_WORD(&dec0_pf3_rowscroll[2*offs]);
	}
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,32,scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	/* I'm not supporting sprite priority because I can't figure out how it works. */
	/* I would exepct e.g. the trees to cover the sprites in level 4 (forest) */
	dec0_drawsprites(bitmap,16,0x00,0x00);
}
else
{
	int scrollx[32];
	int scrolly;


	dec0_pf3_update(0);
	dec0_pf2_update(1);

    /* Background */
	for (offs = 0;offs < 32;offs++)
		scrollx[offs] = -xscroll_b;
	if (READ_WORD(&dec0_pf3_control_1[6]) == 0x04)
	{
		for (offs = 0;offs < 16;offs++)
			scrollx[offs] -= READ_WORD(&dec0_pf3_rowscroll[2*offs]);
	}
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,32,scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Foreground */
	for (offs = 0;offs < 32;offs++)
		scrollx[offs] = -xscroll_f;
	if (READ_WORD(&dec0_pf2_control_1[6]) == 0x04)
	{
		for (offs = 0;offs < 16;offs++)
			scrollx[offs] -= READ_WORD(&dec0_pf2_rowscroll[2*offs]);
	}
   	scrolly=-yscroll_f;
	copyscrollbitmap(bitmap,dec0_pf2_bitmap,32,scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	/* I'm not supporting sprite priority because I can't figure out how it works. */
	/* I would exepct e.g. the trees to cover the sprites in level 4 (forest) */
	dec0_drawsprites(bitmap,16,0x00,0x00);
}


	dec0_pf1_draw(bitmap);


printpfattributes();
}

/******************************************************************************/

void robocop_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int xscroll_f,yscroll_f,xscroll_b,yscroll_b;
   int scrollx,scrolly;

   /* Scroll positions */
   xscroll_b=READ_WORD (&dec0_pf3_control_1[0]);
   yscroll_b=READ_WORD (&dec0_pf3_control_1[2]);
   xscroll_f=READ_WORD (&dec0_pf2_control_1[0]);
   yscroll_f=READ_WORD (&dec0_pf2_control_1[2]);


	dec0_pf1_update(0);


if (dec0_pri & 0x01)
{
	int trans;


	dec0_pf2_update(0);
	dec0_pf3_update(1);

	/* WARNING: inverted wrt Midnight Resistance */
	/* Robocop uses it only for the title screen, so this might be just */
	/* completely wrong. The top 8 bits of the register might mean */
	/* something (they are 0x80 in midres, 0x00 here) */
	if (dec0_pri & 0x04)
		trans = 0x08;
	else trans = 0x00;

    /* Foreground */
    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
   	copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,16,0x08,trans);

    /* Background */
    scrollx=-xscroll_b;
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,16,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,16,0x00,0x00);
}
else
{
	int trans;


	dec0_pf3_update(0);
	dec0_pf2_update(1);

	/* WARNING: inverted wrt Midnight Resistance */
	/* Robocop uses it only for the title screen, so this might be just */
	/* completely wrong. The top 8 bits of the register might mean */
	/* something (they are 0x80 in midres, 0x00 here) */
	if (dec0_pri & 0x04)
		trans = 0x08;
	else trans = 0x00;

    /* Background */
    scrollx=-xscroll_b;
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,16,0x08,trans);

    /* Foreground */
    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
   	copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,16,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,16,0x00,0x00);
}

	dec0_pf1_draw(bitmap);


printpfattributes();
}

/******************************************************************************/

void hbarrel_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int xscroll_f,yscroll_f,xscroll_b,yscroll_b;
   int scrollx,scrolly;

	if (palette_dirty)
	{
		dec0_palette_24bit_remap();
		palette_dirty = 0;
	}

   /* Scroll positions */
   xscroll_b=READ_WORD (&dec0_pf3_control_1[0]);
   yscroll_b=READ_WORD (&dec0_pf3_control_1[2]);
   xscroll_f=READ_WORD (&dec0_pf2_control_1[0]);
   yscroll_f=READ_WORD (&dec0_pf2_control_1[2]);


	dec0_pf1_update(0);


	dec0_pf3_update(0);
	dec0_pf2_update(1);

    /* Background layer */
    scrollx=-xscroll_b;
   	scrolly=-yscroll_b;
   	copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Foreground */
    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
    copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);


	dec0_drawsprites(bitmap,16,0x00,0x00);

	dec0_pf1_draw(bitmap);


printpfattributes();
}



void hippodrm_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int xscroll_f,yscroll_f,xscroll_b,yscroll_b;


	if (palette_dirty)
	{
		dec0_palette_24bit_remap();
		palette_dirty = 0;
	}

	/* Scroll positions */
	xscroll_b=READ_WORD(&dec0_pf3_control_1[0]);
	yscroll_b=READ_WORD(&dec0_pf3_control_1[2]);
	xscroll_f=READ_WORD(&dec0_pf2_control_1[0]);
	yscroll_f=READ_WORD(&dec0_pf2_control_1[2]);


	dec0_pf1_update(0);

	dec0_pf2_update(0);


   {
    int scrollx;
    int scrolly;

    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
   	copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
   }


	dec0_drawsprites(bitmap,16,0x00,0x00);

	dec0_pf1_draw(bitmap);

printpfattributes();
}

/******************************************************************************/

void midres_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int xscroll_f,yscroll_f,xscroll_b,yscroll_b;

	if (palette_dirty)
	{
		dec0_palette_12bit_remap();
		palette_dirty = 0;
	}

   /* Scroll positions */
   xscroll_b=READ_WORD (&dec0_pf3_control_1[0]);
   yscroll_b=READ_WORD (&dec0_pf3_control_1[2]);
   xscroll_f=READ_WORD (&dec0_pf2_control_1[0]);
   yscroll_f=READ_WORD (&dec0_pf2_control_1[2]);


	dec0_pf1_update(16);


   {
    int scrollx;
    int scrolly;

if (dec0_pri & 0x01)
{
	int trans;


	dec0_pf2_update(0);
	dec0_pf3_update(1);

	if (dec0_pri & 0x04)
		trans = 0x00;
	else trans = 0x08;

    /* Foreground */
    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
   	copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,0,0x08,trans);

    /* Background */
    scrollx=-xscroll_b;
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,0,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,0,0x00,0x00);
}
else
{
	int trans;


	dec0_pf3_update(0);
	dec0_pf2_update(1);

	if (dec0_pri & 0x04)
		trans = 0x00;
	else trans = 0x08;

    /* Background */
    scrollx=-xscroll_b;
   	scrolly=-yscroll_b;
    copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,0,0x08,trans);

    /* Foreground */
    scrollx=-xscroll_f;
   	scrolly=-yscroll_f;
   	copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,0,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,0,0x00,0x00);
}
   }


	dec0_pf1_draw(bitmap);

printpfattributes();
}

/******************************************************************************/

void slyspy_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,xscroll_f,yscroll_f,xscroll_b,yscroll_b;

	if (palette_dirty)
	{
		dec0_palette_12bit_remap();
		palette_dirty = 0;
	}

	/* Scroll positions */
	xscroll_b=READ_WORD (&dec0_pf3_control_1[0]);
	yscroll_b=READ_WORD (&dec0_pf3_control_1[2]);
	xscroll_f=READ_WORD (&dec0_pf2_control_1[0]);
	yscroll_f=READ_WORD (&dec0_pf2_control_1[2]);


	dec0_pf1_update(0);

	dec0_pf3_update(0);
	dec0_pf2_update(1);

	{
		int scrollx;
		int scrolly;

		/* Background area */
		scrollx=-xscroll_b;
		scrolly=-yscroll_b;
		copyscrollbitmap(bitmap,dec0_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		/* Foreground area */
		scrolly=-yscroll_f;
		if (READ_WORD(&dec0_pf2_control_1[6]) == 0x04)
		{
			int rscrollx[16];

			for (offs = 0;offs < 16;offs++)
				rscrollx[offs] = -xscroll_f - READ_WORD(&dec0_pf2_rowscroll[2*offs]);
			copyscrollbitmap(bitmap,dec0_pf2_bitmap,16,rscrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
		}
		else
		{
			scrollx = -xscroll_f;
			copyscrollbitmap(bitmap,dec0_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
		}
	}


	dec0_drawsprites(bitmap,16,0x00,0x00);

	dec0_pf1_draw(bitmap);

printpfattributes();
}

/******************************************************************************
	Palette functions for Sly Spy, Midnight Resistance
*/

void dec0_palette_12bit_remap (void)
{
	int i;

	Colours_Allocated = 2;
	Overflow = 0;

//	if (errorlog) fprintf (errorlog,"Dec1 Palette remap\n");

	memset(dec0_pf1_dirty,1,TEXTRAM_SIZE);
	memset(dec0_pf2_dirty,1,TILERAM_SIZE);
	memset(dec0_pf3_dirty,1,TILERAM_SIZE);

	/* Remap sprites, chars, tiles 1, tiles 2 */
	for (i=0x800; i >= 0; i -= 2)
		dec1_recalc_palette(i);
}

/*******************************************************************************/

int Palette_Allocated(int r, int g, int b)
{
	int i;

	for (i=0; i<Colours_Allocated; i++)
	if (dirty_pal_r[i]==r && dirty_pal_b[i]==b && dirty_pal_g[i]==g)
		return i;

	/* Allocate a pen number for this colour, if overflow, allocated random... */
	if (Colours_Allocated >= NUM_COLORS)
	{
		Colours_Allocated++;
		Overflow = 1;
if (errorlog) fprintf(errorlog,"Palette Overflow: %d colours so far..\n",Colours_Allocated);
		return rand() % NUM_COLORS;
	}
	else
	{
		osd_modify_pen(Machine->pens[Colours_Allocated],r,g,b);

		dirty_pal_r[Colours_Allocated] = r;
		dirty_pal_g[Colours_Allocated] = g;
		dirty_pal_b[Colours_Allocated] = b;

		return Colours_Allocated++;
	}
}

void dec0_palette_24bit_remap(void)
{
	int i;

	Overflow=0;

	/* Dirty all scroll data */
	memset(dec0_pf1_dirty,1,TEXTRAM_SIZE);
	memset(dec0_pf2_dirty,1,TILERAM_SIZE);
	memset(dec0_pf3_dirty,1,TILERAM_SIZE);

//	if (errorlog) fprintf(errorlog,"Remapping Whole palette\n");

	Colours_Allocated=2;
	/* Remap sprites, tiles 1, tiles 2 in that order*/
	for (i=0x200; i<0x800; i=i+2)
		dec0_recalc_palette(i);
	/* Ascii characters get whatever colours are left... */
	for (i=0; i<0x200; i=i+2)
		dec0_recalc_palette(i);
}

void robocop_palette_b(int offset, int data)
{
	WRITE_WORD (&palette_ram_b[offset], data);
	dec0_recalc_palette(offset);

	/* Robocop high score table kludge - To make colour cycling work. */
	if (offset==0x61e) memset(dec0_pf3_dirty,1,0x200);

	if (offset==0x7fe || offset==0x3fe)
		dec0_palette_24bit_remap();
}

void dec0_recalc_palette(int offset)
{
	int r,g,b,pen;


	r = (READ_WORD(&palette_ram_rg[offset]) >> 0) & 0xff;
	g = (READ_WORD(&palette_ram_rg[offset]) >> 8) & 0xff;
	b = (READ_WORD(&palette_ram_b[offset]) >> 0) & 0xff;

	if (!r && !g && !b && (offset % 32))
		/* Pull out non-transparent black and remap it */
		pen = 1;
	else
		/* Search list of pens to see if this colour is already allocated */
		pen = Palette_Allocated(r,g,b);

	Machine->gfx[0]->colortable[offset/2] = Machine->pens[pen];
	if (offset%32 == 0) pen = 0;
	Machine->gfx[0]->colortable[1024 + offset/2] = Machine->pens[pen];
}

void dec1_recalc_palette(int offset)
{
	int r,g,b,pen;

	r = 0x11 * ((READ_WORD(&palette_ram_rg[offset]) >> 0) & 0x0f);
	g = 0x11 * ((READ_WORD(&palette_ram_rg[offset]) >> 4) & 0x0f);
	b = 0x11 * ((READ_WORD(&palette_ram_rg[offset]) >> 8) & 0x0f);

	if (!r && !g && !b && (offset % 32))
		/* Pull out non-transparent black and remap it */
		pen = 1;
	else
		/* Search list of pens to see if this colour is already allocated */
		pen = Palette_Allocated(r,g,b);

	Machine->gfx[0]->colortable[offset/2] = Machine->pens[pen];
	if (offset%32 == 0) pen = 0;
	Machine->gfx[0]->colortable[1024 + offset/2] = Machine->pens[pen];
}

/***************************************************************************

	Set up some colours...  Each game will set it's own palette when run

***************************************************************************/
void dec0_dummy_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++) {
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* initialize the color table */
	for (i = 0;i < Machine->drv->total_colors;i++)
		colortable[i] = i;
}

/******************************************************************************/

void dec0_palette_24bit_rg(int offset,int data)
{
	int oldword = READ_WORD(&palette_ram_rg[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&palette_ram_rg[offset],newword);
		palette_dirty = 1;
	}
}

void dec0_palette_24bit_b(int offset,int data)
{
	int oldword = READ_WORD(&palette_ram_b[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&palette_ram_b[offset],newword);
		palette_dirty = 1;
	}
}

void dec0_palette_12bit_w(int offset,int data)
{
	int oldword = READ_WORD(&palette_ram_rg[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&palette_ram_rg[offset],newword);
		palette_dirty = 1;
	}
}



void dec0_pf1_control_0_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf1_control_0[offset],data);
}

void dec0_pf1_control_1_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf1_control_1[offset],data);
}

void dec0_pf1_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf1_rowscroll[offset],data);
}

void dec0_pf1_data_w(int offset,int data)
{
	int oldword = READ_WORD(&dec0_pf1_data[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&dec0_pf1_data[offset],newword);
		dec0_pf1_dirty[offset] = 1;
	}
}

int dec0_pf1_data_r(int offset)
{
	return READ_WORD(&dec0_pf1_data[offset]);
}

void dec0_pf2_control_0_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf2_control_0[offset],data);
}

void dec0_pf2_control_1_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf2_control_1[offset],data);
}

void dec0_pf2_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf2_rowscroll[offset],data);
}

void dec0_pf2_data_w(int offset,int data)
{
	int oldword = READ_WORD(&dec0_pf2_data[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&dec0_pf2_data[offset],newword);
		dec0_pf2_dirty[offset] = 1;
	}
}

int dec0_pf2_data_r(int offset)
{
	return READ_WORD(&dec0_pf2_data[offset]);
}

void dec0_pf3_control_0_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf3_control_0[offset],data);
}

void dec0_pf3_control_1_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf3_control_1[offset],data);
}

void dec0_pf3_rowscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&dec0_pf3_rowscroll[offset],data);
}

void dec0_pf3_data_w(int offset,int data)
{
	int oldword = READ_WORD(&dec0_pf3_data[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&dec0_pf3_data[offset],newword);
		dec0_pf3_dirty[offset] = 1;
	}
}

int dec0_pf3_data_r(int offset)
{
	return READ_WORD(&dec0_pf3_data[offset]);
}



void dec0_priority_w(int offset,int data)
{
	if (offset == 0) dec0_pri = data;
	else
if (errorlog) fprintf(errorlog,"PC %06x write %02x to priority offset %d\n",cpu_getpc(),data,offset);
}

/* Nice 'wrap-around' patch which fixes sprite memory problems :) */
void hippo_sprite_fix(int offset, int data)
{
	WRITE_WORD (&dec0_sprite[offset], data);
}

/******************************************************************************/

int dec0_vh_start (void)
{
	/* Allocate bitmaps */
	if ((dec0_pf1_bitmap = osd_create_bitmap(512,512)) == 0) {
		dec0_vh_stop ();
		return 1;
	}
	dec0_pf1_current_shape = 1;

	if ((dec0_pf2_bitmap = osd_create_bitmap(512,512)) == 0) {
		dec0_vh_stop ();
		return 1;
	}
	dec0_pf2_current_shape = 1;

	if ((dec0_pf3_bitmap = osd_create_bitmap(512,512)) == 0) {
		dec0_vh_stop ();
		return 1;
	}
	dec0_pf3_current_shape = 1;

	dec0_pf1_data = malloc(TEXTRAM_SIZE);
	dec0_pf1_dirty = malloc(TEXTRAM_SIZE);
	dec0_pf3_data = malloc(TILERAM_SIZE);
	dec0_pf3_dirty = malloc(TILERAM_SIZE);
	dec0_pf2_data = malloc(TILERAM_SIZE);
	dec0_pf2_dirty = malloc(TILERAM_SIZE);

	palette_ram_rg=malloc(0x800);
	palette_ram_b=malloc(0x800);

	dirty_pal_r = malloc (NUM_COLORS);
	dirty_pal_g = malloc (NUM_COLORS);
	dirty_pal_b = malloc (NUM_COLORS);

	memset(palette_ram_rg,0,0x800);
	memset(palette_ram_b,0,0x800);

	memset (dirty_pal_r, 0, NUM_COLORS);
	memset (dirty_pal_g, 0, NUM_COLORS);
	memset (dirty_pal_b, 0, NUM_COLORS);

	memset(dec0_pf1_dirty,1,TEXTRAM_SIZE);
	memset(dec0_pf2_dirty,1,TILERAM_SIZE);
	memset(dec0_pf3_dirty,1,TILERAM_SIZE);

	Machine->gfx[0]->colortable[0]=0;
	Machine->gfx[1]->colortable[0]=0;
	Machine->gfx[2]->colortable[0]=0;
	Machine->gfx[3]->colortable[0]=0;

	Machine->gfx[0]->colortable[1]=1;
	Machine->gfx[1]->colortable[1]=1;
	Machine->gfx[2]->colortable[1]=1;
	Machine->gfx[3]->colortable[1]=1;

	osd_modify_pen (Machine->pens[0],0,0,0);
	osd_modify_pen (Machine->pens[1],1,0,0);

	Overflow=0;
	Colours_Allocated=2;

	palette_dirty = 1;

	return 0;
}

/******************************************************************************/

void dec0_vh_stop (void)
{
	#ifdef DE_DEBUG
	int i;

  FILE *fp;
  fp=fopen("robo_hi.ram","wb");
  fwrite(dec0_sprite,1,0x1000,fp);
  fclose(fp);

  fp=fopen("video.ram","wb");
  fwrite(dec0_pf1_data,1,0x2000,fp);
  fclose(fp);

  fp=fopen("fore.ram","wb");
  fwrite(dec0_pf2_data,1,0x800,fp);
  fclose(fp);

  fp=fopen("back.ram","wb");
  fwrite(dec0_pf3_data,1,0x800,fp);
  fclose(fp);

  fp=fopen("pal_rg.ram","wb");
  fwrite(palette_ram_rg,1,0x800,fp);
  fclose(fp);

  fp=fopen("pal_b.ram","wb");
  fwrite(palette_ram_b,1,0x800,fp);
  fclose(fp);

  fp=fopen("system.rom","wb");
  fwrite(RAM,1,0x60000,fp);
  fclose(fp);

  fp=fopen("color.txt","wb");

//  for (i=0; i<600; i++)
//  	fprintf(fp,"Colour table %d uses pen %d\n",i,Machine->gfx[0]->colortable[i]);

  fclose(fp);

  #endif

	osd_free_bitmap(dec0_pf2_bitmap);
	osd_free_bitmap(dec0_pf3_bitmap);

	free(dec0_pf3_data);
	free(dec0_pf2_data);
	free(dec0_pf1_data);
	free(palette_ram_rg);
	free(palette_ram_b);
	free(dirty_pal_r);
	free(dirty_pal_g);
	free(dirty_pal_b);
	free(dec0_pf3_dirty);
	free(dec0_pf2_dirty);
	free(dec0_pf1_dirty);
}
