/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "vidhrdw/generic.h"



extern struct osd_bitmap *tmpbitmap;
unsigned char *gottlieb_paletteram;
unsigned char *gottlieb_characterram;
static unsigned char dirtycharacter[256];
static int dirtypalette;
static unsigned char color_to_lookup[16*16*16];
static int background_priority=0;
static unsigned char hflip=0;
static unsigned char vflip=0;
static unsigned char bottomupchars;
static int rotated90;
static int optimized_palette;
static int currentbank;

int qbert_vh_start(void)
{
	int offs;


	rotated90 = 1;
	bottomupchars = 0;
	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 0;

	return generic_vh_start();
}

int mplanets_vh_start(void)
{
	int offs;


	rotated90 = 1;
	bottomupchars = 255;
	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 0;

	return generic_vh_start();
}

int reactor_vh_start(void)
{
	int offs;


	rotated90 = 0;
	bottomupchars = 0;
	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

int stooges_vh_start(void)
{
	int offs;


	rotated90 = 0;
	bottomupchars = 0;
	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

int krull_vh_start(void)
{
	int offs;


	rotated90 = 1;
	bottomupchars = 0;
	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

void gottlieb_vh_init_optimized_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	unsigned short *colors=(unsigned short *)color_prom;
	optimized_palette=1;
	for (i = 0;i<256 ;i++)
	{
		unsigned int red,green,blue;
		red=(colors[i]>>8) & 0x0F;
		green=(colors[i]>>4) & 0x0F;
		blue=colors[i] & 0x0F;
		palette[3*i]=(red<<4) | red;
		palette[3*i+1]=(green<<4) | green;
		palette[3*i+2]=(blue<<4) | blue;
		if (colors[i]) color_to_lookup[colors[i]]=i;
	}
}

void gottlieb_vh_init_basic_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	optimized_palette=0;
	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 0) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}
}


void gottlieb_video_outputs(int data)
{
	static int last = 0;

	background_priority = data & 1;
	hflip = (data & 2) ? 255 : 0;
	vflip = (data & 4) ? 255 : 0;
	currentbank = (data & 0x10) ? 256 : 0;

	if ((data & 6) != (last & 6))
		memset(dirtybuffer,1,videoram_size);
	last = data;
}


void gottlieb_paletteram_w(int offset,int data)
{
	if (gottlieb_paletteram[offset]!=data) {
		gottlieb_paletteram[offset] = data;
		dirtypalette=1;
	}
}

void gottlieb_characterram_w(int offset,int data)
{
	if (gottlieb_characterram[offset]!=data) {
		dirtycharacter[offset/32]=1;
		gottlieb_characterram[offset]=data;
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gottlieb_vh_screenrefresh(struct osd_bitmap *bitmap)
{
    int offs,i,col;
    if (dirtypalette) { /* rebuild the color lookup table */
	if (optimized_palette)
	    for (i=0;i<16;i++) {
		col = gottlieb_paletteram[2*i] + ((gottlieb_paletteram[2*i+1]&0x0F) << 8);
		Machine->gfx[0]->colortable[i] = Machine->pens[color_to_lookup[col]];
	    }
	else
	    for (i = 0;i < 16;i++) {
		col = (gottlieb_paletteram[2*i+1] >> 1) & 0x07;   /* red component */
		col |= (gottlieb_paletteram[2*i] >> 2) & 0x38;  /* green component */
		col |= (gottlieb_paletteram[2*i] << 4) & 0xc0;      /* blue component */
		Machine->gfx[0]->colortable[i] = Machine->pens[col];
	    }
    }

    /* recompute character graphics */
    for (offs=0;offs<256;offs++)
	if (dirtycharacter[offs])
		decodechar(Machine->gfx[0],offs,gottlieb_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);

    /* for every character in the Video RAM, check if it has been modified */
    /* since last time and update it accordingly. */
    for (offs = videoram_size - 1;offs >= 0;offs--) {
	if (dirtypalette || dirtybuffer[offs] || dirtycharacter[videoram[offs]]) {
	    int sx,sy;

	    dirtybuffer[offs] = 0;

	    if (rotated90) {
		if (hflip) sx=29-offs/32;
		else sx=offs/32;
		if (vflip) sy=offs%32;
		else sy=31-offs%32;
	    } else {
		if (hflip) sx=31-offs%32;
		else sx=offs%32;
		if (vflip) sy=29-offs/32;
		else sy=offs/32;
	    }

	    drawgfx(tmpbitmap,Machine->gfx[0],
			videoram[offs],  /* code */
			0, /* color tuple */
			hflip^bottomupchars, vflip^bottomupchars,
			sx*8,sy*8,
			&Machine->drv->visible_area, /* clip */
			TRANSPARENCY_NONE,
			0       /* transparent color */
		   );
	}
    }

    dirtypalette=0;
    for (offs=0;offs<256;offs++) dirtycharacter[offs]=0;

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
    for (offs = 0;offs < spriteram_size - 8;offs += 4)     /* it seems there's something strange with sprites #62 and #63 */
	{
	    int sx,sy;


	    if (rotated90)
		{
			sx = (hflip ^ spriteram[offs]) - 12;
			if (vflip) sy = (spriteram[offs+1]) + 4;
			else sy = (255 ^ spriteram[offs+1]) - 12;
		}
		else
		{
			if (hflip) sx = (hflip ^ spriteram[offs+1]) - 24;
			else sx = (spriteram[offs+1]) - 4;
			sy = (vflip ^ spriteram[offs]) - 12;
	    }

	    if (spriteram[offs] || spriteram[offs+1])
			drawgfx(bitmap,Machine->gfx[1],
					currentbank+(255^spriteram[offs+2]), /* object # */
					0, /* color tuple */
					hflip, vflip,
					sx,sy,
					&Machine->drv->visible_area,
					background_priority ? TRANSPARENCY_THROUGH : TRANSPARENCY_PEN,0);
	}
}
