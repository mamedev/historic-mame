/***************************************************************************

	Atari System 1 hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Constants
 *
 *************************************/

/* the color and remap PROMs are mapped as follows */
#define PROM1_BANK_4			0x80		/* active low */
#define PROM1_BANK_3			0x40		/* active low */
#define PROM1_BANK_2			0x20		/* active low */
#define PROM1_BANK_1			0x10		/* active low */
#define PROM1_OFFSET_MASK		0x0f		/* postive logic */

#define PROM2_BANK_6_OR_7		0x80		/* active low */
#define PROM2_BANK_5			0x40		/* active low */
#define PROM2_PLANE_5_ENABLE	0x20		/* active high */
#define PROM2_PLANE_4_ENABLE	0x10		/* active high */
#define PROM2_PF_COLOR_MASK		0x0f		/* negative logic */
#define PROM2_BANK_7			0x08		/* active low, plus PROM2_BANK_6_OR_7 low as well */
#define PROM2_MO_COLOR_MASK		0x07		/* negative logic */



/*************************************
 *
 *	Globals we own
 *
 *************************************/

UINT8 atarisys1_translucent;



/*************************************
 *
 *	Statics
 *
 *************************************/

/* temporary bitmap */
static struct osd_bitmap *trans_bitmap_pf;
static struct osd_bitmap *trans_bitmap_mo;

/* playfield parameters */
static data16_t priority_pens;
static data16_t bankselect;

/* INT3 tracking */
static int next_timer_scanline;
static void *scanline_timer;
static void *int3off_timer;

/* graphics bank tracking */
static UINT8 bank_gfx[3][8];

/* basic form of a graphics bank */
static struct GfxLayout objlayout =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	6,		/* 6 bits per pixel */
	{ 5*8*0x08000, 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every sprite takes 8 consecutive bytes */
};



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void update_timers(int scanline);
static int decode_gfx(UINT16 *pflookup, UINT16 *molookup);
static int get_bank(UINT8 prom1, UINT8 prom2, int bpp);



/*************************************
 *
 *	Generic video system start
 *
 *************************************/

int atarisys1_vh_start(void)
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,64,		/* size of the playfield in tiles (x,y) */
		1,64,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x200,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x17fff,	/* tile data index mask */
		0,			/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		8,					/* number of motion object banks */
		1,					/* are the entries linked? */
		1,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0,0x003f }},	/* mask for the link */
		{{ 0,0xff00,0,0 }},	/* mask for the graphics bank */
		{{ 0,0xffff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0xff00,0,0 }},	/* mask for the color */
		{{ 0,0,0x3fe0,0 }},	/* mask for the X position */
		{{ 0x3fe0,0,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0x000f,0,0,0 }},	/* mask for the height, in tiles */
		{{ 0x8000,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x8000,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0,0xffff,0,0 }},	/* mask for the ignore value */
		0xffff,				/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	static const struct atarian_desc andesc =
	{
		0,			/* index to which gfx system */
		64,32,		/* size of the alpha RAM in tiles (x,y) */

		0x000,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* mask of the palette split */

		0x03ff,		/* tile data index mask */
		0x1c00,		/* tile data color mask */
		0,			/* tile data hflip mask */
		0x2000		/* tile data opacity mask */
	};

	UINT16 pftable[256], motable[256];
	UINT32 *pflookup;
	UINT16 *codelookup;
	UINT8 *colorlookup, *gfxlookup;
	int i, size;

	/* allocate the temp bitmap #1 */
	trans_bitmap_pf = bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 8);
	if (!trans_bitmap_pf)
		goto cant_alloc_bitmap_pf;

	/* allocate the temp bitmap #2 */
	trans_bitmap_mo = bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 8);
	if (!trans_bitmap_mo)
		goto cant_alloc_bitmap_mo;

	/* first decode the graphics */
	if (!decode_gfx(pftable, motable))
		goto cant_decode_gfx;

	/* initialize the alphanumerics */
	if (!atarian_init(0, &andesc))
		goto cant_create_an;

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* modify the playfield lookup table */
	pflookup = ataripf_get_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		int entry = (i << ATARIPF_LOOKUP_DATABITS) >> 8;
		int table = pftable[(entry & 0x7f) | ((entry >> 1) & 0x80)];
		int hflip = ATARIPF_LOOKUP_HFLIP(pflookup[i]);
		int code = (table & 0xff) << 8;
		int color = (table >> 12) & 15;
		int gfx = (table >> 8) & 15;
		pflookup[i] = ATARIPF_LOOKUP_ENTRY(gfx, code, color, hflip, 0, 0);
	}

	/* modify the motion object code lookup */
	codelookup = atarimo_get_code_lookup(0, &size);
	for (i = 0; i < size; i++)
		codelookup[i] = (i & 0xff) | ((motable[i >> 8] & 0xff) << 8);

	/* modify the motion object color and gfx lookups */
	colorlookup = atarimo_get_color_lookup(0, &size);
	gfxlookup = atarimo_get_gfx_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		colorlookup[i] = (motable[i] >> 12) & 15;
		gfxlookup[i] = (motable[i] >> 8) & 15;
	}

	/* reset the statics */
	atarimo_set_yscroll(0, 256, 0);
	next_timer_scanline = -1;
	scanline_timer = NULL;
	int3off_timer = NULL;
	return 0;

	/* error cases */
