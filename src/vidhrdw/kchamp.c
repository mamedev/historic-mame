/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* prototypes */
void kchamp_vs_drawsprites( struct mame_bitmap *bitmap );
void kchamp_1p_drawsprites( struct mame_bitmap *bitmap );



typedef void (*kchamp_drawspritesproc)( struct mame_bitmap * );

static kchamp_drawspritesproc kchamp_drawsprites;


/***************************************************************************
  Video hardware start.
***************************************************************************/

VIDEO_START( kchampvs ) {

	kchamp_drawsprites = kchamp_vs_drawsprites;

	return video_start_generic();
}

VIDEO_START( kchamp1p ) {

	kchamp_drawsprites = kchamp_1p_drawsprites;

	return video_start_generic();
}

/***************************************************************************
  Convert color prom.
***************************************************************************/
PALETTE_INIT( kchamp )
{
	int i, red, green, blue;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		red = color_prom[i];
		green = color_prom[Machine->drv->total_colors+i];
		blue = color_prom[2*Machine->drv->total_colors+i];

		palette_set_color(i,red*0x11,green*0x11,blue*0x11);

		*(colortable++) = i;
	}

}

void kchamp_vs_drawsprites( struct mame_bitmap *bitmap ) {

	int offs;
	        /*
                Sprites
                -------
                Offset          Encoding
                  0             YYYYYYYY
                  1             TTTTTTTT
                  2             FGGTCCCC
                  3             XXXXXXXX
        */

        for (offs = 0 ;offs < 0x100;offs+=4)
	{
                int numtile = spriteram[offs+1] + ( ( spriteram[offs+2] & 0x10 ) << 4 );
                int flipx = ( spriteram[offs+2] & 0x80 );
                int sx, sy;
                int gfx = 1 + ( ( spriteram[offs+2] & 0x60 ) >> 5 );
                int color = ( spriteram[offs+2] & 0x0f );

                sx = spriteram[offs+3];
                sy = 240 - spriteram[offs];

                drawgfx(bitmap,Machine->gfx[gfx],
                                numtile,
                                color,
                                0, flipx,
                                sx,sy,
                                &Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void kchamp_1p_drawsprites( struct mame_bitmap *bitmap ) {

	int offs;
	        /*
                Sprites
                -------
                Offset          Encoding
                  0             YYYYYYYY
                  1             TTTTTTTT
                  2             FGGTCCCC
                  3             XXXXXXXX
        */

        for (offs = 0 ;offs < 0x100;offs+=4)
	{
                int numtile = spriteram[offs+1] + ( ( spriteram[offs+2] & 0x10 ) << 4 );
                int flipx = ( spriteram[offs+2] & 0x80 );
                int sx, sy;
                int gfx = 1 + ( ( spriteram[offs+2] & 0x60 ) >> 5 );
                int color = ( spriteram[offs+2] & 0x0f );

                sx = spriteram[offs+3] - 8;
                sy = 247 - spriteram[offs];

                drawgfx(bitmap,Machine->gfx[gfx],
                                numtile,
                                color,
                                0, flipx,
                                sx,sy,
                                &Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( kchamp )
{
        int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
        for ( offs = videoram_size - 1; offs >= 0; offs-- ) {
                if ( dirtybuffer[offs] ) {
			int sx,sy,code;

			dirtybuffer[offs] = 0;

                        sx = (offs % 32);
			sy = (offs / 32);

                        code = videoram[offs] + ( ( colorram[offs] & 7 ) << 8 );

                        drawgfx(tmpbitmap,Machine->gfx[0],
                                        code,
                                        ( colorram[offs] >> 3 ) & 0x1f,
                                        0, /* flip x */
                                        0, /* flip y */
					sx*8,sy*8,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	(*kchamp_drawsprites)( bitmap);
}
