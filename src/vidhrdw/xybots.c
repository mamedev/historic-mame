/***************************************************************************

	Atari Xybots hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int xybots_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,32,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x1fff,		/* tile data index mask */
		0x7800,		/* tile data color mask */
		0x8000,		/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		0,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x300,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x3f }},			/* mask for the link (dummy) */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0x3fff,0,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x000f }},	/* mask for the color */
		{{ 0,0,0,0xff80 }},	/* mask for the X position */
		{{ 0,0,0xff80,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0,0x0007,0 }},	/* mask for the height, in tiles */
		{{ 0x8000,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0x000f,0,0 }},	/* mask for the priority */
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

		0x000,		/* index of palette base */
		0x080,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x03ff,		/* tile data index mask */
		0x7000,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0x8000		/* tile data opacity mask */
	};

	UINT8 *colorlookup;
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

	/* modify the motion object lookup table for the funky palette banking */
	colorlookup = atarimo_get_color_lookup(0, &size);
	for (i = 0; i < size; i++)
		if (colorlookup[i] & 8)
			colorlookup[i] ^= 0x2f;

	return 0;

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

void xybots_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
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
		/* by default, draw anywhere the MO is non-zero */
		data->drawmode = TRANSPARENCY_NONE;
		data->drawpens = 0;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		int mopriority = data->mopriority ^ 15;
		int pfcolor = (data->pfcolor & 0x20) ? (data->pfcolor ^ 0x2f) : data->pfcolor;

		/* overrender if the MO priority is greater than the PF color */
		return (mopriority > pfcolor) ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void xybots_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
