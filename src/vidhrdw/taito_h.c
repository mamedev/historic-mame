/***************************************************************************

Functions to emulate the video hardware of the machine.


Scroll RAM
--------------------------------------------------------------
 +0         +1         +2         +3
 ---- xx--  ---- ----  ---- ----  ---- ---- = screen invert
 ---- --xx  ---- ----  ---- ----  ---- ---- = unknown (always set)
 ---- ----  ---- ----  ---- --xx  xxxx xxxx = BG0 scroll x

 +4         +5         +6         +7
 ---- --xx  xxxx xxxx  ---- ----  ---- ---- = BG1 scroll x
 ---- ----  ---- ----  ---- --xx  xxxx xxxx = BG0 scroll y

 +8         +9         +A         +B
 ---- --xx  xxxx xxxx  ---- ----  ---- ---- = BG1 scroll y
 ---- ----  ---- ----  xxxx xxxx  xxxx xxxx = unknown (Syvalion and Recordbr)

 +C         +D         +E         +F
 -xxx xxxx  ---- ----  ---- ----  ---- ---- = BG0 zoom x (*)
 ---- ----  xxxx xxxx  ---- ----  ---- ---- = BG0 zoom y (*)
 ---- ----  ---- ----  -xxx xxxx  ---- ---- = BG1 zoom x (*)
 ---- ----  ---- ----  ---- ----  xxxx xxxx = BG1 zoom y (*)

(*) BG0 and BG1 zoom x :  0x00 (x1/2)? - 0x3f (x1) - 0x7f (x2)?
    BG0 and BG1 zoom y :  0x00 (x1/4)? - 0x7f (x1) - 0xff (x4)?


BG / chain RAM
-----------------------------------------
[0]  +0         +1
     -xxx xxxx  xxxx xxxx = tile number

[1]  +0         +1
     ---- ----  x--- ---- = flip Y
     ---- ----  -x-- ---- = flip X
     ---- ----  --xx xxxx = color


sprite RAM
--------------------------------------------------------------
 +0         +1         +2         +3
 ---x ----  ---- ----  ---- ----  ---- ---- = unknown
 ---- xx--  ---- ----  ---- ----  ---- ---- = chain y size
 ---- --xx  xxxx xxxx  ---- ----  ---- ---- = sprite x coords
 ---- ----  ---- ----  ---- --xx  xxxx xxxx = sprite y coords

 +4         +5         +6         +7
 --xx xxxx  ---- ----  ---- ----  ---- ---- = zoom x
 ---- ----  --xx xxxx  ---- ----  ---- ---- = zoom y
 ---- ----  ---- ----  ---x xxxx  xxxx xxxx = tile information offset


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"


data16_t	*taitoh_tx_videoram_0;
data16_t	*taitoh_tx_videoram_1;
data16_t	*taitoh_tx_charram;
data16_t	*taitoh_bg0_videoram_0;
data16_t	*taitoh_bg1_videoram_0;
data16_t	*taitoh_bg0_videoram_1;
data16_t	*taitoh_bg1_videoram_1;
data16_t	*taitoh_scrollram;
data16_t	*taitoh_chainram_0;
data16_t	*taitoh_chainram_1;
data16_t	*taitoh_spriteram;

static data16_t		taitoh_bg0_scrollx = 0;
static data16_t		taitoh_bg1_scrollx = 0;
static data16_t		taitoh_bg0_scrolly = 0;
static data16_t		taitoh_bg1_scrolly = 0;

static struct tilemap *tx_tilemap  = NULL;
static struct tilemap *bg0_tilemap = NULL;
static struct tilemap *bg1_tilemap = NULL;

static int		taitoh_tx_char_dirty = 0;
static int		flipscreen = 0;


/* These are hand-tuned value. */
static int zoomy_conv_table[] =
{
/*    +0   +1   +2   +3   +4   +5   +6   +7    +8   +9   +a   +b   +c   +d   +e   +f */
	0x00,0x01,0x01,0x02,0x02,0x03,0x04,0x05, 0x06,0x06,0x07,0x08,0x09,0x0a,0x0a,0x0b,	/* 0x00 */
	0x0b,0x0c,0x0c,0x0d,0x0e,0x0e,0x0f,0x10, 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x16,
	0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e, 0x1f,0x20,0x21,0x22,0x24,0x25,0x26,0x27,
	0x28,0x2a,0x2b,0x2c,0x2e,0x30,0x31,0x32, 0x34,0x36,0x37,0x38,0x3a,0x3c,0x3e,0x3f,

	0x40,0x41,0x42,0x42,0x43,0x43,0x44,0x44, 0x45,0x45,0x46,0x46,0x47,0x47,0x48,0x49,	/* 0x40 */
	0x4a,0x4a,0x4b,0x4b,0x4c,0x4d,0x4e,0x4f, 0x4f,0x50,0x51,0x51,0x52,0x53,0x54,0x55,
	0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d, 0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x66,
	0x67,0x68,0x6a,0x6b,0x6c,0x6e,0x6f,0x71, 0x72,0x74,0x76,0x78,0x80,0x7b,0x7d,0x7f
};


