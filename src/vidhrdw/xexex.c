#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static data8_t xexexbg_regs[8];
static unsigned char *xexexbg_base;
static data16_t *xexexbg_ram, *xexexbg_rammax;
static UINT32 xexexbg_rommask;

int xexexbg_vh_start(int region)
{
	xexexbg_base = memory_region(region);
	xexexbg_ram = malloc(0x1000);
	xexexbg_rammax = xexexbg_ram + 0x800;
	xexexbg_rommask = memory_region_length(region) - 1;

	state_save_register_UINT16("xexexbg", 0, "memory",    xexexbg_ram,  0x800);
	state_save_register_UINT8 ("xexexbg", 0, "registers", xexexbg_regs, 8);
	return 0;
}

void xexexbg_vh_stop(void)
{
	free(xexexbg_ram);
}

WRITE16_HANDLER( xexexbg_w )
{
	if(ACCESSING_LSB)
		xexexbg_regs[offset] = data;
}

READ16_HANDLER( xexexbg_r )
{
	return xexexbg_regs[offset];
}

WRITE16_HANDLER( xexexbg_ram_w )
{
	int off1;
	COMBINE_DATA( xexexbg_ram + offset);
	off1 = offset & ~3;
}

READ16_HANDLER( xexexbg_ram_r )
{
	return xexexbg_ram[offset];
}

READ16_HANDLER( xexexbg_rom_r )
{
	if (!(xexexbg_regs[5] & 1))
		logerror("Back: Reading rom memory with enable=0\n");
	return *(xexexbg_base + 2048*xexexbg_regs[7] + (offset>>1));
}

void xexexbg_draw(struct mame_bitmap *bitmap, int colorbase, int pri)
{
	const struct rectangle area = Machine->visible_area;
	data16_t *line;
	int delta, dim1, dim1_max, dim2_max;
	UINT32 mask1, mask2;
	int sp;

	int orientation = (xexexbg_regs[4] & 8 ? ORIENTATION_FLIP_X : 0)\
		| (xexexbg_regs[4] & 16 ? ORIENTATION_FLIP_Y : 0)
		| (xexexbg_regs[4] & 1 ? 0 : ORIENTATION_SWAP_XY);

	INT16 cur_x = (xexexbg_regs[0] << 8) | xexexbg_regs[1];
	INT16 cur_y = (xexexbg_regs[2] << 8) | xexexbg_regs[3];

	colorbase <<= 4;

	if(orientation & ORIENTATION_SWAP_XY) {
		dim1_max = area.max_x - area.min_x + 1;
		dim2_max = area.max_y - area.min_y + 1;
		// -358 for level 1 boss, huh?
		delta = cur_y - 495;
		line = xexexbg_ram + (((area.min_x + cur_x - 19) & 0x1ff) << 2);
	} else {
		dim1_max = area.max_y - area.min_y + 1;
		dim2_max = area.max_x - area.min_x + 1;
		delta = cur_x + 49;
		line = xexexbg_ram + (((area.min_y + cur_y + 16) & 0x1ff) << 2);
	}

	switch(xexexbg_regs[4] & 0xe0) {
	case 0x00: // Not sure.  Warp level
		mask1 = 0xffff0000;
		mask2 = 0x0000ffff;
		sp = 0;
		break;
	case 0x20:
		mask1 = 0xffff8000;
		mask2 = 0x00007fff;
		sp = 0;
		break;
	case 0x40:
		mask1 = 0xffff0000;
		mask2 = 0x0000ffff;
		sp = 0;
		break;
	case 0x80:
		mask1 = 0xffffc000;
		mask2 = 0x00003fff;
		sp = 0;
		break;
	case 0xe0:
		mask1 = 0xffff0000;
		mask2 = 0x0000ffff;
		sp = 1;
		break;
	default:
		logerror("Unknown mode %02x\n", xexexbg_regs[4] & 0xe0);
		mask1 = 0xffff0000;
		mask2 = 0x0000ffff;
		sp = 0;
		break;
	}

	if(xexexbg_regs[4] & 4)
		mask1 = 0;

	for(dim1 = 0; dim1 < dim1_max; dim1++) {
		data16_t color  = *line++;
		UINT32   start  = *line++;
		data16_t inc    = *line++;
		INT16    offset = *line++;
		int dim2;
		unsigned char *pixel;
		UINT32 cpos;
		unsigned char scanline[512];

		if(line == xexexbg_rammax)
			line = xexexbg_ram;

		if(!color && !start)
			continue;

		pixel = scanline;
		start <<= 7;
		cpos = (offset + delta)*inc;

		for(dim2 = 0; dim2 < dim2_max; dim2++) {
			int romp;
			UINT32 rcpos = cpos;

			if(sp && (rcpos & mask1))
				rcpos += inc << 9;

			if(rcpos & mask1) {
				*pixel++ = 0;
				cpos += inc;
				continue;
			}

			romp = xexexbg_base[(((rcpos & mask2)>>7) + start) & xexexbg_rommask];

			if(rcpos & 0x40)
				romp &= 0xf;
			else
				romp >>= 4;
			*pixel++ = romp;
			cpos += inc;
		}
		if(orientation & ORIENTATION_SWAP_XY)
			pdraw_scanline8(bitmap, area.min_y, area.min_x+dim1, dim2_max, scanline,
							Machine->pens + (colorbase | ((color & 0x0f) << 4)),
							0, orientation, pri);
		else
			pdraw_scanline8(bitmap, area.min_x, area.min_y+dim1, dim2_max, scanline,
							Machine->pens + (colorbase | ((color & 0x0f) << 4)),
							0, orientation, pri);
	}
}


