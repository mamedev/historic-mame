/*
 * vidhrdw/mystwarr.c - Konami "Pre-GX" video hardware (here there be dragons)
 *
 */

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

//static int scrolld[2][4][2] = {
// 	{{ 42-64, 16 }, {42-64, 16}, {42-64-2, 16}, {42-64-4, 16}},
// 	{{ 53-64, 16 }, {53-64, 16}, {53-64-2, 16}, {53-64-4, 16}}
//};

static int scrolld[2][4][2] = {
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};

static int layer_colorbase[5], layers[5], layerpri[5];
static int sprite_colorbase, sprite_subcolorbase, psac_colorbase, last_psac_colorbase, gametype;

static struct tilemap *ult_936_tilemap;

static int roz_enable,roz_clip_enable,roz_rombank;
static int roz_clip_size_x,roz_clip_size_y,roz_clip_x,roz_clip_y;



// for games with 5 bpp tile data
static void mystwarr_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf8) >> 3);
}

// for games with 4 bpp tile data
static void gaiapols_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

static void viostorm_sprite_callback(int *code, int *color, int *priority_mask) //*
{
	int pri = (*color & 0x00f0) >> 2;

	if (pri <  layerpri[3])                       *priority_mask = 0; else // frontmost
	if (pri >= layerpri[3] && pri <  layerpri[2]) *priority_mask = 0xff00; else
	if (pri >= layerpri[2] && pri <  layerpri[1]) *priority_mask = 0xff00|0xf0f0; else
	if (pri >= layerpri[1] && pri <  layerpri[0]) *priority_mask = 0xff00|0xf0f0|0xcccc; else
	*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa; // backmost
	*color = sprite_colorbase | (*color & 0x001f);
}

static void metamrph_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00f0) >> 2;

	if (pri <  layerpri[3])                       *priority_mask = 0; else // frontmost
	if (pri >= layerpri[3] && pri <  layerpri[2]) *priority_mask = 0xff00; else
	if (pri >= layerpri[2] && pri <  layerpri[1]) *priority_mask = 0xff00|0xf0f0; else
	if (pri >= layerpri[1] && pri <  layerpri[0]) *priority_mask = 0xff00|0xf0f0|0xcccc; else
	*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa; // backmost

	// Bit8 & 9 are effect attributes. It is not known whether the effects are handled by external logic.
	if ((*color & 0x300) == 0x300)
		*color = sprite_subcolorbase;
	else
		*color = sprite_colorbase | (*color & 0x001f);
}

static void gaiapols_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = *color;

	if (pri & 0xffff0000) pri >>= 16;
	pri = (pri & 0x00f0) + *priority_mask;

	if (pri <  layerpri[3])                       *priority_mask = 0; else // frontmost
	if (pri >= layerpri[3] && pri <  layerpri[2]) *priority_mask = 0xff00; else
	if (pri >= layerpri[2] && pri <  layerpri[1]) *priority_mask = 0xff00|0xf0f0; else
	if (pri >= layerpri[1] && pri <  layerpri[0]) *priority_mask = 0xff00|0xf0f0|0xcccc; else
	*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa; // backmost
	*color = sprite_colorbase | (*color & 0x001f);
}

static void martchmp_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri;

	if (K055555_read_register(K55_OINPRI_ON) & 0xf0)
		pri = K055555_read_register(K55_PRIINP_8); // use PCU2 internal priority
	else
		pri = *color & 0xf0; // use external color implied priority

	if (pri <  layerpri[3])                       *priority_mask = 0; else // frontmost
	if (pri >= layerpri[3] && pri <  layerpri[2]) *priority_mask = 0xff00; else
	if (pri >= layerpri[2] && pri <  layerpri[1]) *priority_mask = 0xff00|0xf0f0; else
	if (pri >= layerpri[1] && pri <  layerpri[0]) *priority_mask = 0xff00|0xf0f0|0xcccc; else
	*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa; // backmost

	// Bit8 & 9 are effect attributes. It is not known whether the effects are handled by external logic.
	if ((*color & 0x3ff) == 0x11f)
		*color = -1; // draw full shadow
	else
		*color = sprite_colorbase | (*color & 0x001f);
}

