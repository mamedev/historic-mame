/***************************************************************************

	Atari Cyberball hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *cyberbal_paletteram_0;
data16_t *cyberbal_paletteram_1;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 current_screen;
static UINT8 total_screens;
static data16_t current_slip[2];



/*************************************
 *
 *	Video system start
 *
 *************************************/

static int cyberbal_vh_start_common(int screens)
{
	static const struct ataripf_desc pf0desc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */
	
		0x000,		/* index of palette base */
		0x600,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */
	
		0x01fff,	/* tile data index mask */
		0x77800,	/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct ataripf_desc pf1desc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */
	
		0x800,		/* index of palette base */
		0x600,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */
	
		0x01fff,	/* tile data index mask */
		0x77800,	/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc mo0desc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		1,					/* does the neighbor bit affect the next object? */
		1024,				/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x600,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0x07f8,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0x7fff,0,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x000f }},	/* mask for the color */
		{{ 0,0,0,0xffc0 }},	/* mask for the X position */
		{{ 0,0xff80,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0x000f,0,0 }},	/* mask for the height, in tiles */
		{{ 0x8000,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0,0,0,0x0010 }},	/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */
		
		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	static const struct atarimo_desc mo1desc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		1,					/* does the neighbor bit affect the next object? */
		1024,				/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0xe00,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0x07f8,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0x7fff,0,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0,0x000f }},	/* mask for the color */
		{{ 0,0,0,0xffc0 }},	/* mask for the X position */
		{{ 0,0xff80,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0,0x000f,0,0 }},	/* mask for the height, in tiles */
		{{ 0x8000,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0,0,0,0x0010 }},	/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */
		
		{{ 0 }},			/* mask for the ignore value */
		0					/* resulting value to indicate "ignore" */
	};

	static const struct atarian_desc an0desc =
	{
		2,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */
	
		0x780,		/* index of palette base */
		0x080,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x0fff,		/* tile data index mask */
		0x7000,		/* tile data color mask */
		0x8000,		/* tile data hflip mask */
		0			/* tile data opacity mask */
	};

	static const struct atarian_desc an1desc =
	{
		2,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */
	
		0xf80,		/* index of palette base */
		0x080,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x0fff,		/* tile data index mask */
		0x7000,		/* tile data color mask */
		0x8000,		/* tile data hflip mask */
		0			/* tile data opacity mask */
	};
	
	/* set the slip variables */
	atarimo_0_slipram = &current_slip[0];
	atarimo_1_slipram = &current_slip[1];

	/* initialize the playfield */
	if (!ataripf_init(0, &pf0desc))
		goto cant_create_pf0;
	
	/* initialize the motion objects */
	if (!atarimo_init(0, &mo0desc))
		goto cant_create_mo0;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &an0desc))
		goto cant_create_an0;

	/* allocate the second screen if necessary */	
	if (screens == 2)
	{
		/* initialize the playfield */
		if (!ataripf_init(1, &pf1desc))
			goto cant_create_pf1;
		
		/* initialize the motion objects */
		if (!atarimo_init(1, &mo1desc))
			goto cant_create_mo1;
	
		/* initialize the alphanumerics */
		if (!atarian_init(1, &an1desc))
			goto cant_create_an1;
	}
	
	/* reset statics */
	current_slip[0] = 0;
	current_slip[1] = 0;
	total_screens = screens;
	current_screen = 0;
	return 0;

	/* error cases */
cant_create_an1:
cant_create_mo1:
cant_create_pf1:
	atarian_free();
cant_create_an0:
	atarimo_free();
cant_create_mo0:
	ataripf_free();
cant_create_pf0:
	return 1;
}


int cyberbal_vh_start(void)
{
	int result = cyberbal_vh_start_common(2);
	if (!result)
	{
		/* adjust the sprite positions */
		atarimo_set_xscroll(0, 4, 0);
		atarimo_set_xscroll(1, 4, 0);
	}
	return result;
}


int cyberb2p_vh_start(void)
{
	int result = cyberbal_vh_start_common(1);
	if (!result)
	{
		/* adjust the sprite positions */
		atarimo_set_xscroll(0, 5, 0);
	}
	return result;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void cyberbal_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
}



/*************************************
 *
 *	Screen switcher
 *
 *************************************/

void cyberbal_set_screen(int which)
{
	if (which < total_screens)
		current_screen = which;
}



/*************************************
 *
 *	Palette tweaker
 *
 *************************************/

INLINE void set_palette_entry(int entry, UINT16 value)
{
	int r, g, b;

	r = ((value >> 9) & 0x3e) | ((value >> 15) & 1);
	g = ((value >> 4) & 0x3e) | ((value >> 15) & 1);
	b = ((value << 1) & 0x3e) | ((value >> 15) & 1);

	r = (r << 2) | (r >> 4);
	g = (g << 2) | (g >> 4);
	b = (b << 2) | (b >> 4);

	palette_change_color(entry, r, g, b);
}



/*************************************
 *
 *	Palette RAM write handlers
 *
 *************************************/

WRITE16_HANDLER( cyberbal_paletteram_0_w )
{
	int oldword = cyberbal_paletteram_0[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);
	
	if (oldword != newword)
	{
		cyberbal_paletteram_0[offset] = newword;
		set_palette_entry(offset, newword);
	}
}

READ16_HANDLER( cyberbal_paletteram_0_r )
{
	return cyberbal_paletteram_0[offset];
}


WRITE16_HANDLER( cyberbal_paletteram_1_w )
{
	int oldword = cyberbal_paletteram_1[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);
	
	if (oldword != newword)
	{
		cyberbal_paletteram_1[offset] = newword;
		set_palette_entry(offset + 0x800, newword);
	}
}

READ16_HANDLER( cyberbal_paletteram_1_r )
{
	return cyberbal_paletteram_1[offset];
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

void cyberbal_scanline_update(int scanline)
{
	int i;
	
	/* loop over screens */
	for (i = 0; i < total_screens; i++)
	{
		data16_t *vram = atarian_get_vram(i);
		data16_t *base = &vram[((scanline - 8) / 8) * 64 + 47];
	
		/* keep in range */
		if (base < vram)
			base += 0x800;
		else if (base >= &vram[0x800])
			return;
	
		/* update the current parameters */
		if (!(base[3] & 1))
			ataripf_set_bankbits(i, ((base[3] >> 1) & 7) << 16, scanline);
		if (!(base[4] & 1))
			ataripf_set_xscroll(i, 2 * (((base[4] >> 7) + 4) & 0x1ff), scanline);
		if (!(base[5] & 1))
		{
			/* a new vscroll latches the offset into a counter; we must adjust for this */
			ataripf_set_yscroll(i, ((base[5] >> 7) - (scanline)) & 0x1ff, scanline);
		}
		if (!(base[7] & 1))
			current_slip[i] = base[7];

		/* update the MOs with the current parameters */
		atarimo_force_update(i, scanline);
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void cyberbal_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	ataripf_mark_palette(current_screen);
	atarimo_mark_palette(current_screen);
	atarian_mark_palette(current_screen);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
	{
		ataripf_invalidate(0);
		if (total_screens == 2)
			ataripf_invalidate(1);
	}

	/* draw the layers */
	ataripf_render(current_screen, bitmap);
	atarimo_render(current_screen, bitmap, NULL, NULL);
	atarian_render(current_screen, bitmap);
}
