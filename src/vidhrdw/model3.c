#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/poly.h"
#include <math.h>


#define BILINEAR	1



typedef float MATRIX[4][4];
typedef float VECTOR[4];
typedef float VECTOR3[3];

typedef struct {
	float x,y,z;
	UINT32 u,v;
} VERTEX;

typedef struct {
	float x,y,z,d;
} PLANE;

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define RADIAN_TO_DEGREE(x)		((x) * 180.0 / PI)
#define max(x,y)		(((int)(x) > (int)(y)) ? (x) : (y))
#define min(x,y)		(((int)(x) < (int)(y)) ? (x) : (y))


/* forward declarations */
static void real3d_traverse_display_list(void);
static void draw_model(UINT32*);
static void init_matrix_stack(void);
static void get_top_matrix(MATRIX *out);
static void push_matrix_stack(void);
static void pop_matrix_stack(void);
static void multiply_matrix_stack(MATRIX matrix);
static void translate_matrix_stack(float x, float y, float z);
static void traverse_list(UINT32 address);
static void traverse_node(UINT32 address);
static void traverse_root_node(UINT32 address);

/*****************************************************************************/

extern int model3_irq_state;

extern int model3_step;
extern int model3_draw_crosshair;

UINT64 *paletteram64;
static UINT32 *vrom;

static data8_t model3_layer_enable = 0;
static data32_t layer_modulate_r;
static data32_t layer_modulate_g;
static data32_t layer_modulate_b;
static data32_t model3_layer_modulate1;
static data32_t model3_layer_modulate2;
static data64_t layer_scroll[2];

static data64_t *m3_char_ram;
static data64_t *m3_tile_ram;

static data32_t *texture_fifo;
static int texture_fifo_pos = 0;

static UINT16 *texture_ram[2];
static UINT32 *display_list_ram;
static UINT32 *culling_ram;
static UINT32 *polygon_ram;

static UINT16 *pal_lookup;

static int real3d_display_list = 0;

static struct mame_bitmap *bitmap3d;
static struct mame_bitmap *zbuffer;
static struct rectangle clip3d;
static struct rectangle *screen_clip;

#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

#define BYTE_REVERSE16(x)		(((x >> 8) & 0xff) | ((x << 8) & 0xff00))



VIDEO_START( model3 )
{
	vrom = (UINT32*)memory_region(REGION_USER2);

	bitmap3d = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!bitmap3d)
		return 1;

	zbuffer = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 32);
	if (!zbuffer)
		return 1;

	m3_char_ram = auto_malloc(0x100000);
	if( !m3_char_ram )
		return 1;

	m3_tile_ram = auto_malloc(0x8000);
	if( !m3_tile_ram )
		return 1;

	memset(m3_char_ram, 0, 0x100000);
	memset(m3_tile_ram, 0, 0x8000);

	pal_lookup = auto_malloc(65536*2);
	memset(pal_lookup, 0, 65536*2);

	texture_fifo = auto_malloc(0x100000);
	if( !texture_fifo ) {
		return 1;
	}

	/* 2x 4MB texture sheets */
	texture_ram[0] = auto_malloc(0x400000);
	texture_ram[1] = auto_malloc(0x400000);

	/* 1MB Display List RAM */
	display_list_ram = auto_malloc(0x100000);
	/* 4MB for nodes (< Step 2.0 have only 2MB) */
	culling_ram = auto_malloc(0x400000);
	/* 4MB Polygon RAM */
	polygon_ram = auto_malloc(0x400000);

	init_matrix_stack();
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
		int xx = 0;
		for(x = tx; x < tx+8; x++) {
			UINT8 tile0;
			UINT16 pix;
			tile0 = tile[xx^4];
			pix = pal_lookup[c + tile0];
			if((pix & 0x8000) == 0)
			{
				d[x] = pix;
			}
			++xx;
		}
		tile += 8;
	}
}
#if 0
static void draw_texture_sheet(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int x,y;
	for(y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *d = (UINT16*)bitmap->line[y];
		int index = (y*2)*2048;
		for(x = cliprect->min_x; x <= cliprect->max_x; x++) {
			UINT16 pix = texture_ram[0][index];
			index+=4;
			if(pix != 0) {
				d[x] = pix;
			}
		}
	}
}
#endif

static void draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer, int bitdepth)
{
	int x, y;
	int tile_index = 0;
	UINT16 *tiles = (UINT16*)&m3_tile_ram[layer * 0x400];

	//printf("Layer %d: X: %d, Y: %d\n", layer, x1, y1);

	if(layer > 1) {
		int modr = (model3_layer_modulate2 >> 8) & 0xff;
		int modg = (model3_layer_modulate2 >> 16) & 0xff;
		int modb = (model3_layer_modulate2 >> 24) & 0xff;
		if(modr & 0x80) {
			layer_modulate_r = -(0x7f - (modr & 0x7f)) << 10;
		} else {
			layer_modulate_r = (modr & 0x7f) << 10;
		}
		if(modg & 0x80) {
			layer_modulate_g = -(0x7f - (modr & 0x7f)) << 5;
		} else {
			layer_modulate_g = (modr & 0x7f) << 5;
		}
		if(modb & 0x80) {
			layer_modulate_b = -(0x7f - (modr & 0x7f));
		} else {
			layer_modulate_b = (modr & 0x7f);
		}
	} else {
		int modr = (model3_layer_modulate1 >> 8) & 0xff;
		int modg = (model3_layer_modulate1 >> 16) & 0xff;
		int modb = (model3_layer_modulate1 >> 24) & 0xff;
		if(modr & 0x80) {
			layer_modulate_r = -(0x7f - (modr & 0x7f)) << 10;
		} else {
			layer_modulate_r = (modr & 0x7f) << 10;
		}
		if(modg & 0x80) {
			layer_modulate_g = -(0x7f - (modr & 0x7f)) << 5;
		} else {
			layer_modulate_g = (modr & 0x7f) << 5;
		}
		if(modb & 0x80) {
			layer_modulate_b = -(0x7f - (modr & 0x7f));
		} else {
			layer_modulate_b = (modr & 0x7f);
		}
	}

	if(bitdepth)		/* 4-bit */
	{
		for(y = cliprect->min_y; y <= cliprect->max_y; y+=8)
		{
			tile_index = ((y/8) * 64);
			for (x = cliprect->min_x; x <= cliprect->max_x; x+=8) {
				UINT16 tile = tiles[tile_index ^ 0x2];
				draw_tile_4bit(bitmap, x, y, tile);
				++tile_index;
			}
		}
	}
	else				/* 8-bit */
	{
		for(y = cliprect->min_y; y <= cliprect->max_y; y+=8)
		{
			tile_index = ((y/8) * 64);
			for (x = cliprect->min_x; x <= cliprect->max_x; x+=8) {
				UINT16 tile = tiles[tile_index ^ 0x2];
				draw_tile_8bit(bitmap, x, y, tile);
				++tile_index;
			}
		}
	}
}