static void get_gai_936_tile_info(int tile_index)
{
	int tileno, colour;
	unsigned char *ROM = memory_region(REGION_GFX4);
	unsigned char *dat1 = ROM, *dat2 = ROM + 0x20000, *dat3 = ROM + 0x60000;

	tileno = dat3[tile_index] | ((dat2[tile_index]&0x3f)<<8);

	if (tile_index & 1)
	{
		colour = (dat1[tile_index>>1]&0xf);
	}
	else
	{
		colour = ((dat1[tile_index>>1]>>4)&0xf);
	}

	if (dat2[tile_index] & 0x80) colour |= 0x10;

	colour |= psac_colorbase << 4;

	if (roz_clip_enable)
	{
		int x = (tile_index & 0x001ff) >> (0+3);
		int y = (tile_index & 0x3fe00) >> (9+3);
		int sizex,sizey;
		switch (roz_clip_size_x)
		{
			case 0x3:	sizex = 1; break;
			case 0x2:	sizex = 2; break;
			default:	sizex = 4; break;
		}
		switch (roz_clip_size_y)
		{
			case 0x3:	sizey = 1; break;
			case 0x2:	sizey = 2; break;
			default:	sizey = 4; break;
		}

		if (x < roz_clip_x || y < roz_clip_y || x >= roz_clip_x + sizex || y >= roz_clip_y + sizey)
			tileno = 0;	// replace with empty tile
	}

	SET_TILE_INFO(0, tileno, colour, 0)
}

static void get_ult_936_tile_info(int tile_index)
{
	int tileno, colour;
	unsigned char *ROM = memory_region(REGION_GFX4);
	unsigned char *dat1 = ROM, *dat2 = ROM + 0x40000;

	tileno = dat2[tile_index] | ((dat1[tile_index]&0x1f)<<8);

	colour = psac_colorbase;

	if (roz_clip_enable)
	{
		int x = (tile_index & 0x001ff) >> (0+3);
		int y = (tile_index & 0x3fe00) >> (9+3);
		int sizex,sizey;
		switch (roz_clip_size_x)
		{
			case 0x3:	sizex = 1; break;
			case 0x2:	sizex = 2; break;
			default:	sizex = 4; break;
		}
		switch (roz_clip_size_y)
		{
			case 0x3:	sizey = 1; break;
			case 0x2:	sizey = 2; break;
			default:	sizey = 4; break;
		}

		if (x < roz_clip_x || y < roz_clip_y || x >= roz_clip_x + sizex || y >= roz_clip_y + sizey)
			tileno = 0;	// replace with empty tile
	}

	SET_TILE_INFO(0, tileno, colour, (dat1[tile_index]&0x40) ? TILE_FLIPX : 0)
}

VIDEO_START(gaiapols)
{
	K055555_vh_start();

	gametype = 0;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, gaiapols_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -61, 21, gaiapols_sprite_callback))
	{
		return 1;
	}

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, 0, 0);

	ult_936_tilemap = tilemap_create(get_gai_936_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 512, 512);
	tilemap_set_transparent_pen(ult_936_tilemap, 0);

	return 0;
}

VIDEO_START(dadandarn)
{
	K055555_vh_start();

	gametype = 1;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, mystwarr_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -42, 22, gaiapols_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerAssociation(0); // could probably be mapped to one of the unknown K056832 registers
	K056832_set_LayerOffset(0, -2, 0);
	K056832_set_LayerOffset(1, -4, 0);
	K056832_set_LayerOffset(2, -6, 0);
	K056832_set_LayerOffset(3, -7, 0);

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -8, 0);

	ult_936_tilemap = tilemap_create(get_ult_936_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 512, 512);
	tilemap_set_transparent_pen(ult_936_tilemap, 0);

	return 0;
}

