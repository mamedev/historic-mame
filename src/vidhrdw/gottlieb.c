/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "vidhrdw/generic.h"



unsigned char *gottlieb_paletteram;
unsigned char *gottlieb_characterram;
static unsigned char dirtycharacter[256];
static int background_priority=0;
static unsigned char hflip=0;
static unsigned char vflip=0;
static int currentbank;

int qbert_vh_start(void)
{
	int offs;


	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 0;

	return generic_vh_start();
}

int mplanets_vh_start(void)
{
	int offs;


	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 0;

	return generic_vh_start();
}

int reactor_vh_start(void)
{
	int offs;


	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

int stooges_vh_start(void)
{
	int offs;


	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

int krull_vh_start(void)
{
	int offs;


	for (offs = 0;offs < 256;offs++)
		dirtycharacter[offs] = 1;

	return generic_vh_start();
}

void gottlieb_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}


void gottlieb_video_outputs(int data)
{
	static int last = 0;

	background_priority = data & 1;
	hflip = data & 2;
	vflip = data & 4;
	currentbank = (data & 0x10) ? 256 : 0;

	if ((data & 6) != (last & 6))
		memset(dirtybuffer,1,videoram_size);
	last = data;
}


void gottlieb_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b,val;


	gottlieb_paletteram[offset] = data;

	/* red component */
	val = gottlieb_paletteram[offset | 1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	val = gottlieb_paletteram[offset & ~1];
	bit0 = (val >> 4) & 0x01;
	bit1 = (val >> 5) & 0x01;
	bit2 = (val >> 6) & 0x01;
	bit3 = (val >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	val = gottlieb_paletteram[offset & ~1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	osd_modify_pen(Machine->gfx[0]->colortable[offset / 2],r,g,b);
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
    int offs;


    /* recompute character graphics */
    for (offs=0;offs<256;offs++)
	if (dirtycharacter[offs])
		decodechar(Machine->gfx[0],offs,gottlieb_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);

    /* for every character in the Video RAM, check if it has been modified */
    /* since last time and update it accordingly. */
    for (offs = videoram_size - 1;offs >= 0;offs--) {
	if (dirtybuffer[offs] || dirtycharacter[videoram[offs]]) {
	    int sx,sy;

	    dirtybuffer[offs] = 0;

		if (hflip) sx=31-offs%32;
		else sx=offs%32;
		if (vflip) sy=29-offs/32;
		else sy=offs/32;

	    drawgfx(tmpbitmap,Machine->gfx[0],
			videoram[offs],  /* code */
			0, /* color tuple */
			hflip, vflip,
			sx*8,sy*8,
			&Machine->drv->visible_area, /* clip */
			TRANSPARENCY_NONE,
			0       /* transparent color */
		   );
	}
    }

    for (offs=0;offs<256;offs++) dirtycharacter[offs]=0;

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
    for (offs = 0;offs < spriteram_size - 8;offs += 4)     /* it seems there's something strange with sprites #62 and #63 */
	{
	    int sx,sy;


		sx = (spriteram[offs+1]) - 4;
		if (hflip) sx = 233 - sx;
		sy = (spriteram[offs]) - 13;
		if (vflip) sy = 228 - sy;

	    if (spriteram[offs] || spriteram[offs+1])
			drawgfx(bitmap,Machine->gfx[1],
					currentbank+(255^spriteram[offs+2]), /* object # */
					0, /* color tuple */
					hflip, vflip,
					sx,sy,
					&Machine->drv->visible_area,
			/* the background pen for the game is actually 1, not 0, because we reserved */
			/* pen 0 for the background black */
					background_priority ? TRANSPARENCY_THROUGH : TRANSPARENCY_PEN,0);
	}
}