static void copy_screen(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int x,y;
	for(y=cliprect->min_y; y <= cliprect->max_y; y++) {
		UINT16 *d = (UINT16*)bitmap->line[y];
		UINT16 *s = (UINT16*)bitmap3d->line[y];
		for(x=cliprect->min_x; x <= cliprect->max_x; x++) {
			UINT16 pix = s[x];
			if(!(pix & 0x8000)) {
				d[x] = pix;
			}
		}
	}
}

static int tick = 0;
static int debug_layer_disable = 0;

VIDEO_UPDATE( model3 )
{
	int layer_scroll_x[4], layer_scroll_y[4];
	UINT32 layer_data[4];

	layer_data[0] = BYTE_REVERSE32((UINT32)(layer_scroll[0] >> 32));
	layer_data[1] = BYTE_REVERSE32((UINT32)(layer_scroll[0] >> 0));
	layer_data[2] = BYTE_REVERSE32((UINT32)(layer_scroll[1] >> 32));
	layer_data[3] = BYTE_REVERSE32((UINT32)(layer_scroll[1] >> 0));
	layer_scroll_x[0] = (layer_data[0] & 0x8000) ? (layer_data[0] & 0x1ff) : -(layer_data[0] & 0x1ff);
	layer_scroll_y[0] = (layer_data[0] & 0x8000) ? (layer_data[0] & 0x1ff) : -(layer_data[0] & 0x1ff);
	layer_scroll_x[1] = (layer_data[1] & 0x8000) ? (layer_data[1] & 0x1ff) : -(layer_data[1] & 0x1ff);
	layer_scroll_y[1] = (layer_data[1] & 0x8000) ? (layer_data[1] & 0x1ff) : -(layer_data[1] & 0x1ff);
	layer_scroll_x[2] = (layer_data[2] & 0x8000) ? (layer_data[2] & 0x1ff) : -(layer_data[2] & 0x1ff);
	layer_scroll_y[2] = (layer_data[2] & 0x8000) ? (layer_data[2] & 0x1ff) : -(layer_data[2] & 0x1ff);
	layer_scroll_x[3] = (layer_data[3] & 0x8000) ? (layer_data[3] & 0x1ff) : -(layer_data[3] & 0x1ff);
	layer_scroll_y[3] = (layer_data[3] & 0x8000) ? (layer_data[3] & 0x1ff) : -(layer_data[3] & 0x1ff);

	//bitmap3d = bitmap;
	screen_clip = (struct rectangle*)cliprect;

	/* layer disable debug keys */
	tick++;
	if( tick >= 5 ) {
		tick = 0;

		if( code_pressed(KEYCODE_Y) )
			debug_layer_disable ^= 0x1;
		if( code_pressed(KEYCODE_U) )
			debug_layer_disable ^= 0x2;
		if( code_pressed(KEYCODE_I) )
			debug_layer_disable ^= 0x4;
		if( code_pressed(KEYCODE_O) )
			debug_layer_disable ^= 0x8;
		if( code_pressed(KEYCODE_T) )
			debug_layer_disable ^= 0x10;
	}

	fillbitmap(bitmap, 0, cliprect);

	if(!(debug_layer_disable & 0x8)) {
		draw_layer(bitmap, cliprect, 3, (model3_layer_enable >> 3) & 0x1);
	}
	if(!(debug_layer_disable & 0x4)) {
		draw_layer(bitmap, cliprect, 2, (model3_layer_enable >> 2) & 0x1);
	}

	if( !(debug_layer_disable & 0x10) )
	{
		if(real3d_display_list) {
			fillbitmap(bitmap3d, 0x8000, cliprect);
			fillbitmap(zbuffer, 0, cliprect);
			real3d_traverse_display_list();
			copy_screen(bitmap, cliprect);
		} else {
			copy_screen(bitmap, cliprect);
		}
	}

	if(!(debug_layer_disable & 0x2)) {
		draw_layer(bitmap, cliprect, 1, (model3_layer_enable >> 1) & 0x1);
	}
	if(!(debug_layer_disable & 0x1)) {
		draw_layer(bitmap, cliprect, 0, (model3_layer_enable >> 0) & 0x1);
	}

	if(model3_draw_crosshair) {
		int gun1_x, gun1_y, gun2_x, gun2_y;
		gun1_x = readinputport(5);
		gun1_y = readinputport(6);
		gun2_x = readinputport(7);
		gun2_y = readinputport(8);
		draw_crosshair(bitmap, gun1_x, gun1_y, cliprect);
		draw_crosshair(bitmap, gun2_x, gun2_y, cliprect);
	}

	//draw_texture_sheet(bitmap, cliprect);

	real3d_display_list = 0;
}



READ64_HANDLER(model3_char_r)
{
	return m3_char_ram[offset];
}

WRITE64_HANDLER(model3_char_w)
{
	COMBINE_DATA(&m3_char_ram[offset]);
}

READ64_HANDLER(model3_tile_r)
{
	return m3_tile_ram[offset];
}

WRITE64_HANDLER(model3_tile_w)
{
	COMBINE_DATA(&m3_tile_ram[offset]);
}

static UINT64 vid_reg0;
READ64_HANDLER(model3_vid_reg_r)
{
	switch(offset)
	{
		case 0x00/8:	return vid_reg0;
		case 0x08/8:	return U64(0xffffffffffffffff);		/* ??? */
		case 0x20/8:	return (UINT64)model3_layer_enable << 52;
		case 0x40/8:	return ((UINT64)model3_layer_modulate1 << 32) | (UINT64)model3_layer_modulate2;
		default:		printf("read reg %02X\n", offset);break;
	}
	return 0;
}

