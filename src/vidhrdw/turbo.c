/*************************************************************************

    Sega Z80-3D system

*************************************************************************/

#include "driver.h"
#include "turbo.h"
#include "res_net.h"


/* constants */
#define VIEW_WIDTH					(32*8)
#define VIEW_HEIGHT					(28*8)
#define END_OF_ROW_VALUE			0x12345678

/* globals definitions */
UINT8 *sega_sprite_position;
UINT8 turbo_collision;

/* internal data */
static tilemap *fg_tilemap;
static UINT8 *sprite_priority, *sprite_expanded_priority;
static UINT8 *fore_palette, *fore_priority;
static UINT8 *back_data;

/* sprite tracking */
struct sprite_params_data
{
	UINT32 *base;
	UINT8 *enable;
	int offset, rowbytes;
	int yscale, miny, maxy;
	int xscale, xoffs;
	int flip;
};
static struct sprite_params_data sprite_params[16];
static UINT32 *sprite_expanded_data;
static UINT8 *sprite_expanded_enable;

/* misc other stuff */
static UINT8 *buckrog_bitmap_ram;
static UINT8 drew_frame;
static UINT32 sprite_mask;



/*************************************
 *
 *  Palette conversion
 *
 *************************************/

PALETTE_INIT( turbo )
{
	static const int resistances[3] = { 1000, 470, 220 };
	double rweights[3], gweights[3], bweights[2];
	int i;

	/* compute the color output resistor weights */
	compute_resistor_weights(0,	255, -1.0,
			3,	&resistances[0], rweights, 470, 0,
			3,	&resistances[0], gweights, 470, 0,
			2,	&resistances[1], bweights, 470, 0);

	/* initialize the palette with these colors */
	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = combine_3_weights(rweights, bit0, bit1, bit2);

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = combine_3_weights(gweights, bit0, bit1, bit2);

		/* blue component */
		bit0 = (*color_prom >> 6) & 1;
		bit1 = (*color_prom >> 7) & 1;
		b = combine_2_weights(bweights, bit0, bit1);

		palette_set_color(i, r, g, b);
	}
}


PALETTE_INIT( subroc3d )
{
	int i;

	/* Subroc3D uses a common final color PROM with 512 entries */
	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 1;
		bit2 = (*color_prom >> 7) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}


PALETTE_INIT( buckrog )
{
	int i;

	/* Buck Rogers uses 1024 entries for the sprite color PROM */
	for (i = 0; i < 1024; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 1;
		bit2 = (*color_prom >> 7) & 1;
		b = 34 * bit0 + 68 * bit1 + 137 * bit2;

		palette_set_color(i,r,g,b);
	}

	/* then another 512 entries for the character color PROM */
	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 =
		bit1 = (*color_prom >> 0) & 1;
		bit2 = (*color_prom >> 1) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 =
		bit1 = (*color_prom >> 2) & 1;
		bit2 = (*color_prom >> 3) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		b = 34 * bit0 + 68 * bit1 + 137 * bit2;

		palette_set_color(i+1024,r,g,b);
	}

	/* finally, the gradient foreground gets its own set of 256 colors */
	for (i = 0; i < 256; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		/* red component */
		bit0 = 0;
		bit1 = 0;
		bit2 = (i >> 0) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 = (i >> 1) & 1;
		bit1 = (i >> 2) & 1;
		bit2 = (i >> 3) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = (i >> 4) & 1;
		bit1 = (i >> 5) & 1;
		bit2 = (i >> 6) & 1;
		bit3 = (i >> 7) & 1;
		b = 16 * bit0 + 34 * bit1 + 68 * bit2 + 137 * bit3;

		palette_set_color(i+1024+512,r,g,b);
	}
}



/***************************************************************************

    Sprite startup/shutdown

***************************************************************************/

