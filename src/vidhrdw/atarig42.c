/***************************************************************************

	Atari G42 hardware

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

UINT8 atarig42_guardian;



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t current_control;
static UINT32 bankbits;



/*************************************
 *
 *	Video system start
 *
 *************************************/

static int common_vh_start(int gt)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		128,64,		/* size of the playfield in tiles (x,y) */
		1,128,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x000,		/* index of palette base */
		0x800,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x070fff,	/* tile data index mask */
		0x387000,	/* tile data color mask */
		0x008000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarirle_desc modesc =
	{
		REGION_GFX3,/* region where the GFX data lives */
		256,		/* number of entries in sprite RAM */
		0,			/* left clip coordinate */
		0,			/* right clip coordinate */

		0x400,		/* base palette entry */
		0x400,		/* maximum number of colors */

		{{ 0x7fff,0,0,0,0,0,0,0 }},	/* mask for the code index */
		{{ 0,0x03f0,0,0,0,0,0,0 }},	/* mask for the color */
		{{ 0,0,0xffc0,0,0,0,0,0 }},	/* mask for the X position */
		{{ 0,0,0,0xffc0,0,0,0,0 }},	/* mask for the Y position */
		{{ 0,0,0,0,0xffff,0,0,0 }},	/* mask for the scale factor */
		{{ 0x8000,0,0,0,0,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},					/* mask for the vertical flip */
		{{ 0,0,0,0,0,0,0x00ff,0 }}	/* mask for the priority */
	};

	static const struct atarirle_desc modesc_gt =
	{
		REGION_GFX3,/* region where the GFX data lives */
		256,		/* number of entries in sprite RAM */
		0,			/* left clip coordinate */
		0,			/* right clip coordinate */

		0x1000,		/* base palette entry */
		0x1000,		/* maximum number of colors */

		{{ 0x7fff,0,0,0,0,0,0,0 }},	/* mask for the code index */
		{{ 0,0x07f0,0,0,0,0,0,0 }},	/* mask for the color */
		{{ 0,0,0xffc0,0,0,0,0,0 }},	/* mask for the X position */
		{{ 0,0,0,0xffc0,0,0,0,0 }},	/* mask for the Y position */
		{{ 0,0,0,0,0xffff,0,0,0 }},	/* mask for the scale factor */
		{{ 0x8000,0,0,0,0,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},					/* mask for the vertical flip */
		{{ 0,0,0,0,0,0,0x00ff,0 }}	/* mask for the priority */
	};

	static const struct atarian_desc andesc =
	{
		1,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x0fff,		/* tile data index mask */
		0xf000,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0x8000		/* tile data opacity mask */
	};

	/* blend the playfields and free the temporary one */
	ataripf_blend_gfx(0, 2, 0x0f, 0x30);

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarirle_init(0, gt ? &modesc_gt : &modesc))
		goto cant_create_mo;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;

	/* reset statics */
	current_control = 0;
	bankbits = (atarig42_guardian || gt) ? 0x000000 : 0x200000;
	return 0;

	/* error cases */
cant_create_an:
	atarirle_free();
cant_create_mo:
	ataripf_free();
cant_create_pf:
	return 1;
}


int atarig42_vh_start(void)
{
	return common_vh_start(0);
}


int atarigt_vh_start(void)
{
	return common_vh_start(1);
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void atarig42_vh_stop(void)
{
	atarian_free();
	atarirle_free();
	ataripf_free();
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

WRITE16_HANDLER( atarig42_mo_control_w )
{
	logerror("MOCONT = %d (scan = %d)\n", data, cpu_getscanline());

	/* set the control value */
	COMBINE_DATA(&current_control);
}


void atarig42_scanline_update(int scanline)
{
	data16_t *base = &atarian_0_base[(scanline / 8) * 64 + 48];
	int i;

	if (scanline == 0) logerror("-------\n");

	/* keep in range */
	if (base >= &atarian_0_base[0x800])
		return;

	/* update the playfield scrolls */
	for (i = 0; i < 8; i++)
	{
		data16_t word;

		word = *base++;
		if (word & 0x8000)
		{
			ataripf_set_xscroll(0, (word >> 5) & 0x3ff, scanline + i);
			bankbits = (bankbits & ~0x180000) | ((word & 3) << 19);
			ataripf_set_bankbits(0, bankbits, scanline + i);
		}

		word = *base++;
		if (word & 0x8000)
		{
			ataripf_set_yscroll(0, ((word >> 6) - (scanline + i)) & 0x1ff, scanline + i);
			bankbits = (bankbits & ~0x070000) | ((word & 7) << 16);
			ataripf_set_bankbits(0, bankbits, scanline + i);
		}
	}
}


void atarigx2_scanline_update(int scanline)
{
	data32_t *base = &atarian_0_base32[(scanline / 8) * 32 + 24];
	int i;

	if (scanline == 0) logerror("-------\n");

	/* keep in range */
	if (base >= &atarian_0_base32[0x400])
		return;

	/* update the playfield scrolls */
	for (i = 0; i < 8; i++)
	{
		data32_t word = *base++;

		if (word & 0x80000000)
		{
			ataripf_set_xscroll(0, (word >> 21) & 0x3ff, scanline + i);
			bankbits = (bankbits & ~0x180000) | ((word & 0x30000) << 3);
			ataripf_set_bankbits(0, bankbits, scanline + i);
		}

		if (word & 0x00008000)
		{
			ataripf_set_yscroll(0, ((word >> 6) - (scanline + i)) & 0x1ff, scanline + i);
			bankbits = (bankbits & ~0x070000) | ((word & 7) << 16);
			ataripf_set_bankbits(0, bankbits, scanline + i);
		}
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void atarig42_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarirle_render(0, bitmap, NULL);
	atarian_render(0, bitmap);
}