VIDEO_START(mystwarr)
{
	K055555_vh_start();

	gametype = 0;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, mystwarr_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -48, 24, gaiapols_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0,  4, 0);
	K056832_set_LayerOffset(1,  2, 0);
	K056832_set_LayerOffset(2,  0, 0);
	K056832_set_LayerOffset(3, -1, 0);

	return 0;
}

VIDEO_START(metamrph)
{
	int rgn_250 = REGION_GFX3;

	gametype = 0;

	K055555_vh_start();

	K053250_vh_start(1, &rgn_250);

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, gaiapols_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -53, 21, metamrph_sprite_callback))
	{
		return 1;
	}

	// other reference, floor at first boss
	K056832_set_LayerOffset(0,  0, 1); // text
	K056832_set_LayerOffset(1, -2, 1); // attract sea
	K056832_set_LayerOffset(2, -4, 1); // attract red monster in background of sea
	K056832_set_LayerOffset(3, -5, 1); // attract sky background to sea

	return 0;
}

VIDEO_START(viostorm)
{
	gametype = 0;

	K055555_vh_start();

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, gaiapols_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -63, 23, viostorm_sprite_callback))
	{
		return 1;
	}

	return 0;
}

VIDEO_START(martchmp) //*
{
	gametype = 0;
	K055555_vh_start();

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, mystwarr_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -56, 24, martchmp_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0,  4, 0);
	K056832_set_LayerOffset(1,  2, 0);
	K056832_set_LayerOffset(2,  0, 0);
	K056832_set_LayerOffset(3, -1, 0);
	palette_set_shadow_dRGB32(3,-64,-64,-64, 0);

	return 0;
}

#ifdef MAME_DEBUG
void debug_keys(void)
{
	if(keyboard_pressed(KEYCODE_L))
	{
		int i;
		for( i=0; i<4; i++ )
		{
			K056832_mark_plane_dirty(i);
		}
		logerror("DEBUG DUMP\n");
		logerror("Mixer pris A0 %d A1 %d AC %d B0 %d B1 %d BC %d C %d D %d O %d S1 %d S2 %d S3 %d\n",
			K055555_read_register(K55_PRIINP_0),
			K055555_read_register(K55_PRIINP_1),
			K055555_read_register(K55_PRIINP_2),
			K055555_read_register(K55_PRIINP_3),
			K055555_read_register(K55_PRIINP_4),
			K055555_read_register(K55_PRIINP_5),
			K055555_read_register(K55_PRIINP_6),
			K055555_read_register(K55_PRIINP_7),
			K055555_read_register(K55_PRIINP_8),
			K055555_read_register(K55_PRIINP_9),
			K055555_read_register(K55_PRIINP_10),
			K055555_read_register(K55_PRIINP_11));
		logerror("Plane enables = %x\n", K055555_read_register(K55_INPUT_ENABLES));
		logerror("Blend enables = %x\n", K055555_read_register(K55_BLEND_ENABLES));
		logerror("blend factor = %x\n", K054338_read_register(K338_REG_PBLEND)&0x1f);
		logerror("priorities: %d %d %d %d %d\n", layerpri[0], layerpri[1], layerpri[2], layerpri[3], layerpri[4]);
		logerror("palettes: %x %x %x %x %x %x %x %x\n",
			K055555_get_palette_index(0),
			K055555_get_palette_index(1),
			K055555_get_palette_index(2),
			K055555_get_palette_index(3),
			K055555_get_palette_index(4),
			K055555_get_palette_index(5),
			K055555_get_palette_index(6),
			K055555_get_palette_index(7));
	}
}
#endif