static int init_sprites(UINT32 sprite_expand[16], UINT8 sprite_enable[16], int expand_shift)
{
	UINT8 *sprite_gfxdata = memory_region(REGION_GFX1);
	int sprite_length = memory_region_length(REGION_GFX1);
	int sprite_bank_size = sprite_length / 8;
	UINT8 *src, *edst;
	UINT32 *dst;
	int i, j;

	/* allocate the expanded sprite data */
	sprite_expanded_data = auto_malloc(sprite_length * 2 * sizeof(UINT32));

	/* allocate the expanded sprite enable array */
	sprite_expanded_enable = auto_malloc(sprite_length * 2 * sizeof(UINT8));

	/* expand the sprite ROMs */
	src = sprite_gfxdata;
	dst = sprite_expanded_data;
	edst = sprite_expanded_enable;
	for (i = 0; i < 8; i++)
	{
		/* expand this bank */
		for (j = 0; j < sprite_bank_size; j++)
		{
			int bits = *src++;
			*dst++ = sprite_expand[bits >> 4];
			*dst++ = sprite_expand[bits & 15];
			*edst++ = sprite_enable[bits >> 4];
			*edst++ = sprite_enable[bits & 15];
		}

		/* shift for the next bank */
		for (j = 0; j < 16; j++)
		{
			if (sprite_expand[j] != END_OF_ROW_VALUE)
				sprite_expand[j] <<= expand_shift;
			sprite_enable[j] <<= 1;
		}
	}

	/* success */
	return 0;
}



/***************************************************************************

    Video startup

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	SET_TILE_INFO(0, code, code >> 2, 0)
}


VIDEO_START( turbo )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	int i;


	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		UINT32 value = 0;
		if (i & 1) value |= 0x00000001;
		if (i & 2) value |= 0x00000100;
		if (i & 4) value |= 0x00010000;

		/* special value for the end-of-row */
		if ((i & 0x0c) == 0x04) value = END_OF_ROW_VALUE;

		sprite_expand[i] = value;
		sprite_enable[i] = (i >> 3) & 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 1))
		return 1;

	/* initialize the foreground tilemap */
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 32,32);

	/* other stuff */
	drew_frame = 0;
	sprite_mask = 0x7fff;

	/* return success */
	return 0;
}


VIDEO_START( subroc3d )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	int i;

	/* determine ROM/PROM addresses */
	sprite_priority = memory_region(REGION_PROMS) + 0x0500;
	fore_palette = memory_region(REGION_PROMS) + 0x0200;

	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		sprite_expand[i] = (i == 0x03 || i == 0x0f) ? END_OF_ROW_VALUE : i;
		sprite_enable[i] = (i == 0x00 || i == 0x03 || i == 0x0c || i == 0x0f) ? 0 : 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 4))
		return 1;

	/* initialize the foreground tilemap */
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 32,32);

	/* allocate the expanded sprite priority map */
	sprite_expanded_priority = auto_malloc(1 << 12);

	/* expand the sprite priority map */
	for (i = 0; i < (1 << 12); i++)
	{
		int plb = ~i & 0xff;
		int ply = i >> 8;
		int byteval = sprite_priority[(plb | ((ply & 0x0e) << 7)) & 0x1ff];
		sprite_expanded_priority[i] = (ply & 1) ? (byteval >> 4) : (byteval & 0x0f);
		sprite_expanded_priority[i] *= 4;
	}

	/* other stuff */
	sprite_mask = 0xffff;

	/* return success */
	return 0;
}


VIDEO_START( buckrog )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	int i;

	/* determine ROM/PROM addresses */
	fore_priority = memory_region(REGION_PROMS) + 0x400;
	back_data = memory_region(REGION_GFX3);

	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		sprite_expand[i] = (i == 0x0f) ? END_OF_ROW_VALUE : i;
		sprite_enable[i] = (i == 0x00 || i == 0x0f) ? 0 : 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 4))
		return 1;

	/* initialize the foreground tilemap */
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 32,32);

	/* allocate the expanded sprite priority map */
	sprite_expanded_priority = auto_malloc(1 << 8);

	/* expand the sprite priority map */
	for (i = 0; i < (1 << 8); i++)
	{
		if (i & 0x01) sprite_expanded_priority[i] = 0 | 8;
		else if (i & 0x02) sprite_expanded_priority[i] = 1 | 8;
		else if (i & 0x04) sprite_expanded_priority[i] = 2 | 8;
		else if (i & 0x08) sprite_expanded_priority[i] = 3 | 8;
		else if (i & 0x10) sprite_expanded_priority[i] = 4 | 8;
		else if (i & 0x20) sprite_expanded_priority[i] = 5 | 8;
		else if (i & 0x40) sprite_expanded_priority[i] = 6 | 8;
		else if (i & 0x80) sprite_expanded_priority[i] = 7 | 8;
		else sprite_expanded_priority[i] = 0;
		sprite_expanded_priority[i] *= 4;
	}

	/* allocate the bitmap RAM */
	buckrog_bitmap_ram = auto_malloc(0xe000);

	/* other stuff */
	sprite_mask = 0xffff;

	/* return success */
	return 0;
}



