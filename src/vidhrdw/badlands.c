/***************************************************************************

	Atari Bad Lands hardware

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Generic video system start
 *
 *************************************/

int badlands_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,32,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x00,		/* index of palette base */
		0x80,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x11fff,	/* tile data index mask */
		0x0e000,	/* tile data color mask */
		0,			/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		0,					/* are the entries linked? */
		1,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x80,				/* base palette entry */
		0x80,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x1f }},			/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0x0fff,0,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x0007 }},	/* mask for the color */
		{{ 0,0,0,0xff80 }},	/* mask for the X position */
		{{ 0,0xff80,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0x000f,0,0 }},	/* mask for the height, in tiles */
		{{ 0 }},			/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0,0x0008 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	UINT32 *pflookup;
	int i, size;

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* modify the code bits in the playfield lookup to handle our banking */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int entry = i << ATARIPF_LOOKUP_DATABITS;
		int code = entry & 0xfff;
		if (entry & 0x1000)
			code += 0x1000 + ((entry & 0x10000) >> 4);
		ATARIPF_LOOKUP_SET_CODE(pflookup[i], code);
	}

	return 0;

	/* error cases */
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

void badlands_vh_stop(void)
{
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Playfield bank write handler
 *
 *************************************/

WRITE16_HANDLER( badlands_pf_bank_w )
{
	if (ACCESSING_LSB)
		ataripf_set_bankbits(0, (data & 1) << 16, cpu_getscanline() + 1);
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	/* we're all or nothing, so just handle the BEGIN message */
	if (state == OVERRENDER_BEGIN)
	{
		/* if the priority is 1, we don't need to overrender */
		if (data->mopriority == 1)
			return OVERRENDER_NONE;

		/* by default, draw anywhere the MO is non-zero */
		data->drawmode = TRANSPARENCY_PENS;
		data->drawpens = 0x00ff;
		data->maskpens = 0x0001;
		return OVERRENDER_ALL;
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void badlands_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
}
