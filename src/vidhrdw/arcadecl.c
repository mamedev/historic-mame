/***************************************************************************

	Atari Arcade Classics hardware (prototypes)

	Note: this video hardware has some similarities to Shuuz & company
	The sprite offset registers are stored to 3EFF80

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

int rampart_bitmap_init(int _xdim, int _ydim);
void rampart_bitmap_free(void);
void rampart_bitmap_invalidate(void);
void rampart_bitmap_mark_palette(int base);
void rampart_bitmap_render(struct osd_bitmap *bitmap);



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 has_mo;



/*************************************
 *
 *	Video system start
 *
 *************************************/

int arcadecl_vh_start(void)
{
	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
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
		0,					/* callback routine for ignored entries */
	};

	/* initialize the playfield */
	if (!rampart_bitmap_init(43*8, 30*8))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* set the intial scroll offset */
	atarimo_set_xscroll(0, -4, 0);
	atarimo_set_yscroll(0, 0x110, 0);
	has_mo = (Machine->gfx[0]->total_elements > 10);
	return 0;

	/* error cases */
cant_create_mo:
	rampart_bitmap_free();
cant_create_pf:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void arcadecl_vh_stop(void)
{
	atarimo_free();
	rampart_bitmap_free();
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void arcadecl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	rampart_bitmap_mark_palette(0);
	if (has_mo)
		atarimo_mark_palette(0);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
		rampart_bitmap_invalidate();

	/* draw the layers */
	rampart_bitmap_render(bitmap);
	if (has_mo)
		atarimo_render(0, bitmap, NULL, NULL);
}
