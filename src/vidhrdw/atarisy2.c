/***************************************************************************

	Atari System 2 hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "slapstic.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *atarisys2_slapstic;



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t latched_vscroll;
static UINT32 bankbits;
static int videobank;
static data16_t *vram;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int atarisys2_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		128,64,		/* size of the playfield in tiles (x,y) */
		1,128,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x80,		/* index of palette base */
		0x80,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0xff07ff,	/* tile data index mask */
		0x003800,	/* tile data color mask */
		0,			/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0x00c000	/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x00,				/* base palette entry */
		0x40,				/* maximum number of colors */
		15,					/* transparent pen index */

		{{ 0,0,0,0x07f8 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x07ff,0,0 }},	/* mask for the code index */
		{{ 0x0007,0,0,0 }},	/* mask for the upper code index */
		{{ 0,0,0,0x3000 }},	/* mask for the color */
		{{ 0,0,0xffc0,0 }},	/* mask for the X position */
		{{ 0x7fc0,0,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0x3800,0,0 }},	/* mask for the height, in tiles */
		{{ 0,0x4000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0,0xc000 }},	/* mask for the priority */
		{{ 0,0x8000,0,0 }},	/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		2,			/* index to which gfx system */
		64,64,		/* size of the alpha RAM in tiles (x,y) */

		0x40,		/* index of palette base */
		0x40,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x03ff,		/* tile data index mask */
		0xe000,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0			/* tile data opacity mask */
	};

	UINT32 *pflookup;
	int i, size;

	/* allocate banked memory */
	vram = calloc(0x8000, 1);
	if (!vram)
		goto cant_allocate_ram;
	atarian_0_base = &vram[0x0000];
	atarimo_0_spriteram = &vram[0x0c00];
	ataripf_0_base = &vram[0x2000];

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;

	/* modify the playfield lookup table to support our odd banking system */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int code = i << ATARIPF_LOOKUP_DATABITS;
		int bankselect = (code >> 10) & 1;
		int bank = (code >> (16 + 4 * bankselect)) & 15;

		code = (code & 0x3ff) | (bank << 10);
		ATARIPF_LOOKUP_SET_CODE(pflookup[i], code);
	}

	/* reset the statics */
	bankbits = 0;
	videobank = 0;
	return 0;

	/* error cases */
cant_create_mo:
	ataripf_free();
cant_create_pf:
	atarian_free();
cant_create_an:
	free(vram);
cant_allocate_ram:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void atarisys2_vh_stop(void)
{
	free(vram);
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Scroll/playfield bank write
 *
 *************************************/

WRITE16_HANDLER( atarisys2_hscroll_w )
{
	int scanline = cpu_getscanline() + 1;
	int newscroll = (ataripf_get_xscroll(0) << 6) | ((bankbits >> 16) & 0xf);
	COMBINE_DATA(&newscroll);

	/* update the playfield parameters - hscroll is clocked on the following scanline */
	ataripf_set_xscroll(0, (newscroll >> 6) & 0x03ff, scanline);
	bankbits = (bankbits & 0xf00000) | ((newscroll & 0xf) << 16);
	ataripf_set_bankbits(0, bankbits, scanline);
}


WRITE16_HANDLER( atarisys2_vscroll_w )
{
	int scanline = cpu_getscanline() + 1;
	int newscroll = (latched_vscroll << 6) | ((bankbits >> 20) & 0xf);
	COMBINE_DATA(&newscroll);

	/* if bit 4 is zero, the scroll value is clocked in right away */
	latched_vscroll = (newscroll >> 6) & 0x01ff;
	if (!(newscroll & 0x10)) ataripf_set_yscroll(0, latched_vscroll, scanline);

	/* update the playfield parameters */
	bankbits = (bankbits & 0x0f0000) | ((newscroll & 0xf) << 20);
	ataripf_set_bankbits(0, bankbits, scanline);
}



/*************************************
 *
 *	Palette RAM write handler
 *
 *************************************/

WRITE16_HANDLER( atarisys2_paletteram_w )
{
	static const int intensity_table[16] =
	{
		#define ZB 115
		#define Z3 78
		#define Z2 37
		#define Z1 17
		#define Z0 9
		0, ZB+Z0, ZB+Z1, ZB+Z1+Z0, ZB+Z2, ZB+Z2+Z0, ZB+Z2+Z1, ZB+Z2+Z1+Z0,
		ZB+Z3, ZB+Z3+Z0, ZB+Z3+Z1, ZB+Z3+Z1+Z0,ZB+ Z3+Z2, ZB+Z3+Z2+Z0, ZB+Z3+Z2+Z1, ZB+Z3+Z2+Z1+Z0
	};
	static const int color_table[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xe, 0xf, 0xf };

	int newword, inten, red, green, blue;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	inten = intensity_table[newword & 15];
	red = (color_table[(newword >> 12) & 15] * inten) >> 4;
	green = (color_table[(newword >> 8) & 15] * inten) >> 4;
	blue = (color_table[(newword >> 4) & 15] * inten) >> 4;
	palette_set_color(offset, red, green, blue);
}



/*************************************
 *
 *	Video RAM bank read/write handlers
 *
 *************************************/

READ16_HANDLER( atarisys2_slapstic_r )
{
	int result = atarisys2_slapstic[offset];
	slapstic_tweak(offset);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak(0x1234) * 0x1000;
	return result;
}


WRITE16_HANDLER( atarisys2_slapstic_w )
{
	slapstic_tweak(offset);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak(0x1234) * 0x1000;
}



/*************************************
 *
 *	Video RAM read/write handlers
 *
 *************************************/

READ16_HANDLER( atarisys2_videoram_r )
{
	return vram[offset | videobank];
}


WRITE16_HANDLER( atarisys2_videoram_w )
{
	int offs = offset | videobank;

	/* alpharam? */
	if (offs < 0x0c00)
		atarian_0_vram_w(offs, data, mem_mask);

	/* spriteram? */
	else if (offs < 0x1000)
	{
		atarimo_0_spriteram_w(offs - 0x0c00, data, mem_mask);

		/* force an update if the link of object 0 changes */
		if (offs == 0x0c03)
			atarimo_force_update(0, cpu_getscanline() + 1);
	}

	/* playfieldram? */
	else if (offs >= 0x2000)
		ataripf_0_simple_w(offs - 0x2000, data, mem_mask);

	/* generic case */
	else
	{
		COMBINE_DATA(&vram[offs]);
	}
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

void atarisys2_scanline_update(int scanline)
{
	/* latch the Y scroll value */
	if (scanline == 0)
		ataripf_set_yscroll(0, latched_vscroll, 0);
}



/*************************************
 *
 *	Overrender callback
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* by default, draw anywhere the MO pen was 15 */
		data->drawmode = TRANSPARENCY_PENS;
		data->drawpens = 0x00ff;
		data->maskpens = 0x8000;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		int mopriority = data->mopriority << 1;
		int pfpriority = ((~data->pfpriority & 3) << 1) | 1;

		/* this equation comes from the schematics */
		return ((mopriority + pfpriority) & 4) ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void atarisys2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
