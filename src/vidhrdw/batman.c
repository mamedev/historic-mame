/***************************************************************************

	Atari Batman hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int batman_vh_start(void)
{
	static const struct ataripf_desc pf0desc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x300,		/* index of palette base */
		0x500,		/* maximum number of colors */
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

		0x200,		/* index of palette base */
		0x500,		/* maximum number of colors */
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
		1,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x500,				/* maximum number of colors */
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
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
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
		2,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x707ff,	/* tile data index mask */
		0x07800,	/* tile data color mask */
		0,			/* tile data hflip mask */
		0x08000		/* tile data opacity mask */
	};

	UINT32 *pflookup, *anlookup;
	int i, size;

	/* initialize the playfield */
	if (!ataripf_init(0, &pf0desc))
		goto cant_create_pf0;

	/* initialize the second playfield */
	if (!ataripf_init(1, &pf1desc))
		goto cant_create_pf1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;

	/* modify the playfield 0 lookup table to handle the palette bank */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int color = ATARIPF_LOOKUP_COLOR(pflookup[i]);
		if (color & 0x10) color ^= 0x50;
		ATARIPF_LOOKUP_SET_COLOR(pflookup[i], color);
	}

	/* modify the playfield 1 lookup table to handle the palette bank */
	pflookup = ataripf_get_lookup(1, &size);
	for (i = 0; i < size; i++)
	{
		int color = ATARIPF_LOOKUP_COLOR(pflookup[i]);
		if (color & 0x10) color ^= 0x50;
		ATARIPF_LOOKUP_SET_COLOR(pflookup[i], color);
	}

	/* modify the alphanumerics lookup table to handle the code bank */
	anlookup = atarian_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int entry = i << ATARIAN_LOOKUP_DATABITS;
		int code = entry & 0x3ff;
		if (entry & 0x400)
			code += (entry >> 16) * 0x400;
		ATARIAN_LOOKUP_SET_CODE(anlookup[i], code);
	}
	return 0;

	/* error cases */
cant_create_an:
	atarimo_free();
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

void batman_vh_stop(void)
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

void batman_scanline_update(int scanline)
{
	/* update the scanline parameters */
	if (scanline <= Machine->visible_area.max_y && atarivc_state.rowscroll_enable)
	{
		data16_t *base = &atarian_0_base[scanline / 8 * 64 + 48];
		int i;

		for (i = 0; i < 16; i++)
		{
			int data = base[i];
			switch (data & 15)
			{
				case 9:
					atarivc_state.mo_xscroll = (data >> 7) & 0x1ff;
					atarimo_set_xscroll(0, atarivc_state.mo_xscroll, scanline + i/2);
					break;

				case 10:
					atarivc_state.pf1_xscroll_raw = (data >> 7) & 0x1ff;
					atarivc_update_pf_xscrolls();
					ataripf_set_xscroll(0, atarivc_state.pf0_xscroll, scanline + i/2);
					ataripf_set_xscroll(1, atarivc_state.pf1_xscroll, scanline + i/2);
					break;

				case 11:
					atarivc_state.pf0_xscroll_raw = (data >> 7) & 0x1ff;
					atarivc_update_pf_xscrolls();
					ataripf_set_xscroll(0, atarivc_state.pf0_xscroll, scanline + i/2);
					break;

				case 13:
					atarivc_state.mo_yscroll = (data >> 7) & 0x1ff;
					atarimo_set_yscroll(0, atarivc_state.mo_yscroll, scanline + i/2);
					break;

				case 14:
					atarivc_state.pf1_yscroll = (data >> 7) & 0x1ff;
					ataripf_set_yscroll(1, atarivc_state.pf1_yscroll, scanline + i/2);
					break;

				case 15:
					atarivc_state.pf0_yscroll = (data >> 7) & 0x1ff;
					atarimo_set_yscroll(0, atarivc_state.pf0_yscroll, scanline + i/2);
					break;
			}

		}
	}
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static const UINT16 transparency_mask[4] =
{
	0xffff,
	0xff01,
	0xff01,
	0x0001
};

static int overrender0_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* do nothing if the MO priority is 3 */
		if (data->mopriority == 3)
			return OVERRENDER_NONE;

		/* if the MO priority is 0 and the color is 0, overrender pen 1 */
		if (data->mopriority == 0 && (data->mocolor & 0x0f) == 0)
		{
			data->drawmode = TRANSPARENCY_NONE;
			data->drawpens = 0;
			data->maskpens = ~0x0002;
			return OVERRENDER_ALL;
		}

		/* by default, draw anywhere the MO pen was non-zero */
		data->drawmode = TRANSPARENCY_PENS;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		/* if the priority is too low, don't bother */
		if (data->pfpriority <= data->mopriority)
			return OVERRENDER_NO;

		/* otherwise, look it up */
		data->drawpens = transparency_mask[data->pfpriority];
		return (data->drawpens != 0xffff) ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}


static int overrender1_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* do nothing if the MO priority is 3 */
		if (data->mopriority == 3)
			return OVERRENDER_NONE;

		/* if the MO priority is 0 and the color is 0, overrender pen 1 */
		if (data->mopriority == 0 && (data->mocolor & 0x0f) == 0)
		{
			data->drawmode = TRANSPARENCY_NONE;
			data->drawpens = 0;
			data->maskpens = ~0x80000002;
			return OVERRENDER_ALL;
		}

		/* by default, draw anywhere the MO pen was non-zero */
		data->drawmode = TRANSPARENCY_PENS;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		/* if the priority is too low, don't bother */
		if (data->pfpriority <= data->mopriority)
			return OVERRENDER_NO;

		/* otherwise, look it up */
		data->drawpens = transparency_mask[data->pfpriority] | 0x0001;
		return (data->drawpens != 0xffff) ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void batman_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	ataripf_render(1, bitmap);
	atarimo_render(0, bitmap, overrender0_callback, overrender1_callback);
	atarian_render(0, bitmap);
}