/***************************************************************************
  Callbacks for the tilemap code
***************************************************************************/

static void get_bg0_tile_info_0(int tile_index)
{
	int color, tile;

	color = taitoh_bg0_videoram_1[ tile_index ] & 0x001f;
	tile  = taitoh_bg0_videoram_0[ tile_index ] & 0x7fff;

	tile_info.flags = 0;
	if ( taitoh_bg0_videoram_1[ tile_index ] & 0x0080 )
		tile_info.flags |= TILE_FLIPY;
	if ( taitoh_bg0_videoram_1[ tile_index ] & 0x0040 )
		tile_info.flags |= TILE_FLIPX;
	tile_info.priority = 0;

	SET_TILE_INFO(0, tile, color)
}

static void get_bg1_tile_info_0(int tile_index)
{
	int color, tile;

	color = taitoh_bg1_videoram_1[ tile_index ] & 0x001f;
	tile  = taitoh_bg1_videoram_0[ tile_index ] & 0x7fff;

	tile_info.flags = 0;
	if ( taitoh_bg1_videoram_1[ tile_index ] & 0x0080 )
		tile_info.flags |= TILE_FLIPY;
	if ( taitoh_bg1_videoram_1[ tile_index ] & 0x0040 )
		tile_info.flags |= TILE_FLIPX;
	tile_info.priority = 0;

	SET_TILE_INFO(0, tile, color)
}

static void get_tx_tile_info(int tile_index)
{
	/* Only Syvalion has text layer */
	int tile;

	if (!flipscreen)
	{
		if ( (tile_index & 1) )
			tile = (taitoh_tx_videoram_0[tile_index >> 1] & 0x00ff);
		else
			tile = (taitoh_tx_videoram_0[tile_index >> 1] & 0xff00) >> 8;
		tile_info.priority = 0;
	}
	else
	{
		if ( (tile_index & 1) )
			tile = (taitoh_tx_videoram_0[tile_index >> 1] & 0xff00) >> 8;
		else
			tile = (taitoh_tx_videoram_0[tile_index >> 1] & 0x00ff);
		tile_info.priority = 0;
	}

	SET_TILE_INFO(1, tile, 0)
}


/***************************************************************************
  Initialize and destroy video hardware emulation
***************************************************************************/

int syvalion_vh_start(void)
{
	taitoh_tx_charram = (data16_t *)malloc( 0x2000 );
	if ( taitoh_tx_charram == NULL )
		return 1;
	memset( taitoh_tx_charram, 0, 0x2000 );

	/*                           info                 offset             type                 w   h  col  row */
	bg0_tilemap = tilemap_create(get_bg0_tile_info_0, tilemap_scan_rows, TILEMAP_OPAQUE,      16, 16, 64, 64);
	bg1_tilemap = tilemap_create(get_bg1_tile_info_0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64, 64);
	tx_tilemap  = tilemap_create(get_tx_tile_info,    tilemap_scan_rows, TILEMAP_TRANSPARENT,  8,  8, 64, 64);

	if ( !bg0_tilemap || !bg1_tilemap || !tx_tilemap )
	{
		free( taitoh_tx_charram );
		return 1;
	}

	tilemap_set_transparent_pen( tx_tilemap,  0 );
	tilemap_set_transparent_pen( bg0_tilemap, 0 );
	tilemap_set_transparent_pen( bg1_tilemap, 0 );

	return 0;
}

