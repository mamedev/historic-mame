/***************************************************************************

	Atari Toobin' hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

static double brightness;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int toobin_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		128,64,		/* size of the playfield in tiles (x,y) */
		1,128,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x003fff,	/* tile data index mask */
		0x0f0000,	/* tile data color mask */
		0x004000,	/* tile data hflip mask */
		0x008000,	/* tile data vflip mask */
		0x300000	/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		1,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		1024,				/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0x00ff,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x3fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x000f }},	/* mask for the color */
		{{ 0,0,0,0xffc0 }},	/* mask for the X position */
		{{ 0x7fc0,0,0,0 }},	/* mask for the Y position */
		{{ 0x0007,0,0,0 }},	/* mask for the width, in tiles*/
		{{ 0x0038,0,0,0 }},	/* mask for the height, in tiles */
		{{ 0,0x4000,0,0 }},	/* mask for the horizontal flip */
		{{ 0,0x8000,0,0 }},	/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0x8000,0,0,0 }},	/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		2,			/* index to which gfx system */
		64,64,		/* size of the alpha RAM in tiles (x,y) */

		0x200,		/* index of palette base */
		0x040,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x03ff,		/* tile data index mask */
		0xf000,		/* tile data color mask */
		0x0400,		/* tile data hflip mask */
		0			/* tile data opacity mask */
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

void toobin_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Palette RAM write handler
 *
 *************************************/

WRITE16_HANDLER( toobin_paletteram_w )
{
	int newword;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	{
		int red =   (((newword >> 10) & 31) * 224) >> 5;
		int green = (((newword >>  5) & 31) * 224) >> 5;
		int blue =  (((newword      ) & 31) * 224) >> 5;

		if (red) red += 38;
		if (green) green += 38;
		if (blue) blue += 38;

		palette_set_color(offset & 0x3ff, red, green, blue);
		if (!(newword & 0x8000))
			palette_set_brightness(offset & 0x3ff, brightness);
		else
			palette_set_brightness(offset & 0x3ff, 1.0);
	}
}


WRITE16_HANDLER( toobin_intensity_w )
{
	int i;

	if (ACCESSING_LSB)
	{
		brightness = (double)(~data & 0x1f) / 31.0;

		for (i = 0; i < 0x400; i++)
			if (!(paletteram16[i] & 0x8000))
				palette_set_brightness(i, brightness);
	}
}



/*************************************
 *
 *	X/Y scroll handlers
 *
 *************************************/

WRITE16_HANDLER( toobin_hscroll_w )
{
	int scanline = cpu_getscanline() + 1;
	int newscroll = ataripf_get_xscroll(0) << 6;
	COMBINE_DATA(&newscroll);

	ataripf_set_xscroll(0, (newscroll >> 6) & 0x3ff, scanline);
	atarimo_set_xscroll(0, (newscroll >> 6) & 0x3ff, scanline);
}


WRITE16_HANDLER( toobin_vscroll_w )
{
	int scanline = cpu_getscanline() + 1;
	int newscroll = ataripf_get_yscroll(0) << 6;
	COMBINE_DATA(&newscroll);

	ataripf_set_yscroll(0, (newscroll >> 6) & 0x1ff, scanline);
	atarimo_set_yscroll(0, (newscroll >> 6) & 0x1ff, scanline);
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
		/* by default, draw anywhere the MO pen was 1 */
		data->drawmode = TRANSPARENCY_PENS;
		data->drawpens = 0x00ff;
		data->maskpens = 0x0001;
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
	{
		return data->pfpriority ? OVERRENDER_YES : OVERRENDER_NO;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void toobin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}
