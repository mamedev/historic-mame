/***************************************************************************

Functions to emulate the video hardware of the machine.


BG RAM format ( $D800 - $DFFF )
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = 1st - 8th bits of tile number
 ---- ----  xx-- ---- = 9th and 10th bit of tile number
 ---- ----  --x- ---- = 11th bit of tile number
 ---- ----  ---- xxxx = color


Text RAM format ( $D000 - $D7FF )
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  xx-- ---- = high bits of tile number
 ---- ----  ---- xxxx = color


Sprite RAM format ( $F200 - $F7FF )
-----------------------------------------------------------------------------
 +0         +1         +2         +3         +4
 xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ---- = sprite y
 ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ---- = low bits of sprite x
 ---- ----  ---- ----  xx-- ----  ---- ----  ---- ---- = high bits of tile number
 ---- ----  ---- ----  --x- ----  ---- ----  ---- ---- = flip y
 ---- ----  ---- ----  ---x ----  ---- ----  ---- ---- = flip x
 ---- ----  ---- ----  ---- --x-  ---- ----  ---- ---- = high bit of sprite y
 ---- ----  ---- ----  ---- ---x  ---- ----  ---- ---- = high bit of sprite x
 ---- ----  ---- ----  ---- ----  xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  ---- ----  ---- ----  ---- ----  ---- x--- = BG1 / sprite priority (Argus only)
 ---- ----  ---- ----  ---- ----  ---- ----  ---- +xxx = color


Scroll RAM of X and Y coordinates ( $C308 - $C30B )
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = scroll value
 ---- ----  ---- ---x = top bit of scroll value


Video effect RAM ( $C30C )
-----------------------------------------------------------------------------
 +0
 ---- ---x  = BG enable bit
 ---- --x-  = gray scale effect ( Is it used ? )


Flip screen controller
-----------------------------------------------------------------------------
 +0
 x--- ----  = flip screen


BG0 palette intensity ( $C47F, $C4FF )
-----------------------------------------------------------------------------
 +0 (c47f)  +1 (c4ff)
 xxxx ----  ---- ---- = red intensity
 ---- xxxx  ---- ---- = green intensity
 ---- ----  xxxx ---- = blue intensity


(*) Things it is not emulated.
 - Color $000 - 00f, $01e, $02e ... are half transparent color.
 - Maybe, BG0 scroll value of Valtric is used for mosaic effect.

***************************************************************************/

#include "driver.h"
#include "tilemap.h"
#include "vidhrdw/generic.h"



data8_t *argus_paletteram;
data8_t *argus_txram;
data8_t *argus_bg0_scrollx;
data8_t *argus_bg0_scrolly;
data8_t *argus_bg1ram;
data8_t *argus_bg1_scrollx;
data8_t *argus_bg1_scrolly;
data8_t *spriteram;
size_t spriteram_size;

static struct tilemap *tx_tilemap  = NULL;
static struct tilemap *bg0_tilemap  = NULL;
static struct tilemap *bg1_tilemap  = NULL;

static data8_t *argus_dummy_bg0ram;

static data8_t argus_bg_veffect = 0;
static data8_t argus_flipscreen = 0;

static int argus_palette_intensity = 0;

/* VROM scroll related in Argus */
static int lowbitscroll = 0;
static int prvscrollx = 0;


/***************************************************************************
  Callbacks for the tilemap code
***************************************************************************/

static void argus_get_tx_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_txram[  tile_index << 1  ];
	hi = argus_txram[ (tile_index << 1) + 1 ];

	tile = ((hi & 0xc0) << 2) | lo;
	color = hi & 0x0f;

	SET_TILE_INFO(3, tile, color)
}

static void argus_get_bg0_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_dummy_bg0ram[  tile_index << 1  ];
	hi = argus_dummy_bg0ram[ (tile_index << 1) + 1 ];

	tile_info.flags = 0;
	if ( hi & 0x20 )
		tile_info.flags |= TILE_FLIPY;
	if ( hi & 0x10 )
		tile_info.flags |= TILE_FLIPX;

	tile = ((hi & 0xc0) << 2) | lo;
	color = hi & 0x0f;

	SET_TILE_INFO(1, tile, color)
}

