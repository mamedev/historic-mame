/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/


/*  Stuff that work only in MS DOS (Color cycling)
 */

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char tut_paletteram[16];
unsigned char *tut_scrollx;



/***************************************************************************

  Tutankhm doesn't have a color PROM, it uses RAM palette registers.
  This routine sets up the color tables to simulate it.

***************************************************************************/
void tut_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

	/* initialize the color table */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}


void tut_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		unsigned char *lookup = Machine->gfx[0]->colortable;
		unsigned char x, y;

		/* bitmap is rotated -90 deg. */
		x = ( offset >> 7 );
		y = ( ( ~offset ) & 0x7f ) << 1;

		tmpbitmap->line[ y ][ x ] = lookup[data >> 4];
		tmpbitmap->line[ y + 1 ][ x ] = lookup[data & 0x0f];

		videoram[offset] = data;
	}
}


void tut_palette_w(int offset,int data)
{
	int r, g, b;
	int bit0,bit1,bit2;


	tut_paletteram[offset] = data;

	/* red component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	osd_modify_pen (Machine->gfx[0]->colortable[offset], r, g, b);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void tut_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the temporary bitmap to the screen */
	{
		int scroll[32], i;


		for (i = 0;i < 8;i++)
			scroll[i] = 0;
		for (i = 8;i < 32;i++)
			scroll[i] = -(*tut_scrollx);

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}