WRITE64_HANDLER(model3_vid_reg_w)
{
	switch(offset)
	{
		case 0x00/8:	printf("vid_reg0: %08X%08X\n", (UINT32)(data>>32),(UINT32)(data)); vid_reg0 = data; break;
		case 0x08/8:	break;		/* ??? */
		case 0x10/8:	model3_irq_state &= ~(data >> 56); break;		/* VBL IRQ Ack */

		case 0x20/8:	model3_layer_enable = (data >> 52);	break;

		case 0x40/8:	model3_layer_modulate1 = (UINT32)(data >> 32);
						model3_layer_modulate2 = (UINT32)(data);
						break;
		case 0x60/8:	COMBINE_DATA(&layer_scroll[0]); break;
		case 0x68/8:	COMBINE_DATA(&layer_scroll[1]); break;
		default:		printf("model3_vid_reg_w: %02X, %08X%08X\n", offset, (UINT32)(data >> 32), (UINT32)(data)); break;
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
}

READ64_HANDLER( model3_palette_r )
{
	return paletteram64[offset];
}


/*****************************************************************************/
/* Real3D Graphics stuff */

WRITE64_HANDLER( real3d_display_list_w )
{
	if(!(mem_mask & U64(0xffffffff00000000))) {
		display_list_ram[offset*2] = BYTE_REVERSE32((UINT32)(data >> 32));
	}
	if(!(mem_mask & U64(0x00000000ffffffff))) {
		display_list_ram[(offset*2)+1] = BYTE_REVERSE32((UINT32)(data));
	}
}

WRITE64_HANDLER( real3d_polygon_ram_w )
{
	if(!(mem_mask & U64(0xffffffff00000000))) {
		polygon_ram[offset*2] = BYTE_REVERSE32((UINT32)(data >> 32));
	}
	if(!(mem_mask & U64(0x00000000ffffffff))) {
		polygon_ram[(offset*2)+1] = BYTE_REVERSE32((UINT32)(data));
	}
}

const unsigned char texture_decode[64] =
{
	 0,  1,  4,  5,  8,  9, 12, 13,
	 2,  3,  6,  7, 10, 11, 14, 15,
	16, 17, 20, 21, 24, 25, 28, 29,
	18, 19, 22, 23, 26, 27, 30, 31,
	32, 33, 36, 37, 40, 41, 44, 45,
	34, 35, 38, 39, 42, 43, 46, 47,
	48, 49, 52, 53, 56, 57, 60, 61,
	50, 51, 54, 55, 58, 59, 62, 63
};

INLINE void write_texture16(int xpos, int ypos, int width, int height, int page, UINT16 *data)
{
	int x,y,i,j;

	for(y=ypos; y < ypos+height; y+=8)
	{
		for(x=xpos; x < xpos+width; x+=8)
		{
			UINT16 *texture = &texture_ram[page][y*2048+x];
			int b = 0;
			for(j=y; j < y+8; j++) {
				for(i=x; i < x+8; i++) {
					*texture++ = data[texture_decode[b^1]];
					++b;
				}
				texture += 2048-8;
			}
			data += 64;
		}
	}
}

INLINE void write_texture8(int xpos, int ypos, int width, int height, int page, UINT16 *data)
{
	int x,y,i,j;
	UINT16 color = 0x7c00;

	for(y=ypos; y < ypos+(height/2); y+=4)
	{
		for(x=xpos; x < xpos+width; x+=8)
		{
			UINT16 *texture = &texture_ram[page][y*2048+x];
			int b = 0;
			for(j=y; j < y+4; j++) {
				for(i=x; i < x+8; i++) {
					*texture = color;
					texture++;
					++b;
				}
				texture += 2048-8;
			}
		}
	}
}

static void real3d_upload_texture(UINT32 header, UINT32 *data)
{
	int width	= 32 << ((header >> 14) & 0x7);
	int height	= 32 << ((header >> 17) & 0x7);
	int xpos	= (header & 0x3f) * 32;
	int ypos	= ((header >> 7) & 0x1f) * 32;
	int page	= (header >> 20) & 0x1;
	int bitdepth = (header >> 23) & 0x1;

	switch(header >> 24)
	{
		case 0x00:		/* Texture with mipmaps */
			if(bitdepth) {
				write_texture16(xpos, ypos, width, height, page, (UINT16*)data);
			} else {
				/* TODO: 8-bit textures are weird. need to figure out some additional bits */
				//printf("W: %d, H: %d, X: %d, Y: %d, P: %d, Bit: %d, : %08X, %08X\n", width, height, xpos, ypos, page, bitdepth, header & 0x00681040, header);
				//write_texture8(xpos, ypos, width, height, page, (UINT16*)data);
			}
			break;
		case 0x01:		/* Texture without mipmaps */
			if(bitdepth) {
				write_texture16(xpos, ypos, width, height, page, (UINT16*)data);
			} else {
				/* TODO: 8-bit textures are weird. need to figure out some additional bits */
				//printf("W: %d, H: %d, X: %d, Y: %d, P: %d, Bit: %d, : %08X, %08X\n", width, height, xpos, ypos, page, bitdepth, header & 0x00681040, header);
				//write_texture8(xpos, ypos, width, height, page, (UINT16*)data);
			}
			break;
		case 0x02:		/* Only mipmaps */
			break;
		case 0x80:		/* Gamma-table ? */
			break;
		default:
			osd_die("Unknown texture type: %02X: ", header >> 24);
			break;
	}
}

void real3d_display_list_end(void)
{
	/* upload textures if there are any in the FIFO */
	if (texture_fifo_pos > 0)
	{
		int i = 0;
		while(i < texture_fifo_pos)
		{
			int length = (texture_fifo[i] / 2) + 2;
			UINT32 header = texture_fifo[i+1];
			real3d_upload_texture(header, &texture_fifo[i+2]);
			i += length;
		};
	}
	texture_fifo_pos = 0;
	real3d_display_list = 1;
}

void real3d_display_list1_dma(UINT32 src, UINT32 dst, int length, int byteswap)
{
	int i;
	int d = (dst & 0xffffff) / 4;
	for(i=0; i < length; i+=4) {
		UINT32 w;
		if (byteswap) {
			w = BYTE_REVERSE32(program_read_dword_64le(src^4));
		} else {
			w = program_read_dword_64le(src^4);
		}
		display_list_ram[d++] = w;
		src += 4;
	}
}

void real3d_display_list2_dma(UINT32 src, UINT32 dst, int length, int byteswap)
{
	int i;
	int d = (dst & 0xffffff) / 4;
	for(i=0; i < length; i+=4) {
		UINT32 w;
		if (byteswap) {
			w = BYTE_REVERSE32(program_read_dword_64le(src^4));
		} else {
			w = program_read_dword_64le(src^4);
		}
		culling_ram[d++] = w;
		src += 4;
	}
}

void real3d_vrom_texture_dma(UINT32 src, UINT32 dst, int length, int byteswap)
{
	if((dst & 0xff) == 0) {

		UINT32 address, header;
		UINT32 *rom;

		if (byteswap) {
			address = BYTE_REVERSE32(program_read_dword_64le((src+0)^4));
			header = BYTE_REVERSE32(program_read_dword_64le((src+4)^4));
		} else {
			address = program_read_dword_64le((src+0)^4);
			header = program_read_dword_64le((src+4)^4);
		}
		rom = (UINT32*)memory_region(REGION_USER2);
		real3d_upload_texture(header, (UINT32*)&rom[address]);
	}
}

void real3d_texture_fifo_dma(UINT32 src, int length, int byteswap)
{
	int i;
	for(i=0; i < length; i+=4) {
		UINT32 w;
		if (byteswap) {
			w = BYTE_REVERSE32(program_read_dword_64le(src^4));
		} else {
			w = program_read_dword_64le(src^4);
		}
		texture_fifo[texture_fifo_pos] = w;
		texture_fifo_pos++;
		src += 4;
	}
}

void real3d_polygon_ram_dma(UINT32 src, UINT32 dst, int length, int byteswap)
{
	int i;
	int d = (dst & 0xffffff) / 4;
	for(i=0; i < length; i+=4) {
		UINT32 w;
		if (byteswap) {
			w = BYTE_REVERSE32(program_read_dword_64le(src^4));
		} else {
			w = program_read_dword_64le(src^4);
		}
		polygon_ram[d++] = w;
		src += 4;
	}
}

WRITE64_HANDLER( real3d_cmd_w )
{
	real3d_display_list_end();
}

/*****************************************************************************/
/* matrix and vector operations */

INLINE float dot_product(VECTOR a, VECTOR b)
{
	return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
}

INLINE float dot_product3(VECTOR3 a, VECTOR3 b)
{
	return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

/* multiplies a 4-element vector by a 4x4 matrix */
static void matrix_multiply_vector(MATRIX matrix, VECTOR v, VECTOR *p)
{
	VECTOR out;
	out[0] = (v[0] * matrix[0][0]) + (v[1] * matrix[1][0]) + (v[2] * matrix[2][0]) + (v[3] * matrix[3][0]);
	out[1] = (v[0] * matrix[0][1]) + (v[1] * matrix[1][1]) + (v[2] * matrix[2][1]) + (v[3] * matrix[3][1]);
	out[2] = (v[0] * matrix[0][2]) + (v[1] * matrix[1][2]) + (v[2] * matrix[2][2]) + (v[3] * matrix[3][2]);
	out[3] = (v[0] * matrix[0][3]) + (v[1] * matrix[1][3]) + (v[2] * matrix[2][3]) + (v[3] * matrix[3][3]);
	memcpy(p, &out, sizeof(VECTOR));
}

/* multiplies a 4x4 matrix with another 4x4 matrix */
static void matrix_multiply(MATRIX a, MATRIX b, MATRIX *out)
{
	int i,j;
	MATRIX tmp;

	for( i=0; i < 4; i++ ) {
		for( j=0; j < 4; j++ ) {
			tmp[i][j] = (a[i][0] * b[0][j]) + (a[i][1] * b[1][j]) + (a[i][2] * b[2][j]) + (a[i][3] * b[3][j]);
		}
	}
	memcpy(out, &tmp, sizeof(MATRIX));
}

/* matrix stack */
#define MATRIX_STACK_SIZE	256

static int matrix_stack_ptr = 0;
static MATRIX matrix_stack[MATRIX_STACK_SIZE];

static void init_matrix_stack(void)
{
	/* initialize the first matrix as identity */
	matrix_stack[0][0][0] = 1.0f;
	matrix_stack[0][0][1] = 0.0f;
	matrix_stack[0][0][2] = 0.0f;
	matrix_stack[0][0][3] = 0.0f;
	matrix_stack[0][1][0] = 0.0f;
	matrix_stack[0][1][1] = 1.0f;
	matrix_stack[0][1][2] = 0.0f;
	matrix_stack[0][1][3] = 0.0f;
	matrix_stack[0][2][0] = 0.0f;
	matrix_stack[0][2][1] = 0.0f;
	matrix_stack[0][2][2] = 1.0f;
	matrix_stack[0][2][3] = 0.0f;
	matrix_stack[0][3][0] = 0.0f;
	matrix_stack[0][3][1] = 0.0f;
	matrix_stack[0][3][2] = 0.0f;
	matrix_stack[0][3][3] = 1.0f;

	matrix_stack_ptr = 0;
}

static void get_top_matrix(MATRIX *out)
{
	memcpy( out, &matrix_stack[matrix_stack_ptr], sizeof(MATRIX));
}

void push_matrix_stack(void)
{
	matrix_stack_ptr++;
	if (matrix_stack_ptr >= MATRIX_STACK_SIZE)
	{
		osd_die("push_matrix_stack: matrix stack overflow");
	}

	memcpy( &matrix_stack[matrix_stack_ptr], &matrix_stack[matrix_stack_ptr-1], sizeof(MATRIX));
}

void pop_matrix_stack(void)
{
	matrix_stack_ptr--;
	if (matrix_stack_ptr < 0)
	{
		osd_die("pop_matrix_stack: matrix stack underflow");
	}
}

void multiply_matrix_stack(MATRIX matrix)
{
	matrix_multiply(matrix, matrix_stack[matrix_stack_ptr], &matrix_stack[matrix_stack_ptr]);
}

void translate_matrix_stack(float x, float y, float z)
{
	MATRIX tm;

	tm[0][0] = 1.0f;	tm[0][1] = 0.0f;	tm[0][2] = 0.0f;	tm[0][3] = 0.0f;
	tm[1][0] = 0.0f;	tm[1][1] = 1.0f;	tm[1][2] = 0.0f;	tm[1][3] = 0.0f;
	tm[2][0] = 0.0f;	tm[2][1] = 0.0f;	tm[2][2] = 1.0f;	tm[2][3] = 0.0f;
	tm[3][0] = x;		tm[3][1] = y;		tm[3][2] = z;		tm[3][3] = 1.0f;

	matrix_multiply(tm, matrix_stack[matrix_stack_ptr], &matrix_stack[matrix_stack_ptr]);
}

/*****************************************************************************/
/* transformation and rasterizing */

static int texture_x;
static int texture_y;
static int texture_width;
static int texture_height;
static UINT32 texture_width_mask;
static UINT32 texture_height_mask;
static int texture_page;
static int texture_coord_shift = 16;

static UINT32 viewport_priority;

#define ZBUFFER_SCALE		16777216.0
#define ZDIVIDE_SHIFT		16

/* TODO: make the render functions with macros instead of copy-pasting */

static void draw_triangle_tex1555(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;

	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = (UINT16*)bitmap3d->line[y];
			UINT32 *d = (UINT32*)zbuffer->line[y];

			x1 = scan->sx;
			x2 = scan->ex;
			if(x1 < clip3d.min_x)
				x1 = clip3d.min_x;
			if(x2 > clip3d.max_x)
				x2 = clip3d.max_x;

			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			if(x1 < clip3d.max_x && x2 > clip3d.min_x) {
				for(x = x1; x <= x2; x++) {
//					UINT16 pix;
					int iu, iv;

					UINT32 iz = z >> 16;

					if (iz) {
						u2 = (u << ZDIVIDE_SHIFT) / iz;
						v2 = (v << ZDIVIDE_SHIFT) / iz;
					} else {
						u2 = 0;
						v2 = 0;
					}

					iz |= viewport_priority;

					if(iz > d[x]) {
						iu = (u2 >> texture_coord_shift) & texture_width_mask;
						iv = (v2 >> texture_coord_shift) & texture_height_mask;
#if BILINEAR
						{
							int iu2 = ((u2 >> texture_coord_shift) + 1) & texture_width_mask;
							int iv2 = ((v2 >> texture_coord_shift) + 1) & texture_height_mask;
							UINT32 sr[4], sg[4], sb[4];
							UINT32 ur[2], ug[2], ub[2];
							UINT32 fr, fg, fb;
							UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
							UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
							UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
							UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
							int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
							int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
							int u_sub0 = 0xffff - u_sub1;
							int v_sub0 = 0xffff - v_sub1;
							sr[0] = (pix0 & 0x7c00);
							sg[0] = (pix0 & 0x03e0);
							sb[0] = (pix0 & 0x001f);
							sr[1] = (pix1 & 0x7c00);
							sg[1] = (pix1 & 0x03e0);
							sb[1] = (pix1 & 0x001f);
							sr[2] = (pix2 & 0x7c00);
							sg[2] = (pix2 & 0x03e0);
							sb[2] = (pix2 & 0x001f);
							sr[3] = (pix3 & 0x7c00);
							sg[3] = (pix3 & 0x03e0);
							sb[3] = (pix3 & 0x001f);

							/* Calculate weighted U-samples */
							ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
							ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
							ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
							ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
							ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
							ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
							/* Calculate the final sample */
							fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
							fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
							fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));
							p[x] = (fr & 0x7c00) | (fg & 0x3e0) | (fb & 0x1f);
						}
#else
						pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						p[x] = pix & 0x7fff;
#endif
						d[x] = iz;		/* write new zbuffer value */
					}

					z += dz;
					u += du;
					v += dv;
				}
			}
		}
	}
}