WRITE8_HANDLER( turbo_videoram_w )
{
	videoram[offset] = data;
	if (offset < 0x400)
		tilemap_mark_tile_dirty(fg_tilemap, offset);
}



/***************************************************************************

    Sprite data gathering

***************************************************************************/

static void turbo_update_sprite_info(void)
{
	struct sprite_params_data *data = sprite_params;
	int i;

/*
    Sprite RAM processing:

    Sprite RAM is 64x16-bit words. One word is clocked per H clock during the HBLANK
    period (256-319), allowing all 64 sprites to be processed for the following line.


    Bytes 0+1:

        VPOS + (~data[0]) -> AL0-AL7, /carry to LS164 shift reg A
        VPOS + (~data[1]) -> AL8-AL15, carry to LS164 shift reg B

        - The A and B inputs to the shift register are ANDed
        - The output from the shift register becomes the VLn (vertical enable) signal
        - The VLn and HEn (horizontal enable) signals are ANDed to form the LEn (line enable) signal


    Bytes 2+3:

        The output of the ALU above is still on the AL0-15 lines when the X/Y scale
        values are clocked. The lower result (VPOS + (~data[0])) is combined with
        the low 5 bits of the Y scale and a single bit is extracted from the lookup
        PROM at IC50, which outputs as the W-LINE signal. I suspect this controls
        whether or not to add the rowbytes to the offset for this scanline.


    Bytes 4+5:

        This is a 16-bit bytes-per-row value.


    Bytes 6+7:

        This is a 16-bit offset. Since the logic is such that writes to sprite RAM can
        occur during execution, I suspect this value is incremented each row as
        appropriate and written back.


    Sprite ROM mapping:
        J/K flip flop controls state (0=off, 1=on)
            - held clear during blank
            - set if (LSTn and ROAD) [ROAD only matters for sprites 3-7]
            - clocked clear if (PLBn=0 & CDBn=1) [end of row marker]

        4xLS191 up/down counters control the address (CW0-15)
            - loaded on /ADLn signal from the AL0-AL15 lines
            - up/down direction controlled by high bit (CW15: 0=up, 1=down)
            - enabled if /CWEn=0 and J/K flip flop is on (1)
            - clocked on CLKn signal

        CW1-13 goes to pair of ROMs, A0-A12
        CW14 selects which of the pair to load from
        CW0 triggers a load from ROM every other pixel

        ROM outputs PD0-PD7 go to a pair of LS195 shift registers
            - clocked on CLKn signal
            - cleared on output from J/K flip flop
*/


	/* first loop over all sprites and update those whose scanlines intersect ours */
	for (i = 0; i < 16; i++, data++)
	{
		UINT8 *sprite_base = spriteram + 16 * i;

		/* snarf all the data */
		data->base = sprite_expanded_data + (i & 7) * 0x8000;
		data->enable = sprite_expanded_enable + (i & 7) * 0x8000;
		data->offset = (sprite_base[6] + 256 * sprite_base[7]) & sprite_mask;
		data->rowbytes = (INT16)(sprite_base[4] + 256 * sprite_base[5]);
		data->miny = sprite_base[0];
		data->maxy = sprite_base[1];
		data->xscale = ((5 * 256 - 4 * sprite_base[2]) << 16) / (5 * 256);
		data->yscale = (4 << 16) / ((sprite_base[3] & 0x1f) + 4);
		data->xoffs = -1;
		data->flip = 0;
	}

	/* now find the X positions */
	for (i = 0; i < 0x200; i++)
	{
		int value = sega_sprite_position[i];
		if (value)
		{
			int base = (i & 0x100) >> 5;
			int which;
			for (which = 0; which < 8; which++)
				if (value & (1 << which))
					sprite_params[base + which].xoffs = i & 0xff;
		}
	}
}


