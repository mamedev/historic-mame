#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *text_layer;
static struct tilemap *back_layer;
static struct tilemap *mid_layer;
static struct tilemap *fore_layer;

UINT32 *back_ram, *mid_ram, *fore_ram, *scroll_ram;
UINT32 *back_rowscroll_ram, *mid_rowscroll_ram, *fore_rowscroll_ram;
int old_vidhw;
int bg_size;
static UINT32 layer_bank;
static UINT32 layer_enable;

static int rf2_layer_bank[3];

READ32_HANDLER( spi_layer_bank_r )
{
	return layer_bank;
}

WRITE32_HANDLER( spi_layer_bank_w )
{
	COMBINE_DATA( &layer_bank );
}

void rf2_set_layer_banks(int banks)
{
	if (rf2_layer_bank[0] != BIT(banks,0))
	{
		rf2_layer_bank[0] = BIT(banks,0);
		tilemap_mark_all_tiles_dirty(back_layer);
	}

	if (rf2_layer_bank[1] != BIT(banks,1))
	{
		rf2_layer_bank[1] = BIT(banks,1);
		tilemap_mark_all_tiles_dirty(mid_layer);
	}

	if (rf2_layer_bank[2] != BIT(banks,2))
	{
		rf2_layer_bank[2] = BIT(banks,2);
		tilemap_mark_all_tiles_dirty(fore_layer);
	}
}

WRITE32_HANDLER( spi_layer_enable_w )
{
	COMBINE_DATA( &layer_enable );
	tilemap_set_enable(back_layer, (layer_enable & 0x1) ^ 0x1);
	tilemap_set_enable(mid_layer, ((layer_enable >> 1) & 0x1) ^ 0x1);
	tilemap_set_enable(fore_layer, ((layer_enable >> 2) & 0x1) ^ 0x1);
}

static int sprite_xtable[2][8] =
{
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 }
};
static int sprite_ytable[2][8] =
{
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 }
};

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri_mask)
{
	INT16 xpos, ypos;
	int tile_num, color;
	int width, height;
	int flip_x = 0, flip_y = 0;
	int a;
	int priority;
	int x,y, x1, y1;
	const struct GfxElement *gfx = Machine->gfx[2];

	if( layer_enable & 0x10 )
		return;

	for( a = 0x400 - 2; a >= 0; a -= 2 ) {
		tile_num = (spriteram32[a + 0] >> 16) & 0xffff;
		if( spriteram32[a + 1] & 0x1000 )
			tile_num |= 0x10000;

		if( !tile_num )
			continue;

		priority = (spriteram32[a + 0] >> 6) & 0x3;
		if( pri_mask != priority )
			continue;

		xpos = spriteram32[a + 1] & 0x3ff;
		if( xpos & 0x200 )
			xpos |= 0xfc00;
		ypos = (spriteram32[a + 1] >> 16) & 0x1ff;
		if( ypos & 0x100 )
			ypos |= 0xfe00;
		color = (spriteram32[a + 0] & 0x3f);

		width = ((spriteram32[a + 0] >> 8) & 0x7) + 1;
		height = ((spriteram32[a + 0] >> 12) & 0x7) + 1;
		flip_x = (spriteram32[a + 0] >> 11) & 0x1;
		flip_y = (spriteram32[a + 0] >> 15) & 0x1;
		x1 = 0;
		y1 = 0;

		if( flip_x ) {
			x1 = 8 - width;
			width = width + x1;
		}
		if( flip_y ) {
			y1 = 8 - height;
			height = height + y1;
		}

		for( x=x1; x < width; x++ ) {
			for( y=y1; y < height; y++ ) {
				drawgfx(
					bitmap,
					gfx,
					tile_num,
					color, flip_x, flip_y, xpos + sprite_xtable[flip_x][x], ypos + sprite_ytable[flip_y][y],
					cliprect,
					TRANSPARENCY_PEN, 63
					);

				/* xpos seems to wrap-around to 0 at 512 */
				if( (xpos + (16 * x) + 16) >= 512 ) {
					drawgfx(
					bitmap,
					gfx,
					tile_num,
					color, flip_x, flip_y, xpos - 512 + sprite_xtable[flip_x][x], ypos + sprite_ytable[flip_y][y],
					cliprect,
					TRANSPARENCY_PEN, 63
					);
				}

				tile_num++;
			}
		}
	}
}

