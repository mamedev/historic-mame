#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static int xexexbg_ctrla, xexexbg_ctrlb, xexexbg_select, xexexbg_x, xexexbg_y;
static unsigned char *xexexbg_base;

void xexexbg_vh_start(int region)
{
	xexexbg_base = memory_region(region);
}

void xexexbg_vh_stop(void)
{
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
			if(old != xexexbg_x)
				logerror("Back.x %04x\n", xexexbg_x);
			break;
		case 0x1:
			old = xexexbg_x;
			xexexbg_x = (xexexbg_x & 0xff00) | data;
			if(old != xexexbg_x)
				logerror("Back.x %04x\n", xexexbg_x);
			break;
		case 0x2:
			old = xexexbg_y;
			xexexbg_y = (xexexbg_y & 0xff) | (data << 8);
			if(old != xexexbg_y)
				logerror("Back.y %04x\n", xexexbg_y);
			break;
		case 0x3:
			old = xexexbg_y;
			xexexbg_y = (xexexbg_y & 0xff00) | data;
			if(old != xexexbg_y)
				logerror("Back.y %04x\n", xexexbg_y);
			break;
		case 0x4:
			if((xexexbg_ctrlb & 0xfd) != (data & 0xfd))
				logerror("Back.b %02x\n", data);
			xexexbg_ctrlb = data;
			break;
		case 0x5:
			if(xexexbg_ctrla != data)
				logerror("Back.a %02x\n", data);
			xexexbg_ctrla = data;
			break;
		case 0x7:
			if(xexexbg_select != data)
				logerror("Back.s %02x\n", data);
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

READ16_HANDLER( xexexbg_rom_r )
{
	if (!(xexexbg_ctrla & 1))
		logerror("Back: Reading rom memory with enable=0\n");
	return *(xexexbg_base + 2048*xexexbg_select + (offset>>2));
}




static int sprite_colorbase;
static int layer_colorbase[4], bg_colorbase, layerpri[4];


static void xexex_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

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
	xexexbg_vh_start(REGION_GFX3);
	if (K054157_vh_start(REGION_GFX1, 1, xexex_scrolld, NORMAL_PLANE_ORDER, xexex_tile_callback))
	{
		return 1;
	}
	if (K053247_vh_start(REGION_GFX2, -28, 32, NORMAL_PLANE_ORDER, xexex_sprite_callback))
	{
		K054157_vh_stop();
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

#if 1
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;
	fp=fopen("bg.dmp", "w+b");
	if (fp)
	{
		fwrite(cpu_bankbase[4], 0x8000, 1, fp);
		usrintf_showmessage("saved");
		fclose(fp);
	}
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

	palette_recalc();

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);
	layer[3] = -1;
	layerpri[3] = -1 /*K053251_get_priority(K053251_CI1)*/;

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
	K054157_tilemap_draw(bitmap,layer[0], 0, 1);
	K054157_tilemap_draw(bitmap,layer[1], 0, 2);
	K054157_tilemap_draw(bitmap,layer[2], 0, 4);

	K053247_sprites_draw(bitmap);

	K054157_tilemap_draw(bitmap, 3, 0,0);
}
