#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *text_layer;
static struct tilemap *back_layer;
static struct tilemap *mid_layer;
static struct tilemap *fore_layer;

UINT32 *scroll_ram;

int old_vidhw;
int bg_size;
static UINT32 layer_bank;
static UINT32 layer_enable;
static UINT32 video_dma_length;
static UINT32 video_dma_address;

static int rf2_layer_bank[3];
extern data32_t *spimainram;
static UINT32 *tilemap_ram;
static UINT32 *palette_ram;
static UINT32 *sprite_ram;

static int mid_layer_offset;
static int fore_layer_offset;
static int text_layer_offset;

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

READ32_HANDLER( spi_layer_enable_r )
{
	return layer_enable;
}

WRITE32_HANDLER( spi_layer_enable_w )
{
	COMBINE_DATA( &layer_enable );
	tilemap_set_enable(back_layer, (layer_enable & 0x1) ^ 0x1);
	tilemap_set_enable(mid_layer, ((layer_enable >> 1) & 0x1) ^ 0x1);
	tilemap_set_enable(fore_layer, ((layer_enable >> 2) & 0x1) ^ 0x1);
}

WRITE32_HANDLER( tilemap_dma_start_w )
{
	int i;
	int index = (video_dma_address / 4) - 0x200;

	if (!old_vidhw)
	{
		/* back layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i] != tile) {
				tilemap_ram[i] = tile;
				tilemap_mark_tile_dirty( back_layer, (i * 2) );
				tilemap_mark_tile_dirty( back_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* back layer row scroll */
		memcpy(&tilemap_ram[0x800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* fore layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+fore_layer_offset] != tile) {
				tilemap_ram[i+fore_layer_offset] = tile;
				tilemap_mark_tile_dirty( fore_layer, (i * 2) );
				tilemap_mark_tile_dirty( fore_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* fore layer row scroll */
		memcpy(&tilemap_ram[0x1800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* mid layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+mid_layer_offset] != tile) {
				tilemap_ram[i+mid_layer_offset] = tile;
				tilemap_mark_tile_dirty( mid_layer, (i * 2) );
				tilemap_mark_tile_dirty( mid_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* mid layer row scroll */
		memcpy(&tilemap_ram[0x1800/4], &spimainram[index], 0x800/4);
		index += 0x800/4;

		/* text layer */
		for (i=0; i < 0x1000/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+text_layer_offset] != tile) {
				tilemap_ram[i+text_layer_offset] = tile;
				tilemap_mark_tile_dirty( text_layer, (i * 2) );
				tilemap_mark_tile_dirty( text_layer, (i * 2) + 1 );
			}
			index++;
		}
	}
	else
	{
		/* back layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i] != tile) {
				tilemap_ram[i] = tile;
				tilemap_mark_tile_dirty( back_layer, (i * 2) );
				tilemap_mark_tile_dirty( back_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* fore layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+fore_layer_offset] != tile) {
				tilemap_ram[i+fore_layer_offset] = tile;
				tilemap_mark_tile_dirty( fore_layer, (i * 2) );
				tilemap_mark_tile_dirty( fore_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* mid layer */
		for (i=0; i < 0x800/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+mid_layer_offset] != tile) {
				tilemap_ram[i+mid_layer_offset] = tile;
				tilemap_mark_tile_dirty( mid_layer, (i * 2) );
				tilemap_mark_tile_dirty( mid_layer, (i * 2) + 1 );
			}
			index++;
		}

		/* text layer */
		for (i=0; i < 0x1000/4; i++) {
			UINT32 tile = spimainram[index];
			if (tilemap_ram[i+text_layer_offset] != tile) {
				tilemap_ram[i+text_layer_offset] = tile;
				tilemap_mark_tile_dirty( text_layer, (i * 2) );
				tilemap_mark_tile_dirty( text_layer, (i * 2) + 1 );
			}
			index++;
		}
	}
}

WRITE32_HANDLER( palette_dma_start_w )
{
	int i;
	for (i=0; i < ((video_dma_length+1) * 2) / 4; i++)
	{
		int r1,g1,b1,r2,g2,b2;

		UINT32 color = spimainram[(video_dma_address / 4) + i - 0x200];
		if (palette_ram[i] != color) {
			palette_ram[i] = color;
			b1 = ((palette_ram[i] & 0x7c000000) >>26) << 3;
			g1 = ((palette_ram[i] & 0x03e00000) >>21) << 3;
			r1 = ((palette_ram[i] & 0x001f0000) >>16) << 3;
			b2 = ((palette_ram[i] & 0x00007c00) >>10) << 3;
			g2 = ((palette_ram[i] & 0x000003e0) >>5) << 3;
			r2 = ((palette_ram[i] & 0x0000001f) >>0) << 3;

			palette_set_color( (i * 2), r2, g2, b2 );
			palette_set_color( (i * 2) + 1, r1, g1, b1 );
		}
	}
}

