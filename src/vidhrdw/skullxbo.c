/***************************************************************************

	Atari Skull & Crossbones hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int skullxbo_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		1,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0x00ff,		/* latch mask */
		0,			/* transparent pen mask */

		0x07fff,	/* tile data index mask */
		0xf0000,	/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		2,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		1,					/* number of scanlines between MO updates */

		0x000,				/* base palette entry */
		0x200,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x00ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x7fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xffc0,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x000f }},	/* mask for the height, in tiles */
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x0030,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0,					/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		2,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */

		0x300,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x07ff,		/* tile data index mask */
		0x7800,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0x8000		/* tile data opacity mask */
	};

	UINT32 *anlookup;
	int i, size;

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;

	/* add in the code XOR for the alphas */
	anlookup = atarian_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int code = ATARIAN_LOOKUP_CODE(anlookup[i], 0);
		ATARIAN_LOOKUP_SET_CODE(anlookup[i], code ^ 0x400);
	}
	return 0;

	/* error cases */
cant_create_an:
	atarimo_free();
cant_create_mo:
	ataripf_free();
cant_create_pf:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void skullxbo_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Video data latch
 *
 *************************************/

WRITE16_HANDLER( skullxbo_hscroll_w )
{
	int scanline = cpu_getscanline();
	int newscroll = (ataripf_get_xscroll(0) / 2) << 7;

	COMBINE_DATA(&newscroll);
	newscroll = 2 * (newscroll >> 7);
	ataripf_set_xscroll(0, newscroll, scanline);
	atarimo_set_xscroll(0, newscroll, scanline);
}


WRITE16_HANDLER( skullxbo_vscroll_w )
{
	int scanline = cpu_getscanline();
	int newscroll = ataripf_get_yscroll(0) << 7;

	COMBINE_DATA(&newscroll);
	newscroll = ((newscroll >> 7) - scanline) & 0x1ff;
	ataripf_set_yscroll(0, newscroll, scanline);
	atarimo_set_yscroll(0, newscroll, scanline);
}



/*************************************
 *
 *	Motion object bank handler
 *
 *************************************/

WRITE16_HANDLER( skullxbo_mobmsb_w )
{
	atarimo_set_bank(0, (offset >> 9) & 1, cpu_getscanline() + 1);
}



/*************************************
 *
 *	Playfield latch write handler
 *
 *************************************/

WRITE16_HANDLER( skullxbo_playfieldlatch_w )
{
	if (ACCESSING_LSB)
		ataripf_set_latch(0, data & 0xff);
}



/*************************************
 *
 *	Periodic playfield updater
 *
 *************************************/

void skullxbo_scanline_update(int scanline)
{
	data16_t *base = &atarian_0_base[(scanline / 8) * 64 + 42];
	int x;

	/* keep in range */
	if (base >= &atarian_0_base[0x7c0])
		return;

	/* update the current parameters */
	for (x = 42; x < 64; x++)
	{
		data16_t data = *base++;
		data16_t command = data & 0x000f;

		if (command == 0x0d)
		{
			/* a new vscroll latches the offset into a counter; we must adjust for this */
			int newscroll = ((data >> 7) - scanline) & 0x1ff;
			ataripf_set_yscroll(0, newscroll, scanline);
			atarimo_set_yscroll(0, newscroll, scanline);
		}
	}
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	static const UINT16 overrender_matrix[4] =
	{
		0xf000,
		0xff00,
		0x0ff0,
		0x00f0
	};

	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* by default, draw anywhere the MO pen was non-zero */
		data->drawmode = TRANSPARENCY_PENS;
		data->drawpens = 0x00ff;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
		return (overrender_matrix[data->mopriority] & (1 << data->pfcolor)) ? OVERRENDER_YES : OVERRENDER_NO;
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void skullxbo_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
