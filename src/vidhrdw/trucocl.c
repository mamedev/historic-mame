/***************************************************************************

Truco Clemente (c) 1991 Miky SRL

driver by Ernesto Corvi

Notes:
- Wrong colors.
- Audio is almost there.
- I think this runs on a heavily modified PacMan type of board.

----------------------------------
Additional Notes (Roberto Fresca):
----------------------------------
Mainboard: Pacman bootleg jamma board.
Daughterboard: Custom made, plugged in the 2 roms and Z80 mainboard sockets.

  - 01 x Z80
  - 03 x 27c010
  - 02 x am27s19
  - 03 x GAL 16v8b      (All of them have the same contents... Maybe read protected.)
  - 01 x PAL CE 20v8h   (The fuse map is suspect too)
  - 01 x lm324n

  To not overload the driver, I put the rest of technical info in
  http://www.mameworld.net/robbie/trucocl.htm

- Added 2 "hidden" color proms (am27s19)
- One GAL is connected to the color proms inputs.
- Hooked color decode routines, but still wrong due to the GAL issue.
- The name of the company is "Miky SRL" instead of "Caloi Miky SRL".
  Caloi (Carlos Loiseau), is the Clemente's creator.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static tilemap *bg_tilemap;

PALETTE_INIT( trucocl )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
}

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
