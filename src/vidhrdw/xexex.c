#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int xexexbg_ctrla, xexexbg_ctrlb, xexexbg_select, xexexbg_x, xexexbg_y;
static unsigned char *xexexbg_base;
static data16_t *xexexbg_ram, *xexexbg_rammax;

int xexexbg_vh_start(int region)
{
	xexexbg_base = memory_region(region);
	xexexbg_ram = malloc(0x1000);
	xexexbg_rammax = xexexbg_ram + 0x800;
	return 0;
}

void xexexbg_vh_stop(void)
{
	free(xexexbg_ram);
}

WRITE16_HANDLER( xexexbg_w )
{
	if(ACCESSING_LSB) {
		int old;
		data &= 0xff;
		switch(offset) {
		case 0x0:
			old = xexexbg_x;
			xexexbg_x = (xexexbg_x & 0xff) | (data << 8);
			break;
		case 0x1:
			old = xexexbg_x;
			xexexbg_x = (xexexbg_x & 0xff00) | data;
			break;
		case 0x2:
			old = xexexbg_y;
			xexexbg_y = (xexexbg_y & 0xff) | (data << 8);
			break;
		case 0x3:
			old = xexexbg_y;
			xexexbg_y = (xexexbg_y & 0xff00) | data;
			break;
		case 0x4:
			if((xexexbg_ctrlb & 0xe5) != (data & 0xe5))
				logerror("Back.b %02x\n", data & 0xe5);
			xexexbg_ctrlb = data;
			break;
		case 0x5:
			if(xexexbg_ctrla != data)
				logerror("Back.a %02x\n", data);
			xexexbg_ctrla = data;
			break;
		case 0x7:
			xexexbg_select = data;
			break;
		}
	}
}

READ16_HANDLER( xexexbg_r )
{
	switch(offset) {
	case 0x5:
		return xexexbg_ctrla;
	case 0x7:
		return xexexbg_select;
	default:
		logerror("xexexbg: Reading %x at %x\n", offset*2, cpu_get_pc());
		return 0;
	}
}

WRITE16_HANDLER( xexexbg_ram_w )
{
	COMBINE_DATA( xexexbg_ram + offset);
}

READ16_HANDLER( xexexbg_ram_r )
{
	return xexexbg_ram[offset];
}

READ16_HANDLER( xexexbg_rom_r )
{
	if (!(xexexbg_ctrla & 1))
		logerror("Back: Reading rom memory with enable=0\n");
	return *(xexexbg_base + 2048*xexexbg_select + (offset>>2));
}

#if 1
int ddy = -358;
int ddx = -19;
#else
int ddy = -358 + 299 - (-201);
int ddx = -19 - 201 - 299;
#endif

void xexexbg_mark_colors(int colorbase)
{
	int cmap = 0;
	int i, j;
	for(i=0; i<512*8; i+=8)
		if(xexexbg_ram[i] || xexexbg_ram[i+1])
			cmap |= 1 << (xexexbg_ram[i] & 0xf);
	for(i=0; i<16; i++)
		if(cmap & (1<<i))
			for(j=0; j<16; j++)
				palette_used_colors[colorbase*16 + i * 16 + j] = PALETTE_COLOR_VISIBLE;
}

void xexexbg_draw(struct osd_bitmap *bitmap, int colorbase, int pri)
{
	const struct rectangle area = Machine->visible_area;
	data16_t *line;
	int delta, dim1, dim1_max, dim2_max;
	UINT32 mask1, mask2;

	int orientation = (xexexbg_ctrlb & 8 ? ORIENTATION_FLIP_X : 0)\
		| (xexexbg_ctrlb & 16 ? ORIENTATION_FLIP_Y : 0)
		| (xexexbg_ctrlb & 1 ? 0 : ORIENTATION_SWAP_XY);

	colorbase <<= 4;

	if(orientation & ORIENTATION_SWAP_XY) {
		dim1_max = area.max_x - area.min_x + 1;
		dim2_max = area.max_y - area.min_y + 1;
		delta = (INT16)xexexbg_y + ddy;
		line = xexexbg_ram + (((area.min_x + (INT16)xexexbg_x + ddx) & 0x1ff) << 2);
	} else {
		dim1_max = area.max_y - area.min_y + 1;
		dim2_max = area.max_x - area.min_x + 1;
		delta = (INT16)xexexbg_x + 49;
		line = xexexbg_ram + (((area.min_y + (INT16)xexexbg_y + 16) & 0x1ff) << 2);
	}

	if(xexexbg_ctrlb & 0x80) {
		mask1 = 0xffffc000;
		mask2 = 0x00003fff;
	} else {
		mask1 = 0xffff0000;
		mask2 = 0x0000ffff;
	}

	if(xexexbg_ctrlb & 4)
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
			if(cpos & mask1) {
				*pixel++ = 0;
				cpos += inc;
				continue;
			}

			romp = xexexbg_base[((cpos & mask2)>>7) + start];

			if(cpos & 0x40)
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
		cur_alpha_level = data & 0x1f;
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

static int xexex_scrolld[2][4][2] = {
 	{{ 42-64, 16 }, {42-64-4, 16}, {42-64-2, 16}, {42-64, 16}},
 	{{ 53-64, 16 }, {53-64-4, 16}, {53-64-2, 16}, {53-64, 16}}
};

int xexex_vh_start(void)
{
	cur_alpha = 0;
	cur_alpha_level = 0x1f;

	xexexbg_vh_start(REGION_GFX3);
	if (K054157_vh_start(REGION_GFX1, 1, xexex_scrolld, NORMAL_PLANE_ORDER, xexex_tile_callback))
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

void xexex_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int layer[4];
	int plane;

#if 0
if (keyboard_pressed(KEYCODE_D))
{
#if 0
	FILE *fp;
	fp=fopen("bg.dmp", "w+b");
	if (fp)
	{
		fwrite(cpu_bankbase[4], 0x8000, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
#endif
	int i, j;
	for(i=0; i<16*64; i+=16) {
		fprintf(stderr, "%03x:", i*2);
		for(j=0; j<16; j++)
			fprintf(stderr, " %04x", xexexbg_ram[i+j]);
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}
#endif

	bg_colorbase       = K053251_get_palette_index(K053251_CI1);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[3] = 0x70;

	K054157_tilemap_update();

	palette_init_used_colors();
	K053247_mark_sprites_colors();
	xexexbg_mark_colors(bg_colorbase);

	palette_recalc();

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);
	layer[3] = -1;
	layerpri[3] = K053251_get_priority(K053251_CI1);

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
	for(plane=0; plane<4; plane++)
		if(layer[plane] < 0)
			xexexbg_draw(bitmap, bg_colorbase, 1<<plane);
		else if(1 || !cur_alpha || !layer[plane])
			K054157_tilemap_draw(bitmap, layer[plane], 0, 1<<plane);

	K053247_sprites_draw(bitmap);
#if 0
	if(cur_alpha)
		K054157_tilemap_draw(bitmap, 0, TILEMAP_ALPHA, cur_alpha_level);
#endif
	K054157_tilemap_draw(bitmap, 3, 0, 0);
}