static void subroc3d_update_sprite_info(void)
{
	struct sprite_params_data *data = sprite_params;
	int i;

	/* first loop over all sprites and update those whose scanlines intersect ours */
	for (i = 0; i < 16; i++, data++)
	{
		UINT8 *sprite_base = spriteram + 8 * i;

		/* snarf all the data */
		data->base = sprite_expanded_data + (i & 7) * 0x10000;
		data->enable = sprite_expanded_enable + (i & 7) * 0x10000;
		data->offset = ((sprite_base[6] + 256 * sprite_base[7]) * 2) & sprite_mask;
		data->rowbytes = (INT16)(sprite_base[4] + 256 * sprite_base[5]) * 2;
		data->miny = sprite_base[0] ^ 0xff;
		data->maxy = (sprite_base[1] ^ 0xff) - 1;
		data->xscale = 65536.0 * (1.0 - 0.004 * (double)(sprite_base[2] - 0x40));
		data->yscale = (4 << 16) / (sprite_base[3] + 4);
		data->xoffs = -1;
		data->flip = sprite_base[7]>>7;
	}

	/* now find the X positions */
	for (i = 0; i < 0x200; i++)
	{
		int value = sega_sprite_position[i];
		if (value)
		{
			int base = (i & 0x01) << 3;
			int which;
			for (which = 0; which < 8; which++)
				if (value & (1 << which))
					sprite_params[base + which].xoffs = ((i & 0x1fe) >> 1);
		}
	}
}



/***************************************************************************

    Sprite rendering

***************************************************************************/

static void draw_one_sprite(const struct sprite_params_data *data, UINT32 *dest, UINT8 *edest, int xclip, int scanline)
{
	int xstep = data->flip ? -data->xscale : data->xscale;
	int xoffs = data->xoffs;
	UINT32 xcurr;
	UINT32 *src;
	UINT8 *esrc;
	int offset;

	/* xoffs of -1 means don't draw */
	if (xoffs == -1 || data->xscale <= 0) return;

	/* compute the current data offset */
	scanline = ((scanline - data->miny) * data->yscale) >> 16;
	offset = data->offset + (scanline + 1) * data->rowbytes;

	/* clip to the road */
	xcurr = offset << 16;
	if (xoffs < xclip)
	{
		/* the pixel clock starts on xoffs regardless of clipping; take this into account */
		xcurr += ((xclip - xoffs) * xstep) & 0xffff;
		xoffs = xclip;
	}

	/* determine the bitmap location */
	src = data->base;
	esrc = data->enable;

	{
		int xdir = (xstep < 0) ? -1 : 1;

		/* loop over columns */
		while (xoffs < VIEW_WIDTH)
		{
			int xint = (xcurr >> 16) & sprite_mask, newxint;
			UINT32 srcval = src[xint];
			UINT8 srcenable = esrc[xint];

			/* stop on the end-of-row signal */
			if (srcval == END_OF_ROW_VALUE)
				break;

			/* OR in the bits from this pixel */
			dest[xoffs] |= srcval;
			edest[xoffs++] |= srcenable;
			xcurr += xstep;

			/* make sure we don't hit any end of rows along the way */
			newxint = (xcurr >> 16) & sprite_mask;
			for ( ; xint != newxint; xint = (xint + xdir) & sprite_mask)
				if (src[xint] == END_OF_ROW_VALUE)
					break;
		}
	}
}


static void draw_sprites(UINT32 *dest, UINT8 *edest, int scanline, UINT8 mask, int xclip)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		const struct sprite_params_data *data;

		/* check the mask */
		if (mask & (1 << i))
		{
			/* if the sprite intersects this scanline, draw it */
			data = &sprite_params[i];
			if (scanline >= data->miny && scanline < data->maxy)
				draw_one_sprite(data, dest, edest, xclip, scanline);

			/* if the sprite intersects this scanline, draw it */
			data = &sprite_params[8 + i];
			if (scanline >= data->miny && scanline < data->maxy)
				draw_one_sprite(data, dest, edest, xclip, scanline);
		}
	}
}



/***************************************************************************

    Main refresh

***************************************************************************/

