/***************************************************************************

Atari Super Bug video emulation

***************************************************************************/

#include "driver.h"

UINT8* superbug_alpha_num_ram;
UINT8* superbug_playfield_ram;

int superbug_car_rot;
int superbug_skid_in;
int superbug_crash_in;
int superbug_arrow_off;
int superbug_flash;

static struct mame_bitmap* helper;

static const int car_x = 112;
static const int car_y = 104;

static struct tilemap* tilemap;


static UINT32 get_memory_offset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return num_cols * row + col;
}


static void get_tile_info(int tile_index)
{
	UINT8 code = superbug_playfield_ram[tile_index];

	int color = code >> 6;

	if (superbug_arrow_off && (code & 0x18) == 0x08)
	{
		color = 0;
	}
	if (superbug_flash)
	{
		color |= 4;
	}

	/* we need bits #3 and #4 for collision detection */

	color |= code & 0x18;

	SET_TILE_INFO(1, code & 0x3f, color, 0)
}


WRITE_HANDLER( superbug_vert_w )
{
	tilemap_set_scrolly(tilemap, 0, data);
}

WRITE_HANDLER( superbug_horz_w )
{
	tilemap_set_scrollx(tilemap, 0, data);
}


VIDEO_START( superbug )
{
	helper = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	if (helper == NULL)
		return 1;

	tilemap = tilemap_create(get_tile_info, get_memory_offset, TILEMAP_OPAQUE, 16, 16, 16, 16);

	if (tilemap == NULL)
		return 1;

	return 0;
}


VIDEO_UPDATE( superbug )
{
	int x;
	int y;

	tilemap_mark_all_tiles_dirty(tilemap);

	/* draw playfield */

	tilemap_draw(bitmap, cliprect, tilemap, 16, 0);

	/* draw sprite to auxiliary bitmap */

	{
		int number = superbug_car_rot & 0x03;
		int flip_x = superbug_car_rot & 0x04;
		int flip_y = superbug_car_rot & 0x08;
		int mirror = superbug_car_rot & 0x10;

		drawgfx(helper, Machine->gfx[mirror ? 2 : 3], number ^ 3, superbug_flash ? 1 : 0,
			flip_x, flip_y, car_x, car_y, cliprect, TRANSPARENCY_NONE, 0);
	}

	/* check for collisions and copy sprite */

	for (x = car_x; x < car_x + 32; x++)
	{
		for (y = car_y; y < car_y + 32; y++)
		{
			pen_t a = read_pixel(helper, x, y);
			pen_t b = read_pixel(bitmap, x, y);

			if (a & 1)
			{
				if ((b & 0x21) == 0x20)
				{
					superbug_crash_in = 1;
				}
				if ((b & 0x31) == 0x00)
				{
					superbug_skid_in = 1;
				}

				plot_pixel(bitmap, x, y, a);
			}
		}
	}

	/* draw alpha numerics */

	for (x = 0; x < 2; x++)
	{
		for (y = 0; y < 16; y++)
		{
			int code = superbug_alpha_num_ram[y + 16 * x];

			drawgfx(bitmap, Machine->gfx[0], code & 31, 0, 0, 0,
				x == 0 ? 240 : 0, 16 * y, cliprect, TRANSPARENCY_NONE, 0);
		}
	}
}