cant_create_mo:
	ataripf_free();
cant_create_pf:
	atarian_free();
cant_create_an:
cant_decode_gfx:
	bitmap_free(trans_bitmap_mo);
cant_alloc_bitmap_mo:
	bitmap_free(trans_bitmap_pf);
cant_alloc_bitmap_pf:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void atarisys1_vh_stop(void)
{
	atarian_free();
	atarimo_free();
	ataripf_free();
	bitmap_free(trans_bitmap_mo);
	bitmap_free(trans_bitmap_pf);
}



/*************************************
 *
 *	Graphics bank selection
 *
 *************************************/

WRITE16_HANDLER( atarisys1_bankselect_w )
{
	int oldselect = bankselect, diff;
	int scanline = cpu_getscanline();

	/* update memory */
	COMBINE_DATA(&bankselect);
	diff = oldselect ^ bankselect;

	/* sound CPU reset */
	if (diff & 0x0080)
	{
		cpu_set_reset_line(1, (bankselect & 0x0080) ? CLEAR_LINE : ASSERT_LINE);
		if (!(bankselect & 0x0080)) atarigen_sound_reset();
	}

	/* motion object bank select */
	atarimo_set_bank(0, (bankselect >> 3) & 7, scanline + 1);
	update_timers(scanline);

	/* playfield bank select */
	ataripf_set_bankbits(0, (bankselect & 0x04) << 14, scanline + 1);
}



/*************************************
 *
 *	Playfield priority pens
 *
 *************************************/

WRITE16_HANDLER( atarisys1_priority_w )
{
	COMBINE_DATA(&priority_pens);
}



/*************************************
 *
 *	Playfield horizontal scroll
 *
 *************************************/

WRITE16_HANDLER( atarisys1_hscroll_w )
{
	int oldscroll = ataripf_get_xscroll(0);
	int newscroll = oldscroll;
	COMBINE_DATA(&newscroll);
	ataripf_set_xscroll(0, newscroll & 0x1ff, cpu_getscanline() + 1);
}



/*************************************
 *
 *	Playfield vertical scroll
 *
 *************************************/

WRITE16_HANDLER( atarisys1_vscroll_w )
{
	int scanline = cpu_getscanline() + 1;
	int oldscroll = ataripf_get_yscroll(0);
	int newscroll = oldscroll;

	COMBINE_DATA(&newscroll);

	/* because this latches a new value into the scroll base,
	   we need to adjust for the scanline */
	if (scanline <= Machine->visible_area.max_y) newscroll -= scanline;
	ataripf_set_yscroll(0, newscroll & 0x1ff, scanline);
}



/*************************************
 *
 *	Sprite RAM write handler
 *
 *************************************/

