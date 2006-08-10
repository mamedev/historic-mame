/***************************************************************************

    Sega G-80 raster hardware

***************************************************************************/

#include "driver.h"
#include "segag80r.h"
#include "res_net.h"


UINT8 *segar_characterram;
UINT8 *segar_characterram2;
UINT8 *segar_mem_colortable;
UINT8 *segar_mem_bcolortable;

static double rweights[5], gweights[5], bweights[4];

static UINT8 *dirtychar;
static UINT8 video_control;
static UINT8 video_flip;
static UINT8 vblank_latch;

static tilemap *spaceod_bg_htilemap;
static tilemap *spaceod_bg_vtilemap;
static UINT16 spaceod_hcounter;
static UINT16 spaceod_vcounter;
static UINT8 spaceod_fixed_color;
static UINT8 spaceod_bg_control;
static UINT8 spaceod_bg_enable;
static UINT8 spaceod_bg_detect;
static UINT8 spaceod_bg_detect_tile_color = 1;

static tilemap *monsterb_bg_tilemap;
static UINT8 monsterb_bg_control;
static UINT8 *monsterb_bg_palette;

static tilemap *pignewt_bg_tilemap;
static UINT8 *pignewt_bg_palette;
static UINT16 pignewt_bg_scrollx;
static UINT16 pignewt_bg_scrolly;
static UINT8 pignewt_bg_color_offset;
static UINT8 pignewt_bg_char_bank;
static UINT8 pignewt_bg_enable;
static UINT8 pignewt_bg_flip;


static void vblank_latch_clear(int param)
{
	vblank_latch = 0;
}


INTERRUPT_GEN( segar_vblank_start )
{
	/* latch the current flip state */
	video_flip = video_control & 1;

	/* set a timer to mimic the 555 timer that drives the EDGINT signal */
	/* the 555 is run in monostable mode with R=56000 and C=1000pF */
	vblank_latch = 1;
	timer_set(1.1 * 1000e-12 * 56000, 0, vblank_latch_clear);

	/* if interrupts are enabled, clock one */
	if (video_control & 0x04)
		irq0_line_hold();
}


static void g80_set_palette_entry(int entry, UINT8 data)
{
	int bit0, bit1, bit2;
	int r, g, b;

	/* extract the raw RGB bits */
	r = (data & 0x07) >> 0;
	g = (data & 0x38) >> 3;
	b = (data & 0xc0) >> 6;

	/* red component */
	bit0 = (r >> 0) & 0x01;
	bit1 = (r >> 1) & 0x01;
	bit2 = (r >> 2) & 0x01;
	r = combine_3_weights(rweights, bit0, bit1, bit2);

	/* green component */
	bit0 = (g >> 0) & 0x01;
	bit1 = (g >> 1) & 0x01;
	bit2 = (g >> 2) & 0x01;
	g = combine_3_weights(gweights, bit0, bit1, bit2);

	/* blue component */
	bit0 = (b >> 0) & 0x01;
	bit1 = (b >> 1) & 0x01;
	b = combine_2_weights(bweights, bit0, bit1);

	palette_set_color(entry, r, g, b);
}


VIDEO_START( segar )
{
	static const int rg_resistances[3] = { 4700, 2400, 1200 };
	static const int b_resistances[2] = { 2000, 1000 };

	/* compute the color output resistor weights at startup */
	compute_resistor_weights(0,	255, -1.0,
			3,	rg_resistances, rweights, 220, 0,
			3,	rg_resistances, gweights, 220, 0,
			2,	b_resistances,  bweights, 220, 0);

	/* allocate paletteram */
	dirtychar = auto_malloc(256);
	memset(dirtychar, 1, 256);
	paletteram = auto_malloc(0x40);

	/* register for save states */
	state_save_register_global_pointer(paletteram, 0x40);
	return 0;
}


READ8_HANDLER( segar_videoram_r )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1000) && (video_control & 0x02))
	{
		offset &= 0x3f;
		return paletteram[offset];
	}

	/* all other accesses go to videoram */
	return videoram[offset];
}

