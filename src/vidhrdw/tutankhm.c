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

	for( i = 0; i < 256; i++ )
	{
		int bit0, bit1, bit2;

		bit0 = ( i >> 0 ) & 0x01;
		bit1 = ( i >> 1 ) & 0x01;
		bit2 = ( i >> 2 ) & 0x01;
		palette[ 3 * i ] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = ( i >> 3 ) & 0x01;
		bit1 = ( i >> 4 ) & 0x01;
		bit2 = ( i >> 5 ) & 0x01;
		palette[ 3 * i + 1 ] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = ( i >> 6 ) & 0x01;
		bit2 = ( i >> 7 ) & 0x01;
		palette[ 3 * i + 2 ] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}
}


void tut_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		unsigned char x, y;

		/* bitmap is rotated -90 deg. */
		x = ( offset >> 7 );
		y = ( ( ~offset ) & 0x7f ) << 1;

#if defined(MSDOS_ONLY) || defined(WIN32)
		tmpbitmap->line[ y ][ x ] = data >> 4;
		tmpbitmap->line[ y + 1 ][ x ] = data & 0x0f;
#else
		tmpbitmap->line[ y ][ x ] = tut_paletteram[ data >> 4 ];
		tmpbitmap->line[ y + 1 ][ x ] = tut_paletteram[ data & 0x0f ];
#endif
		videoram[offset] = data;
	}
}



#ifdef MSDOS_ONLY
#include <allegro.h>       /*  for RGB  */

/*
 *  This is not portable, it access the allegro library that is
 *  used on IBM.
 */

void tut_palette_w(int offset,int data)
{
	RGB rgb;

        tut_paletteram[offset] = data;

	      rgb.r = ((data & 0x07)<<3);
        if(rgb.r != 0)
  	      rgb.r += 7;
	      rgb.g = (((data>>3) & 0x07)<<3);
        if(rgb.g != 0)
  	      rgb.g += 7;
	      rgb.b = (((data>>6) & 0x03)<<4);
        if(rgb.b != 0)
  	      rgb.b += 3;

	set_color(offset,&rgb);
}
#else
#ifdef WIN32
void tut_palette_w(int offset,int data)
{
   tut_paletteram[offset] = data;
   osd_win32_set_color(offset,data);
}
#else
void tut_palette_w(int offset,int data)
{
        tut_paletteram[offset] = data;
}
#endif
#endif

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