WRITE16_HANDLER( atarisys1_spriteram_w )
{
	int oldword = atarimo_0_spriteram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	/* let the MO handler do the basic work */
	atarimo_0_spriteram_w(offset, data, 0);

	/* if the data changed, see if it affected a timer */
	if (oldword != newword)
	{
		/* if modifying a timer, beware */
		if (((offset & 0xc0) == 0x00 && atarimo_0_spriteram[offset | 0x40] == 0xffff) ||
		    ((offset & 0xc0) == 0x40 && (newword == 0xffff || oldword == 0xffff)))
		{
			/* if the timer is in the active bank, update the display list */
			if ((offset >> 8) == atarimo_get_bank(0))
				update_timers(cpu_getscanline());
		}
	}
}



/*************************************
 *
 *	MO interrupt handlers
 *
 *************************************/

static void int3off_callback(int param)
{
	/* clear the state */
	atarigen_scanline_int_ack_w(0, 0, 0);

	/* make this timer go away */
	int3off_timer = NULL;
}


static void int3_callback(int scanline)
{
	/* update the state */
	atarigen_scanline_int_gen();

	/* set a timer to turn it off */
	if (int3off_timer)
		timer_remove(int3off_timer);
	int3off_timer = timer_set(cpu_getscanlineperiod(), 0, int3off_callback);

	/* determine the time of the next one */
	scanline_timer = NULL;
	next_timer_scanline = -1;
	update_timers(scanline);
}



/*************************************
 *
 *	MO interrupt state read
 *
 *************************************/

READ16_HANDLER( atarisys1_int3state_r )
{
	return atarigen_scanline_int_state ? 0x0080 : 0x0000;
}



/*************************************
 *
 *	Timer updater
 *
 *************************************/

static void update_timers(int scanline)
{
	UINT16 *base = &atarimo_0_spriteram[atarimo_get_bank(0) * 64 * 4];
	int link = 0, best = scanline, found = 0;
	UINT8 spritevisit[64];

	/* track which ones we've visited */
	memset(spritevisit, 0, sizeof(spritevisit));

	/* walk the list until we loop */
	while (!spritevisit[link])
	{
		/* timers are indicated by 0xffff in entry 2 */
		if (base[link + 0x40] == 0xffff)
		{
			int data = base[link];
			int vsize = (data & 15) + 1;
			int ypos = (256 - (data >> 5) - vsize * 8 - 1) & 0x1ff;

			/* note that we found something */
			found = 1;

			/* is this a better entry than the best so far? */
			if (best <= scanline)
			{
				if ((ypos <= scanline && ypos < best) || ypos > scanline)
					best = ypos;
			}
			else
			{
				if (ypos < best)
					best = ypos;
			}
		}

		/* link to the next */
		spritevisit[link] = 1;
		link = base[link + 0xc0] & 0x3f;
	}

	/* if nothing was found, use scanline -1 */
	if (!found)
		best = -1;

	/* update the timer */
	if (best != next_timer_scanline)
	{
		next_timer_scanline = best;

		/* remove the old one */
		if (scanline_timer)
			timer_remove(scanline_timer);
		scanline_timer = NULL;

		/* set a new one */
		if (best != -1)
			scanline_timer = timer_set(cpu_getscanlinetime(best), best, int3_callback);
	}
}