static int sprite_colorbase;
static int layer_colorbase[4], bg_colorbase, layerpri[4];
static int cur_alpha, cur_alpha_level;

void xexex_set_alpha(int on)
{
	cur_alpha = on;
}

WRITE16_HANDLER(xexex_alpha_level_w)
{
	if(ACCESSING_LSB)
		cur_alpha_level = ((data & 0x1f) << 3) | ((data & 0x1f) >> 2);
}

static void xexex_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[3])								*priority_mask = 0;
	else if (pri > layerpri[3] && pri <= layerpri[2])	*priority_mask = 0xff00;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xff00|0xf0f0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xff00|0xf0f0|0xcccc;
	else 												*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static void xexex_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

static int scrolld[2][4][2] = {
 	{{ 42-64, 16 }, {42-64, 16}, {42-64-2, 16}, {42-64-4, 16}},
 	{{ 53-64, 16 }, {53-64, 16}, {53-64-2, 16}, {53-64-4, 16}}
};

int xexex_vh_start(void)
{
	cur_alpha = 0;
	cur_alpha_level = 0x1f;

	K053251_vh_start();

	xexexbg_vh_start(REGION_GFX3);
	if (K054157_vh_start(REGION_GFX1, 1, scrolld, NORMAL_PLANE_ORDER, xexex_tile_callback))
	{
		xexexbg_vh_stop();
		return 1;
	}
	if (K053247_vh_start(REGION_GFX2, -28, 32, NORMAL_PLANE_ORDER, xexex_sprite_callback))
	{
		K054157_vh_stop();
		xexexbg_vh_stop();
		return 1;
	}

	// cur_alpha is saved as part of "control2" in the main driver
	state_save_register_int ("video", 0, "alpha", &cur_alpha_level);
	return 0;
}

void xexex_vh_stop(void)
{
	xexexbg_vh_stop();
	K054157_vh_stop();
	K053247_vh_stop();
}

/* useful function to sort the four tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayers(int *layer, int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(2, 3)
}
int xdump = 0;
void xexex_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int layer[4];
	int plane;

	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);
	bg_colorbase       = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = 0x70;
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[3] = K053251_get_palette_index(K053251_CI4);

	K054157_tilemap_update();

	layer[0] = 1;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 2;
	layerpri[1] = K053251_get_priority(K053251_CI3);
	layer[2] = 3;
	layerpri[2] = K053251_get_priority(K053251_CI4);
	layer[3] = -1;
	layerpri[3] = K053251_get_priority(K053251_CI1);

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
	for(plane=0; plane<4; plane++)
		if(layer[plane] < 0)
			xexexbg_draw(bitmap, bg_colorbase, 1<<plane);
		else if(!cur_alpha || (layer[plane] != 1))
			K054157_tilemap_draw(bitmap, layer[plane], 0, 1<<plane);

	K053247_sprites_draw(bitmap);

	if(cur_alpha) {
		alpha_set_level(cur_alpha_level);
		K054157_tilemap_draw(bitmap, 1, TILEMAP_ALPHA, 0);
	}

	K054157_tilemap_draw(bitmap, 0, 0, 0);
}
