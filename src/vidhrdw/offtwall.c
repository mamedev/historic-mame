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

int offtwall_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		64,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0xff00,		/* latch mask */
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
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;
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

void offtwall_vh_stop(void)
{
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void offtwall_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, NULL, NULL);
}