int recordbr_vh_start(void)
{
	bg0_tilemap = tilemap_create(get_bg0_tile_info_0, tilemap_scan_rows, TILEMAP_OPAQUE,      16, 16, 64, 64);
	bg1_tilemap = tilemap_create(get_bg1_tile_info_0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64, 64);

	if ( !bg0_tilemap || !bg1_tilemap )
	{
		return 1;
	}

	tilemap_set_transparent_pen( bg0_tilemap, 0 );
	tilemap_set_transparent_pen( bg1_tilemap, 0 );

	return 0;
}

void syvalion_vh_stop(void)
{
	free( taitoh_tx_charram );
	return;
}


/***************************************************************************
  Memory handler
***************************************************************************/

READ16_HANDLER( taitoh_tx_charram_0_r )
{
	return taitoh_tx_charram[ offset ];
}

READ16_HANDLER( taitoh_tx_charram_1_r )
{
	return taitoh_tx_charram[ offset + 0x0800 ];
}

READ16_HANDLER( taitoh_tx_videoram_0_r )
{
	return taitoh_tx_videoram_0[ offset ];
}

READ16_HANDLER( taitoh_tx_videoram_1_r )
{
	return taitoh_tx_videoram_1[ offset ];
}

READ16_HANDLER( taitoh_bg0_videoram_0_r )
{
	return taitoh_bg0_videoram_0[ offset ];
}

READ16_HANDLER( taitoh_bg1_videoram_0_r )
{
	return taitoh_bg1_videoram_0[ offset ];
}

READ16_HANDLER( taitoh_bg0_videoram_1_r )
{
	return taitoh_bg0_videoram_1[ offset ];
}

READ16_HANDLER( taitoh_bg1_videoram_1_r )
{
	return taitoh_bg1_videoram_1[ offset ];
}

READ16_HANDLER( taitoh_scrollram_r )
{
	return taitoh_scrollram[ offset ];
}

WRITE16_HANDLER( taitoh_tx_charram_0_w )
{
	data16_t oldword = taitoh_tx_charram[ offset ];

	if ( oldword != data )
	{
		taitoh_tx_char_dirty = 1;
		COMBINE_DATA(&taitoh_tx_charram[ offset ]);

	}
}

WRITE16_HANDLER( taitoh_tx_charram_1_w )
{
	data16_t oldword = taitoh_tx_charram[ offset + 0x800 ];

	if ( oldword != data )
	{
		taitoh_tx_char_dirty = 1;
		COMBINE_DATA(&taitoh_tx_charram[ offset + 0x800 ]);
	}
}

WRITE16_HANDLER( taitoh_tx_videoram_0_w )
{
	data16_t oldword = taitoh_tx_videoram_0[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_tx_videoram_0[offset]);
		tilemap_mark_tile_dirty( tx_tilemap, offset << 1 );
		tilemap_mark_tile_dirty( tx_tilemap, (offset << 1) + 1 );
	}
}

WRITE16_HANDLER( taitoh_tx_videoram_1_w )
{
	data16_t oldword = taitoh_tx_videoram_1[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_tx_videoram_1[ offset ]);
		/* TX RAM in this region is not revealed what to do. */
		/* So, currently tilemap updates is not implemented  */
		/* for speed.                                        */
//		tilemap_mark_tile_dirty( tx_tilemap, offset << 1 );
//		tilemap_mark_tile_dirty( tx_tilemap, (offset << 1) + 1 );
	}
}

WRITE16_HANDLER( taitoh_bg0_videoram_0_w )
{
	data16_t oldword = taitoh_bg0_videoram_0[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_bg0_videoram_0[ offset ]);
		tilemap_mark_tile_dirty( bg0_tilemap, offset );
	}
}

WRITE16_HANDLER( taitoh_bg1_videoram_0_w )
{
	data16_t oldword = taitoh_bg1_videoram_0[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_bg1_videoram_0[ offset ]);
		tilemap_mark_tile_dirty( bg1_tilemap, offset );
	}
}

WRITE16_HANDLER( taitoh_bg0_videoram_1_w )
{
	data16_t oldword = taitoh_bg0_videoram_1[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_bg0_videoram_1[ offset ]);
		tilemap_mark_tile_dirty( bg0_tilemap, offset );
	}
}

