/***************************************************************************

	Atari Vindicators hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

int vindictr_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */
	
		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		1,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */
	
		0x70fff,	/* tile data index mask */
		0x07000,	/* tile data color mask */
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
		0,					/* callback routine for ignored entries */
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
	
	/* modify the playfield lookup table to account for the shifted color bits */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int color = ATARIPF_LOOKUP_COLOR(pflookup[i]);
		ATARIPF_LOOKUP_SET_COLOR(pflookup[i], color << 1);
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

void vindictr_vh_stop(void)
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

void vindictr_scanline_update(int scanline)
{
	data16_t *base = &atarian_0_base[(scanline / 8) * 64 + 42];
	int x;

	/* update the current parameters */
	if (base < &atarian_0_base[0x7c0])
		for (x = 42; x < 64; x++)
		{
			data16_t data = *base++;
			data16_t command = data & 0x7e00;

			if (command == 0x7400)
				ataripf_set_bankbits(0, (data & 7) << 16, scanline + 8);
			else if (command == 0x7600)
			{
				ataripf_set_xscroll(0, data & 0x1ff, scanline + 8);
				atarimo_set_xscroll(0, data & 0x1ff, scanline + 8);
			}
			else if (command == 0x7800)
				;
			else if (command == 0x7a00)
				;
			else if (command == 0x7c00)
				;
			else if (command == 0x7e00)
			{
				/* a new vscroll latches the offset into a counter; we must adjust for this */
				int offset = scanline + 8;
				if (offset >= 240)
					offset -= 240;
				ataripf_set_yscroll(0, (data - offset) & 0x1ff, scanline + 8);
				atarimo_set_yscroll(0, (data - offset) & 0x1ff, scanline + 8);
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
		return (data->mocolor != 0) ? OVERRENDER_SOME : OVERRENDER_ALL;
	}
	
	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		/* swap to the shadow color */
		data->pfcolor ^= 1;
		return OVERRENDER_YES;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void vindictr_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	ataripf_mark_palette(0);
	atarimo_mark_palette(0);
	atarian_mark_palette(0);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
		ataripf_invalidate(0);

	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