WRITE8_HANDLER( segar_videoram_w )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1000) && (video_control & 0x02))
	{
		offset &= 0x3f;
		paletteram[offset] = data;
		g80_set_palette_entry(offset, data);
		return;
	}

	/* all other accesses go to videoram */
	videoram[offset] = data;

	/* track which characters are dirty */
	if (offset & 0x800)
		dirtychar[(offset & 0x7ff) / 8] = TRUE;
}



READ8_HANDLER( segar_video_port_r )
{
	if (offset == 0)
	{
		logerror("%04X:segar_video_port_r(%d)\n", activecpu_get_pc(), offset);
		return 0xff;
	}
	else
	{
		/*
            D0 = 555 timer output from U10 (goes to EDGINT as well)
            D1 = current latched FLIP state
            D2 = interrupt enable state
            D3 = n/c
        */
		return (vblank_latch << 0) | (video_flip << 1) | (video_control & 0x04) | 0xf8;
	}
}


WRITE8_HANDLER( segar_video_port_w )
{
	if (offset == 0)
	{
		logerror("%04X:segar_video_port_w(%d) = %02X\n", activecpu_get_pc(), offset, data);
	}
	else
	{
		/*
            D0 = FLIP (latched at VSYNC)
            D1 = if low, allows writes to the upper 4k of video RAM
               = if high, allows writes to palette RAM
            D2 = interrupt enable
            D3 = n/c
        */
		video_control = data;
	}
}