WRITE16_HANDLER( taitoh_bg1_videoram_1_w )
{
	data16_t oldword = taitoh_bg1_videoram_1[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_bg1_videoram_1[ offset ]);
		tilemap_mark_tile_dirty( bg1_tilemap, offset );
	}
}

WRITE16_HANDLER( taitoh_scrollram_w )
{
	/* Visible area of the screen is different between flip and normal screens. */
	/* So, it is needed to compensate scroll value in both X and Y coords.      */

	data16_t oldword = taitoh_scrollram[ offset ];

	if ( oldword != data )
	{
		COMBINE_DATA(&taitoh_scrollram[ offset ]);

		switch ( offset )
		{
			case 0x00:				/* screen invert */
				flipscreen = data & 0x0c00;
				tilemap_set_flip( ALL_TILEMAPS, flipscreen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0 );
				if (flipscreen)
				{
					if (tx_tilemap)
						tilemap_set_scrolly( tx_tilemap, 0, -48 + 496 );
					tilemap_set_scrollx( bg0_tilemap, 0, -taitoh_bg0_scrollx - 511 );
					tilemap_set_scrollx( bg1_tilemap, 0, -taitoh_bg1_scrollx - 511 );
					tilemap_set_scrolly( bg0_tilemap, 0, taitoh_bg0_scrolly + 3);
					tilemap_set_scrolly( bg1_tilemap, 0, taitoh_bg1_scrolly + 3);
				}
				else
				{
					if (tx_tilemap)
						tilemap_set_scrolly( tx_tilemap, 0, -48 );
					tilemap_set_scrollx( bg0_tilemap, 0, -taitoh_bg0_scrollx + 1);
					tilemap_set_scrollx( bg1_tilemap, 0, -taitoh_bg1_scrollx + 1);
					tilemap_set_scrolly( bg0_tilemap, 0, taitoh_bg0_scrolly + 1);
					tilemap_set_scrolly( bg1_tilemap, 0, taitoh_bg1_scrolly + 1);
				}
				break;

			case 0x01:				/* BG0 scroll X */
				taitoh_bg0_scrollx = (!flipscreen) ? ((data & 0x03ff) + 1) : ((data & 0x3ff) + 512);
				tilemap_set_scrollx( bg0_tilemap, 0, -taitoh_bg0_scrollx );
				break;

			case 0x02:				/* BG1 scroll X */
				taitoh_bg1_scrollx = (!flipscreen) ? ((data & 0x03ff) + 1) : ((data & 0x3ff) + 512);
				tilemap_set_scrollx( bg1_tilemap, 0, -taitoh_bg1_scrollx );
				break;

			case 0x03:				/* BG0 scroll Y */
				taitoh_bg0_scrolly = (!flipscreen) ? ((data & 0x03ff) - 1) : ((data & 0x3ff) + 2);
				tilemap_set_scrolly( bg0_tilemap, 0, taitoh_bg0_scrolly );
				break;

			case 0x04:				/* BG1 scroll Y */
				taitoh_bg1_scrolly = (!flipscreen) ?  ((data & 0x03ff) - 1) : ((data & 0x3ff) + 2);
				tilemap_set_scrolly( bg1_tilemap, 0, taitoh_bg1_scrolly );
				break;

			default:
				break;
		}
	}
}


/***************************************************************************
  Screen refresh
***************************************************************************/

