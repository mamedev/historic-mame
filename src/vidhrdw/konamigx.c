/*
 * vidhrdw/konamigx.c - Konami GX video hardware (here there be dragons)
 *
 */

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

/*
> the color DAC has a 8 bits r/g/b and a 8bits brightness
> but there is 1 ram per color channel for the palette and no ram for brightness
> I guess you have a global brightness control in the '338 ?
> hmmm, to add insult to injury, the 5x5 has inputs for 4 tilemaps and 3 sprite-ish thingies
> but only 2 of the spriteish thingies are actually connected
> one is the output
> it outputs a spriteish thingy, probably so that you can cascade them
> specifically, you have 4 10bpp tilemaps inputs
> 2 8bpp color, 8bpp priority, 2 mix, 2 bri, 2 shadow inputs
         and one 17bpp, 8bpp pri, 2 mix and 2 bri that goes to the rom board
> one of the two pure sprite ones is grounded
> and you have a 17bpp, 8 pri, 1 dot on, 2 mix, 2 bri, 2 shadow output
> I guess the dot on tells the 338 when to insert its own background color
> the 338 gets in 13 of the color bits, and all the meta
> I guess in case of blending the 5x5 outputs more than one color per pixel

> pasc4 = 56540
> the structure seems to be psac2-vram-h/c/rom-psac4 in that order
> the psac 2 computing the coordinates
> ok, the psac2 output 2 11 bits coordinates
> bits 4-11 of each plus a double buffering bit lookup in 3 8bits rams to the get 24 bits data
> the top 8 bits are weird, the bottom 16 are directly an address, once you shift in the 8 remaining bits
> i.e., it's 16x16 tiles
> in fact the top 8 are xored with the bottom 8 bits, i.e. they fuzz the coordinates
> oh no, they don't fuzz
> they're flipx/y

*/

static int scrolld[2][4][2] = {
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};

static int layer_colorbase[4], layers[4], layerpri[4];
static int sprite_colorbase;
static int gx_tilebanks[8], gx_oldbanks[8];
static int gx_invertlayersBC;

extern data8_t gx_control1;

static void konamigx_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);

	*color = layer_colorbase[layer] | ((*color & 0xfc) >> 2);
	*code = (gx_tilebanks[(*code & 0xe000)>>13]*0x2000)+(*code&0x1fff);
}

/*
> bits 8-13 are the low priority bits
> i.e. pri 0-5
> pri 6-7 can be either 1, bits 14,15 or bits 16,17
> contro.bit 2 being 0 forces the 1
> when control.bit 2 is 1, control.bit 3 selects between the two
> 0 selects 16,17
> that gives you the entire 8 bits of the sprite priority
> ok, lemme see if I've got this.  bit2 = 0 means the top bits are 11, bit2=1 means the top bits are bits 14/15 (of the whatever word?) else
+16+17?
> bit3=1 for the second

 *   6  | ---------xxxxxxx | "color", but depends on external connections


> there are 8 color lines entering the 5x5
> that means the palette is 4 bits, not 5 as you currently have
> the bits 4-9 are the low priority bits
> bits 10/11 or 12/13 are the two high priority bits, depending on the control word
> and bits 14/15 are the shadow bits
> mix0/1 and brit0/1 come from elsewhere
> they come from the '673 all right, but not from word 6
> and in fact the top address bits are highly suspect
> only 18 of the address bits go to the roms
> the next 2 go to cai0/1 and the next 4 to bk0-3
> (the '246 indexes the roms, the '673 reads the result)
> the roms are 64 bits wide
> so, well, the top bits of the code are suspicious
*/

static void konamigx_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x03e0) >> 3;

	if (pri <= layerpri[3])					*priority_mask = 0;
	else if (pri > layerpri[3] && pri <= layerpri[2])	*priority_mask = 0xff00;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xff00|0xf0f0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xff00|0xf0f0|0xcccc;
	else 							*priority_mask = 0xff00|0xf0f0|0xcccc|0xaaaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static int _gxcommoninitnosprites(void)
{
	int i;

	K054338_vh_start();
	K055555_vh_start();

	for (i = 0; i < 8; i++)
	{
		gx_tilebanks[i] = gx_oldbanks[i] = 0;
	}

	state_save_register_INT32("KGXVideo", 0, "tilebanks", gx_tilebanks, 8);

	return 0;
}

static int _gxcommoninit(void)
{
	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX, -48, 24, konamigx_sprite_callback))
	{
		return 1;
	}

	gx_invertlayersBC = 0;

	return _gxcommoninitnosprites();
}