static void draw_triangle_tex4444(VERTEX v1, VERTEX v2, VERTEX v3)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;
	v1.u = (UINT32)((UINT64)(v1.u * v1.z) >> ZDIVIDE_SHIFT);
	v1.v = (UINT32)((UINT64)(v1.v * v1.z) >> ZDIVIDE_SHIFT);
	v2.u = (UINT32)((UINT64)(v2.u * v2.z) >> ZDIVIDE_SHIFT);
	v2.v = (UINT32)((UINT64)(v2.v * v2.z) >> ZDIVIDE_SHIFT);
	v3.u = (UINT32)((UINT64)(v3.u * v3.z) >> ZDIVIDE_SHIFT);
	v3.v = (UINT32)((UINT64)(v3.v * v3.z) >> ZDIVIDE_SHIFT);

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;	vert[0].p[1] = v1.u;	vert[0].p[2] = v1.v;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;	vert[1].p[1] = v2.u;	vert[1].p[2] = v2.v;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;	vert[2].p[1] = v3.u;	vert[2].p[2] = v3.v;
	scans = setup_triangle_3(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 du, dv, dz;
		dz = scans->dp[0];
		du = scans->dp[1];
		dv = scans->dp[2];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 u, v, z;
			INT64 u2, v2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = (UINT16*)bitmap3d->line[y];
			UINT32 *d = (UINT32*)zbuffer->line[y];

			x1 = scan->sx;
			x2 = scan->ex;
			if(x1 < clip3d.min_x)
				x1 = clip3d.min_x;
			if(x2 > clip3d.max_x)
				x2 = clip3d.max_x;

			z = scans->scanline[y - scans->sy].p[0];
			u = scans->scanline[y - scans->sy].p[1];
			v = scans->scanline[y - scans->sy].p[2];

			if(x1 < clip3d.max_x && x2 > clip3d.min_x) {
				for(x = x1; x <= x2; x++) {
//					UINT16 pix;
//					UINT16 r,g,b;
					int iu, iv;

					UINT32 iz = z >> 16;

					if (iz) {
						u2 = (u << ZDIVIDE_SHIFT) / iz;
						v2 = (v << ZDIVIDE_SHIFT) / iz;
					} else {
						u2 = 0;
						v2 = 0;
					}

					iz |= viewport_priority;

					if(iz > d[x]) {
						iu = (u2 >> texture_coord_shift) & texture_width_mask;
						iv = (v2 >> texture_coord_shift) & texture_height_mask;
#if BILINEAR
						{
							int iu2 = ((u2 >> texture_coord_shift) + 1) & texture_width_mask;
							int iv2 = ((v2 >> texture_coord_shift) + 1) & texture_height_mask;
							UINT32 sr[4], sg[4], sb[4];
							UINT32 ur[2], ug[2], ub[2];
							UINT32 fr, fg, fb;
							UINT16 pix0 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
							UINT16 pix1 = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu2)];
							UINT16 pix2 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu)];
							UINT16 pix3 = texture_ram[texture_page][(texture_y+iv2) * 2048 + (texture_x+iu2)];
							int u_sub1 = (u2 >> (texture_coord_shift-16)) & 0xffff;
							int v_sub1 = (v2 >> (texture_coord_shift-16)) & 0xffff;
							int u_sub0 = 0xffff - u_sub1;
							int v_sub0 = 0xffff - v_sub1;
							sr[0] = (pix0 & 0xf000);
							sg[0] = (pix0 & 0x0f00);
							sb[0] = (pix0 & 0x00f0);
							sr[1] = (pix1 & 0xf000);
							sg[1] = (pix1 & 0x0f00);
							sb[1] = (pix1 & 0x00f0);
							sr[2] = (pix2 & 0xf000);
							sg[2] = (pix2 & 0x0f00);
							sb[2] = (pix2 & 0x00f0);
							sr[3] = (pix3 & 0xf000);
							sg[3] = (pix3 & 0x0f00);
							sb[3] = (pix3 & 0x00f0);

							/* Calculate weighted U-samples */
							ur[0] = (((sr[0] * u_sub0) >> 16) + ((sr[1] * u_sub1) >> 16));
							ug[0] = (((sg[0] * u_sub0) >> 16) + ((sg[1] * u_sub1) >> 16));
							ub[0] = (((sb[0] * u_sub0) >> 16) + ((sb[1] * u_sub1) >> 16));
							ur[1] = (((sr[2] * u_sub0) >> 16) + ((sr[3] * u_sub1) >> 16));
							ug[1] = (((sg[2] * u_sub0) >> 16) + ((sg[3] * u_sub1) >> 16));
							ub[1] = (((sb[2] * u_sub0) >> 16) + ((sb[3] * u_sub1) >> 16));
							/* Calculate the final sample */
							fr = (((ur[0] * v_sub0) >> 16) + ((ur[1] * v_sub1) >> 16));
							fg = (((ug[0] * v_sub0) >> 16) + ((ug[1] * v_sub1) >> 16));
							fb = (((ub[0] * v_sub0) >> 16) + ((ub[1] * v_sub1) >> 16));
							p[x] = ((fr & 0xf800) >> 1) | ((fg & 0x0f80) >> 2) | ((fb & 0x00f8) >> 3);
						}
