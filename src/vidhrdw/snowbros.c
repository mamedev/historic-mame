/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *snowbros_paletteram;
unsigned char *snowbros_spriteram;

int snowbros_spriteram_size;
int palettechanged;


void snowbros_paletteram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&snowbros_paletteram[offset], data);
    palettechanged = 1;
}

int  snowbros_paletteram_r (int offset)
{
	return READ_WORD (&snowbros_paletteram[offset]);
}


/* Put in case screen can be optimised later */

void snowbros_spriteram_w (int offset, int data)
{
  	COMBINE_WORD_MEM (&snowbros_spriteram[offset], data);
}

int  snowbros_spriteram_r (int offset)
{
	return READ_WORD (&snowbros_spriteram[offset]);
}

void snowbros_vh_screenrefresh(struct osd_bitmap *bitmap)
{
    int x=0,y=0,offs;

    /* Colour maps */

	int pom,i;

    if (palettechanged)
    {
    	palettechanged = 0;

	    for (pom = 0; pom < 16; pom++)
	    {
	        Machine->gfx[0]->colortable[pom*16] = Machine->pens[0];
	        for (i = 1; i < 16; i++)
	        {
		        int palette = READ_WORD(&snowbros_paletteram[pom*32+i*2]);
		        int red = palette & 31;
		        int green = (palette >> 5) & 31;
		        int blue = (palette >> 10) & 31;

		        red = (red << 3) + (red >> 2);
		        green = (green << 3) + (green >> 2);
		        blue = (blue << 3) + (blue >> 2);
		        setgfxcolorentry (Machine->gfx[0], pom*16+i, red, green, blue);
	        }
        }
    }

    /*
     * Sprite Tile Format
     * ------------------
     *
     * Byte(s) | Bit(s)   | Use
     * --------+-76543210-+----------------
     *  0-5    | -------- | ?
     *    6    | -------- | ?
     *    7    | xxxx.... | Palette Bank
     *    7    | .......x | XPos - Sign Bit
     *    9    | xxxxxxxx | XPos
     *    7    | ......x. | YPos - Sign Bit
     *    B    | xxxxxxxx | YPos
     *    7    | .....x.. | Use Relative offsets
     *    C    | -------- | ?
     *    D    | xxxxxxxx | Sprite Number (low 8 bits)
     *    E    | -------- | ?
     *    F    | ....xxxx | Sprite Number (high 4 bits)
     *    F    | x....... | Flip Sprite Y-Axis
     *    F    | .x...... | Flip Sprite X-Axis
     */

	/* This clears & redraws the entire screen each pass */

  	fillbitmap(bitmap,Machine->gfx[0]->colortable[0],&Machine->drv->visible_area);

	for (offs = 0;offs < 0x1e00; offs += 16)
	{
		int sx = READ_WORD(&snowbros_spriteram[8+offs]) & 0xff;
		int sy = READ_WORD(&snowbros_spriteram[0x0a+offs]) & 0xff;
    	int tilecolour = READ_WORD(&snowbros_spriteram[6+offs]);

        if (tilecolour & 1) sx = -1 - (sx ^ 0xff);

        if (tilecolour & 2) sy = -1 - (sy ^ 0xff);

        if (tilecolour & 4)
        {
        	x += sx;
            y += sy;
        }
        else
        {
        	x = sx;
            y = sy;
        }

        if (x > 511) x &= 0x1ff;
        if (y > 511) y &= 0x1ff;

        if ((x>-16) && (y>0) && (x<256) && (y<240))
        {
            int flip = READ_WORD(&snowbros_spriteram[0x0e + offs]);
        	int tile = ((flip & 0x0f) << 8) + (READ_WORD(&snowbros_spriteram[0x0c+offs]) & 0xff);

			drawgfx(bitmap,Machine->gfx[0],
				tile,
				tilecolour >> 4,
				flip & 0x80, flip & 0x40,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