VIDEO_START(konamigx_5bpp)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 1, scrolld, konamigx_tile_callback))
	{
		return 1;
	}

	/* here are some hand tuned per game scroll offsets to go with the per game visible areas,
	   i see no better way of doing this for now... */

	if (!strcmp(Machine->gamedrv->name,"tbyahhoo"))
	{
		K056832_set_LayerOffset(0, 0x18, 1);
		K056832_set_LayerOffset(1, 0x16, 1); // tv screen
		K056832_set_LayerOffset(2, 0x14, 1); // background on title screen
		K056832_set_LayerOffset(3, 0x13, 1);
	}

	if (!strcmp(Machine->gamedrv->name,"puzldama"))
	{
		K056832_set_LayerOffset(0, 5, 1);
		K056832_set_LayerOffset(1, 3, 1);
		K056832_set_LayerOffset(2, 1, 1);
		K056832_set_LayerOffset(3, 0, 1);
	}

	if (!strcmp(Machine->gamedrv->name,"daiskiss"))
	{
		K056832_set_LayerOffset(0, 23, 0);
		K056832_set_LayerOffset(1, 22, 0);
		K056832_set_LayerOffset(2, 20, 0);
		K056832_set_LayerOffset(3, 19, 0);
	}

	if ((!strcmp(Machine->gamedrv->name,"gokuparo")) || (!strcmp(Machine->gamedrv->name,"sexyparo")) || (!strcmp(Machine->gamedrv->name,"fantjour")) )
	{
		// references = intro, select screens etc.
		K056832_set_LayerOffset(0, 8, 1);
		K056832_set_LayerOffset(1, 6, 1);
		K056832_set_LayerOffset(2, 4, 1);
		K056832_set_LayerOffset(3, 2, 1);
	}


	return _gxcommoninit();
}

VIDEO_START(le2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_8, 1, scrolld, konamigx_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_LE2, -48, 24, konamigx_sprite_callback))
	{
		return 1;
	}

	gx_invertlayersBC = 1;

	return _gxcommoninitnosprites();
}

VIDEO_START(konamigx_6bpp)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 1, scrolld, konamigx_tile_callback))
	{
		return 1;
	}

	_gxcommoninit();

	if (!strcmp(Machine->gamedrv->name,"tokkae"))
	{
		K056832_set_LayerOffset(0, 4, 0);
		K056832_set_LayerOffset(1, 2, 0);
		K056832_set_LayerOffset(2, 0, 0);
		K056832_set_LayerOffset(3, -1, 0);
	}
	return 0;
}

VIDEO_START(konamigx_6bpp_2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 1, scrolld, konamigx_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX6, -48, 24, konamigx_sprite_callback))
	{
		return 1;
	}

	return _gxcommoninitnosprites();
}

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

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(2, 3)
}