static void g80v_draw_videoram(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int flipmask = video_flip ? 0x1f : 0x00;
	int x, y;

	/* iterate over the screen and draw visible tiles */
	for (y = cliprect->min_y / 8; y <= cliprect->max_y / 8; y++)
	{
		int effy = video_flip ? 27 - y : y;
		for (x = cliprect->min_x / 8; x <= cliprect->max_x / 8; x++)
		{
			int offs = effy * 32 + (x ^ flipmask);
			UINT8 tile = videoram[offs];

			/* if the tile is dirty, decode it */
			if (dirtychar[tile])
			{
				decodechar(Machine->gfx[0], tile, &videoram[0x800], Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtychar[tile] = 0;
			}

			/* draw the tile */
			drawgfx(bitmap, Machine->gfx[0], tile, tile >> 4, video_flip, video_flip, x*8, y*8, cliprect, TRANSPARENCY_NONE, 0);
		}
	}

}


VIDEO_UPDATE( segar )
{
	g80v_draw_videoram(bitmap, cliprect);
	return 0;
}





static void spaceod_get_tile_info(int tile_index)
{
	int code = memory_region(REGION_GFX2)[tile_index + 0x1000 * (spaceod_bg_control >> 6)];
	SET_TILE_INFO(1, code + 0x100 * ((spaceod_bg_control >> 2) & 1), 0, 0)
}

static UINT32 spaceod_scan_rows(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* this works for both horizontal and vertical tilemaps */
	/* which are 4 32x32 sections */
	return (row & 31) * 32 + (col & 31) + ((row >> 5) * 32*32) + ((col >> 5) * 32*32);
}

VIDEO_START( spaceod )
{
	static const int resistances[2] = { 1800, 1200 };
	double trweights[2], tgweights[2], tbweights[2];
	int i;

	/* initialize the standard G80 video board first */
	if (video_start_segar())
		return 1;

	/* compute the color output resistor weights at startup */
	compute_resistor_weights(0,	255, -1.0,
			2,	resistances, trweights, 220, 0,
			2,	resistances, tgweights, 220, 0,
			2,	resistances, tbweights, 220, 0);

	/* initialize the fixed background palette */
	for (i = 0; i < 64; i++)
	{
		int bit0, bit1;
		int r, g, b;

		/* extract the raw RGB bits */
		r = (i & 0x30) >> 4;
		g = (i & 0x0c) >> 2;
		b = (i & 0x03) >> 0;

		/* red component */
		bit0 = (r >> 0) & 0x01;
		bit1 = (r >> 1) & 0x01;
		r = combine_2_weights(trweights, bit0, bit1);

		/* green component */
		bit0 = (g >> 0) & 0x01;
		bit1 = (g >> 1) & 0x01;
		g = combine_2_weights(tgweights, bit0, bit1);

		/* blue component */
		bit0 = (b >> 0) & 0x01;
		bit1 = (b >> 1) & 0x01;
		b = combine_2_weights(tbweights, bit0, bit1);

		palette_set_color(64 + i, r, g, b);
	}

	/* allocate two tilemaps, one for horizontal, one for vertical */
	spaceod_bg_htilemap = tilemap_create(spaceod_get_tile_info, spaceod_scan_rows, TILEMAP_OPAQUE, 8,8, 128,32);
	spaceod_bg_vtilemap = tilemap_create(spaceod_get_tile_info, spaceod_scan_rows, TILEMAP_OPAQUE, 8,8, 32,128);
	return 0;
}


static void spaceod_draw_background(mame_bitmap *bitmap, const rectangle *cliprect)
{
	mame_bitmap *pixmap = tilemap_get_pixmap(!(spaceod_bg_control & 0x02) ? spaceod_bg_htilemap : spaceod_bg_vtilemap);
	int flipmask = (spaceod_bg_control & 0x01) ? 0xff : 0x00;
	int xoffset = (spaceod_bg_control & 0x02) ? 0x10 : 0x00;
	int xmask = pixmap->width - 1;
	int ymask = pixmap->height - 1;
	int x, y;

	/* The H and V counters on this board are independent of the ones on */
	/* the main board. The H counter starts counting from 0 when EXT BLK */
	/* goes to 0; this coincides with H=0, so that's fine. However, the V */
	/* counter starts counting from 0 when VSYNC=0, which happens at line */
	/* 240, giving us an offset of (262-240) = 22 scanlines. */

	/* now fill in the background wherever there are black pixels */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		int effy = (y + spaceod_vcounter + 22) ^ flipmask;
		UINT16 *src = (UINT16 *)pixmap->base + (effy & ymask) * pixmap->rowpixels;
		UINT16 *dst = (UINT16 *)bitmap->base + y * bitmap->rowpixels;

		/* loop over horizontal pixels */
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int effx = ((x + spaceod_hcounter) ^ flipmask) + xoffset;
			UINT8 fgpix = paletteram[dst[x]];
			UINT8 bgpix = src[effx & xmask] & 0x3f;

			/* the background detect flag is set if:
                - bgpix != 0 AND
                - fgpix != 0 AND
                - the original tile color == DIP switches
            */
			if (bgpix != 0 && fgpix != 0 && (dst[x] >> 2) == spaceod_bg_detect_tile_color)
				spaceod_bg_detect = 1;

			/* the background graphics are only displayed if:
                - fgpix == 0 AND
                - !EXTBLK (not blanked) AND
                - bg_enable == 0
            */
			if (fgpix == 0 && spaceod_bg_enable == 0)
				dst[x] = bgpix | spaceod_fixed_color | 0x40;
		}
	}
}

VIDEO_UPDATE( spaceod )
{
	/* do the regular G80 update first */
	g80v_draw_videoram(bitmap, cliprect);

	/* then draw the background */
	spaceod_draw_background(bitmap, cliprect);
	return 0;
}


READ8_HANDLER( spaceod_back_port_r )
{
	/* force an update to get the current detection value */
	force_partial_update(0, cpu_getscanline());
	return 0xfe | spaceod_bg_detect;
}