WRITE32_HANDLER( spi_paletteram32_xBBBBBGGGGGRRRRR_w )
{
	int r1,g1,b1,r2,g2,b2;
	COMBINE_DATA(&paletteram32[offset]);

	b1 = ((paletteram32[offset] & 0x7c000000) >>26) << 3;
	g1 = ((paletteram32[offset] & 0x03e00000) >>21) << 3;
	r1 = ((paletteram32[offset] & 0x001f0000) >>16) << 3;
	b2 = ((paletteram32[offset] & 0x00007c00) >>10) << 3;
	g2 = ((paletteram32[offset] & 0x000003e0) >>5) << 3;
	r2 = ((paletteram32[offset] & 0x0000001f) >>0) << 3;

	palette_set_color( (offset * 2), r2, g2, b2 );
	palette_set_color( (offset * 2) + 1, r1, g1, b1 );
}



WRITE32_HANDLER( spi_textlayer_w )
{
	COMBINE_DATA(&videoram32[offset]);
	tilemap_mark_tile_dirty( text_layer, (offset * 2) );
	tilemap_mark_tile_dirty( text_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_back_layer_w )
{
	COMBINE_DATA(&back_ram[offset]);
	tilemap_mark_tile_dirty( back_layer, (offset * 2) );
	tilemap_mark_tile_dirty( back_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_mid_layer_w )
{
	COMBINE_DATA(&mid_ram[offset]);
	tilemap_mark_tile_dirty( mid_layer, (offset * 2) );
	tilemap_mark_tile_dirty( mid_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_fore_layer_w )
{
	COMBINE_DATA(&fore_ram[offset]);
	tilemap_mark_tile_dirty( fore_layer, (offset * 2) );
	tilemap_mark_tile_dirty( fore_layer, (offset * 2) + 1 );
}

static void get_text_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (videoram32[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0, tile, color, 0)
}

static void get_back_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (back_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;

	if( rf2_layer_bank[0] )
		tile |= 0x4000;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (mid_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;
	tile |= 0x2000;

	if( rf2_layer_bank[1] )
		tile |= 0x4000;

	SET_TILE_INFO(1, tile, color + 16, 0)
}

static void get_fore_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (fore_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;
	switch(bg_size)
	{
		case 0: tile |= 0x2000; break;
		case 1:	tile |= 0x4000; break;
		case 2: tile |= 0x8000; break;
	}

	if( rf2_layer_bank[2] )
		tile |= 0x4000;

	tile |= ((layer_bank >> 27) & 0x1) << 13;

	SET_TILE_INFO(1, tile, color + 8, 0)
}

VIDEO_START( spi )
{
	text_layer	= tilemap_create( get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32 );
	back_layer	= tilemap_create( get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,16,32,32 );
	mid_layer	= tilemap_create( get_mid_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );
	fore_layer	= tilemap_create( get_fore_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );

	tilemap_set_transparent_pen(text_layer, 31);
	tilemap_set_transparent_pen(mid_layer, 63);
	tilemap_set_transparent_pen(fore_layer, 63);
	return 0;
}

static void set_rowscroll(struct tilemap *layer, int scroll, INT16* rows)
{
	int i;
	int x = scroll_ram[scroll] & 0xffff;
	int y = (scroll_ram[scroll] >> 16) & 0xffff;
	tilemap_set_scroll_rows(layer, 512);
	for( i=0; i < 512; i++ ) {
		tilemap_set_scrollx(layer, i, x + rows[i]);
	}
	tilemap_set_scrolly(layer, 0, y);
}

static void set_scroll(struct tilemap *layer, int scroll)
{
	int x = scroll_ram[scroll] & 0xffff;
	int y = (scroll_ram[scroll] >> 16) & 0xffff;
	tilemap_set_scrollx(layer, 0, x);
	tilemap_set_scrolly(layer, 0, y);
}


VIDEO_UPDATE( spi )
{
	if( !old_vidhw ) {
		set_rowscroll(back_layer, 0, (INT16*)back_rowscroll_ram);
		set_rowscroll(mid_layer, 1, (INT16*)mid_rowscroll_ram);
		set_rowscroll(fore_layer, 2, (INT16*)fore_rowscroll_ram);
	} else {
		set_scroll(back_layer, 0);
		set_scroll(mid_layer, 1);
		set_scroll(fore_layer, 2);
	}

	if( layer_enable & 0x1 )
		fillbitmap(bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, back_layer, 0,0);

	draw_sprites(bitmap, cliprect, 0);

	tilemap_draw(bitmap, cliprect, mid_layer, 0,0);

	draw_sprites(bitmap, cliprect, 1);
	draw_sprites(bitmap, cliprect, 2);

	tilemap_draw(bitmap, cliprect, fore_layer, 0,0);

	draw_sprites(bitmap, cliprect, 3);

	tilemap_draw(bitmap, cliprect, text_layer, 0,0);
}
