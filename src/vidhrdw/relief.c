/***************************************************************************

	Atari "Round" hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int relief_vh_start(void)
{
	static const struct ataripf_desc pf0desc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0x003f,		/* latch mask */
		0,			/* transparent pen mask */

		0x007fff,	/* tile data index mask */
		0x4f0000,	/* tile data color mask */
		0x008000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0x300000	/* tile data priority mask */
	};

	static const struct ataripf_desc pf1desc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0x3f00,		/* latch mask */
		0x0001,		/* transparent pen mask */

		0x007fff,	/* tile data index mask */
		0x4f0000,	/* tile data color mask */
		0x008000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0x300000	/* tile data priority mask */
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
		8,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x00ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x7fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xff80,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x0007 }},	/* mask for the height, in tiles */
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x0008,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	/* blend the MO graphics */
	ataripf_blend_gfx(1, 2, 0x0f, 0x10);

	/* initialize the playfield */
	if (!ataripf_init(0, &pf0desc))
		goto cant_create_pf0;

	/* initialize the second playfield */
	if (!ataripf_init(1, &pf1desc))
		goto cant_create_pf1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;
	return 0;

	/* error cases */
cant_create_mo:
cant_create_pf1:
	ataripf_free();
cant_create_pf0:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void relief_vh_stop(void)
{
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender1_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* by default, draw anywhere the MO pen was non-zero */
		data->drawmode = TRANSPARENCY_PEN;
		data->drawpens = 0;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
		return (data->pfcolor >= data->mopriority) ? OVERRENDER_YES : OVERRENDER_NO;
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void relief_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	ataripf_render(1, bitmap);
	atarimo_render(0, bitmap, NULL, overrender1_callback);
}