WRITE8_HANDLER( spaceod_back_port_w )
{
	switch (offset & 7)
	{
		/* port 0: latches D0-D7 into LS377 at U39 (SH4)

            d0 = counter direction: controls U/D on LS191 counters
            d1 = horizontal (0) or vertical (1) scrolling
            d2 = character bank (0/1)
            d6 = background ROM select 0
            d7 = background ROM select 1
        */
		case 0:
			if ((spaceod_bg_control ^ data) & 0xc4)
			{
				tilemap_mark_all_tiles_dirty(spaceod_bg_htilemap);
				tilemap_mark_all_tiles_dirty(spaceod_bg_vtilemap);
			}
			spaceod_bg_control = data;
			break;

		/* port 1: loads both H and V counters with 0 */
		case 1:
			spaceod_hcounter = 0;
			spaceod_vcounter = 0;
			break;

		/* port 2: clocks either the H or V counters (based on port 0:d1) */
		/* either up or down (based on port 0:d0) */
		case 2:
			if (!(spaceod_bg_control & 0x02))
			{
				if (!(spaceod_bg_control & 0x01))
					spaceod_hcounter++;
				else
					spaceod_hcounter--;
			}
			else
			{
				if (!(spaceod_bg_control & 0x01))
					spaceod_vcounter++;
				else
					spaceod_vcounter--;
			}
			break;

		/* port 3: clears the background detection flag */
		case 3:
			force_partial_update(0, cpu_getscanline());
			spaceod_bg_detect = 0;
			break;

		/* port 4: enables (0)/disables (1) the background */
		case 4:
			spaceod_bg_enable = data & 1;
			break;

		/* port 5: specifies fixed background color */
		/* top two bits are not connected */
		case 5:
			spaceod_fixed_color = data & 0x3f;
			break;

		/* port 6: latches D0-D7 into LS377 at U37 -> CN1-11/12/13/14/15/16/17/18 */
		/* port 7: latches D0-D5 into LS174 at U40 -> CN2-1/2/3/4/5/6 */
		case 6:
		case 7:
			break;
	}
}





READ8_HANDLER( monsterb_videoram_r )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1fc0) == 0x1040 && (video_control & 0x40))
		return monsterb_bg_palette[offset];

	/* handle everything else */
	return segar_videoram_r(offset);
}

WRITE8_HANDLER( monsterb_videoram_w )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1fc0) == 0x1040 && (video_control & 0x40))
	{
		offset &= 0x3f;
		monsterb_bg_palette[offset] = data;
		g80_set_palette_entry(offset | 0x40, data);
		return;
	}

	/* handle everything else */
	segar_videoram_w(offset, data);
}

WRITE8_HANDLER( monsterb_back_port_w )
{
	switch (offset & 7)
	{
		/* port 0: not used (looks like latches for C7-C10 = background color) */
		case 0:
			break;

		/* port 1: not used (looks like comparator tile color value for collision detect)  */
		case 1:
			break;

		/* port 2: not connected */
		/* port 3: not connected */
		case 2:
		case 3:
			break;

		/* port 4: main control latch

            d0 = CG0 - BG MSB ROM bank select bit 0
            d1 = CG1 - BG MSB ROM bank select bit 1
            d2 = CG2 - BG LSB ROM bank select bit 0
            d3 = CG3 - BG LSB ROM bank select bit 1
            d4 = SCN0 - background select bit 0
            d5 = SCN1 - background select bit 1
            d6 = SCN2 - background select bit 2
            d7 = BKGEN - background enable
         */
		case 4:
			if ((monsterb_bg_control ^ data) & 0x7f)
				tilemap_mark_all_tiles_dirty(monsterb_bg_tilemap);
			monsterb_bg_control = data;
			break;

		/* port 5: not connected */
		case 5:
			break;

		/* port 6: not connected (overlaps with G80 board port BE) */
		case 6:
			segar_video_port_w(0, data);
			break;

		/* port 7: snooped (overlaps with G80 board port BF)

            d3 = FILD
            d6 = CRAMSEL
         */
		case 7:
			segar_video_port_w(1, data);
			break;
	}
}

static void monsterb_get_tile_info(int tile_index)
{
	int code = memory_region(REGION_GFX2)[tile_index + 0x400 * ((monsterb_bg_control >> 4) & 7)];
	SET_TILE_INFO(1, code + (monsterb_bg_control & 0x0f) * 0x100, code >> 4, 0)
}