#else
						pix = texture_ram[texture_page][(texture_y+iv) * 2048 + (texture_x+iu)];
						r = (pix & 0xf000) >> 1;
						g = (pix & 0x0f00) >> 2;
						b = (pix & 0x00f0) >> 3;
						p[x] = r | g | b;
#endif
						d[x] = iz;		/* write new zbuffer value */
					}

					z += dz;
					u += du;
					v += dv;
				}
			}
		}
	}
}


static void draw_triangle_color(VERTEX v1, VERTEX v2, VERTEX v3, UINT16 color)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	v1.z = (1.0 / v1.z) * ZBUFFER_SCALE;
	v2.z = (1.0 / v2.z) * ZBUFFER_SCALE;
	v3.z = (1.0 / v3.z) * ZBUFFER_SCALE;

	vert[0].x = v1.x;	vert[0].y = v1.y;	vert[0].p[0] = (UINT32)v1.z;
	vert[1].x = v2.x;	vert[1].y = v2.y;	vert[1].p[0] = (UINT32)v2.z;
	vert[2].x = v3.x;	vert[2].y = v3.y;	vert[2].p[0] = (UINT32)v3.z;

	scans = setup_triangle_1(&vert[0], &vert[1], &vert[2], &clip3d);

	if(scans)
	{
		INT64 dz = scans->dp[0];
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			INT64 z;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT16 *p = (UINT16*)bitmap3d->line[y];
			UINT32 *d = (UINT32*)zbuffer->line[y];

			x1 = scan->sx;
			x2 = scan->ex;
			if(x1 < clip3d.min_x)
				x1 = clip3d.min_x;
			if(x2 > clip3d.max_x)
				x2 = clip3d.max_x;

			z = scans->scanline[y - scans->sy].p[0];

			if(x1 < clip3d.max_x && x2 > clip3d.min_x) {
				for(x = x1; x <= x2; x++) {
					UINT32 iz = z >> 16;

					iz |= viewport_priority;

					if(iz > d[x]) {
						p[x] = color;
						d[x] = iz;		/* write new zbuffer value */
					}
					z += dz;
				}
			}
		}
	}
}