/*************************************
 *
 *	Overrender callback
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	static struct osd_bitmap *real_dest;

	/* Rendering for the high-priority case here is tricky         */
	/* If the priority bit is set for an MO, then the MO/playfield */
	/* interaction is altered. Anywhere the MO pen is 0 or 1, the  */
	/* playfield gets priority. Anywhere the MO pen is 2-15, the   */
	/* color is determined via the translucency color map at 0x300 */

	/* handle the startup case */
	if (state == OVERRENDER_BEGIN)
	{
		/* low priority case */
		if (!data->mopriority)
		{
			/* if there are no priority pens, do nothing */
			if (!priority_pens)
				return OVERRENDER_NONE;

			/* otherwise, we need to handle it tile-by-tile */
			data->drawmode = TRANSPARENCY_PENS;
			data->drawpens = ~priority_pens;
			data->maskpens = 0x0001;
			return OVERRENDER_SOME;
		}

		/* translucent case: here we end up blending the low 4 bits of the MO */
		/* with the low 4 bits of the playfield and reading from the 256 palette */
		/* entries at 0x300. Since we already have a raw copy of the MO in the */
		/* priority buffer, we just need to blend the raw playfield bits into it */
		/* and then copy the result through the color table */
		else
		{
			/* special case: if all the MO pens are 0 or 1, we just need to  */
			/* handle the high priority playfield case, with no translucency */
			if (!(data->mousage & ~3))
			{
				data->drawmode = TRANSPARENCY_NONE;
				data->drawpens = 0;
				data->maskpens = 0x0001;
				return OVERRENDER_ALL;
			}

			/* save the real destination bitmap for later, and replace it */
			/* with the transparency bitmap */
			real_dest = data->bitmap;
			data->bitmap = trans_bitmap_pf;

			/* draw in raw pens to the playfield translucency bitmap */
			data->drawmode = TRANSPARENCY_NONE_RAW;
			data->drawpens = 0;
			data->maskpens = 0;

			/* and then handle it tile-by-tile */
			return OVERRENDER_SOME;
		}
	}

	/* handle queries */
	else if (state == OVERRENDER_QUERY)
	{
		/* low priority case */
		if (!data->mopriority)
			return data->pfcolor ? OVERRENDER_NO : OVERRENDER_YES;

		/* translucent case */
		data->pfcolor = 0;
		return OVERRENDER_YES;
	}

	/* handle the final overdrawing (translucent case only) */
	else if (state == OVERRENDER_FINISH)
	{
		struct GfxElement dummygfx;

		/* bail if this isn't the translucent case */
		if (!data->mopriority || !(data->mousage & ~3))
			return 0;

		/* first copy the raw pens from the priority map into our tranlucency bitmap */
		copybitmap(trans_bitmap_mo, priority_bitmap, 0, 0, 0, 0, &data->clip, TRANSPARENCY_NONE, 0);

		/* now blend in the playfield */
		copybitmap(trans_bitmap_mo, trans_bitmap_pf, 0, 0, 0, 0, &data->clip, TRANSPARENCY_BLEND_RAW, 4);

		/* make a dummy GfxElement to draw from; we can't use copybitmap because */
		/* it assumes that the source and dest are the same depth */
		dummygfx.width = data->clip.max_x - data->clip.min_x + 1;
		dummygfx.height = data->clip.max_y - data->clip.min_y + 1;
		dummygfx.total_elements = 1;
		dummygfx.color_granularity = 1;
		dummygfx.colortable = &Machine->remapped_colortable[0x300];
		dummygfx.total_colors = 1;
		dummygfx.pen_usage = NULL;
		dummygfx.gfxdata = &trans_bitmap_mo->line[data->clip.min_y][data->clip.min_x];
		dummygfx.line_modulo = trans_bitmap_mo->line[1] - trans_bitmap_mo->line[0];
		dummygfx.char_modulo = 0;
		pdrawgfx(real_dest, &dummygfx, 0, 0, 0, 0,
				data->clip.min_x, data->clip.min_y, &data->clip, TRANSPARENCY_NONE, 0, 0x0003);

		/* if we also need to handle straight playfield priority, do that */
		if (data->mousage & 0x0002)
		{
			dummygfx.colortable = &Machine->remapped_colortable[0x200];
			dummygfx.gfxdata = &trans_bitmap_pf->line[data->clip.min_y][data->clip.min_x];
			dummygfx.line_modulo = trans_bitmap_pf->line[1] - trans_bitmap_pf->line[0];
			pdrawgfx(real_dest, &dummygfx, 0, 0, 0, 0,
					data->clip.min_x, data->clip.min_y, &data->clip, TRANSPARENCY_NONE, 0, ~0x0002);
		}
	}
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void atarisys1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	ataripf_mark_palette(0);
	atarimo_mark_palette(0);
	atarian_mark_palette(0);
	memset(&palette_used_colors[0x300], PALETTE_COLOR_USED, 256);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
		ataripf_invalidate(0);

	/* draw the layers */
	ataripf_render(0, bitmap);
	atarimo_render(0, bitmap, overrender_callback, NULL);
	atarian_render(0, bitmap);
}