WRITE32_HANDLER( sprite_dma_start_w )
{
	memcpy( sprite_ram, &spimainram[(video_dma_address / 4) - 0x200], 0x1000);
}

WRITE32_HANDLER( video_dma_length_w )
{
	COMBINE_DATA( &video_dma_length );
}

WRITE32_HANDLER( video_dma_address_w )
{
	COMBINE_DATA( &video_dma_address );
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
	int transparency;
	int x,y, x1, y1;
	const struct GfxElement *gfx = Machine->gfx[2];

	if( layer_enable & 0x10 )
		return;

	for( a = 0x400 - 2; a >= 0; a -= 2 ) {
		tile_num = (sprite_ram[a + 0] >> 16) & 0xffff;
		if( sprite_ram[a + 1] & 0x1000 )
			tile_num |= 0x10000;

		if( !tile_num )
			continue;

		priority = (sprite_ram[a + 0] >> 6) & 0x3;
		if( pri_mask != priority )
			continue;

		xpos = sprite_ram[a + 1] & 0x3ff;
		if( xpos & 0x200 )
			xpos |= 0xfc00;
		ypos = (sprite_ram[a + 1] >> 16) & 0x1ff;
		if( ypos & 0x100 )
			ypos |= 0xfe00;
		color = (sprite_ram[a + 0] & 0x3f);

		if (color == 0x1c || color == 0x1e) {
			transparency = TRANSPARENCY_ALPHA;
			alpha_set_level(0x7f);
		}
		else {
			transparency = TRANSPARENCY_PEN;
		}


		width = ((sprite_ram[a + 0] >> 8) & 0x7) + 1;
		height = ((sprite_ram[a + 0] >> 12) & 0x7) + 1;
		flip_x = (sprite_ram[a + 0] >> 11) & 0x1;
		flip_y = (sprite_ram[a + 0] >> 15) & 0x1;
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
					transparency, 63
					);

				/* xpos seems to wrap-around to 0 at 512 */
				if( (xpos + (16 * x) + 16) >= 512 ) {
					drawgfx(
					bitmap,
					gfx,
					tile_num,
					color, flip_x, flip_y, xpos - 512 + sprite_xtable[flip_x][x], ypos + sprite_ytable[flip_y][y],
					cliprect,
					transparency, 63
					);
				}

				tile_num++;
			}
		}
	}
}

static void get_text_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs + text_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0, tile, color, 0)
}

static void get_back_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 13) & 0x7;

	tile &= 0x1fff;

	if( rf2_layer_bank[0] )
		tile |= 0x4000;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (tilemap_ram[offs + mid_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
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
	int tile = (tilemap_ram[offs + fore_layer_offset] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
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
	int i;
	text_layer	= tilemap_create( get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32 );
	back_layer	= tilemap_create( get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,16,32,32 );
	mid_layer	= tilemap_create( get_mid_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );
	fore_layer	= tilemap_create( get_fore_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );

	tilemap_set_transparent_pen(text_layer, 31);
	tilemap_set_transparent_pen(mid_layer, 63);
	tilemap_set_transparent_pen(fore_layer, 63);

	tilemap_ram = auto_malloc(0x4000);
	palette_ram = auto_malloc(0x3000);
	sprite_ram = auto_malloc(0x1000);
	memset(tilemap_ram, 0, 0x4000);
	memset(palette_ram, 0, 0x3000);
	memset(sprite_ram, 0, 0x1000);

	for (i=0; i < 6144; i++) {
		palette_set_color(i, 0, 0, 0);
	}

	if (old_vidhw) {
		fore_layer_offset = 0x800 / 4;
		mid_layer_offset = 0x1000 / 4;
		text_layer_offset = 0x1800 / 4;
	}
	else {
		fore_layer_offset = 0x1000 / 4;
		mid_layer_offset = 0x2000 / 4;
		text_layer_offset = 0x3000 / 4;
	}
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
		set_rowscroll(back_layer, 0, (INT16*)&tilemap_ram[0x200]);
		set_rowscroll(mid_layer, 1, (INT16*)&tilemap_ram[0x600]);
		set_rowscroll(fore_layer, 2, (INT16*)&tilemap_ram[0xa00]);
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