#if 0
static void turbo_prepare_sprites(UINT8 y)
{
	const UINT8 *pr1119 = memory_region(REGION_PROMS) + 0xc00;
	UINT16 sproffs[8];
	UINT8 vl = 0;
	int sprnum;

	/* compute the sprite information, which was done on the previous scanline during HBLANK */
	for (sprnum = 0; sprnum < 8; sprnum++)
	{
		UINT8 *rambase = &spriteram[sprnum * 0x10];
		UINT32 sum, clo, chi;

		/* perform the first ALU to see if we are within the scanline */
		sum = (y | (y << 8)) + (rambase[0] + (rambase[1] << 8));
		clo = ((sum >> 8) ^ y) & 1;
		chi = sum >> 16;
		vl |= (clo & (chi ^ 1)) << sprnum;

		/* load the offset */
		if (vl & (1 << sprnum))
		{
			int xscale = rambase[2];
			int yscale = rambase[3];
			sproffs[sprnum] = rambase[6] + (rambase[7] << 8);

			/* look up the low byte of the sum plus the yscale value in */
			/* IC50/PR1119 to determine if we write back the sum of the */
			/* offset and the rowbytes this scanline (p. 138) */
			offs = (sum & 0xff) |			/* A0-A7 = AL0-AL7 */
				   ((~yscale >> 3) & 3);	/* A8-A9 = /RO11-/RO12 */

			/* one of the bits is selected based on the low 7 bits of yscale */
			if ((pr1119[offs] >> (~yscale & 7)) & 1)
			{
				UINT16 temp = sproffs[sprnum] + rambase[4] + (rambase[5] << 8);
				rambase[6] = temp;
				rambase[7] = temp >> 8;
			}
		}
	}
}
#endif