/* useful function to sort the four tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayers(int *layer, int *pri)
{

#define SWAP(a,b) \
	if (pri[a] <= pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	#ifdef MAME_DEBUG
	debug_keys();
	#endif

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(2, 3)
}

static void sortlayers5(int *layer, int *pri)
{

#define SWAP(a,b) \
	if (pri[a] <= pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	#ifdef MAME_DEBUG
	debug_keys();
	#endif

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(0, 4)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(1, 4)
	SWAP(2, 3)
	SWAP(2, 4)
	SWAP(3, 4)
}

VIDEO_UPDATE(mystwarr)
{
	int i, old;
	static UINT8 enablemasks[4] = { K55_INP_VRAM_A, K55_INP_VRAM_B, K55_INP_VRAM_C, K55_INP_VRAM_D };
	static UINT8 blendmasks[4] = { 3, 0xc, 0x30, 0xc0 };
	K054338_update_all_shadows(); //*

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if( old != layer_colorbase[i] )
		{
			K056832_mark_plane_dirty(i);
		}
	}

	sprite_colorbase = K055555_get_palette_index(4)<<5;
	sprite_subcolorbase = K055555_get_palette_index(5)<<5;

	K056832_tilemap_update();

	fillbitmap(priority_bitmap, 0, NULL);
	K054338_fill_backcolor(bitmap, 0); //*

	layers[0] = 0;
	layers[1] = 1;
	layers[2] = 2;
	layers[3] = 3;
	layerpri[0] = K055555_read_register(K55_PRIINP_0);
	layerpri[1] = K055555_read_register(K55_PRIINP_3);
	layerpri[2] = K055555_read_register(K55_PRIINP_6);
	layerpri[3] = K055555_read_register(K55_PRIINP_7);

	sortlayers(layers, layerpri);

	for(i=0; i<4; i++)
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[i]])
		{
			if (K055555_read_register(K55_BLEND_ENABLES) & blendmasks[layers[i]])
			{
			    	if (K054338_set_alpha_level((K055555_read_register(K55_BLEND_ENABLES)>>(2*layers[i]))&0x3))
					K056832_tilemap_draw(bitmap, cliprect, layers[i], TILEMAP_ALPHA, 1<<i);
			}
			else
			{
				K056832_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
			}
		}
	}

	K053247GP_sprites_draw(bitmap, cliprect);
}

VIDEO_UPDATE(metamrph)
{
	int i, old;
	static UINT8 enablemasks[4] = { K55_INP_VRAM_A, K55_INP_VRAM_B, K55_INP_VRAM_C, K55_INP_VRAM_D };
	static UINT8 blendmasks[4] = { 3, 0xc, 0x30, 0xc0 };
	K054338_update_all_shadows();

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if (old != layer_colorbase[i]) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<4;
	sprite_subcolorbase = K055555_get_palette_index(5)<<4;

	K056832_tilemap_update();

	fillbitmap(priority_bitmap, 0, NULL);
	K054338_fill_backcolor(bitmap, 0); //*

	layers[0] = 0; layerpri[0] = K055555_read_register(K55_PRIINP_0);
	layers[1] = 1; layerpri[1] = K055555_read_register(K55_PRIINP_3);
	layers[2] = 2; layerpri[2] = K055555_read_register(K55_PRIINP_6);
	layers[3] = 3; layerpri[3] = K055555_read_register(K55_PRIINP_7);

	sortlayers(layers, layerpri);

#if 0
	printf("%x %x %x (%x %x)\n",
		K054338_read_register(K338_REG_PBLEND),
		K054338_read_register(K338_REG_PBLEND+1),
		K054338_read_register(K338_REG_PBLEND+2),
		K055555_read_register(K55_BLEND_ENABLES),
		K055555_read_register(K55_VINMIX_ON));
#endif

	for(i=0; i<4; i++)
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[i]])
		{
			if (K055555_read_register(K55_BLEND_ENABLES) & blendmasks[layers[i]])
			{
			    	if (K054338_set_alpha_level((K055555_read_register(K55_BLEND_ENABLES)>>(2*layers[i]))&0x3))
					K056832_tilemap_draw(bitmap, cliprect, layers[i], TILEMAP_ALPHA, 1<<i);
			}
			else
			{
				K056832_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
			}
		}
	}

	//K053250_draw(bitmap, cliprect, 0, 0, 0);

	K053247GP_sprites_draw(bitmap, cliprect);
}



WRITE16_HANDLER(ddd_053936_enable_w)
{
	if (ACCESSING_MSB)
	{
		roz_enable = data & 0x0100;
		roz_rombank = (data & 0xc000)>>14;
	}
}


static void mark_roz_clip_dirty(void)
{
	int x,y,sizex,sizey;

	switch (roz_clip_size_x)
	{
		case 0x3:	sizex = 1; break;
		case 0x2:	sizex = 2; break;
		default:	sizex = 4; break;
	}
	switch (roz_clip_size_y)
	{
		case 0x3:	sizey = 1; break;
		case 0x2:	sizey = 2; break;
		default:	sizey = 4; break;
	}

	for (y = roz_clip_y * 8;y < (roz_clip_y + sizey) * 8;y++)
	{
		for (x = roz_clip_x * 8;x < (roz_clip_x + sizex) * 8;x++)
		{
			tilemap_mark_tile_dirty(ult_936_tilemap,x | (y << 9));
		}
	}
}

WRITE16_HANDLER(ddd_053936_clip_w)
{
	if (offset == 1)
	{
		if (ACCESSING_MSB)
		{
			if (roz_clip_enable != (data & 0x0100))
			{
				roz_clip_enable = data & 0x0100;
				tilemap_mark_all_tiles_dirty(ult_936_tilemap);
			}
		}
	}
	else
	{
		static data16_t clip;
		data16_t old = clip;
		COMBINE_DATA(&clip);
		if (clip != old)
		{

			/* mark dirty the OLD clip area */
			mark_roz_clip_dirty();

			roz_clip_size_x = (clip & 0x3000) >> 12;
			roz_clip_size_y = (clip & 0xc000) >> 14;
			roz_clip_y = (clip & 0x0fc0) >> 6;
			roz_clip_x = (clip & 0x003f) >> 0;

			/* mark dirty the NEW clip area */
			mark_roz_clip_dirty();
		}
	}
}