static void mark_sprite_colors(void)
{
	unsigned short palette_map[32];

	int size[] = { 1, 2, 4, 4 };
	int rx, ry;
	int offs;					/* sprite RAM offset */
	int tile_offs;				/* sprite chain offset */
	int i;

	memset (palette_map, 0x00, sizeof (palette_map));

	/* Find colors used in the sprites */
	for (offs = 0x03f8 / 2 ; offs >= 0 ; offs -= 0x008 / 2)
	{
		tile_offs = (taitoh_spriteram[offs + 3] & 0x1fff) << 2;

		for (ry = 0 ; ry < size[ ( taitoh_spriteram[ offs ] & 0x0c00 ) >> 10 ] ; ry ++)
		{
			for (rx = 0 ; rx < 4 ; rx ++)
			{
				if ((!tx_tilemap || tile_offs >= 0x1000) && tile_offs < 0x6000)
				{
					int tile, color;

					if (tx_tilemap)			/* check if exist text layer */
					{
						tile  = taitoh_chainram_0[ tile_offs - 0x1000 ] & 0x7fff;
						color = taitoh_chainram_1[ tile_offs - 0x1000 ] & 0x001f;
					}
					else
					{
						tile  = taitoh_chainram_0[ tile_offs ] & 0x7fff;
						color = taitoh_chainram_1[ tile_offs ] & 0x001f;
					}

					palette_map[color] |= Machine -> gfx[0] -> pen_usage[tile];
				}
				tile_offs ++;
			}
		}
	}

	/* Tell the palette system about used colors */
	for (i = 0 ; i < 32 ; i ++)
	{
		int j;

		palette_used_colors[i * 16] = PALETTE_COLOR_USED;
		if (palette_map[i])
		{
			for (j = 1 ; j < 16 ; j ++)
			{
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
			}
		}

	}
}

static void syvalion_draw_sprites(struct osd_bitmap *bitmap)
{
	/* Y chain size is 16/32?/64/64? pixels. X chain size is always 64 pixels. */
	int size[] = { 1, 2, 4, 4 };
	int x0, y0, x, y, dx, ex, zx;
	int ysize;
	int j, k;
	int offs;					/* sprite RAM offset */
	int tile_offs;				/* sprite chain offset */
	int zoomx;					/* zoomx value */

	for (offs = 0x03f8 / 2 ; offs >= 0 ; offs -= 0x008 / 2)
	{
		x0        = (taitoh_spriteram[offs + 1]) & 0x3ff;
		y0        = (taitoh_spriteram[offs + 0]) & 0x3ff;
		zoomx     = (taitoh_spriteram[offs + 2] & 0x7f00) >> 8;
		tile_offs = (taitoh_spriteram[offs + 3] & 0x1fff) << 2;
		ysize     = size[ ( taitoh_spriteram[ offs ] & 0x0c00 ) >> 10 ];

		tile_offs -= 0x1000;

		/* The increasing ratio of expansion is different whether zoom value */
		/* is less or more than 63.                                          */
		if (zoomx < 63)
		{
			dx = 8 + (zoomx + 2) / 8;
			ex = (zoomx + 2) % 8;
			zx = ((dx << 1) + ex) << 11;
		}
		else
		{
			dx = 16 + (zoomx - 63) / 4;
			ex = (zoomx - 63) % 4;
			zx = (dx + ex) << 12;
		}

		if (x0 >= 0x200) x0 -= 0x400;
		if (y0 >= 0x200) y0 -= 0x400;

		if (flipscreen)
		{
			x0 = 497 - x0;
			y0 = 498 - y0;
			dx = -dx;
		}
		else
		{
			x0 += 1;
			y0 += 2;
		}

		y = y0;
		for ( j = 0 ; j < ysize ; j ++ )
		{
			x = x0;
			for (k = 0 ; k < 4 ; k ++ )
			{
				if (tile_offs < 0x5000)
				{
					int tile, color, flipx, flipy;

					tile  = taitoh_chainram_0[ tile_offs ] & 0x7fff;
					color = taitoh_chainram_1[ tile_offs ] & 0x001f;
					flipx = taitoh_chainram_1[ tile_offs ] & 0x0040;
					flipy = taitoh_chainram_1[ tile_offs ] & 0x0080;

					if (flipscreen)
					{
						flipx ^= 0x0040;
						flipy ^= 0x0080;
					}

					drawgfxzoom( bitmap, Machine -> gfx[0],
								 tile,
								 color,
								 flipx, flipy,
								 x, y,
								 &Machine->visible_area,
								 TRANSPARENCY_PEN, 0,
								 zx, zx
					);
				}
				tile_offs ++;
				x += dx;
			}
			y += dx;
		}
	}
}

