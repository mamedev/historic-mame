/***************************************************************************

	Atari Dominos hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *dominos_sound_ram;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( dominos_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] & 0x3f;
	int color = (videoram[tile_index] & 0x80) >> 7;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( dominos )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( dominos )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	/* The video circuitry updates our sound registers. */
	discrete_sound_w(0, dominos_sound_ram[0] % 16);	// Freq
	discrete_sound_w(1, dominos_sound_ram[2] % 16);	// Amp
}