static void argus_get_bg1_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_bg1ram[  tile_index << 1  ];
	hi = argus_bg1ram[ (tile_index << 1) + 1 ];

	tile_info.flags = 0;
	if ( hi & 0x20 )
		tile_info.flags |= TILE_FLIPY;
	if ( hi & 0x10 )
		tile_info.flags |= TILE_FLIPX;

	tile =  lo;
	color = hi & 0x0f;

	SET_TILE_INFO(2, tile, color)
}

static void valtric_get_tx_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_txram[  tile_index << 1  ];
	hi = argus_txram[ (tile_index << 1) + 1 ];

	tile = ((hi & 0xc0) << 2) | lo;
	color = hi & 0x0f;

	SET_TILE_INFO(2, tile, color)
}

static void valtric_get_bg_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_bg1ram[  tile_index << 1  ];
	hi = argus_bg1ram[ (tile_index << 1) + 1 ];

	tile = ((hi & 0xc0) << 2) | ((hi & 0x20) << 5) | lo;
	color = hi & 0x0f;

	SET_TILE_INFO(1, tile, color)
}


/***************************************************************************
  Initialize and destroy video hardware emulation
***************************************************************************/

int argus_vh_start(void)
{
	lowbitscroll = 0;
	/*                           info                      offset             type                  w   h  col  row */
	bg0_tilemap = tilemap_create(argus_get_bg0_tile_info,  tilemap_scan_cols, TILEMAP_OPAQUE,      16, 16, 32, 32);
	bg1_tilemap = tilemap_create(argus_get_bg1_tile_info,  tilemap_scan_cols, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	tx_tilemap  = tilemap_create(argus_get_tx_tile_info,   tilemap_scan_cols, TILEMAP_TRANSPARENT,  8,  8, 32, 32);

	if ( !tx_tilemap || !bg0_tilemap || !bg1_tilemap )
	{
		return 1;
	}

	/* dummy RAM for back ground */
	argus_dummy_bg0ram = malloc( 0x800 );
	if ( argus_dummy_bg0ram == NULL )
		return 1;
	memset( argus_dummy_bg0ram, 0, 0x800 );

	memset( argus_bg0_scrollx, 0x00, 2 );

	tilemap_set_transparent_pen( bg0_tilemap, 15 );
	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( tx_tilemap,  15 );

	return 0;
}

void argus_vh_stop(void)
{
	free(argus_dummy_bg0ram);
}

int valtric_vh_start(void)
{
	/*                           info                       offset             type                 w   h  col  row */
	bg1_tilemap = tilemap_create(valtric_get_bg_tile_info,  tilemap_scan_cols, TILEMAP_OPAQUE,      16, 16, 32, 32);
	tx_tilemap  = tilemap_create(valtric_get_tx_tile_info,  tilemap_scan_cols, TILEMAP_TRANSPARENT,  8,  8, 32, 32);

	if ( !tx_tilemap || !bg1_tilemap )
	{
		return 1;
	}

	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( tx_tilemap,  15 );
	return 0;
}


/***************************************************************************
  Functions for handler of MAP roms in Argus and palette color
***************************************************************************/

/* write bg0 pattern data to dummy bg0 ram */
static void argus_write_dummy_rams( int dramoffs, int vromoffs )
{
	int i;
	int voffs;
	int offs;

	data8_t *VROM1 = memory_region( REGION_USER1 );		/* "ag_15.bin" */
	data8_t *VROM2 = memory_region( REGION_USER2 );		/* "ag_16.bin" */

	/* offset in pattern data */
	offs = VROM1[ vromoffs ] | ( VROM1[ vromoffs + 1 ] << 8 );
	offs &= 0x7ff;

	voffs = offs * 16;
	for (i = 0 ; i < 8 ; i ++)
	{
		argus_dummy_bg0ram[ dramoffs ] = VROM2[ voffs ];
		argus_dummy_bg0ram[ dramoffs + 1 ] = VROM2[ voffs + 1 ];
		tilemap_mark_tile_dirty( bg0_tilemap, dramoffs >> 1 );
		dramoffs += 2;
		voffs += 2;
	}
}

static void argus_change_palette(int color, int data)
{
	int r, g, b;

	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_change_color(color, r, g, b);
}

static void argus_change_bg_palette(int color, int data)
{
	int r, g, b;
	int ir, ig, ib;

	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	ir = (argus_palette_intensity >> 12) & 0x0f;
	ig = (argus_palette_intensity >>  8) & 0x0f;
	ib = (argus_palette_intensity >>  4) & 0x0f;

	r = (r - ir > 0) ? r - ir : 0;
	g = (g - ig > 0) ? g - ig : 0;
	b = (b - ib > 0) ? b - ib : 0;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_change_color(color, r, g, b);
}


/***************************************************************************
  Memory handler
***************************************************************************/

READ_HANDLER( argus_txram_r )
{
	return argus_txram[ offset ];
}

WRITE_HANDLER( argus_txram_w )
{
	if (argus_txram[ offset ] != data)
	{
		argus_txram[ offset ] = data;
		tilemap_mark_tile_dirty(tx_tilemap, offset >> 1);
	}
}

READ_HANDLER( argus_bg1ram_r )
{
	return argus_bg1ram[ offset ];
}

WRITE_HANDLER( argus_bg1ram_w )
{
	if (argus_bg1ram[ offset ] != data)
	{
		argus_bg1ram[ offset ] = data;
		tilemap_mark_tile_dirty(bg1_tilemap, offset >> 1);
	}
}

WRITE_HANDLER ( argus_bg0_scrollx_w )
{
	if (argus_bg0_scrollx[ offset ] != data)
	{
		argus_bg0_scrollx[ offset ] = data;
	}
}

WRITE_HANDLER( argus_bg0_scrolly_w )
{
	if (argus_bg0_scrolly[ offset ] != data)
	{
		int scrolly;
		argus_bg0_scrolly[ offset ] = data;
		scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrolly( bg0_tilemap, 0, scrolly );
		else
			tilemap_set_scrolly( bg0_tilemap, 0, (scrolly + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg1_scrollx_w )
{
	if (argus_bg1_scrollx[ offset ] != data)
	{
		int scrollx;
		argus_bg1_scrollx[ offset ] = data;
		scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrollx( bg1_tilemap, 0, scrollx );
		else
			tilemap_set_scrollx( bg1_tilemap, 0, (scrollx + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg1_scrolly_w )
{
	if (argus_bg1_scrolly[ offset ] != data)
	{
		int scrolly;
		argus_bg1_scrolly[ offset ] = data;
		scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrolly( bg1_tilemap, 0, scrolly );
		else
			tilemap_set_scrolly( bg1_tilemap, 0, (scrolly + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg_veffect_w )
{
	if (argus_bg_veffect != data)
	{
		argus_bg_veffect = data;
		tilemap_set_enable(bg1_tilemap, data & 1);
	}
}

WRITE_HANDLER( argus_flipscreen_w )
{
	if (argus_flipscreen != (data >> 7))
	{
		argus_flipscreen = data >> 7;
		tilemap_set_flip( ALL_TILEMAPS, (argus_flipscreen) ? TILEMAP_FLIPY | TILEMAP_FLIPX : 0);
		if (!argus_flipscreen)
		{
			int scrollx, scrolly;

			if (bg0_tilemap != NULL)
			{
				scrollx = argus_bg0_scrollx[0] | ( (argus_bg0_scrollx[1] & 0x01) << 8);
				tilemap_set_scrollx(bg0_tilemap, 0, scrollx & 0x1ff);

				scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
				tilemap_set_scrolly(bg0_tilemap, 0, scrolly);
			}
			scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
			tilemap_set_scrollx(bg1_tilemap, 0, scrollx);

			scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
			tilemap_set_scrolly(bg1_tilemap, 0, scrolly);
		}
		else
		{
			int scrollx, scrolly;

			if (bg0_tilemap != NULL)
			{
				scrollx = argus_bg0_scrollx[0] | ( (argus_bg0_scrollx[1] & 0x01) << 8);
				tilemap_set_scrollx(bg0_tilemap, 0, (scrollx + 256) & 0x1ff);

				scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
				tilemap_set_scrolly(bg0_tilemap, 0, (scrolly + 256) & 0x1ff);
			}
			scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
			tilemap_set_scrollx(bg1_tilemap, 0, (scrollx + 256) & 0x1ff);

			scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
			tilemap_set_scrolly(bg1_tilemap, 0, (scrolly + 256) & 0x1ff);
		}
	}
}

READ_HANDLER( argus_paletteram_r )
{
	return argus_paletteram[ offset ];
}

WRITE_HANDLER( argus_paletteram_w )
{
	int offs;

	argus_paletteram[ offset ] = data;

	if (offset != 0x007f && offset != 0x00ff)
	{
		if (offset >= 0x0000 && offset <= 0x00ff)				/* sprite color */
		{
			if (offset & 0x80)
				offset -= 0x80;

			argus_change_palette( offset,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x80] );
		}

		else if ( (offset >= 0x0400 && offset <= 0x04ff) ||
				  (offset >= 0x0800 && offset <= 0x08ff) )		/* BG0 color */
		{
			if (offset >= 0x0800)
				offset -= 0x0400;

			argus_change_bg_palette( (offset - 0x0400) + 128,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}

		else if ( (offset >= 0x0500 && offset <= 0x05ff) ||
				  (offset >= 0x0900 && offset <= 0x09ff) )		/* BG1 color */
		{
			if (offset >= 0x0900)
				offset -= 0x0400;

			argus_change_palette( (offset - 0x0500) + 384,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}

		else if ( (offset >= 0x0700 && offset <= 0x07ff) ||
				  (offset >= 0x0b00 && offset <= 0x0bff) )		/* text color */
		{
			if (offset >= 0x0b00)
				offset -= 0x0400;

			argus_change_palette( (offset - 0x0700) + 640,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}
	}
	else
	{
		argus_palette_intensity = (argus_paletteram[0x007f] << 8) | argus_paletteram[0x00ff];

		for (offs = 0x400 ; offs < 0x500 ; offs ++)
		{
			argus_change_bg_palette( (offs - 0x0400) + 128,
				(argus_paletteram[offs] << 8) | argus_paletteram[offs + 0x0400] );
		}
	}
}

WRITE_HANDLER( valtric_paletteram_w )
{
	int offs;

	argus_paletteram[ offset ] = data;

	if (offset != 0x01fe && offset != 0x01ff)
	{
		if (offset >= 0x0000 && offset <= 0x01ff)
		{
			argus_change_palette( offset >> 1,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
		else if (offset >= 0x0400 && offset <= 0x05ff )
		{
			argus_change_palette( ((offset - 0x0400) >> 1) + 256,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
		else if (offset >= 0x0600 && offset <= 0x07ff )
		{
			argus_change_palette( ((offset - 0x0600) >> 1) + 512,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
	}
	else
	{
		argus_palette_intensity = (argus_paletteram[0x01fe] << 8) | argus_paletteram[0x01ff];

		for (offs = 0x400 ; offs < 0x500 ; offs += 2)
		{
			argus_change_bg_palette( ((offset - 0x0400) >> 1) + 256,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
	}
}

/***************************************************************************
  Screen refresh
***************************************************************************/

static void mark_sprite_colors(int color_mask)
{
	unsigned short palette_map[16];

	int offs;
	int i;

	memset ( palette_map, 0x00, sizeof(palette_map) );

	/* Find colors used in the sprites */
	for (offs = 11; offs < spriteram_size; offs += 16)
	{
		if ( !(spriteram[offs+4] == 0 && spriteram[offs] == 0xf0) )
		{
			int tile, color;
			tile  = spriteram[offs+3] + ((spriteram[offs+2] & 0xc0) << 2);
			color = spriteram[offs+4] & color_mask;
			palette_map[color] |= Machine -> gfx[0] -> pen_usage[tile];
		}
	}

	for (i = 0; i < 16; i ++)
	{
		int j;

		if (palette_map[i])
		{
			for (j = 0 ; j < 15 ; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
			palette_used_colors[16 * i + 15] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}

static void argus_bg0_scroll_handle( void )
{
	int delta;
	int scrollx;
	int dcolumn;

	/* deficit between previous and current scroll value */
	scrollx = argus_bg0_scrollx[0] | (argus_bg0_scrollx[1] << 8);
	delta = scrollx - prvscrollx;
	prvscrollx = scrollx;

	if ( delta == 0 )
		return;

	if (delta > 0)
	{
		lowbitscroll += delta % 16;
		dcolumn = delta / 16;

		if (lowbitscroll >= 16)
		{
			dcolumn ++;
			lowbitscroll -= 16;
		}

		if (dcolumn != 0)
		{
			int i, j;
			int col, woffs, roffs;

			col = ( (scrollx / 16) + 16 ) % 32;
			woffs = 32 * 2 * col;
			roffs = ( ( (scrollx / 16) + 16 ) * 8 ) % 0x8000;

			if ( dcolumn >= 18 )
				dcolumn = 18;

			for ( i = 0 ; i < dcolumn ; i ++ )
			{
				for ( j = 0 ; j < 4 ; j ++ )
				{
					argus_write_dummy_rams( woffs, roffs );
					woffs += 16;
					roffs += 2;
				}
				woffs -= 128;
				roffs -= 16;
				if (woffs < 0)
					woffs += 0x800;
				if (roffs < 0)
					roffs += 0x8000;
			}
		}
	}
	else
	{
		lowbitscroll += (delta % 16);
		dcolumn = -(delta / 16);

		if (lowbitscroll <= 0)
		{
			dcolumn ++;
			lowbitscroll += 16;
		}

		if (dcolumn != 0)
		{
			int i, j;
			int col, woffs, roffs;

			col = ( (scrollx / 16) + 31 ) % 32;
			woffs = 32 * 2 * col;
			roffs = ( (scrollx / 16) - 1 ) * 8;
			if (roffs < 0)
				roffs += 0x08000;

			if (dcolumn >= 18)
				dcolumn = 18;

			for ( i = 0 ; i < dcolumn ; i ++ )
			{
				for ( j = 0 ; j < 4 ; j ++ )
				{
					argus_write_dummy_rams( woffs, roffs );
					woffs += 16;
					roffs += 2;
				}
				if (woffs >= 0x800)
					woffs -= 0x800;
				if (roffs >= 0x8000)
					roffs -= 0x8000;
			}
		}
	}

	if (!argus_flipscreen)
		tilemap_set_scrollx(bg0_tilemap, 0, scrollx & 0x1ff);
	else
		tilemap_set_scrollx(bg0_tilemap, 0, (scrollx + 256) & 0x1ff);

}

static void argus_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	int offs;

	/* Draw the sprites */
	for (offs = 11 ; offs < spriteram_size ; offs += 16)
	{
		if ( !(spriteram[offs+4] == 0 && spriteram[offs] == 0xf0) )
		{
			int sx, sy, tile, flipx, flipy, color, pri;

			sx = spriteram[offs + 1];
			sy = spriteram[offs];

			if (argus_flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}

			if (!argus_flipscreen)
			{
				if (  spriteram[offs+2] & 0x01)  sx -= 256;
				if (!(spriteram[offs+2] & 0x02)) sy -= 256;
			}
			else
			{
				if (  spriteram[offs+2] & 0x01)  sx += 256;
				if (!(spriteram[offs+2] & 0x02)) sy += 256;
			}

			tile	 = spriteram[offs+3] + ((spriteram[offs+2] & 0xc0) << 2);
			flipx	 = spriteram[offs+2] & 0x10;
			flipy	 = spriteram[offs+2] & 0x20;
			color	 = spriteram[offs+4] & 0x07;
			pri      = (spriteram[offs+4] & 0x08) >> 3;

			if (argus_flipscreen)
			{
				flipx ^= 0x10;
				flipy ^= 0x20;
			}

			if (priority != pri)
				drawgfx(bitmap,Machine -> gfx[0],
							tile,
							color,
							flipx, flipy,
							sx, sy,
							&Machine -> visible_area,
							TRANSPARENCY_PEN, 15
				);
		}
	}
}

static void valtric_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	/* Draw the sprites */
	for (offs = 11 ; offs < spriteram_size ; offs += 16)
	{
		if ( !(spriteram[offs+4] == 0 && spriteram[offs] == 0xf0) )
		{
			int sx, sy, tile, flipx, flipy, color;

			sx = spriteram[offs + 1];
			sy = spriteram[offs];

			if (argus_flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}

			if (!argus_flipscreen)
			{
				if (  spriteram[offs+2] & 0x01)  sx -= 256;
				if (!(spriteram[offs+2] & 0x02)) sy -= 256;
			}
			else
			{
				if (  spriteram[offs+2] & 0x01)  sx += 256;
				if (!(spriteram[offs+2] & 0x02)) sy += 256;
			}

			tile	 = spriteram[offs+3] + ((spriteram[offs+2] & 0xc0) << 2);
			flipx	 = spriteram[offs+2] & 0x10;
			flipy	 = spriteram[offs+2] & 0x20;
			color	 = spriteram[offs+4] & 0x0f;

			if (argus_flipscreen)
			{
				flipx ^= 0x10;
				flipy ^= 0x20;
			}

			drawgfx(bitmap,Machine -> gfx[0],
						tile,
						color,
						flipx, flipy,
						sx, sy,
						&Machine -> visible_area,
						TRANSPARENCY_PEN, 15);
		}
	}
}

#if MAME_DEBUG
static void argus_log_vram(void)
{
	int offs;

	if ( keyboard_pressed(KEYCODE_M) )
	{
		int i;
		logerror("\nSprite RAM\n");
		logerror("---------------------------------------\n");
		for (offs = 0 ; offs < spriteram_size ; offs += 16)
		{
			for (i = 0 ; i < 16 ; i ++)
			{
				if (i == 0)
					logerror("%04x : ", offs + 0xf200);
				else if (i == 7)
					logerror("%02x  ", spriteram[offs + 7]);
				else if (i == 15)
					logerror("%02x\n", spriteram[offs + 15]);
				else
					logerror("%02x ", spriteram[offs + i]);
			}
		}
		logerror("\nColor RAM\n");
		logerror("---------------------------------------\n");
		for (offs = 0 ; offs < 0xbf0 ; offs += 16)
		{
			for (i = 0 ; i < 16 ; i ++)
			{
				if (i == 0)
					logerror("%04x : ", offs + 0xc400);
				else if (i == 7)
					logerror("%02x  ", argus_paletteram[offs + 7]);
				else if (i == 15)
					logerror("%02x\n", argus_paletteram[offs + 15]);
				else
					logerror("%02x ", argus_paletteram[offs + i]);
			}
		}
	}
	if ( keyboard_pressed(KEYCODE_N) )
		logerror("---------------------------------------\n");
}
#endif

void argus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* scroll BG0 and render tile at proper position */
	argus_bg0_scroll_handle();

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprite_colors(0x07);
	palette_recalc();

	fillbitmap(bitmap, palette_transparent_pen, &Machine -> visible_area);

	tilemap_draw(bitmap, bg0_tilemap, 0, 0);
	argus_draw_sprites(bitmap, 0);
	tilemap_draw(bitmap, bg1_tilemap, 0, 0);
	argus_draw_sprites(bitmap, 1);
	tilemap_draw(bitmap, tx_tilemap,  0, 0);


#if MAME_DEBUG
	argus_log_vram();
#endif
}

void valtric_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprite_colors(0x0f);
	palette_recalc();

	fillbitmap(bitmap, palette_transparent_pen, &Machine -> visible_area);

	tilemap_draw(bitmap, bg1_tilemap, 0, 0);
	valtric_draw_sprites(bitmap);
	tilemap_draw(bitmap, tx_tilemap,  0, 0);
}