static void recordbr_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	/* Y chain size is 16/32?/64/64? pixels. X chain size is always 64 pixels. */
	int size[] = { 1, 2, 4, 4 };
	int x0, y0, x, y, dx, dy, ex, ey, zx, zy;
	int ysize;
	int j, k;
	int offs;					/* sprite RAM offset */
	int tile_offs;				/* sprite chain offset */
	int zoomx, zoomy;			/* zoom value */

	for (offs = 0x03f8 / 2 ; offs >= 0 ; offs -= 0x008 / 2)
	{
		if (offs <  0x01b0 && priority == 0)	continue;
		if (offs >= 0x01b0 && priority == 1)	continue;

		x0        = (taitoh_spriteram[offs + 1]) & 0x3ff;
		y0        = (taitoh_spriteram[offs + 0]) & 0x3ff;
		zoomx     = (taitoh_spriteram[offs + 2] & 0x7f00) >> 8;
		zoomy     = (taitoh_spriteram[offs + 2] & 0x007f);
		tile_offs = (taitoh_spriteram[offs + 3] & 0x1fff) << 2;
		ysize     = size[ ( taitoh_spriteram[ offs ] & 0x0c00 ) >> 10 ];

		/* Convert zoomy value to regal value as zoomx */
		zoomy = zoomy_conv_table[zoomy];

		if (zoomx < 63)
		{
			dx = 8 + (zoomx + 2) / 8;
			ex = (zoomx + 2) % 8;
			zx = ((dx << 1) + ex) << 11;
		}
		else
		{
			dx = 16 + (zoomx - 63) / 4;
			ex = (zoomx - 63) % 4;
			zx = (dx + ex) << 12;
		}

		if (zoomy < 63)
		{
			dy = 8 + (zoomy + 2) / 8;
			ey = (zoomy + 2) % 8;
			zy = ((dy << 1) + ey) << 11;
		}
		else
		{
			dy = 16 + (zoomy - 63) / 4;
			ey = (zoomy - 63) % 4;
			zy = (dy + ey) << 12;
		}

		if (x0 >= 0x200) x0 -= 0x400;
		if (y0 >= 0x200) y0 -= 0x400;

		if (flipscreen)
		{
			x0 = 497 - x0;
			y0 = 498 - y0;
			dx = -dx;
			dy = -dy;
		}
		else
		{
			x0 += 1;
			y0 += 2;
		}

		y = y0;
		for (j = 0 ; j < ysize ; j ++)
		{
			x = x0;
			for (k = 0 ; k < 4 ; k ++)
			{
				if (tile_offs < 0x6000)
				{
					int tile, color, flipx, flipy;

					tile  = taitoh_chainram_0[ tile_offs ] & 0x7fff;
					color = taitoh_chainram_1[ tile_offs ] & 0x001f;
					flipx = taitoh_chainram_1[ tile_offs ] & 0x0040;
					flipy = taitoh_chainram_1[ tile_offs ] & 0x0080;

					if (flipscreen)
					{
						flipx ^= 0x0040;
						flipy ^= 0x0080;
					}

					drawgfxzoom( bitmap, Machine -> gfx[0],
								 tile,
								 color,
								 flipx, flipy,
								 x, y,
								 &Machine->visible_area,
								 TRANSPARENCY_PEN, 0,
								 zx, zy
					);
				}
				tile_offs ++;
				x += dx;
			}
			y += dy;
		}
	}
}