VIDEO_UPDATE( turbo )
{
	mame_bitmap *fgpixmap = tilemap_get_pixmap(fg_tilemap);
	const UINT8 *prom_base = memory_region(REGION_PROMS);
	const UINT8 *pr1122 = prom_base + 0x200;
	const UINT8 *pr1123 = prom_base + 0x600;
	const UINT8 *pr1118 = prom_base + 0xa00;
	const UINT8 *pr1114 = prom_base + 0xb00;
	const UINT8 *pr1117 = prom_base + 0xb20;
	const UINT8 *pr1115 = prom_base + 0xb40;
	const UINT8 *pr1116 = prom_base + 0xb60;
	const UINT8 *road_gfxdata = memory_region(REGION_GFX3);
	int x, y;

	/* suck up the sprite parameter data */
	turbo_update_sprite_info();

	/* loop over rows */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (bitmap != NULL) ? ((UINT16 *)bitmap->base + y * bitmap->rowpixels) : NULL;
		UINT16 *fore = (UINT16 *)fgpixmap->base + y * fgpixmap->rowpixels;
		int sel, coch, babit, slipar_acciar, area, offs, areatmp, road = 0;
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];

		/* compute the Y sum between opa and the current scanline (p. 141) */
		int va = (y + turbo_opa) & 0xff;

		/* the upper bit of OPC inverts the road (p. 141) */
		if (!(turbo_opc & 0x80)) va ^= 0xff;

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0x07, 0);

		/* loop over columns */
		for (x = 0; x <= cliprect->max_x; x++)
		{
			int bacol, red, grn, blu, priority, foreraw, forebits, mx;
			UINT32 sprite = sprite_buffer[x];
			UINT8 plb = sprite_enable[x];

			/* compute the X sum between opb and the current column; only the carry matters (p. 141) */
			int carry = (x + turbo_opb) >> 8;

			/* the carry selects which inputs to use (p. 141) */
			if (carry)
			{
				sel	 = turbo_ipb;
				coch = turbo_ipc >> 4;
			}
			else
			{
				sel	 = turbo_ipa;
				coch = turbo_ipc & 15;
			}

			/* look up AREA1 and AREA2 (p. 142) */
			offs = va |							/*  A0- A7 = VA0-VA7 */
				   ((sel & 0x0f) << 8);			/*  A8-A11 = SEL0-3 */

			areatmp = road_gfxdata[0x0000 | offs];
			areatmp = ((areatmp + x) >> 8) & 0x01;
			area = areatmp << 0;

			areatmp = road_gfxdata[0x1000 | offs];
			areatmp = ((areatmp + x) >> 8) & 0x01;
			area |= areatmp << 1;

			/* look up AREA3 and AREA4 (p. 142) */
			offs = va |							/*  A0- A7 = VA0-VA7 */
				   ((sel & 0xf0) << 4);			/*  A8-A11 = SEL4-7 */

			areatmp = road_gfxdata[0x2000 | offs];
			areatmp = ((areatmp + x) >> 8) & 0x01;
			area |= areatmp << 2;

			areatmp = road_gfxdata[0x3000 | offs];
			areatmp = ((areatmp + x) >> 8) & 0x01;
			area |= areatmp << 3;

			/* look up AREA5 (p. 141) */
			offs = (x >> 3) |					/*  A0- A4 = H3-H7 */
				   ((turbo_opc & 0x3f) << 5);	/*  A5-A10 = OPC0-5 */

			areatmp = road_gfxdata[0x4000 | offs];
			areatmp = (areatmp << (x & 7)) & 0x80;
			area |= areatmp >> 3;

			/* compute the final area value and look it up in IC18/PR1115 (p. 144) */
			babit = pr1115[area] & 0x07;

			/* note: SLIPAR is 0 on the road surface only */
			/*       ACCIAR is 0 on the road surface and the striped edges only */
			slipar_acciar = pr1115[area] & 0x30;
			if (!road && (slipar_acciar & 0x20))
			{
				road = 1;
				draw_sprites(sprite_buffer, sprite_enable, y, 0xf8, x + 2);
			}

			/* perform collision detection here via lookup in IC20/PR1116 (p. 144) */
			turbo_collision |= pr1116[(plb & 7) | (slipar_acciar >> 1)];

			/* also use the coch value to look up color info in IC13/PR1114 and IC21/PR1117 (p. 144) */
			offs = (coch & 0x0f) |					/* A0-A3: CONT0-3 = COCH0-3 */
				   ((turbo_fbcol & 0x01) << 4);		/*    A4: COL0 */
			bacol = pr1114[offs] | (pr1117[offs] << 8);

			/* at this point, do the character lookup; due to the shift register loading in */
			/* the sync PROM, we latch character 0 during pixel 6 and start clocking in pixel */
			/* 8, effectively shifting the display by 8; at pixel 0x108, the color latch is */
			/* forced clear and isn't touched until the next shift register load */
			foreraw = (x < 8 || x >= 0x108) ? 0 : fore[x - 8];

			/* perform the foreground color table lookup in IC99/PR1118 (p. 137) */
			forebits = pr1118[foreraw];

			/* look up the sprite priority in IC11/PR1122 (p. 144) */
			priority = ((plb & 0xfe) >> 1) |		/* A0-A6: PLB1-7 */
					   ((turbo_fbpla & 0x07) << 7);	/* A7-A9: PLA0-2 */
			priority = pr1122[priority];

			/* use that to look up the overall priority in IC12/PR1123 (p. 144) */
			mx = (priority & 7) | 					/* A0-A2: PR-1122 output, bits 0-2 */
				 ((plb & 0x01) << 3) | 				/*    A3: PLB0 */
				 ((foreraw & 0x80) >> 3) | 			/*    A4: PLBE */
				 ((forebits & 0x08) << 2) | 		/*    A5: PLBF */
				 (babit << 6) |						/* A6-A8: BABIT1-3 */
				 ((turbo_fbpla & 0x08) << 6);		/*    A9: PLA3 */
			mx = pr1123[mx];

			/* the MX output selects one of 16 inputs; build up a 16-bit pattern to match */
			/* these in red, green, and blue (p. 144) */
			red = ((sprite & 0x0000ff) >> 0) |		/*  D0- D7: CDR0-CDR7 */
				  ((forebits & 0x01) << 8) |		/*      D8: CDRF */
				  ((bacol & 0x001f) << 9) |			/*  D9-D13: BAR0-BAR4 */
				  (1 << 14) |						/*     D14: 1 */
				  (0 << 15);						/*     D15: 0 */

			grn = ((sprite & 0x00ff00) >> 8) |		/*  D0- D7: CDG0-CDG7 */
				  ((forebits & 0x02) << 7) |		/*      D8: CDGF */
				  ((bacol & 0x03e0) << 4) |			/*  D9-D13: BAG0-BAG4 */
				  (1 << 14) |						/*     D14: 1 */
				  (0 << 15);						/*     D15: 0 */

			blu = ((sprite & 0xff0000) >> 16) |		/*  D0- D7: CDB0-CDB7 */
				  ((forebits & 0x04) << 6) |		/*      D8: CDBF */
				  ((bacol & 0x7c00) >> 1) |			/*  D9-D13: BAB0-BAB4 */
				  (1 << 14) |						/*     D14: 1 */
				  (0 << 15);						/*     D15: 0 */

			/* we then go through a muxer to select one of the 16 outputs computed above (p. 144) */
			dest[x] = mx |							/* A0-A3: MX0-MX3 */
					  (((~red >> mx) & 1) << 4) |	/*    A4: CDR */
					  (((~grn >> mx) & 1) << 5) |	/*    A5: CDG */
					  (((~blu >> mx) & 1) << 6) |	/*    A6: CDB */
					  ((turbo_fbcol & 6) << 6);		/* A7-A8: COL1-2 */
		}
	}

	/* draw the LEDs for the scores */
	turbo_update_segments();
	return 0;
}


