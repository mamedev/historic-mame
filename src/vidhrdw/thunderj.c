/***************************************************************************

	Atari ThunderJaws hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT32 *start_end;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void special_callback(struct osd_bitmap *bitmap, struct rectangle *clip, int code, int color, int xpos, int ypos);



/*************************************
 *
 *	Video system start
 *
 *************************************/

int thunderj_vh_start(void)
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
		
		{{ 0,0,0x0040,0 }},	/* mask for the ignore value */
		1,					/* resulting value to indicate "ignore" */
		special_callback	/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		2,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */
	
		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0x00f,		/* mask of the palette split */

		0x703ff,	/* tile data index mask */
		0x07c00,	/* tile data color mask */
		0,			/* tile data hflip mask */
		0x08000		/* tile data opacity mask */
	};

	UINT32 *pflookup, *anlookup;
	int i, size;
	
	/* allocate temp memory */
	start_end = malloc(sizeof(UINT32) * 512);
	if (!start_end)
		goto cant_allocate_startend;

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
		int code = entry & 0x1ff;
		if (entry & 0x200)
			code += (entry >> 16) * 0x200;
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
	free(start_end);
cant_allocate_startend:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void thunderj_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
	free(start_end);
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

void thunderj_scanline_update(int scanline)
{
	/* check for a new palette bank */
	int new_bank = atarivc_state.palette_bank;
	ataripf_set_bankbits(1, new_bank << 22, scanline);
	atarimo_set_palettebase(0, new_bank * 0x400 + 0x100, scanline);
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static const UINT16 transparency_mask[4] =
{
	0xffff, 
	0x00ff, 
	0x00ff, 
	0x00ff
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
 *	Special case rendering
 *
 *************************************/

static void special_callback(struct osd_bitmap *bitmap, struct rectangle *clip, int code, int color, int xpos, int ypos)
{
	struct GfxElement *gfx = Machine->gfx[1];
	UINT32 temp = start_end[ypos & 0x1ff];
	UINT32 start = temp >> 16;
	UINT32 stop = temp & 0xffff;
	
	/* update the data */
	if (code == 2)
		start = xpos & 0x1ff;
	else if (code == 4)
		stop = xpos & 0x1ff;
	start_end[ypos & 0x1ff] = (start << 16) | stop;
	
	/* render if complete */
	if (start != 0xffff && stop != 0xffff)
	{
		struct rectangle temp_clip = *clip;
		int x;
		
		/* adjust coordinates */
		if (start >= Machine->visible_area.max_x) start -= 0x200;
		if (start > stop) stop += 0x200;

		/* set up a clipper */
		temp_clip.min_x = (start < temp_clip.min_x) ? temp_clip.min_x : start;
		temp_clip.max_x = (stop > temp_clip.max_x) ? temp_clip.max_x : stop;
		
		/* draw it */
		for (x = start; x < stop; x += 8)
			drawgfx(bitmap, gfx, 2, color, 0, 0, x, ypos, &temp_clip, TRANSPARENCY_PEN, 0);
		start_end[ypos & 0x1ff] = 0xffffffff;
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void thunderj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	ataripf_mark_palette(0);
	ataripf_mark_palette(1);
	atarimo_mark_palette(0);
	atarian_mark_palette(0);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
	{
		ataripf_invalidate(0);
		ataripf_invalidate(1);
	}

	/* draw the layers */
	ataripf_render(0, bitmap);
	ataripf_render(1, bitmap);
	atarimo_render(0, bitmap, overrender0_callback, overrender1_callback);
	atarian_render(0, bitmap);
}
