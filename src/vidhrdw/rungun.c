/*
   Run and Gun
   (c) 1993 Konami

   Video hardware emulation.

   Driver by R. Belmont
*/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int ttl_gfx_index;
static struct tilemap *ttl_tilemap;
static unsigned short ttl_vram[0x1000];

static int sprite_colorbase;

/* TTL text plane stuff */

static void ttl_get_tile_info(int tile_index)
{
	data32_t *lvram = (data32_t *)ttl_vram;
	int attr, code;

	code = (lvram[tile_index]>>16)&0xffff;
	code |= (lvram[tile_index]&0x0f)<<8;	/* tile "bank" */

	attr = ((lvram[tile_index]&0xf0)>>4);	/* palette */

	tile_info.flags = 0;

	SET_TILE_INFO(ttl_gfx_index, code, attr, tile_info.flags);
}

static UINT32 ttl_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */

	return row*64 + col;
}

static void rng_sprite_callback(int *code, int *color, int *priority_mask)
{
//	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
//	if (pri <= layerpri[3])								*priority_mask = 0;
//	else if (pri > layerpri[3] && pri <= layerpri[2])	*priority_mask = 0xff00;
//	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xff00|0xf0f0;
//	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xff00|0xf0f0|0xcccc;
//	else 							*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa;

	*priority_mask = 0;
	*color = sprite_colorbase | (*color & 0x001f);
}

READ16_HANDLER( ttl_ram_r )
{
	return(ttl_vram[offset]);
}

WRITE16_HANDLER( ttl_ram_w )
{
	COMBINE_DATA(&ttl_vram[offset]);
}


VIDEO_START(rng)
{
	static struct GfxLayout charlayout =
	{
		8, 8,		// 8x8
		4096,		// # of tiles
		4,	   	// 4bpp
		{ 0, 1, 2, 3 },	// plane offsets
		{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },		// X offsets
		{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },	// Y offsets
		8*8*4
	};

	K053251_vh_start();

	if (K055673_vh_start(REGION_GFX2, 1, -28, 32, rng_sprite_callback))
	{
		return 1;
	}

	/* find first empty slot to decode gfx */
	for (ttl_gfx_index = 0; ttl_gfx_index < MAX_GFX_ELEMENTS; ttl_gfx_index++)
		if (Machine->gfx[ttl_gfx_index] == 0)
			break;

	if (ttl_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	// decode the ttl layer's gfx
	Machine->gfx[ttl_gfx_index] = decodegfx(memory_region(REGION_GFX3), &charlayout);

	if (Machine->drv->color_table_len)
	{
	        Machine->gfx[ttl_gfx_index]->colortable = Machine->remapped_colortable;
	        Machine->gfx[ttl_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
	        Machine->gfx[ttl_gfx_index]->colortable = Machine->pens;
	        Machine->gfx[ttl_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	// create the tilemap
	ttl_tilemap = tilemap_create(ttl_get_tile_info, ttl_scan, TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(ttl_tilemap, 0);

	state_save_register_UINT16("RnGTTL", 0, "VRAM", ttl_vram, 0x1000);

	sprite_colorbase = 0x20;

	return 0;
}

VIDEO_UPDATE(rng)
{
	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	tilemap_mark_all_tiles_dirty(ttl_tilemap);
	K053247_sprites_draw(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, ttl_tilemap, 0, 1<<0);
}