VIDEO_START( monsterb )
{
	/* initialize the standard G80 video board first */
	if (video_start_segar())
		return 1;

	monsterb_bg_palette = auto_malloc(0x40);

	monsterb_bg_tilemap = tilemap_create(monsterb_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 32,32);
	return 0;
}

static void monsterb_draw_background(mame_bitmap *bitmap, const rectangle *cliprect)
{
	mame_bitmap *pixmap = tilemap_get_pixmap(monsterb_bg_tilemap);
	int flipmask = (video_control & 0x08) ? 0xff : 0x00;
	int xmask = pixmap->width - 1;
	int ymask = pixmap->height - 1;
	int x, y;

	/* if disabled, draw nothing */
	if (!(monsterb_bg_control & 0x80))
		return;

	/* now fill in the background wherever there are black pixels */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		int effy = y ^ flipmask;
		UINT16 *src = (UINT16 *)pixmap->base + (effy & ymask) * pixmap->rowpixels;
		UINT16 *dst = (UINT16 *)bitmap->base + y * bitmap->rowpixels;

		/* loop over horizontal pixels */
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int effx = x ^ flipmask;
			UINT8 fgpix = paletteram[dst[x]];
			UINT8 bgpix = src[effx & xmask];

			/* the background graphics are only displayed if:
                - fgpix == 0 AND
                - !EXTBLK (not blanked) AND
                - bg_enable == 0
            */
			if (fgpix == 0)
				dst[x] = bgpix;
		}
	}
}

VIDEO_UPDATE( monsterb )
{
	/* do the regular G80 update first */
	g80v_draw_videoram(bitmap, cliprect);

	/* then draw the background */
	monsterb_draw_background(bitmap, cliprect);
	return 0;
}





READ8_HANDLER( pignewt_videoram_r )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1fc0) == 0x1040 && (video_control & 0x02))
		return pignewt_bg_palette[offset];

	/* handle everything else */
	return segar_videoram_r(offset);
}

WRITE8_HANDLER( pignewt_videoram_w )
{
	/* accesses to the upper half of VRAM go to paletteram if selected */
	if ((offset & 0x1fc0) == 0x1040 && (video_control & 0x02))
	{
		offset &= 0x3f;
		pignewt_bg_palette[offset] = data;
		g80_set_palette_entry(offset | 0x40, data);
		return;
	}

	/* handle everything else */
	segar_videoram_w(offset, data);
}

WRITE8_HANDLER( pignewt_back_color_w )
{
	if (offset == 0)
		pignewt_bg_color_offset = data;
	else
		printf("pignewt_back_color_w(%d) = %02X\n", pignewt_bg_color_offset, data);
//      g80_set_palette_entry(pignewt_bg_color_offset | 0x40, data);
}

WRITE8_HANDLER( pignewt_back_port_w )
{
if (offset != 7) printf("back(%d) = %02X\n", offset, data);

	switch (offset & 7)
	{
		/* port 0: scroll offset low */
		case 0:
			pignewt_bg_scrollx = (pignewt_bg_scrollx & 0x300) | data;
			break;

		/* port 1: scroll offset high */
		case 1:
			pignewt_bg_scrollx = (pignewt_bg_scrollx & 0x0ff) | ((data << 8) & 0x300);
			pignewt_bg_enable = data & 0x80;
			break;

		/* port 2: scroll offset low */
		case 2:
			pignewt_bg_scrolly = (pignewt_bg_scrolly & 0x300) | data;
			break;

		/* port 3: scroll offset high */
		case 3:
			pignewt_bg_scrolly = (pignewt_bg_scrolly & 0x0ff) | ((data << 8) & 0x300);
			pignewt_bg_flip = data & 0x80;
			break;

		/* port 4: background character bank control

            d0 = CG0 - BG MSB ROM bank select bit 0
            d1 = CG1 - BG MSB ROM bank select bit 1
            d2 = CG2 - BG LSB ROM bank select bit 0
            d3 = CG3 - BG LSB ROM bank select bit 1
         */
		case 4:
data = ((data >> 2) & 3) | (data & 0x0c);
			if ((pignewt_bg_char_bank ^ data) & 0x0f)
				tilemap_mark_all_tiles_dirty(pignewt_bg_tilemap);
			pignewt_bg_char_bank = data & 0x0f;
			break;

		/* port 5: not connected? */
		case 5:
			break;

		/* port 6: not connected (overlaps with G80 board port BE) */
		case 6:
			segar_video_port_w(0, data);
			break;

		/* port 7: snooped (overlaps with G80 board port BF) */
		case 7:
			segar_video_port_w(1, data);
			break;
	}
}

