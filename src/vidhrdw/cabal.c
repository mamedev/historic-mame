/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "vidhrdw/generic.h"

static void draw_background( struct osd_bitmap *bitmap ) {
    int offs;
    const struct GfxElement *gfx = Machine->gfx[1];

    for( offs = 0; offs < videoram_size/2; offs++ )
	{
        if( dirtybuffer[offs] ) {
            int data = videoram16[offs];
            int numtile = ( data & 0xfff );
            int color = ( data & 0xf000 ) >> 12;
            int sx = ( offs % 16 ) * 16;
            int sy = ( offs / 16 ) * 16;

            dirtybuffer[offs] = 0;

            drawgfx( tmpbitmap,gfx,
                numtile,
                color,
                0,0,
                sx,sy,
                0,TRANSPARENCY_NONE,0);
        }
    }

    copybitmap( bitmap, tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0 );
}

static void draw_text( struct osd_bitmap *bitmap ) {
    int offs;
    const struct rectangle *clip = &Machine->visible_area;

    for ( offs = 0; offs < 0x400; offs++) {
        unsigned short data = colorram16[offs];
        int tile_number = data&0x3ff;

        if ( tile_number != 0xd ) {
            int color = data>>10;
            int sx = 8 * ( offs % 32 );
            int sy = 8 * ( offs / 32 );

            drawgfx( bitmap,Machine->gfx[0],
                tile_number,
                color,
                0,0, /* no flip */
                sx,sy,
                clip,TRANSPARENCY_PEN,0x3);
        }
    }
}

static void draw_sprites( struct osd_bitmap *bitmap ){
    const struct rectangle *clip = &Machine->visible_area;
    const struct GfxElement *gfx = Machine->gfx[2];
    int offs;

    for( offs = spriteram_size/2 - 4; offs >= 0; offs -= 4 ) {
        int data0 = spriteram16[offs];
        int data1 = spriteram16[offs+1];
        int data2 = spriteram16[offs+2];
//      int data3 = spriteram16[offs+3];

        /*
            -------E YYYYYYYY
            ----BBTT TTTTTTTT
            -CCCCF-X XXXXXXXX
            -------- --------
        */
        if( data0 & 0x100 ) {
            int tile_number = data1 & 0xfff;
            int color   = ( data2 & 0x7800 ) >> 11;
            int sy = ( data0 & 0xff );
            int sx = ( data2 & 0x1ff );
            int hflip = ( data2 & 0x0400 );

            if ( sx > 256 )
                sx -= 512;

            drawgfx( bitmap,gfx,
                tile_number,
                color,
                hflip,0,
                sx,sy,
                clip,TRANSPARENCY_PEN,0xf );
        }
    }
}

void cabal_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh )
{
    if (palette_recalc())
        memset(dirtybuffer, 1, videoram_size/2);

    draw_background( bitmap );
    draw_sprites( bitmap );
    draw_text( bitmap );
}
