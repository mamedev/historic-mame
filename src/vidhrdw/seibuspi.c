#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *text_layer;
static struct tilemap *back_layer;
static struct tilemap *mid_layer;
static struct tilemap *fore_layer;

UINT32 *back_ram, *mid_ram, *fore_ram;

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri_mask)
{
	int xpos, ypos, tile_num, color;
	int flip_x = 0, flip_y = 0;
	int a;
	const struct GfxElement *gfx = Machine->gfx[2];

	for( a = 0x400 - 2; a >= 0; a -= 2 ) {
		tile_num = spriteram32[a + 0] & 0xffff;

		if( !tile_num )
			continue;

		xpos = spriteram32[a + 1] & 0xfff;
		ypos = (spriteram32[a + 1] >> 16) & 0xfff;
		color = (spriteram32[a + 0] >> 12) & 0xf;

		drawgfx(
			bitmap,
			gfx,
			tile_num,
			color, flip_x, flip_y, xpos, ypos,
			cliprect,
			TRANSPARENCY_PEN, 63
			);
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

	palette_set_color( (offset * 2), r1, g1, b1 );
	palette_set_color( (offset * 2) + 1, r2, g2, b2 );
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
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (mid_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_fore_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (fore_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(1, tile, color, 0)
}

VIDEO_START( spi )
{
	text_layer	= tilemap_create( get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32 );
	back_layer	= tilemap_create( get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,16,64,32 );
	mid_layer	= tilemap_create( get_mid_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,64,32 );
	fore_layer	= tilemap_create( get_fore_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,64,32 );

	tilemap_set_transparent_pen(text_layer, 63);
	tilemap_set_transparent_pen(mid_layer, 63);
	tilemap_set_transparent_pen(fore_layer, 63);
	return 0;
}

VIDEO_UPDATE( spi )
{
	tilemap_draw(bitmap, cliprect, back_layer, 0,0);
	tilemap_draw(bitmap, cliprect, mid_layer, 0,0);
	tilemap_draw(bitmap, cliprect, fore_layer, 0,0);

//	draw_sprites(bitmap, cliprect, 0);

	tilemap_draw(bitmap, cliprect, text_layer, 0,0);

	draw_sprites(bitmap, cliprect, 0); // draw on top so we can see them over the encrypted text_layer
}
