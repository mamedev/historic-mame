/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


/* in sndhrdw/pleiads.c */
WRITE_HANDLER( pleiads_sound_control_c_w );

static unsigned char *videoram_pg1;
static unsigned char *videoram_pg2;
static unsigned char *current_videoram_pg;
static int current_videoram_pg_index;
static int fg_palette_bank, bg_palette_bank;
static int protection_question;
static struct tilemap *fg_tilemap, *bg_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Phoenix has two 256x4 palette PROMs, one containing the high bits and the
  other the low bits (2x2x2 color space).
  The palette PROMs are connected to the RGB output this way:

  bit 3 --
        -- 270 ohm resistor  -- GREEN
        -- 270 ohm resistor  -- BLUE
  bit 0 -- 270 ohm resistor  -- RED

  bit 3 --
        -- GREEN
        -- BLUE
  bit 0 -- RED

  plus 270 ohm pullup and pulldown resistors on all lines

***************************************************************************/

void phoenix_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;

		color_prom++;
	}

	/* first bank of characters use colors 0-31 and 64-95 */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 2;j++)
		{
			COLOR(0,4*i + j*4*8) = i + j*64;
			COLOR(0,4*i + j*4*8 + 1) = 8 + i + j*64;
			COLOR(0,4*i + j*4*8 + 2) = 2*8 + i + j*64;
			COLOR(0,4*i + j*4*8 + 3) = 3*8 + i + j*64;
		}
	}

	/* second bank of characters use colors 32-63 and 96-127 */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 2;j++)
		{
			COLOR(1,4*i + j*4*8) = i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 1) = 8 + i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 2) = 2*8 + i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 3) = 3*8 + i + 32 + j*64;
		}
	}
}

void pleiads_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;

		color_prom++;
	}

	/* first bank of characters use colors 0x00-0x1f, 0x40-0x5f, 0x80-0x9f and 0xc0-0xdf */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 4;j++)
		{
			COLOR(0,4*i + j*4*8 + 0) = 0*8 + i + (3-j)*64;
			COLOR(0,4*i + j*4*8 + 1) = 1*8 + i + (3-j)*64;
			COLOR(0,4*i + j*4*8 + 2) = 2*8 + i + (3-j)*64;
			COLOR(0,4*i + j*4*8 + 3) = 3*8 + i + (3-j)*64;
		}
	}

	/* second bank of characters use colors 0x20-0x3f, 0x60-0x7f, 0xa0-0xbf and 0xe0-0xff */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 4;j++)
		{
			COLOR(1,4*i + j*4*8 + 0) = 0*8 + i + 32 + (3-j)*64;
			COLOR(1,4*i + j*4*8 + 1) = 1*8 + i + 32 + (3-j)*64;
			COLOR(1,4*i + j*4*8 + 2) = 2*8 + i + 32 + (3-j)*64;
			COLOR(1,4*i + j*4*8 + 3) = 3*8 + i + 32 + (3-j)*64;
		}
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code;

	code = current_videoram_pg[tile_index];
	SET_TILE_INFO(1, code, (code >> 5) | (fg_palette_bank << 3));
	tile_info.flags = (tile_index & 0x1f) ? 0 : TILE_IGNORE_TRANSPARENCY;	/* first row (column) is opaque */
}

static void get_bg_tile_info(int tile_index)
{
	int code;

	code = current_videoram_pg[tile_index + 0x800];
	SET_TILE_INFO(0, code, (code >> 5) | (bg_palette_bank << 3));
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int phoenix_vh_start(void)
{
	if ((videoram_pg1 = malloc(0x1000)) == 0)
		return 1;

	if ((videoram_pg2 = malloc(0x1000)) == 0)
		return 1;

    current_videoram_pg_index = -1;
	current_videoram_pg = videoram_pg1;		/* otherwise, hiscore loading crashes */


	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,32,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void phoenix_vh_stop(void)
{
	free(videoram_pg1);
	free(videoram_pg2);

	videoram_pg1 = 0;
	videoram_pg2 = 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( phoenix_videoram_r )
{
	return current_videoram_pg[offset];
}

WRITE_HANDLER( phoenix_videoram_w )
{
	current_videoram_pg[offset] = data;

	if ((offset & 0x7ff) < 0x340)
	{
		if (offset & 0x800)
			tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
		else
			tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
	}
}


WRITE_HANDLER( phoenix_videoreg_w )
{
    if (current_videoram_pg_index != (data & 1))
	{
		/* set memory bank */
		current_videoram_pg_index = data & 1;
		current_videoram_pg = current_videoram_pg_index ? videoram_pg2 : videoram_pg1;

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* Phoenix has only one palette select effecting both layers */
	if (fg_palette_bank != ((data >> 1) & 1))
	{
		fg_palette_bank = bg_palette_bank = (data >> 1) & 1;

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( pleiads_videoreg_w )
{
    if (current_videoram_pg_index != (data & 1))
	{
		/* set memory bank */
		current_videoram_pg_index = data & 1;
		current_videoram_pg = current_videoram_pg_index ? videoram_pg2 : videoram_pg1;

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* Pleiads has seperate palette selects for each layer */
    if (bg_palette_bank != ((data >> 2) & 1))
	{
		bg_palette_bank = (data >> 2) & 1;

		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	if (fg_palette_bank != ((data >> 3) & 1))
	{
		fg_palette_bank = (data >> 3) & 1;

		tilemap_mark_all_tiles_dirty(fg_tilemap);
	}

	protection_question = data & 0xfc;

	/* send two bits to sound control C (not sure if they are there) */
	pleiads_sound_control_c_w(offset, data);
}


WRITE_HANDLER( phoenix_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap,0,data);
}


READ_HANDLER( pleiads_input_port_0_r )
{
	int ret = input_port_0_r(0) & 0xf7;

	/* handle Pleiads protection */
	switch (protection_question)
	{
	case 0x00:
	case 0x20:
		/* Bit 3 is 0 */
		break;
	case 0x0c:
	case 0x30:
		/* Bit 3 is 1 */
		ret	|= 0x08;
		break;
	default:
		logerror("Unknown protection question %02X at %04X\n", protection_question, cpu_get_pc());
	}

	return ret;
}


/***************************************************************************

  Display refresh

***************************************************************************/

void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0,0);
	tilemap_draw(bitmap,fg_tilemap,0,0);
}
