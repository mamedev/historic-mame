/***************************************************************************

	Atari ThunderJaws hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Globals
 *
 *************************************/

UINT8 thunderj_alpha_tile_bank;



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha[tile_index];
	int code = ((data & 0x200) ? (thunderj_alpha_tile_bank * 0x200) : 0) + (data & 0x1ff);
	int color = ((data >> 10) & 0x0f) | ((data >> 9) & 0x20);
	int opaque = data & 0x8000;
	SET_TILE_INFO(2, code, color, opaque ? TILE_IGNORE_TRANSPARENCY : 0);
}


static void get_playfield_tile_info(int tile_index)
{
	UINT16 data1 = atarigen_playfield[tile_index];
	UINT16 data2 = atarigen_playfield_upper[tile_index] & 0xff;
	int code = data1 & 0x7fff;
	int color = 0x10 + (data2 & 0x0f);
	SET_TILE_INFO(0, code, color, (data1 >> 15) & 1);
	tile_info.priority = (data2 >> 4) & 3;
}


static void get_playfield2_tile_info(int tile_index)
{
	UINT16 data1 = atarigen_playfield2[tile_index];
	UINT16 data2 = atarigen_playfield_upper[tile_index] >> 8;
	int code = data1 & 0x7fff;
	int color = data2 & 0x0f;
	SET_TILE_INFO(0, code, color, (data1 >> 15) & 1);
	tile_info.priority = (data2 >> 4) & 3;
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( thunderj )
{
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
		0,					/* pixel offset for SLIPs */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
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
		{{ 0,0,0x0070,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the special value */
		0,					/* resulting value to indicate "special" */
		NULL				/* callback routine for special entries */
	};

	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 8,8, 64,64);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* initialize the second playfield */
	atarigen_playfield2_tilemap = tilemap_create(get_playfield2_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8,8, 64,64);
	if (!atarigen_playfield2_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_playfield2_tilemap, 0);

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		return 1;

	/* initialize the alphanumerics */
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32);
	if (!atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);

	return 0;
}



/*************************************
 *
 *	Mark high palette bits starting
 *	at the given X,Y and continuing
 *	until a stop or the end of line
 *
 *************************************/
 
void thunderj_mark_high_palette(struct mame_bitmap *bitmap, UINT16 *pf, UINT16 *mo, int x, int y)
{
	#define END_MARKER	((4 << ATARIMO_PRIORITY_SHIFT) | 4)
	
	/* advance forward to the X position given */
	pf += x;
	mo += x;
	
	/* handle non-swapped case */
	if (!(Machine->orientation & ORIENTATION_SWAP_XY))
	{
		/* standard orientation: move forward along X */
		if (!(Machine->orientation & ORIENTATION_FLIP_X))
		{
			for ( ; x < bitmap->width && (*mo & END_MARKER) != END_MARKER; mo++, pf++, x++)
				*pf |= 0x400;
		}

		/* flipped orientation: move backward along X */
		else
		{
			for ( ; x >= 0 && (*mo & END_MARKER) != END_MARKER; mo--, pf--, x--)
				*pf |= 0x400;
		}
	}
	
	/* handle swapped case */
	else
	{
		/* standard orientation: move forward along Y */
		if (!(Machine->orientation & ORIENTATION_FLIP_Y))
		{
			for ( ; (*mo & END_MARKER) != END_MARKER && y < bitmap->height; mo += bitmap->rowpixels, pf += bitmap->rowpixels, y++)
				*pf |= 0x400;
		}

		/* flipped orientation: move backward along Y */
		else
		{
			for ( ; (*mo & END_MARKER) != END_MARKER && y >= 0; mo -= bitmap->rowpixels, pf -= bitmap->rowpixels, y--)
				*pf |= 0x400;
		}
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/
 
VIDEO_UPDATE( thunderj )
{
	static const UINT16 transparency_mask[4] =
	{
		0xffff,
		0x00ff,
		0x00ff,
		0x00ff
	};
	struct atarimo_rect_list rectlist;
	struct mame_bitmap *mobitmap;
	int x, y, r;

	/* draw the playfield */
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 0, 0x00);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 1, 0x01);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 2, 0x02);
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 3, 0x03);
	tilemap_draw(bitmap, cliprect, atarigen_playfield2_tilemap, 0, 0x80);
	tilemap_draw(bitmap, cliprect, atarigen_playfield2_tilemap, 1, 0x84);
	tilemap_draw(bitmap, cliprect, atarigen_playfield2_tilemap, 2, 0x88);
	tilemap_draw(bitmap, cliprect, atarigen_playfield2_tilemap, 3, 0x8c);
	
	/* draw and merge the MO */
	mobitmap = atarimo_render(0, cliprect, &rectlist);
	for (r = 0; r < rectlist.numrects; r++, rectlist.rect++)
		for (y = rectlist.rect->min_y; y <= rectlist.rect->max_y; y++)
		{
			UINT16 *mo = (UINT16 *)mobitmap->base + mobitmap->rowpixels * y;
			UINT16 *pf = (UINT16 *)bitmap->base + bitmap->rowpixels * y;
			UINT8 *pri = (UINT8 *)priority_bitmap->base + priority_bitmap->rowpixels * y;
			for (x = rectlist.rect->min_x; x <= rectlist.rect->max_x; x++)
				if (mo[x])
				{
					/* not yet verified; all logic controlled via PAL
					*/
					int mopriority = mo[x] >> ATARIMO_PRIORITY_SHIFT;
					
					/* upper bit of MO priority signals special rendering and doesn't draw anything */
					if (mopriority & 4)
						continue;

					/* MO priority 3 always displays */
					if (mopriority == 3)
						pf[x] = mo[x] & ATARIMO_DATA_MASK;
					
					/* MO priority 0, color 0, pen 1 is playfield priority */
					else if (mopriority == 0 && (mo[x] & 0xf0) == 0)
					{
						if ((mo[x] & 0x0f) != 1)
							pf[x] = mo[x] & ATARIMO_DATA_MASK;
					}
					
					/* everything else depends on the playfield priority */
					else
					{
						int pfpriority = (pri[x] & 0x80) ? ((pri[x] >> 2) & 3) : (pri[x] & 3);
						
						if (mopriority >= pfpriority)
							pf[x] = mo[x] & ATARIMO_DATA_MASK;
						else if (transparency_mask[pfpriority] & (1 << (pf[x] & 0x0f)))
							pf[x] = mo[x] & ATARIMO_DATA_MASK;
					}
					
					/* don't erase yet -- we need to make another pass later */
				}
		}
	
	/* add the alpha on top */
	tilemap_draw(bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);

	/* now go back and process the upper bit of MO priority */
	rectlist.rect -= rectlist.numrects;
	for (r = 0; r < rectlist.numrects; r++, rectlist.rect++)
		for (y = rectlist.rect->min_y; y <= rectlist.rect->max_y; y++)
		{
			UINT16 *mo = (UINT16 *)mobitmap->base + mobitmap->rowpixels * y;
			UINT16 *pf = (UINT16 *)bitmap->base + bitmap->rowpixels * y;
			for (x = rectlist.rect->min_x; x <= rectlist.rect->max_x; x++)
				if (mo[x])
				{
					int mopriority = mo[x] >> ATARIMO_PRIORITY_SHIFT;
					
					/* upper bit of MO priority might mean palette kludges */
					if (mopriority & 4)
					{
						/* if bit 2 is set, start setting high palette bits */
						if (mo[x] & 2)
							thunderj_mark_high_palette(bitmap, pf, mo, x, y);
					}
					
					/* erase behind ourselves */
					mo[x] = 0;
				}
		}
}