INLINE int is_point_inside(float x, float y, float z, PLANE cp)
{
	float s = (x * cp.x) + (y * cp.y) + (z * cp.z) + cp.d;
	if (s >= 0.0f)
		return 1;
	else
		return 0;
}

INLINE float line_plane_intersection(VERTEX *v1, VERTEX *v2, PLANE cp)
{
	float x = v1->x - v2->x;
	float y = v1->y - v2->y;
	float z = v1->z - v2->z;
	float t = ((cp.x * v1->x) + (cp.y * v1->y) + (cp.z * v1->z)) / ((cp.x * x) + (cp.y * y) + (cp.z * z));
	return t;
}

static int clip_polygon(VERTEX *v, int num_vertices, PLANE cp, VERTEX *vout)
{
	VERTEX clipv[10];
	int clip_verts = 0;
	float t;
	int i;

	int pv = num_vertices - 1;

	for (i=0; i < num_vertices; i++)
	{
		int v1_in = is_point_inside(v[i].x, v[i].y, v[i].z, cp);
		int v2_in = is_point_inside(v[pv].x, v[pv].y, v[pv].z, cp);

		if (v1_in && v2_in)			/* edge is completely inside the volume */
		{
			memcpy(&clipv[clip_verts], &v[i], sizeof(VERTEX));
			++clip_verts;
		}
		else if (!v1_in && v2_in)	/* edge is entering the volume */
		{
			/* insert vertex at intersection point */
			t = line_plane_intersection(&v[i], &v[pv], cp);
			clipv[clip_verts].x = v[i].x + ((v[pv].x - v[i].x) * t);
			clipv[clip_verts].y = v[i].y + ((v[pv].y - v[i].y) * t);
			clipv[clip_verts].z = v[i].z + ((v[pv].z - v[i].z) * t);
			clipv[clip_verts].u = (UINT32)((float)v[i].u + (((float)v[pv].u - (float)v[i].u) * t));
			clipv[clip_verts].v = (UINT32)((float)v[i].v + (((float)v[pv].v - (float)v[i].v) * t));
			++clip_verts;
		}
		else if (v1_in && !v2_in)	/* edge is leaving the volume */
		{
			/* insert vertex at intersection point */
			t = line_plane_intersection(&v[i], &v[pv], cp);
			clipv[clip_verts].x = v[i].x + ((v[pv].x - v[i].x) * t);
			clipv[clip_verts].y = v[i].y + ((v[pv].y - v[i].y) * t);
			clipv[clip_verts].z = v[i].z + ((v[pv].z - v[i].z) * t);
			clipv[clip_verts].u = (UINT32)((float)v[i].u + (((float)v[pv].u - (float)v[i].u) * t));
			clipv[clip_verts].v = (UINT32)((float)v[i].v + (((float)v[pv].v - (float)v[i].v) * t));
			++clip_verts;

			/* insert the existing vertex */
			memcpy(&clipv[clip_verts], &v[i], sizeof(VERTEX));
			++clip_verts;
		}

		pv = i;
	}
	memcpy(&vout[0], &clipv[0], sizeof(VERTEX) * clip_verts);
	return clip_verts;
}

