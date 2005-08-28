/***************************************************************************

Truco Clemente (c) 1991 Caloi Miky SRL

driver by Ernesto Corvi

Notes:
- Wrong colors.
- Audio is almost there.
- I think this runs on a heavily modified PacMan type of board.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static tilemap *bg_tilemap;

WRITE8_HANDLER( trucocl_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( trucocl_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int gfxsel = colorram[tile_index] & 1;
	int bank = ( ( colorram[tile_index] >> 2 ) & 0x07 );
	int code = videoram[tile_index];

	code |= ( bank & 1 ) << 10;
	code |= ( bank & 2 ) << 8;
	code += ( bank & 4 ) << 6;

	SET_TILE_INFO(gfxsel,code,0,0)
}

VIDEO_START( trucocl )
{
	bg_tilemap = tilemap_create( get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 8, 32, 32 );

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( trucocl )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
}
