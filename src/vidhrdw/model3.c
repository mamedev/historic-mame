#include "driver.h"
#include "vidhrdw/generic.h"


extern int model3_irq_state;

UINT64 *paletteram64;

static data8_t model3_layer_enable = 0xf;
static data32_t model3_layer_modulate;
static data32_t model3_layer_modulate1;
static data32_t model3_layer_modulate2;

static data64_t *m3_char_ram;
static data64_t *m3_tile_ram;
static data8_t *m3_dirty_map;
static int m3_gfx_index;
//static int m3_char_dirty;
//static struct tilemap *m3_tile_layer[4];

static UINT16 *pal_lookup;


#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

#define BYTE_REVERSE16(x)		(((x >> 8) & 0xff) | ((x << 8) & 0xff00))


#define MODEL3_NUM_TILES		32768

static struct GfxLayout m3_char_layout =
{
	8, 8,
	MODEL3_NUM_TILES,
	4,
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 1*32, 0*32, 3*32, 2*32, 5*32, 4*32, 7*32, 6*32 },
	8*32
};

/*static void m3_layer0_tile_info(int tile_index)
{
	UINT64 val = m3_tile_ram[tile_index/4];
	UINT16 tile_data = 0;
	switch(tile_index & 0x3)
	{
		case 0: tile_data = BYTE_REVERSE16((UINT16)(val >> 32)); break;
		case 1: tile_data = BYTE_REVERSE16((UINT16)(val >> 48)); break;
		case 2: tile_data = BYTE_REVERSE16((UINT16)(val >> 0)); break;
		case 3: tile_data = BYTE_REVERSE16((UINT16)(val >> 16)); break;
	}
	int color = (tile_data >> 4) & 0x7ff;
	int tile = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	SET_TILE_INFO(m3_gfx_index, tile, color, 0);
}

static void m3_layer1_tile_info(int tile_index)
{
	UINT64 val = m3_tile_ram[(tile_index/4)+1024];
	UINT16 tile_data = 0;
	switch(tile_index & 0x3)
	{
		case 0: tile_data = BYTE_REVERSE16((UINT16)(val >> 32)); break;
		case 1: tile_data = BYTE_REVERSE16((UINT16)(val >> 48)); break;
		case 2: tile_data = BYTE_REVERSE16((UINT16)(val >> 0)); break;
		case 3: tile_data = BYTE_REVERSE16((UINT16)(val >> 16)); break;
	}
	int color = (tile_data >> 4) & 0x7ff;
	int tile = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	SET_TILE_INFO(m3_gfx_index, tile, color, 0);
}

static void m3_layer2_tile_info(int tile_index)
{
	UINT64 val = m3_tile_ram[(tile_index/4)+2048];
	UINT16 tile_data = 0;
	switch(tile_index & 0x3)
	{
		case 0: tile_data = BYTE_REVERSE16((UINT16)(val >> 32)); break;
		case 1: tile_data = BYTE_REVERSE16((UINT16)(val >> 48)); break;
		case 2: tile_data = BYTE_REVERSE16((UINT16)(val >> 0)); break;
		case 3: tile_data = BYTE_REVERSE16((UINT16)(val >> 16)); break;
	}
	int color = (tile_data >> 4) & 0x7ff;
	int tile = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	SET_TILE_INFO(m3_gfx_index, tile, color, 0);
}

static void m3_layer3_tile_info(int tile_index)
{
	UINT64 val = m3_tile_ram[(tile_index/4)+3072];
	UINT16 tile_data = 0;
	switch(tile_index & 0x3)
	{
		case 0: tile_data = BYTE_REVERSE16((UINT16)(val >> 32)); break;
		case 1: tile_data = BYTE_REVERSE16((UINT16)(val >> 48)); break;
		case 2: tile_data = BYTE_REVERSE16((UINT16)(val >> 0)); break;
		case 3: tile_data = BYTE_REVERSE16((UINT16)(val >> 16)); break;
	}
	int color = (tile_data >> 4) & 0x7ff;
	int tile = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	SET_TILE_INFO(m3_gfx_index, tile, color, 0);
}

static void model3_tile_update(void)
{
	if(m3_char_dirty) {
		int i;
		for(i=0; i<MODEL3_NUM_TILES; i++) {
			if(m3_dirty_map[i]) {
				m3_dirty_map[i] = 0;
				decodechar(Machine->gfx[m3_gfx_index], i, (UINT8 *)m3_char_ram, &m3_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(m3_tile_layer[0]);
		tilemap_mark_all_tiles_dirty(m3_tile_layer[1]);
		tilemap_mark_all_tiles_dirty(m3_tile_layer[2]);
		tilemap_mark_all_tiles_dirty(m3_tile_layer[3]);
		m3_char_dirty = 0;
	}
}
*/