static const int num_bits[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

static MATRIX coordinate_system;
static float viewport_focal_length = 300;
static int viewport_region_x = 0;
static int viewport_region_y = 0;
static int viewport_region_width = 496;
static int viewport_region_height = 384;

static PLANE clip_plane[5];

static void draw_model(UINT32 *model)
{
	UINT32 header[7];
	int index = 0;
	int last_polygon = 0;
	int num_vertices, num_reused_vertices;
	int i, v, vi;
	float fixed_point_fraction;
	VERTEX vertex[4];
	VERTEX prev_vertex[4];
	VERTEX clip_vert[10];

	int polynum = 0;
	MATRIX transform_matrix;
	int texture_format;
	float center_x, center_y;

	if(model3_step < 0x15) {	/* position coordinates are 17.15 fixed-point in Step 1.0 */
		fixed_point_fraction = 32768.0f;
	} else {					/* 13.19 fixed-point in other Steps */
		fixed_point_fraction = 524288.0f;
	}

	get_top_matrix(&transform_matrix);

	/* current viewport center coordinates on screen */
	center_x = (float)(viewport_region_x + (viewport_region_width / 2));
	center_y = (float)(viewport_region_y + (viewport_region_height / 2));

	while(!last_polygon)
	{
		UINT16 color;
		VECTOR3 normal;
		VECTOR3 sn;
		VECTOR p[4];

		for(i=0; i < 7; i++) {
			header[i] = model[index++];
		}

		if(header[6] == 0) {
			//printf("draw_model: header word 6 == 0\n");
			return;
		}

		if(header[1] & 0x4)
			last_polygon = 1;

		if(header[0] & 0x40)
			num_vertices = 4;
		else
			num_vertices = 3;

		num_reused_vertices = num_bits[header[0] & 0xf];

		if(header[1] & 0x40) {			/* texture coordinates are 16.0 fixed-point */
			texture_coord_shift = 16;
		} else {						/* texture coordinates are 13.3 fixed-point */
			texture_coord_shift = 19;
		}

		/* polygon normal (sign + 1.22 fixed-point) */
		normal[0] = (float)((int)header[1] >> 8) / 4194304.0f;
		normal[1] = (float)((int)header[2] >> 8) / 4194304.0f;
		normal[2] = (float)((int)header[3] >> 8) / 4194304.0f;

		vi = 0;

		/* load reused vertices */
		for(v=0; v < 4; v++)
		{
			if(header[0] & (1 << v)) {
				memcpy(&vertex[vi], &prev_vertex[v], sizeof(VERTEX));
				++vi;
			}
		}

		/* load new vertices */
		for(v=0; v < (num_vertices - num_reused_vertices); v++)
		{
			vertex[vi].x = (float)((int)model[index++]) / fixed_point_fraction;
			vertex[vi].y = (float)((int)model[index++]) / fixed_point_fraction;
			vertex[vi].z = (float)((int)model[index++]) / fixed_point_fraction;

			vertex[vi].u = (UINT16)(model[index] >> 16);
			vertex[vi].v = (UINT16)(model[index]);
			++index;
			++vi;
		}

		/* Copy current vertices as previous vertices */
		memcpy(prev_vertex, vertex, sizeof(VERTEX) * 4);

		color = (((header[4] >> 27) & 0x1f) << 10) | (((header[4] >> 19) & 0x1f) << 5) | ((header[4] >> 11) & 0x1f);

		/* transform polygon normal to view-space */
		sn[0] = (normal[0] * transform_matrix[0][0]) +
				(normal[1] * transform_matrix[1][0]) +
				(normal[2] * transform_matrix[2][0]);
		sn[1] = (normal[0] * transform_matrix[0][1]) +
				(normal[1] * transform_matrix[1][1]) +
				(normal[2] * transform_matrix[2][1]);
		sn[2] = (normal[0] * transform_matrix[0][2]) +
				(normal[1] * transform_matrix[1][2]) +
				(normal[2] * transform_matrix[2][2]);

		/* TODO: backface culling */
		/* transform vertices */
		for(i=0; i < num_vertices; i++) {
			VECTOR vect;

			vect[0] = vertex[i].x;
			vect[1] = vertex[i].y;
			vect[2] = vertex[i].z;
			vect[3] = 1.0f;

			/* transform to world-space */
			matrix_multiply_vector(transform_matrix, vect, &p[i]);

			/* apply coordinate system */
			clip_vert[i].x = p[i][0] * coordinate_system[0][1];
			clip_vert[i].y = p[i][1] * coordinate_system[1][2];
			clip_vert[i].z = p[i][2] * coordinate_system[2][0];
			clip_vert[i].u = vertex[i].u;
			clip_vert[i].v = vertex[i].v;
		}

		/* clip against view frustum */
		num_vertices = clip_polygon(clip_vert, num_vertices, clip_plane[0], clip_vert);
		num_vertices = clip_polygon(clip_vert, num_vertices, clip_plane[1], clip_vert);
		num_vertices = clip_polygon(clip_vert, num_vertices, clip_plane[2], clip_vert);
		num_vertices = clip_polygon(clip_vert, num_vertices, clip_plane[3], clip_vert);
		num_vertices = clip_polygon(clip_vert, num_vertices, clip_plane[4], clip_vert);

		/* homogeneous Z-divide, screen-space transformation */
		for(i=0; i < num_vertices; i++) {
			float ooz = 1.0f / clip_vert[i].z;
			clip_vert[i].x = ((clip_vert[i].x * ooz) * viewport_focal_length) + center_x;
			clip_vert[i].y = ((clip_vert[i].y * ooz) * viewport_focal_length) + center_y;
		}

		/* TODO: setup rest of the surface parameters */
		texture_page = (header[4] & 0x40) ? 1 : 0;
		texture_x = 32 * ((header[4] & 0x1f) << 1) | ((header[5] >> 7) & 0x1);
		texture_y = 32 * (header[5] & 0x1f);
		texture_width = 32 << ((header[3] >> 3) & 0x7);
		texture_height = 32 << (header[3] & 0x7);
		texture_width_mask = texture_width - 1;
		texture_height_mask = texture_height - 1;
		texture_format = (header[6] >> 7) & 0x7;

		/* TODO: lighting */

		if( (header[6] & 0x4000000) != 0 )		/* textured */
		{
			for (i=2; i < num_vertices; i++) {
				switch(texture_format)
				{
					case 0: draw_triangle_tex1555(clip_vert[0], clip_vert[i-1], clip_vert[i]); break;			/* ARGB1555 */
					case 1: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: 4-bit */
					case 2: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: 4-bit */
					case 3: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: 4-bit */
					case 4: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: A4L4 */
					case 5: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: A8 */
					case 6: draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color); break;		/* TODO: 4-bit */
					case 7: draw_triangle_tex4444(clip_vert[0], clip_vert[i-1], clip_vert[i]); break;			/* ARGB4444 */
				}
			}
		}
		else									/* non-textured */
		{
			for (i=2; i < num_vertices; i++) {
				draw_triangle_color(clip_vert[0], clip_vert[i-1], clip_vert[i], color);
			}
		}
		++polynum;
	};
}


/*****************************************************************************/
/* display list parser */

static UINT32 matrix_base_address;

static UINT32 *get_memory_pointer(UINT32 address)
{
	if (address & 0x800000)
	{
		if (address >= 0x840000) {
			osd_die("get_memory_pointer: invalid display list memory address %08X\n", address);
		}
		return &display_list_ram[address & 0x7fffff];
	}
	else
	{
		if (address >= 0x100000) {
			osd_die("get_memory_pointer: invalid node ram address %08X\n", address);
		}
		return &culling_ram[address];
	}
	return 0;
}

static void load_matrix(int matrix_num, MATRIX *out)
{
	MATRIX m;
	float *matrix = (float*)get_memory_pointer(matrix_base_address + (matrix_num*12));

	m[0][0] = matrix[3];	m[0][1] = matrix[6];	m[0][2] = matrix[9];	m[0][3] = 0.0f;
	m[1][0] = matrix[4];	m[1][1] = matrix[7];	m[1][2] = matrix[10];	m[1][3] = 0.0f;
	m[2][0] = matrix[5];	m[2][1] = matrix[8];	m[2][2] = matrix[11];	m[2][3] = 0.0f;
	m[3][0] = matrix[0];	m[3][1] = matrix[1];	m[3][2] = matrix[2];	m[3][3] = 1.0f;

	memcpy(out, &m, sizeof(MATRIX));
}

static void traverse_list4(UINT32 address)
{
	UINT32 *list = get_memory_pointer(address);
	UINT32 link = list[0];

	if ((link & 0xffffff) > 0x100000)		/* VROM model */
	{
		draw_model(&vrom[link & 0xffffff]);
	}
	else {		/* model in polygon ram */
		/* TODO: polygon ram actually overrides the lowest 4MB of VROM.
				 by moving the polygon ram there we could get rid of this distinction */
		draw_model(&polygon_ram[link & 0xffffff]);
	}
}

