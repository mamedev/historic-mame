/***************************************************************************

	Atari Skull & Crossbones hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha[tile_index];
	int code = (data ^ 0x400) & 0x7ff;
	int color = (data >> 11) & 0x0f;
	int opaque = data & 0x8000;
	SET_TILE_INFO(2, code, color, opaque ? TILE_IGNORE_TRANSPARENCY : 0);
}


static void get_playfield_tile_info(int tile_index)
{
	UINT16 data1 = atarigen_playfield[tile_index];
	UINT16 data2 = atarigen_playfield_upper[tile_index] & 0xff;
	int code = data1 & 0x7fff;
	int color = data2 & 0x0f;
	SET_TILE_INFO(1, code, color, (data1 >> 15) & 1);
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( skullxbo )
{
	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		2,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		0,					/* pixel offset for SLIPs */

		0x000,				/* base palette entry */
		0x200,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x00ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x7fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xffc0,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x000f }},	/* mask for the height, in tiles */
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x0030,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the special value */
		0,					/* resulting value to indicate "special" */
		0,					/* callback routine for special entries */
	};

	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,8, 64,64);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		return 1;

	/* initialize the alphanumerics */
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,8, 64,32);
	if (!atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);

	return 0;
}



/*************************************
 *
 *	Video data latch
 *
 *************************************/

WRITE16_HANDLER( skullxbo_xscroll_w )
{
	/* combine data */
	data16_t oldscroll = *atarigen_xscroll;
	data16_t newscroll = oldscroll;
	COMBINE_DATA(&newscroll);

	/* if something changed, force an update */
	if (oldscroll != newscroll)
		force_partial_update(cpu_getscanline());
	
	/* adjust the actual scrolls */
	tilemap_set_scrollx(atarigen_playfield_tilemap, 0, 2 * (newscroll >> 7));
	atarimo_set_xscroll(0, 2 * (newscroll >> 7));

	/* update the data */
	*atarigen_xscroll = newscroll;
}


WRITE16_HANDLER( skullxbo_yscroll_w )
{
	/* combine data */
	int scanline = cpu_getscanline();
	data16_t oldscroll = *atarigen_yscroll;
	data16_t newscroll = oldscroll;
	data16_t effscroll;
	COMBINE_DATA(&newscroll);

	/* if something changed, force an update */
	if (oldscroll != newscroll)
		force_partial_update(scanline);
	
	/* adjust the effective scroll for the current scanline */
	if (scanline > Machine->visible_area.max_y)
		scanline = 0;
	effscroll = (newscroll >> 7) - scanline;
	
	/* adjust the actual scrolls */
	tilemap_set_scrolly(atarigen_playfield_tilemap, 0, effscroll);
	atarimo_set_yscroll(0, effscroll & 0x1ff);

	/* update the data */
	*atarigen_yscroll = newscroll;
}



/*************************************
 *
 *	Motion object bank handler
 *
 *************************************/

WRITE16_HANDLER( skullxbo_mobmsb_w )
{
	force_partial_update(cpu_getscanline());
	atarimo_set_bank(0, (offset >> 9) & 1);
}



/*************************************
 *
 *	Playfield latch write handler
 *
 *************************************/

WRITE16_HANDLER( skullxbo_playfieldlatch_w )
{
	atarigen_set_playfield_latch(data);
}



/*************************************
 *
 *	Periodic playfield updater
 *
 *************************************/

void skullxbo_scanline_update(int scanline)
{
	data16_t *base = &atarigen_alpha[(scanline / 8) * 64 + 42];
	int x;

	/* keep in range */
	if (base >= &atarigen_alpha[0x7c0])
		return;
	
	/* special case: scanline 0 should re-latch the previous raw scroll */
	if (scanline == 0)
	{
		int newscroll = (*atarigen_yscroll >> 7) & 0x1ff;
		tilemap_set_scrolly(atarigen_playfield_tilemap, 0, newscroll);
		atarimo_set_yscroll(0, newscroll);
	}
	
	/* update the current parameters */
	for (x = 42; x < 64; x++)
	{
		data16_t data = *base++;
		data16_t command = data & 0x000f;

		/* only command I've ever seen */
		if (command == 0x0d)
		{
			/* a new vscroll latches the offset into a counter; we must adjust for this */
			int newscroll = ((data >> 7) - scanline) & 0x1ff;
			
			/* force a partial update with the previous scroll */
			force_partial_update(scanline - 1);
			
			/* update the new scroll */
			tilemap_set_scrolly(atarigen_playfield_tilemap, 0, newscroll);
			atarimo_set_yscroll(0, newscroll);
			
			/* make sure we change this value so that writes to the scroll register */
			/* know whether or not they are a different scroll */
			*atarigen_yscroll = data;
		}
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( skullxbo )
{
	static const UINT16 overrender_matrix[4] =
	{
		0xf000,
		0xff00,
		0x0ff0,
		0x00f0
	};
	struct atarimo_rect_list rectlist;
	struct mame_bitmap *mobitmap;
	int x, y, r;

	/* draw the playfield */
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 0, 0);

	/* draw and merge the MO */
	mobitmap = atarimo_render(0, cliprect, &rectlist);
	for (r = 0; r < rectlist.numrects; r++, rectlist.rect++)
		for (y = rectlist.rect->min_y; y <= rectlist.rect->max_y; y++)
		{
			UINT16 *mo = (UINT16 *)mobitmap->base + mobitmap->rowpixels * y;
			UINT16 *pf = (UINT16 *)bitmap->base + bitmap->rowpixels * y;
			for (x = rectlist.rect->min_x; x <= rectlist.rect->max_x; x++)
				if (mo[x])
				{
					/* not verified: logic is all controlled in a PAL
					
						factors: LBPRI1-0, LBPIX3, ANPIX1-0, PFPAL3-2S, PFPIX3-2,
						         (LBPIX4 | LBPIX3 | LBPIX2 | LBPIX1 | LBPIX0)
					*/

					int mopriority = mo[x] >> ATARIMO_PRIORITY_SHIFT;
					int pfcolor = (pf[x] >> 4) & 0x0f;
					int pfpix = pf[x] & 0x0f;
					
					/* just a guess; seems to mostly work */
					if (!(pfpix & 8) || !(overrender_matrix[mopriority] & (1 << pfcolor)))
						pf[x] = mo[x] & ATARIMO_DATA_MASK;
					
					/* erase behind ourselves */
					mo[x] = 0;
				}
		}

	/* add the alpha on top */
	tilemap_draw(bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);
}