VIDEO_START( model3 )
{
	for(m3_gfx_index = 0; m3_gfx_index < MAX_GFX_ELEMENTS; m3_gfx_index++)
		if (Machine->gfx[m3_gfx_index] == 0)
			break;
	if(m3_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	m3_char_ram = auto_malloc(0x100000);
	if( !m3_char_ram )
		return 1;

	m3_tile_ram = auto_malloc(0x8000);
	if( !m3_tile_ram )
		return 1;

	m3_dirty_map = auto_malloc(MODEL3_NUM_TILES);
	if( !m3_dirty_map ) {
		free(m3_char_ram);
		free(m3_tile_ram);
		return 1;
	}

	/*m3_tile_layer[0] = tilemap_create(m3_layer0_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	m3_tile_layer[1] = tilemap_create(m3_layer1_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	m3_tile_layer[2] = tilemap_create(m3_layer2_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	m3_tile_layer[3] = tilemap_create(m3_layer3_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);

	tilemap_set_transparent_pen(m3_tile_layer[0], 0);
	tilemap_set_transparent_pen(m3_tile_layer[1], 0);
	tilemap_set_transparent_pen(m3_tile_layer[2], 0);
	tilemap_set_transparent_pen(m3_tile_layer[3], 0);

	if( !m3_tile_layer[0] || !m3_tile_layer[1] || !m3_tile_layer[2] || !m3_tile_layer[3] ) {
		free(m3_dirty_map);
		free(m3_tile_ram);
		free(m3_char_ram);
	}*/

	memset(m3_char_ram, 0, 0x100000);
	memset(m3_tile_ram, 0, 0x8000);
	memset(m3_dirty_map, 0, MODEL3_NUM_TILES);

	Machine->gfx[m3_gfx_index] = decodegfx((UINT8*)m3_char_ram, &m3_char_layout);
	if( !Machine->gfx[m3_gfx_index] ) {
		free(m3_dirty_map);
		free(m3_tile_ram);
		free(m3_char_ram);
	}

	if (Machine->drv->color_table_len)
	{
		Machine->gfx[m3_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[m3_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[m3_gfx_index]->colortable = Machine->pens;
		Machine->gfx[m3_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	pal_lookup = auto_malloc(65536*2);
	return 0;
}

static void draw_tile_4bit(struct mame_bitmap *bitmap, int tx, int ty, int tilenum)
{
	int x, y;
	UINT8 *tile_base = (UINT8*)m3_char_ram;
	UINT8 *tile;

	int tile_data = (BYTE_REVERSE16(tilenum));
	int c = tile_data & 0x7ff0;
	int tile_index = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	tile_index *= 32;

	tile = &tile_base[tile_index];

	for(y = ty; y < ty+8; y++) {
		UINT16 *d = (UINT16*)bitmap->line[y^1];
		for(x = tx; x < tx+8; x+=2) {
			UINT8 tile0, tile1;
			UINT16 pix0, pix1;
			tile0 = *tile >> 4;
			tile1 = *tile & 0xf;
			pix0 = pal_lookup[c + tile0];
			pix1 = pal_lookup[c + tile1];
			if((pix0 & 0x8000) == 0)
			{
				d[x+0] = pix0;
			}
			if((pix1 & 0x8000) == 0)
			{
				d[x+1] = pix1;
			}
			++tile;
		}
	}
}

static void draw_tile_8bit(struct mame_bitmap *bitmap, int tx, int ty, int tilenum)
{
	int x, y;
	UINT8 *tile_base = (UINT8*)m3_char_ram;
	UINT8 *tile;

	int tile_data = (BYTE_REVERSE16(tilenum));
	int c = tile_data & 0x7f00;
	int tile_index = ((tile_data << 1) & 0x7ffe) | ((tile_data >> 15) & 0x1);
	tile_index *= 32;

	tile = &tile_base[tile_index];

	for(y = ty; y < ty+8; y++) {
		UINT16 *d = (UINT16*)bitmap->line[y];
		for(x = tx; x < tx+8; x++) {
			UINT8 tile0;
			UINT16 pix;
			tile0 = *tile;
			pix = pal_lookup[c + tile0];
			if((pix & 0x8000) == 0)
			{
				d[x^4] = pix;
			}
			++tile;
		}
	}
}

static void draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer, int bitdepth)
{
	int x, y;
	int tile_index = 0;
	UINT16 *tiles = (UINT16*)&m3_tile_ram[layer * 0x400];

	if(layer > 1)
		model3_layer_modulate = model3_layer_modulate2;
	else
		model3_layer_modulate = model3_layer_modulate1;

	if(bitdepth)		/* 4-bit */
	{
		for(y = cliprect->min_y; y <= cliprect->max_y; y+=8)
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x+=8) {
				UINT16 tile = tiles[tile_index ^ 0x2];
				draw_tile_4bit(bitmap, x, y, tile);
				++tile_index;
			}
			tile_index += 2;
		}
	} 
	else				/* 8-bit */
	{
		for(y = cliprect->min_y; y <= cliprect->max_y; y+=8)
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x+=8) {
				UINT16 tile = tiles[tile_index ^ 0x2];
				draw_tile_8bit(bitmap, x, y, tile);
				++tile_index;
			}
			tile_index += 2;
		}
	}
}