static void pignewt_get_tile_info(int tile_index)
{
	int code = memory_region(REGION_GFX2)[tile_index];
	SET_TILE_INFO(1, code + pignewt_bg_char_bank * 0x100, code >> 4, 0)
}

VIDEO_START( pignewt )
{
	/* initialize the standard G80 video board first */
	if (video_start_segar())
		return 1;

	pignewt_bg_palette = auto_malloc(0x40);

	pignewt_bg_tilemap = tilemap_create(pignewt_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 128,128);
	return 0;
}

static void pignewt_draw_background(mame_bitmap *bitmap, const rectangle *cliprect)
{
	mame_bitmap *pixmap = tilemap_get_pixmap(pignewt_bg_tilemap);
	int flipmask = pignewt_bg_flip ? 0x3ff : 0x00;
	int xmask = pixmap->width - 1;
	int ymask = pixmap->height - 1;
	int x, y;

	/* if disabled, draw nothing */
	if (!pignewt_bg_enable)
		return;

	/* now fill in the background wherever there are black pixels */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		int effy = (y + pignewt_bg_scrolly) ^ flipmask;
		UINT16 *src = (UINT16 *)pixmap->base + (effy & ymask) * pixmap->rowpixels;
		UINT16 *dst = (UINT16 *)bitmap->base + y * bitmap->rowpixels;

		/* loop over horizontal pixels */
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int effx = (x + pignewt_bg_scrollx) ^ flipmask;
			UINT8 fgpix = dst[x];
			UINT8 bgpix = src[effx & xmask];

			/* the background graphics are only displayed if:
                - fgpix == 0 AND
                - !EXTBLK (not blanked) AND
                - bg_enable == 0
            */
			if ((fgpix & 0x03) == 0)
				dst[x] = bgpix;
		}
	}
}

VIDEO_UPDATE( pignewt )
{
	/* do the regular G80 update first */
	g80v_draw_videoram(bitmap, cliprect);

	/* then draw the background */
	pignewt_draw_background(bitmap, cliprect);
	return 0;
}

/*
pignewt:
    ($b4) = index
    ($b5) = data (palette)?

    ($b8) = ($0300 - value).lo
    ($b9) = ($0300 - value).hi | $80
    ($ba) = ($0320 - value).lo
    ($bb) = ($0320 - value).hi | $80
    ($bc) = 0,5,$A
    ($be) = value & 7
    ($bf) = 4,6,$D
*/



typedef struct
{
	unsigned char dirtychar[256];		// graphics defined in RAM, mark when changed

	unsigned char colorRAM[0x40];		// stored in a 93419 (vid board)
	unsigned char bcolorRAM[0x40];		// stored in a 93419 (background board)
	unsigned char color_write_enable;	// write-enable the 93419 (vid board)
	unsigned char flip;					// cocktail flip mode (vid board)
	unsigned char bflip;				// cocktail flip mode (background board)

	unsigned char refresh;				// refresh the screen
	unsigned char brefresh;				// refresh the background
	unsigned char char_refresh;			// refresh the character graphics

	unsigned char has_bcolorRAM;		// do we have background color RAM?
	unsigned char background_enable;	// draw the background?
	unsigned int back_scene;
	unsigned int back_charset;

	// used for Pig Newton
	unsigned int bcolor_offset;

	// used for Space Odyssey
	unsigned char backfill;
	unsigned char fill_background;
	unsigned int backshift;
	mame_bitmap *horizbackbitmap;
	mame_bitmap *vertbackbitmap;
} SEGAR_VID_STRUCT;

