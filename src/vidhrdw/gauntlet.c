/***************************************************************************

	Atari Gauntlet hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

UINT8 vindctr2_screen_refresh;
data16_t *gauntlet_yscroll;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT32 bankbits;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int gauntlet_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		8,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x30fff,	/* tile data index mask */
		0x47000,	/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		1,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		1,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0,0x03ff }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0x7fff,0,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0x000f,0,0 }},	/* mask for the color */
		{{ 0,0xff80,0,0 }},	/* mask for the X position */
		{{ 0,0,0xff80,0 }},	/* mask for the Y position */
		{{ 0,0,0x0038,0 }},	/* mask for the width, in tiles*/
		{{ 0,0,0x0007,0 }},	/* mask for the height, in tiles */
		{{ 0,0,0x0040,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		1,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0x00f,		/* mask of the palette split */

		0x03ff,		/* tile data index mask */
		0x7c00,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0x8000		/* tile data opacity mask */
	};

	UINT32 *pflookup;
	UINT16 *codelookup;
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

	/* modify the playfield lookup table to account for the code XOR */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int code = ATARIPF_LOOKUP_CODE(pflookup[i], 0);
		ATARIPF_LOOKUP_SET_CODE(pflookup[i], code ^ 0x800);
	}

	/* modify the motion object code lookup table to account for the code XOR */
	codelookup = atarimo_get_code_lookup(0, &size);
	for (i = 0; i < size; i++)
		codelookup[i] ^= 0x800;

	/* set up the base color for the playfield */
	bankbits = vindctr2_screen_refresh ? 0x00000 : 0x40000;
	ataripf_set_bankbits(0, bankbits, 0);
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

void gauntlet_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Horizontal scroll register
 *
 *************************************/

WRITE16_HANDLER( gauntlet_xscroll_w )
{
	int scanline = cpu_getscanline();
	int oldscroll = ataripf_get_xscroll(0);
	int newscroll = oldscroll;

	COMBINE_DATA(&newscroll);
	ataripf_set_xscroll(0, newscroll & 0x1ff, scanline);
	atarimo_set_xscroll(0, newscroll & 0x1ff, scanline);
}



/*************************************
 *
 *	Vertical scroll/PF bank register
 *
 *************************************/

WRITE16_HANDLER( gauntlet_yscroll_w )
{
	int scanline = cpu_getscanline();
	int oldscroll = *gauntlet_yscroll;

	COMBINE_DATA(gauntlet_yscroll);
	if (oldscroll != *gauntlet_yscroll)
	{
		ataripf_set_yscroll(0, (*gauntlet_yscroll >> 7) & 0x1ff, scanline);
		atarimo_set_yscroll(0, (*gauntlet_yscroll >> 7) & 0x1ff, scanline);
		ataripf_set_bankbits(0, ((*gauntlet_yscroll & 3) << 16) | bankbits, scanline);
	}
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* do nothing if the MO didn't draw anything in pen 1 */
		if (!(data->mousage & 0x0002))
			return OVERRENDER_NONE;

		/* by default, draw anywhere the MO pen was 1 */
		data->drawmode = TRANSPARENCY_NONE;
		data->drawpens = 0;
		data->maskpens = ~0x0002;

		/* only need to query if we are modifying the color */
		return (data->mocolor != 0 || vindctr2_screen_refresh) ? OVERRENDER_SOME : OVERRENDER_ALL;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		/* swap to the shadow color */
		data->pfcolor ^= 8;
		return OVERRENDER_YES;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void gauntlet_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