/*************************************
 *
 *	Graphics decoding
 *
 *************************************/

static int decode_gfx(UINT16 *pflookup, UINT16 *molookup)
{
	UINT8 *prom1 = &memory_region(REGION_PROMS)[0x000];
	UINT8 *prom2 = &memory_region(REGION_PROMS)[0x200];
	int obj, i;

	/* reset the globals */
	memset(&bank_gfx[0][0], 0, sizeof(bank_gfx));

	/* loop for two sets of objects */
	for (obj = 0; obj < 2; obj++)
	{
		/* loop for 256 objects in the set */
		for (i = 0; i < 256; i++, prom1++, prom2++)
		{
			int bank, bpp, color, offset;

			/* determine the bpp */
			bpp = 4;
			if (*prom2 & PROM2_PLANE_4_ENABLE)
			{
				bpp = 5;
				if (*prom2 & PROM2_PLANE_5_ENABLE)
					bpp = 6;
			}

			/* determine the offset */
			offset = *prom1 & PROM1_OFFSET_MASK;

			/* determine the bank */
			bank = get_bank(*prom1, *prom2, bpp);
			if (bank < 0)
				return 0;

			/* set the value */
			if (obj == 0)
			{
				/* playfield case */
				color = (~*prom2 & PROM2_PF_COLOR_MASK) >> (bpp - 4);
				if (bank == 0)
				{
					bank = 1;
					offset = color = 0;
				}
				pflookup[i] = offset | (bank << 8) | (color << 12);
			}
			else
			{
				/* motion objects (high bit ignored) */
				color = (~*prom2 & PROM2_MO_COLOR_MASK) >> (bpp - 4);
				molookup[i] = offset | (bank << 8) | (color << 12);
			}
		}
	}
	return 1;
}



/*************************************
 *
 *	Graphics bank mapping
 *
 *************************************/

static int get_bank(UINT8 prom1, UINT8 prom2, int bpp)
{
	int bank_offset[8] = { 0, 0x00000, 0x30000, 0x60000, 0x90000, 0xc0000, 0xe0000, 0x100000 };
	int bank_index, i, gfx_index;

	/* determine the bank index */
	if ((prom1 & PROM1_BANK_1) == 0)
		bank_index = 1;
	else if ((prom1 & PROM1_BANK_2) == 0)
		bank_index = 2;
	else if ((prom1 & PROM1_BANK_3) == 0)
		bank_index = 3;
	else if ((prom1 & PROM1_BANK_4) == 0)
		bank_index = 4;
	else if ((prom2 & PROM2_BANK_5) == 0)
		bank_index = 5;
	else if ((prom2 & PROM2_BANK_6_OR_7) == 0)
	{
		if ((prom2 & PROM2_BANK_7) == 0)
			bank_index = 7;
		else
			bank_index = 6;
	}
	else
		return 0;

	/* find the bank */
	if (bank_gfx[bpp - 4][bank_index])
		return bank_gfx[bpp - 4][bank_index];

	/* if the bank is out of range, call it 0 */
	if (bank_offset[bank_index] >= memory_region_length(REGION_GFX2))
		return 0;

	/* don't have one? let's make it ... first find any empty slot */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == NULL)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return -1;

	/* tweak the structure for the number of bitplanes we have */
	objlayout.planes = bpp;
	for (i = 0; i < bpp; i++)
		objlayout.planeoffset[i] = (bpp - i - 1) * 0x8000 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(&memory_region(REGION_GFX2)[bank_offset[bank_index]], &objlayout);
	if (!Machine->gfx[gfx_index])
		return -1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = &Machine->remapped_colortable[256];
	Machine->gfx[gfx_index]->total_colors = 48 >> (bpp - 4);

	/* set the entry and return it */
	return bank_gfx[bpp - 4][bank_index] = gfx_index;
}
