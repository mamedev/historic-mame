/***************************************************************************

	Atari Escape hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int eprom_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
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
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		1,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		1,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x03ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x7fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xff80,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x0007 }},	/* mask for the height, in tiles */
		{{ 0,0,0,0x0008 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x0030,0 }},	/* mask for the priority */
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

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;
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

void eprom_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

void eprom_scanline_update(int scanline)
{
	/* update the playfield */
	if (scanline == 0)
	{
		int xscroll = (atarian_0_base[0x780] >> 7) & 0x1ff;
		int yscroll = (atarian_0_base[0x781] >> 7) & 0x1ff;
		ataripf_set_xscroll(0, xscroll, 0);
		ataripf_set_yscroll(0, yscroll, 0);
		atarimo_set_xscroll(0, xscroll, 0);
		atarimo_set_yscroll(0, yscroll, 0);
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
		/* do nothing if the MO priority is max */
		if (data->mopriority == 3)
			return OVERRENDER_NONE;

		/* by default, draw anywhere the MO pen was non-0 */
		data->drawmode = TRANSPARENCY_PENS;
		data->drawpens = 0xff00;
		data->maskpens = 0x0001;

		/* we need to query on each tile the color */
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
		return (data->pfcolor >= 13 + data->mopriority) ? OVERRENDER_YES : OVERRENDER_NO;
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void eprom_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