static void dleague_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	/* Y chain size is 16/32?/64/64? pixels. X chain size is always 64 pixels. */
	int size[] = { 1, 2, 4, 4 };
	int x0, y0, x, y, dx, ex, zx;
	int ysize;
	int j, k;
	int offs;					/* sprite RAM offset */
	int tile_offs;				/* sprite chain offset */
	int zoomx;					/* zoomx value */
	int pribit;

	for (offs = 0x03f8 / 2 ; offs >= 0 ; offs -= 0x008 / 2)
	{
		x0        = (taitoh_spriteram[offs + 1]) & 0x3ff;
		y0        = (taitoh_spriteram[offs + 0]) & 0x3ff;
		zoomx     = (taitoh_spriteram[offs + 2] & 0x7f00) >> 8;
		tile_offs = (taitoh_spriteram[offs + 3] & 0x1fff) << 2;
		pribit    = (taitoh_spriteram[offs + 0] & 0x1000) >> 12;
		ysize     = size[ ( taitoh_spriteram[ offs ] & 0x0c00 ) >> 10 ];

		/* The increasing ratio of expansion is different whether zoom value */
		/* is less or more than 63.                                          */
		if (zoomx < 63)
		{
			dx = 8 + (zoomx + 2) / 8;
			ex = (zoomx + 2) % 8;
			zx = ((dx << 1) + ex) << 11;
			pribit = 0;
		}
		else
		{
			dx = 16 + (zoomx - 63) / 4;
			ex = (zoomx - 63) % 4;
			zx = (dx + ex) << 12;
		}

		if (taitoh_scrollram[ 0x0002 ] & 0x8000)
			pribit = 1;

		if (x0 >= 0x200) x0 -= 0x400;
		if (y0 >= 0x200) y0 -= 0x400;

		if (flipscreen)
		{
			x0 = 497 - x0;
			y0 = 498 - y0;
			dx = -dx;
		}
		else
		{
			x0 += 1;
			y0 += 2;
		}

		if ( priority == pribit )
		{
			y = y0;
			for ( j = 0 ; j < ysize ; j ++ )
			{
				x = x0;
				for (k = 0 ; k < 4 ; k ++ )
				{
					if (tile_offs < 0x6000)
					{
						int tile, color, flipx, flipy;

						tile  = taitoh_chainram_0[ tile_offs ] & 0x7fff;
						color = taitoh_chainram_1[ tile_offs ] & 0x001f;
						flipx = taitoh_chainram_1[ tile_offs ] & 0x0040;
						flipy = taitoh_chainram_1[ tile_offs ] & 0x0080;

						if (flipscreen)
						{
							flipx ^= 0x0040;
							flipy ^= 0x0080;
						}

						drawgfxzoom( bitmap, Machine -> gfx[0],
									 tile,
									 color,
									 flipx, flipy,
									 x, y,
									 &Machine->visible_area,
									 TRANSPARENCY_PEN, 0,
									 zx, zx
						);
					}
					tile_offs ++;
					x += dx;
				}
				y += dx;
			}
		}
	}

}

#ifdef MAME_DEBUG
static void taitoh_log_vram(void)
{
	int offs;

	if ( keyboard_pressed(KEYCODE_M))
	{
		int i;

		logerror("\n\nTaito-H scroll RAM\n");
		logerror("===============================\n");
		logerror("  +0   +1   +2   +3   +4   +5   +6   +7\n");
		for (i = 0 ; i < 8 ; i ++)
		{
			logerror("%04x ", taitoh_scrollram[i]);
		}
		logerror("\n\n");
		logerror("\n\nTaito-H sprite RAM\n");
		logerror("===============================\n");
		logerror("        +0   +1   +2   +3\n");
		for (offs = 0x03f8 / 2 ; offs >= 0 ; offs -= 0x008 / 2)
		{
			logerror("%04x: ", offs);
			for (i = 0 ; i < 4 ; i ++)
			{
				logerror("%04x ", taitoh_spriteram[offs + i]);
			}
			logerror("(%3d, %3d)", taitoh_spriteram[offs + 1] & 0x3ff,
						taitoh_spriteram[offs + 0] & 0x3ff );
			logerror("\n");
		}
	}
}
#endif

static void taitoh_tilemap_draw(struct osd_bitmap *bitmap, struct tilemap *tilemap, data16_t zoom, int startx, int starty)
{
	int zoomx, zoomy;

	zoomx = (zoom & 0xff00) >> 8;
	zoomy = zoom & 0x00ff;

	if (zoomx == 0x3f && zoomy == 0x7f)		/* normal size */
	{
		tilemap_draw(bitmap, tilemap, 0, 0);
	}
	else
	{
		int zx, zy, dx, dy, ex, ey;
		int sx,sy;

		struct osd_bitmap *srcbitmap = tilemap_get_pixmap(tilemap);

		sx = -0x10000 * startx;
		sy = -0x10000 * starty;

		if (zoomx < 63)
		{
			dx = 16 - (zoomx + 2) / 8;
			ex = (zoomx + 2) % 8;
			zx = ((dx << 3) - ex) << 10;
		}
		else
		{
			dx = 32 - (zoomx - 63) / 4;
			ex = (zoomx - 63) % 4;
			zx = ((dx << 2) - ex) << 9;
		}

		if (zoomy < 127)
		{
			dy = 16 - (zoomy + 2) / 16;
			ey = (zoomy + 2) % 16;
			zy = ((dy << 4) - ey) << 9;
		}
		else
		{
			dy = 32 - (zoomy - 127) / 8;
			ey = (zoomy - 127) % 8;
			zy = ((dy << 3) - ey) << 8;
		}
		copyrozbitmap(bitmap, srcbitmap, sx, sy,
			zx, 0, 0, zy,
			0,	/* copy with wraparound */
			&Machine->visible_area, TRANSPARENCY_COLOR, 0, 0);
	}
}


