/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* prototypes */
void kchamp_vs_draw_sprites( struct mame_bitmap *bitmap );
void kchamp_1p_draw_sprites( struct mame_bitmap *bitmap );

typedef void (*kchamp_draw_spritesproc)( struct mame_bitmap * );

static kchamp_draw_spritesproc kchamp_draw_sprites;

static struct tilemap *bg_tilemap;

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

WRITE_HANDLER( kchamp_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( kchamp_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((colorram[tile_index] & 7) << 8);
	int color = (colorram[tile_index] >> 3) & 0x1f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( kchampvs )
{
	kchamp_draw_sprites = kchamp_vs_draw_sprites;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_START( kchamp1p )
{
	kchamp_draw_sprites = kchamp_1p_draw_sprites;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

void kchamp_vs_draw_sprites( struct mame_bitmap *bitmap ) {

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

void kchamp_1p_draw_sprites( struct mame_bitmap *bitmap ) {

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

VIDEO_UPDATE( kchamp )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	(*kchamp_draw_sprites)(bitmap);
}