static SEGAR_VID_STRUCT sv;



#if 0
	static unsigned char color_scale[] = {0x00, 0x40, 0x80, 0xC0 };
	int i;

	/* Space Odyssey uses a static palette for the background, so
       our choice of colors isn't exactly arbitrary.  S.O. uses a
       6-bit color setup, so we make sure that every 0x40 colors
       gets a nice 6-bit palette.

       (All of the other G80 games overwrite the default colors on startup)
    */
	for (i = 0;i < (Machine->drv->total_colors - 1);i++)
	{
		int r = color_scale[((i & 0x30) >> 4)];
		int g = color_scale[((i & 0x0C) >> 2)];
		int b = color_scale[((i & 0x03) << 0)];
		palette_set_color(i+1,r,g,b);
	}

	for (i = 0;i < Machine->drv->total_colors;i++)
		colortable[i] = i;
#endif

/***************************************************************************
The two bit planes are separated in memory.  If either bit plane changes,
mark the character as modified.
***************************************************************************/

WRITE8_HANDLER( segar_characterram_w )
{
	sv.dirtychar[offset / 8] = 1;

	segar_characterram[offset] = data;
}

WRITE8_HANDLER( segar_characterram2_w )
{
	sv.dirtychar[offset / 8] = 1;

	segar_characterram2[offset] = data;
}