void syvalion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

#ifdef MAME_DEBUG
	taitoh_log_vram();
#endif

	palette_init_used_colors();
	mark_sprite_colors();
	palette_recalc();

	fillbitmap(bitmap, palette_transparent_pen, &Machine -> visible_area);

	/* text tile patterns are dinamically changed */
	if ( taitoh_tx_char_dirty )
	{
		int i;
		data8_t *taitoh_tx_charram8 = (data8_t *)(taitoh_tx_charram);

		for ( i = 0 ; i < 256 ; i ++ )
		{
			decodechar(Machine -> gfx[1], i, taitoh_tx_charram8,
						Machine -> drv -> gfxdecodeinfo[1].gfxlayout);
			tilemap_mark_all_tiles_dirty( tx_tilemap );
		}
		taitoh_tx_char_dirty = 0;
	}

	taitoh_tilemap_draw(bitmap, bg0_tilemap, taitoh_scrollram[6], 1, 0);
	taitoh_tilemap_draw(bitmap, bg1_tilemap, taitoh_scrollram[7], 1, 0);
	syvalion_draw_sprites(bitmap);
	tilemap_draw(bitmap, tx_tilemap, 0, 0);
}

void recordbr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

#ifdef MAME_DEBUG
	taitoh_log_vram();
#endif

	palette_init_used_colors();
	mark_sprite_colors();
	palette_recalc();

	fillbitmap(bitmap, palette_transparent_pen, &Machine -> visible_area);

#ifdef MAME_DEBUG
	if ( !keyboard_pressed(KEYCODE_A) )
		taitoh_tilemap_draw(bitmap, bg0_tilemap, taitoh_scrollram[6], 16 + 8 + 1, 32);
	if ( !keyboard_pressed(KEYCODE_S) )
		recordbr_draw_sprites(bitmap, 0);
	if ( !keyboard_pressed(KEYCODE_D) )
		taitoh_tilemap_draw(bitmap, bg1_tilemap, taitoh_scrollram[7], 16 + 8 + 1, 32);
	if ( !keyboard_pressed(KEYCODE_F) )
		recordbr_draw_sprites(bitmap, 1);
#else
	taitoh_tilemap_draw(bitmap, bg0_tilemap, taitoh_scrollram[6], 16 + 8 + 1, 32);
	recordbr_draw_sprites(bitmap, 0);
	taitoh_tilemap_draw(bitmap, bg1_tilemap, taitoh_scrollram[7], 16 + 8 + 1, 32);
	recordbr_draw_sprites(bitmap, 1);
#endif
}

void dleague_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

#ifdef MAME_DEBUG
	taitoh_log_vram();
#endif

	palette_init_used_colors();
	mark_sprite_colors();
	palette_recalc();

	fillbitmap(bitmap, palette_transparent_pen, &Machine -> visible_area);

#ifdef MAME_DEBUG
	if ( !keyboard_pressed(KEYCODE_A) )
		taitoh_tilemap_draw(bitmap, bg0_tilemap, taitoh_scrollram[6], 16 + 8 + 1, 32);
	if ( !keyboard_pressed(KEYCODE_F) )
		dleague_draw_sprites(bitmap, 0);
	if ( !keyboard_pressed(KEYCODE_D) )
		taitoh_tilemap_draw(bitmap, bg1_tilemap, taitoh_scrollram[7], 16 + 8 + 1, 32);
	if ( !keyboard_pressed(KEYCODE_S) )
		dleague_draw_sprites(bitmap, 1);
#else
	taitoh_tilemap_draw(bitmap, bg0_tilemap, taitoh_scrollram[6], 16 + 8 + 1, 32);
	dleague_draw_sprites(bitmap, 0);
	taitoh_tilemap_draw(bitmap, bg1_tilemap, taitoh_scrollram[7], 16 + 8 + 1, 32);
	dleague_draw_sprites(bitmap, 1);
#endif
}