VIDEO_UPDATE(konamigx)
{
	int i, old, banking_recalc = 0;
#if 0
	int x, y;
	UINT32 *pLine;
#endif
	static UINT8 enablemasks[4] = { K55_INP_VRAM_A, K55_INP_VRAM_B, K55_INP_VRAM_C, K55_INP_VRAM_D };
	static UINT8 blendmasks[4] = { 3, 0xc, 0x30, 0xc0 };

	#ifdef MAME_DEBUG
	if (K055555_read_register(K55_CONTROL) & K55_CTL_FLIPPRI)
	{
		logerror("WARNING: inverted priority semantics enabled!\n");
	}
	#endif

	K054338_update_all_shadows();


	K056832_tilemap_update();

	fillbitmap(priority_bitmap, 0, NULL);

	/* control register selects gradient vs. solid BG color */
	K054338_fill_backcolor(bitmap, (gx_control1 & 0x20));
#if 0
	if (!(gx_control1 & 0x20))
	{
		K054338_fill_solid_bg(bitmap);
	}
	else	/* Use the K055555's background */
	{
		int pbase = K055555_read_register(K55_PALBASE_BG) << 9;
		int ctl = K055555_read_register(K55_CONTROL);

		if (ctl & K55_CTL_GRADENABLE)
		{
			/* it's a gradient, check which direction */
			if (ctl & K55_CTL_GRADDIR)
			{	/* horizontal gradient */
				for (y = 0; y < bitmap->height; y++)
				{
					pLine = (UINT32 *)bitmap->base;
					pLine += ((bitmap->rowbytes / 4)*y);
					for (x = 0; x < bitmap->width; x++)
						*pLine++ = paletteram32[pbase+x];
				}
			}
			else	/* vertical gradient */
			{
				for (y = 0; y < bitmap->height; y++)
				{
					pLine = (UINT32 *)bitmap->base;
					pLine += ((bitmap->rowbytes / 4)*y);
					for (x = 0; x < bitmap->width; x++)
						*pLine++ = paletteram32[pbase+y];
				}
			}
		}
		else	/* not a gradient */
		{
			for (y = 0; y < bitmap->height; y++)
			{
				pLine = (UINT32 *)bitmap->base;
				pLine += ((bitmap->rowbytes / 4)*y);
				for (x = 0; x < bitmap->width; x++)
					*pLine++ = paletteram32[pbase];
			}
		}
	}
#endif
	layers[0] = 0;
	layers[1] = 1;
	layers[2] = 2;
	layers[3] = 3;
	layerpri[0] = K055555_read_register(K55_PRIINP_0);

	/* LE2 works better with this, no idea why, as it contradicts the documentation */
	if (gx_invertlayersBC)
	{
		layerpri[1] = K055555_read_register(K55_PRIINP_6);
		layerpri[2] = K055555_read_register(K55_PRIINP_3);
	}
	else
	{
		layerpri[1] = K055555_read_register(K55_PRIINP_3);
		layerpri[2] = K055555_read_register(K55_PRIINP_6);
	}
	layerpri[3] = K055555_read_register(K55_PRIINP_7);

	sortlayers(layers, layerpri);

#ifdef MAME_DEBUG
	if (keyboard_pressed(KEYCODE_D))
	{
		logerror("DEBUG DUMP\n");
		logerror("Plane enables = %x\n", K055555_read_register(K55_INPUT_ENABLES));
		logerror("Blend enables = %x\n", K055555_read_register(K55_BLEND_ENABLES));
		logerror("blend factor = %x\n", K054338_read_register(K338_REG_PBLEND)&0x1f);
		logerror("mixer pri: %d %d %d %d %d %d %d %d\n", K055555_read_register(K55_PRIINP_0), K055555_read_register(K55_PRIINP_1), K055555_read_register(K55_PRIINP_2),
			K055555_read_register(K55_PRIINP_3), K055555_read_register(K55_PRIINP_4), K055555_read_register(K55_PRIINP_5), K055555_read_register(K55_PRIINP_6),
			K055555_read_register(K55_PRIINP_7));
		logerror("priorities: %d %d %d %d\n", layerpri[0], layerpri[1], layerpri[2], layerpri[3]);
		logerror("layers in pri order: %d %d %d %d\n", layers[0], layers[1], layers[2], layers[3]);
		logerror("palettes: %x %x %x %x %x %x\n",
			K055555_get_palette_index(0),
			K055555_get_palette_index(1),
			K055555_get_palette_index(2),
			K055555_get_palette_index(3),
			K055555_get_palette_index(4),
			K055555_get_palette_index(5));
	}
#endif

	/* if any banks are different from last render, we need to flush the planes */
	for (i = 0; i < 8; i++)
	{
		if (gx_oldbanks[i] != gx_tilebanks[i])
		{
			gx_oldbanks[i] = gx_tilebanks[i];
			banking_recalc = 1;
		}
	}

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<6;
		if ((old != layer_colorbase[i]) || (banking_recalc)) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<5;

	for(i=0; i<4; i++)
	{
		if (K055555_read_register(K55_INPUT_ENABLES) & enablemasks[layers[i]])
		{
			if ((K055555_read_register(K55_BLEND_ENABLES) &	blendmasks[layers[i]]) &&
				(K055555_read_register(K55_BLEND_ENABLES) != 0xff))
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

	if (K055555_read_register(K55_INPUT_ENABLES) & 0x10)
	{
		K053247GP_sprites_draw(bitmap, cliprect);
	}

	if( gx_invertlayersBC )
	{
		draw_crosshair( bitmap, readinputport( 9)*287/0xff+24, readinputport(10)*223/0xff+16, cliprect );
		draw_crosshair( bitmap, readinputport(11)*287/0xff+24, readinputport(12)*223/0xff+16, cliprect );
	}

// debug code for adjusting layer offsets
#if 0
	{
		static int lay0_offset,lay1_offset,lay2_offset,lay3_offset;

		if ( keyboard_pressed_memory(KEYCODE_Q) ) lay0_offset++;
		if ( keyboard_pressed_memory(KEYCODE_A) ) lay0_offset--;
		if ( keyboard_pressed_memory(KEYCODE_W) ) lay1_offset++;
		if ( keyboard_pressed_memory(KEYCODE_S) ) lay1_offset--;
		if ( keyboard_pressed_memory(KEYCODE_E) ) lay2_offset++;
		if ( keyboard_pressed_memory(KEYCODE_D) ) lay2_offset--;
		if ( keyboard_pressed_memory(KEYCODE_R) ) lay3_offset++;
		if ( keyboard_pressed_memory(KEYCODE_F) ) lay3_offset--;

		K056832_set_LayerOffset(0, lay0_offset, 1);
		K056832_set_LayerOffset(1, lay1_offset, 1);
		K056832_set_LayerOffset(2, lay2_offset, 1);
		K056832_set_LayerOffset(3, lay3_offset, 1);

		usrintf_showmessage	( "%02d %02d %02d %02d",lay0_offset,lay1_offset,lay2_offset,lay3_offset);
	}
#endif
}

WRITE32_HANDLER( konamigx_palette_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

 	r = (paletteram32[offset] >>16) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	b = (paletteram32[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

WRITE32_HANDLER( konamigx_tilebank_w )
{
	if (!(mem_mask & 0xff000000))
		gx_tilebanks[offset*4] = (data>>24)&0xff;
	if (!(mem_mask & 0xff0000))
		gx_tilebanks[offset*4+1] = (data>>16)&0xff;
	if (!(mem_mask & 0xff00))
		gx_tilebanks[offset*4+2] = (data>>8)&0xff;
	if (!(mem_mask & 0xff))
		gx_tilebanks[offset*4+3] = data&0xff;
}

WRITE32_HANDLER( konamigx_sprbank_w )
{
	logerror("Write %x (mask %x) to spritebank at %x\n", data, mem_mask, offset);
}