VIDEO_UPDATE( model3 )
{
	fillbitmap(bitmap, 0, cliprect);
	/*model3_tile_update();
	tilemap_draw(bitmap, cliprect, m3_tile_layer[3], 0,0);
	tilemap_draw(bitmap, cliprect, m3_tile_layer[2], 0,0);
	tilemap_draw(bitmap, cliprect, m3_tile_layer[1], 0,0);
	tilemap_draw(bitmap, cliprect, m3_tile_layer[0], 0,0);*/
	draw_layer(bitmap, cliprect, 3, (model3_layer_enable >> 3) & 0x1);
	draw_layer(bitmap, cliprect, 2, (model3_layer_enable >> 2) & 0x1);
	//draw_layer(bitmap, cliprect, 1, (model3_layer_enable >> 1) & 0x1);
	draw_layer(bitmap, cliprect, 0, (model3_layer_enable >> 0) & 0x1);
}



READ64_HANDLER(model3_char_r)
{
	return m3_char_ram[offset];
}

WRITE64_HANDLER(model3_char_w)
{	
	COMBINE_DATA(&m3_char_ram[offset]);
	//m3_dirty_map[offset / 4] = 1;
	//m3_char_dirty = 1;
}

READ64_HANDLER(model3_tile_r)
{
	return m3_tile_ram[offset];
}

WRITE64_HANDLER(model3_tile_w)
{
	COMBINE_DATA(&m3_tile_ram[offset]);
	//tilemap_mark_tile_dirty(m3_tile_layer[offset>>10], ((offset & 0x3ff) * 4) + 0);
	//tilemap_mark_tile_dirty(m3_tile_layer[offset>>10], ((offset & 0x3ff) * 4) + 1);
	//tilemap_mark_tile_dirty(m3_tile_layer[offset>>10], ((offset & 0x3ff) * 4) + 2);
	//tilemap_mark_tile_dirty(m3_tile_layer[offset>>10], ((offset & 0x3ff) * 4) + 3);
}

READ64_HANDLER(model3_vid_reg_r)
{
	switch(offset)
	{
		case 0x20/8:	return (UINT64)model3_layer_enable << 52;
		case 0x40/8:	return ((UINT64)model3_layer_modulate1 << 32) | (UINT64)model3_layer_modulate2;
	}
	return 0;
}

WRITE64_HANDLER(model3_vid_reg_w)
{
	switch(offset)
	{
		case 0x00/8:	printf("vid_reg_w_0: %08X %08X\n", (UINT32)(data >> 32), (UINT32)data); break;
		case 0x10/8:	model3_irq_state = 0;	break;		/* VBL IRQ Ack */
		
		case 0x20/8:	model3_layer_enable = (data >> 52);		break;

		case 0x40/8:	model3_layer_modulate1 = (UINT32)(data >> 32);
						model3_layer_modulate2 = (UINT32)(data); break;
	}
}

WRITE64_HANDLER( model3_palette_w )
{
	int r1,g1,b1,r2,g2,b2;
	UINT32 data1,data2;

	COMBINE_DATA(&paletteram64[offset]);
	data1 = BYTE_REVERSE32((UINT32)(paletteram64[offset] >> 32));
	data2 = BYTE_REVERSE32((UINT32)(paletteram64[offset] >> 0));

	r1 = ((data1 >> 0) & 0x1f);
	g1 = ((data1 >> 5) & 0x1f);
	b1 = ((data1 >> 10) & 0x1f);
	r2 = ((data2 >> 0) & 0x1f);
	g2 = ((data2 >> 5) & 0x1f);
	b2 = ((data2 >> 10) & 0x1f);

	pal_lookup[(offset*2)+0] = (data1 & 0x8000) | (r1 << 10) | (g1 << 5) | b1;
	pal_lookup[(offset*2)+1] = (data2 & 0x8000) | (r2 << 10) | (g2 << 5) | b2;

	b1 = (b1 << 3) | (b1 >> 2);
	g1 = (g1 << 3) | (g1 >> 2);
	r1 = (r1 << 3) | (r1 >> 2);
	b2 = (b2 << 3) | (b2 >> 2);
	g2 = (g2 << 3) | (g2 >> 2);
	r2 = (r2 << 3) | (r2 >> 2);

	palette_set_color((offset*2)+0, r1, g1, b1);
	palette_set_color((offset*2)+1, r2, g2, b2);	
}

READ64_HANDLER( model3_palette_r )
{
	return paletteram64[offset];
}