static void traverse_list(UINT32 address)
{
	UINT32 *list = get_memory_pointer(address);
	int end = 0;
	int list_ptr = 0;

	/* TODO: traverse from end to beginning may be correct rendering order */

	while(!end)
	{
		UINT32 address = list[list_ptr++];

		//if (address & 0x02000000 || address == 0) {
		if(address == 0 || (address >> 24) != 0) {
			end = 1;
		}

		address &= 0xffffff;

		if (address != 0 && address != 0x800800) {
			traverse_node(address);
		}
	};
}

static void traverse_node(UINT32 address)
{
	UINT32 *node;
	UINT32 link;
	int node_matrix;
	float x, y, z;
	MATRIX matrix;
	int offset;

	if(model3_step < 0x15) {
		offset = 2;
	} else {
		offset = 0;
	}

	node = get_memory_pointer(address);

	/* apply node transformation and translation */
	node_matrix = node[3-offset] & 0xfff;
	load_matrix(node_matrix, &matrix);

	push_matrix_stack();
	if (node_matrix != 0) {
		multiply_matrix_stack(matrix);
	}
	if (node[0] & 0x10) {
		x = *(float*)&node[4-offset];
		y = *(float*)&node[5-offset];
		z = *(float*)&node[6-offset];
		translate_matrix_stack(x, y, z);
	}

	/* handle the link */
	/* TODO: should we handle the second one first ? */
	link = node[7-offset];

	if (node[0] & 0x08) {
		/* TODO: this is a pointer list with 4 links. suspected to do level of detail (LOD) selection. */
		traverse_list4(link & 0xffffff);
	}
	else {
		if (link != 0x0fffffff && link != 0x00800800 && link != 0)
		{
			if (link != 0x01000000) {
				switch(link >> 24)
				{
					case 0x00:		/* link to another node */
						traverse_node(link & 0xffffff);
						break;

					case 0x01:
					case 0x03:		/* both of these link to models, is there any difference ? */
						if ((link & 0xffffff) > 0x100000)		/* VROM model */
						{
							draw_model(&vrom[link & 0xffffff]);
						}
						else {		/* model in polygon ram */
							/* TODO: polygon ram actually overrides the lowest 4MB of VROM.
									 by moving the polygon ram there we could get rid of this distinction */
							draw_model(&polygon_ram[link & 0xffffff]);
						}
						break;

					case 0x04:		/* list of links */
						traverse_list(link & 0xffffff);
						break;

					default:
						printf("traverse_node %08X: link 1 = %08X\n", address, link);
						break;
				}
			}
		} else {
			printf("traverse_node %08X: link 1 = %08X\n", address, link);
		}
	}

	pop_matrix_stack();

	/* handle the second link */
	link = node[8-offset];

	if (link != 0x00800800 && link != 0)
	{
		if (link != 0x01000000 && (link >> 24) == 0) {
			traverse_node(link & 0xffffff);
		}
	} else {
		printf("traverse_node %08X: link 2 = %08X\n", address, link);
	}
}

static void traverse_root_node(UINT32 address)
{
	UINT32 *node;
	UINT32 link_address, link_node;
	float viewport_left, viewport_right, viewport_top, viewport_bottom;
	float fov_x, fov_y;

	node = get_memory_pointer(address);

	link_address = node[1];
	if (link_address == 0) {
		return;
	}

	/* traverse to the link node before drawing this viewport */
	/* check this is correct as this affects the rendering order */
	if ((link_address >> 24) == 0) {
		traverse_root_node(link_address & 0xffffff);
	}
	else {
		if ((link_address >> 24) != 0x01) {
			printf("traverse_root_node: link address = %08X\n", link_address);
		}
	}

	/* draw this viewport */

	/* set viewport parameters */
	viewport_region_x		= ((node[26] & 0xffff) >> 4);			/* 12.4 fixed point */
	viewport_region_y		= (((node[26] >> 16) & 0xffff) >> 4);
	viewport_region_width	= ((node[20] & 0xffff) >> 2);			/* 14.2 fixed point */
	viewport_region_height	= (((node[20] >> 16) & 0xffff) >> 2);

	viewport_priority = (((node[0] >> 3) & 0x3)) << 29;			/* we use priority as 2 top bits of depth buffer value */

	clip3d.min_x = max(viewport_region_x, screen_clip->min_x);
	clip3d.max_x = min(viewport_region_x+viewport_region_width, screen_clip->max_x);
	clip3d.min_y = max(viewport_region_y, screen_clip->min_y);
	clip3d.max_y = min(viewport_region_y+viewport_region_height, screen_clip->max_y);

	/* frustum plane angles */
	viewport_left		= RADIAN_TO_DEGREE(asin(*(float*)&node[12]));
	viewport_right		= RADIAN_TO_DEGREE(asin(*(float*)&node[16]));
	viewport_top		= RADIAN_TO_DEGREE(asin(*(float*)&node[14]));
	viewport_bottom		= RADIAN_TO_DEGREE(asin(*(float*)&node[18]));

	clip_plane[0].x = *(float*)&node[13];	clip_plane[0].y = 0.0f;		clip_plane[0].z = *(float*)&node[12];	clip_plane[0].d = 0.0f;
	clip_plane[1].x = *(float*)&node[17];	clip_plane[1].y = 0.0f;		clip_plane[1].z = *(float*)&node[16];	clip_plane[1].d = 0.0f;
	clip_plane[2].x = 0.0f;		clip_plane[2].y = *(float*)&node[15];	clip_plane[2].z = *(float*)&node[14];	clip_plane[2].d = 0.0f;
	clip_plane[3].x = 0.0f;		clip_plane[3].y = *(float*)&node[19];	clip_plane[3].z = *(float*)&node[18];	clip_plane[3].d = 0.0f;
	clip_plane[4].x = 0.0f;		clip_plane[4].y = 0.0f;		clip_plane[4].z = 1.0f;	clip_plane[4].d = 1.0f;

	fov_x = viewport_left + viewport_right;
	fov_y = viewport_top + viewport_bottom;
	viewport_focal_length = (viewport_region_height / 2) / tan( (fov_y * 3.1415926535 / 180.0f) / 2.0f );

	matrix_base_address = node[22];
	/* TODO: where does node[23] point to ? LOD table ? */

	/* TODO: set lighting parameters */

	/* set coordinate system matrix */
	load_matrix(0, &coordinate_system);

	link_node = node[2];
	switch(link_node >> 24)
	{
		case 0x00:		/* basic node */
			traverse_node(link_node);
			break;

		default:
			printf("traverse_root_node: node link = %08X\n", link_node);
			break;
	}
}

static void real3d_traverse_display_list(void)
{
	init_matrix_stack();
	traverse_root_node(0x800000);
}