// reference: 223e5c in gaiapolis (ROMs 34j and 36m)
READ16_HANDLER(gai_053936_tilerom_0_r)
{
	data8_t *ROM1 = (data8_t *)memory_region(REGION_GFX4);
	data8_t *ROM2 = (data8_t *)memory_region(REGION_GFX4);

	ROM1 += 0x20000;
	ROM2 += 0x20000+0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

READ16_HANDLER(ddd_053936_tilerom_0_r)
{
	data8_t *ROM1 = (data8_t *)memory_region(REGION_GFX4);
	data8_t *ROM2 = (data8_t *)memory_region(REGION_GFX4);

	ROM2 += 0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

// reference: 223e1a in gaiapolis (ROM 36j)
READ16_HANDLER(ddd_053936_tilerom_1_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX4);

	return ROM[offset/2];
}

// reference: 223db0 in gaiapolis (ROMs 32n, 29n, 26n)
READ16_HANDLER(gai_053936_tilerom_2_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX3);

	offset += (roz_rombank * 0x100000);

	return ROM[offset/2]<<8;
}

READ16_HANDLER(ddd_053936_tilerom_2_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX3);

	offset += (roz_rombank * 0x100000);

	return ROM[offset]<<8;
}
VIDEO_UPDATE(dadandarn) /* and gaiapols */
{
	int i, old, dirty;
	static UINT8 enablemasks[4] = { K55_INP_VRAM_A, K55_INP_VRAM_B, K55_INP_VRAM_C, K55_INP_VRAM_D };
	K054338_update_all_shadows();

	if (gametype == 0)
	{
		for (i = 0; i < 4; i++)
		{
			old = layer_colorbase[i];
			layer_colorbase[i] = K055555_get_palette_index(i)<<4;
			if (old != layer_colorbase[i])
			{
				K056832_mark_plane_dirty(i);
			}
		}

		sprite_colorbase = (K055555_get_palette_index(4)<<4)&0x7f;
		sprite_subcolorbase = (K055555_get_palette_index(5)<<4)&0x7f;
	}
	else
	{
		for (dirty=0, i=0; i<4; i++)
		{
			old = layer_colorbase[i];
			layer_colorbase[i] = K055555_get_palette_index(i)<<4;
			if (old != layer_colorbase[i]) dirty = 1;
		}
		if (dirty) K056832_MarkAllTilemapsDirty();

		sprite_colorbase = (K055555_get_palette_index(4)<<3)&0x7f;
		sprite_subcolorbase = (K055555_get_palette_index(5)<<3)&0x7f;
	}

	last_psac_colorbase = psac_colorbase;
	psac_colorbase = K055555_get_palette_index(5);

	if (last_psac_colorbase != psac_colorbase)
	{
		tilemap_mark_all_tiles_dirty(ult_936_tilemap);
	}

	K056832_tilemap_update();

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), cliprect);

	layers[0] = 0; layerpri[0] = K055555_read_register(K55_PRIINP_0);
	layers[1] = 1; layerpri[1] = K055555_read_register(K55_PRIINP_3);
	layers[2] = 2; layerpri[2] = K055555_read_register(K55_PRIINP_6);
	layers[3] = 3; layerpri[3] = K055555_read_register(K55_PRIINP_7);
	layers[4] = 4; layerpri[4] = K055555_read_register(K55_PRIINP_9);
	sortlayers5(layers, layerpri);

	for(i=0; i<4; i++)
	{
		if (layers[i] == 4)
		{
			if (K055555_read_register(K55_INPUT_ENABLES) & K55_INP_SUB1)
			{
				if (roz_enable) K053936_0_zoom_draw(bitmap, cliprect, ult_936_tilemap, 0, 1<<i);
			}
		}
		else
		{
			if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[i]])
			{
				K056832_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
			}
		}
	}

	K053247GP_sprites_draw(bitmap, cliprect);

	// HACK: To get around the priority system's four-layer limitation we have to assume the frontmost
	// plane is alway on top of everything else. This should be safe for Gaiapolis and Dadandarn.
	if (layers[4] == 4)
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & K55_INP_SUB1)
		{
			if (roz_enable) K053936_0_zoom_draw(bitmap, cliprect, ult_936_tilemap, 0, 0);
		}
	}
	else
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[4]])
		{
			K056832_tilemap_draw(bitmap, cliprect, layers[4], 0, 0);
		}
	}
}
VIDEO_UPDATE(martchmp) //*
{
	int i, old;
	static UINT8 enablemasks[4] = { K55_INP_VRAM_A, K55_INP_VRAM_B, K55_INP_VRAM_C, K55_INP_VRAM_D };
	K054338_update_all_shadows();

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if (old != layer_colorbase[i]) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<5;
	sprite_subcolorbase = K055555_get_palette_index(5)<<5;

	K056832_tilemap_update();

	fillbitmap(priority_bitmap, 0, NULL);
	K054338_fill_backcolor(bitmap, 0);

	layers[0] = 0; layerpri[0] = K055555_read_register(K55_PRIINP_0);
	layers[1] = 1; layerpri[1] = K055555_read_register(K55_PRIINP_3);
	layers[2] = 2; layerpri[2] = K055555_read_register(K55_PRIINP_6);
	layers[3] = 3; layerpri[3] = K055555_read_register(K55_PRIINP_7);

	// HACK: couldn't figure how the game disables BG layers; it doesn't do it through K55_INPUT_ENABLES.
	enablemasks[1] = (layerpri[1] > 0x0f) ? K55_INP_VRAM_B : 0;

	sortlayers(layers, layerpri);

	for(i=0; i<4; i++)
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[i]])
		{
			K056832_tilemap_draw(bitmap, cliprect, layers[i], 0, 1<<i);
		}
	}

	K053247GP_sprites_draw(bitmap, cliprect);
}