VIDEO_UPDATE( subroc3d )
{
	UINT8 *sprite_priority_base = &sprite_expanded_priority[(subroc3d_ply & 15) << 8];
	mame_bitmap *fgpixmap = tilemap_get_pixmap(fg_tilemap);
	int y;

	/* suck up the sprite parameter data */
	subroc3d_update_sprite_info();

	/* loop over rows */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (bitmap != NULL) ? ((UINT16 *)bitmap->base + y * bitmap->rowpixels) : NULL;
		UINT16 *fore = (UINT16 *)fgpixmap->base + y * fgpixmap->rowpixels;
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];
		int x;

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0xff, 0);

		/* loop over columns */
		for (x = 0; x <= cliprect->max_x; x++)
		{
			int bits, foreraw, forebits, mux, mplb;

			/* at this point, do the character lookup */
			foreraw = fore[x];
			forebits = fore_palette[foreraw] & 0x0f;

			/* determine the value of mplb */
			mplb = (forebits == 0 || (forebits & 0x80));

			/* look up the sprite priority in IC11/PR1122 */
			mux = mplb ? sprite_priority_base[sprite_enable[x]] : 0;

			/* mux3 selects either sprite or foreground */
			if (mux & 0x20)
				bits = (sprite_buffer[x] >> (mux & 0x1c)) & 0x0f;
			else
				bits = forebits;

			dest[x] = ((subroc3d_col & 15) << 5) | ((mux & 0x20) >> 1) | bits;
		}
	}

	/* draw the LEDs for the scores */
	turbo_update_segments();
	return 0;
}


VIDEO_UPDATE( buckrog )
{
	mame_bitmap *fgpixmap = tilemap_get_pixmap(fg_tilemap);
	int y;

	/* suck up the sprite parameter data */
	subroc3d_update_sprite_info();

	/* loop over rows */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (bitmap != NULL) ? ((UINT16 *)bitmap->base + y * bitmap->rowpixels) : NULL;
		UINT16 *fore = (UINT16 *)fgpixmap->base + y * fgpixmap->rowpixels;
		UINT8 *stars = &buckrog_bitmap_ram[y * 256];
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];
		int bgcolor;
		int x;

		/* determine background color for this scanline */
		bgcolor = 1024 | 512 | back_data[(buckrog_mov << 8) | y];

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0xff, 0);

		/* loop over columns */
		for (x = 0; x <= cliprect->max_x; x++)
		{
			int bits, foreraw, forebits, forepri, mux;

			/* at this point, do the character lookup */
			foreraw = fore[x];
			forebits = (foreraw & 3) | ((foreraw >> 1) & 0x7c) | ((buckrog_fchg << 7) & 0x180);

			/* look up the foreground priority */
			forepri = fore_priority[forebits];

			/* look up the sprite priority in IC11/PR1122 */
			mux = sprite_expanded_priority[sprite_enable[x]];

			/* final result is based on sprite/foreground/star priorities */
			if (!(forepri & 0x80))
				bits = 1024 | forebits;
			else if (mux & 0x20)
				bits = (buckrog_obch << 7) | ((mux & 0x1c) << 2) | ((sprite_buffer[x] >> (mux & 0x1c)) & 0x0f);
			else if (!(forepri & 0x40))
				bits = 1024 | forebits;
			else if (stars[x])
				bits = 1024 | 512 | 255;
			else
				bits = bgcolor;

			dest[x] = bits;
		}
	}

	/* draw the LEDs for the scores */
	turbo_update_segments();
	return 0;
}



/***************************************************************************

    Buck Rogers misc

***************************************************************************/

WRITE8_HANDLER( buckrog_bitmap_w )
{
	buckrog_bitmap_ram[offset] = data & 1;
}