/***************************************************************************
If a color changes, refresh the entire screen because it's possible that the
color change affected the transparency (switched either to or from black)
***************************************************************************/
WRITE8_HANDLER( segar_colortable_w )
{
	static unsigned char red[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char grn[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char blu[] = {0x00, 0x55, 0xAA, 0xFF };

	if (sv.color_write_enable)
	{
		int r,g,b;

		b = blu[(data & 0xC0) >> 6];
		g = grn[(data & 0x38) >> 3];
		r = red[(data & 0x07)];

		palette_set_color(offset+1,r,g,b);

		if (data == 0)
			Machine->gfx[0]->colortable[offset] = Machine->pens[0];
		else
			Machine->gfx[0]->colortable[offset] = Machine->pens[offset+1];

		// refresh the screen if the color switched to or from black
		if (sv.colorRAM[offset] != data)
		{
			if ((sv.colorRAM[offset] == 0) || (data == 0))
			{
				sv.refresh = 1;
			}
		}

		sv.colorRAM[offset] = data;
	}
	else
	{
		logerror("color %02X:%02X (write=%d)\n",offset,data,sv.color_write_enable);
		segar_mem_colortable[offset] = data;
	}
}

WRITE8_HANDLER( segar_bcolortable_w )
{
	static unsigned char red[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char grn[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char blu[] = {0x00, 0x55, 0xAA, 0xFF };

	int r,g,b;

	if (sv.has_bcolorRAM)
	{
		sv.bcolorRAM[offset] = data;

		b = blu[(data & 0xC0) >> 6];
		g = grn[(data & 0x38) >> 3];
		r = red[(data & 0x07)];

		palette_set_color(offset+0x40+1,r,g,b);
	}

	// Needed to pass the self-tests
	segar_mem_bcolortable[offset] = data;
}





/* for set 2 */
/* other ports are also used */
static UINT8 monster2_bbdata;
static UINT8 monster2_b9data;

WRITE8_HANDLER( monster2_bb_back_port_w )
{
	unsigned int temp_scene, temp_charset;

	/* maybe */
	monster2_bbdata=data;

	temp_scene   = (monster2_b9data & 0x03)|monster2_bbdata<<2;
	temp_charset = monster2_b9data & 0x03;

	temp_scene = 0x400*temp_scene;

	sv.back_scene = temp_scene;
	sv.refresh=1;

	sv.back_charset = temp_charset;
	sv.refresh=1;

//  printf("bb_data %02x\n",data);
}

WRITE8_HANDLER( monster2_b9_back_port_w )
{
	unsigned int temp_scene, temp_charset;

	monster2_b9data = data;


	temp_scene   = (monster2_b9data & 0x03)|monster2_bbdata<<2;
	temp_charset = monster2_b9data & 0x03;

	temp_scene = 0x400*temp_scene;

//  printf("data %02x\n",data);

	if (sv.back_scene != temp_scene)
	{
		sv.back_scene = temp_scene;
		sv.refresh=1;
	}
	if (sv.back_charset != temp_charset)
	{
		sv.back_charset = temp_charset;
		sv.refresh=1;
	}

	/* This bit turns the background off and on. */
	if ((data & 0x80) && (sv.background_enable==0))
	{
		sv.background_enable=1;
		sv.refresh=1;
	}
	else if (((data & 0x80)==0) && (sv.background_enable==1))
	{
		sv.background_enable=0;
		sv.refresh=1;
	}
}



/***************************************************************************
 ---------------------------------------------------------------------------
 Sinbad Mystery Functions
 ---------------------------------------------------------------------------
***************************************************************************/

/***************************************************************************
Controls the background image
***************************************************************************/

WRITE8_HANDLER( sindbadm_back_port_w )
{
	unsigned int tempscene;

	/* Bit D7 turns the background off and on? */
	if ((data & 0x80) && (sv.background_enable==0))
	{
		sv.background_enable=1;
		sv.refresh=1;
	}
	else if (((data & 0x80)==0) && (sv.background_enable==1))
	{
		sv.background_enable=0;
		sv.refresh=1;
	}
	/* Bits D2-D6 select the background? */
	tempscene = (data >> 2) & 0x1F;
	if (sv.back_scene != tempscene)
	{
		sv.back_scene = tempscene;
		sv.refresh=1;
	}
	/* Bits D0-D1 select the background char set? */
	if (sv.back_charset != (data & 0x03))
	{
		sv.back_charset = data & 0x03;
		sv.refresh=1;
	}
}

/***************************************************************************
Special refresh for Sinbad Mystery, this code refreshes the static background.
***************************************************************************/

VIDEO_UPDATE( sindbadm )
{
	int offs;
	int charcode;
	int sprite_transparency;
	unsigned long backoffs;
	unsigned long back_scene;

	unsigned char *back_charmap = memory_region(REGION_USER1);

	if (get_vh_global_attribute_changed())
		sv.refresh = 1;

	sprite_transparency=TRANSPARENCY_NONE;

	/* If the background is turned on, refresh it first. */
	if (sv.background_enable)
	{
		/* for every character in the Video RAM, check if it has been modified */
		/* since last time and update it accordingly. */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if ((sv.char_refresh) || (sv.dirtychar[videoram[offs]]))
				dirtybuffer[offs]=1;

			/* Redraw every background character if our palette or scene changed */
			if ((dirtybuffer[offs]) || sv.refresh)
			{
				int sx,sy;

				sx = 8 * (offs % 32);
				sy = 8 * (offs / 32);

				if (sv.flip)
				{
					sx = 31*8 - sx;
					sy = 27*8 - sy;
				}

				// NOTE: Pig Newton has 16 backgrounds, Sinbad Mystery has 32
				back_scene = (sv.back_scene & 0x1C) << 10;

				backoffs = (offs & 0x01F) + ((offs & 0x3E0) << 2) + ((sv.back_scene & 0x03) << 5);

				charcode = back_charmap[backoffs + back_scene];

				drawgfx(tmpbitmap,Machine->gfx[1 + sv.back_charset],
					charcode,((charcode & 0xF0)>>4),
					sv.flip,sv.flip,sx,sy,
					&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
			}
		}
		sprite_transparency=TRANSPARENCY_PEN;
	}

	/* Refresh the "standard" graphics */
//  segar_common_screenrefresh(bitmap, sprite_transparency, TRANSPARENCY_NONE);

	return 0;
}

