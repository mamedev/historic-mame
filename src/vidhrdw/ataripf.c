/*##########################################################################

	ataripf.c

	Common playfield management functions for Atari raster games.

##########################################################################*/

#include "driver.h"
#include "ataripf.h"



/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* internal state structure containing values that can change scanline-by-scanline */
struct ataripf_state
{
	int					scanline;			/* scanline where we are valid */
	int					xscroll;			/* xscroll value */
	int					yscroll;			/* yscroll value */
	int					bankbits;			/* bank bits */
};


/* internal variant of the gfxelement that contains extra data */
struct ataripf_gfxelement
{
	struct GfxElement		element;
	int						initialized;
	struct ataripf_usage *	usage;
	int						usage_words;
	int						colorshift;
};


/* internal structure containing the state of a playfield */
struct ataripf_data
{
	int					initialized;		/* true if we're initialized */
	int					timerallocated;		/* true if we've allocated the timer */
	int					gfxchanged;			/* true if the gfx info has changed */

	int					colshift;			/* bits to shift X coordinate when looking up in VRAM */
	int 				rowshift;			/* bits to shift Y coordinate when looking up in VRAM */
	int					colmask;			/* mask to use when wrapping X coordinate in VRAM */
	int 				rowmask;			/* mask to use when wrapping Y coordinate in VRAM */
	int					vrammask;			/* combined mask when accessing VRAM with raw addresses */
	int					vramsize;			/* total size of VRAM, in entries */

	int					tilexshift;			/* bits to shift X coordinate when drawing */
	int					tileyshift;			/* bits to shift Y coordinate when drawing */
	int					tilewidth;			/* width of a single tile */
	int					tileheight;			/* height of a single tile */
	int					bitmapwidth;		/* width of the full playfield bitmap */
	int					bitmapheight;		/* height of the full playfield bitmap */
	int					bitmapxmask;		/* x coordinate mask for the playfield bitmap */
	int					bitmapymask;		/* y coordinate mask for the playfield bitmap */

	int					palettebase;		/* base palette entry */
	int					maxcolors;			/* maximum number of colors */
	int					shadowxor;			/* color XOR for shadow effect (if any) */
	UINT32				transpens;			/* transparent pen */
	int					transpen;			/* transparent pen */

	int					lookupmask;			/* mask for the lookup table */

	int					latchval;			/* value for latching */
	int					latchdata;			/* shifted value for latching */
	int					latchmask;			/* mask for latching */

	struct osd_bitmap *	bitmap;				/* backing bitmap */
	UINT32 *			vram;				/* pointer to VRAM */
	UINT32 *			dirtymap;			/* dirty bitmap */
	UINT8 *				visitmap;			/* visiting bitmap */
	UINT32 *			lookup;				/* pointer to lookup table */

	struct ataripf_state curstate;			/* current state */
	struct ataripf_state *statelist;		/* list of changed states */
	int					stateindex;			/* index of the next state */

	struct rectangle	process_clip;		/* (during processing) the clip rectangle */
	struct rectangle	process_tiles;		/* (during processing) the tiles rectangle */
	void *				process_param;		/* (during processing) the callback parameter */

	struct ataripf_gfxelement gfxelement[MAX_GFX_ELEMENTS]; /* graphics element copies */
	int 				max_usage_words;	/* maximum words of usage */
};


/* callback function for the internal playfield processing mechanism */
typedef void (*pf_callback)(struct ataripf_data *pf, const struct ataripf_state *state);



/*##########################################################################
	MACROS
##########################################################################*/

/* verification macro for void functions */
#define VERIFY(cond, msg) if (!(cond)) { logerror(msg); return; }

/* verification macro for non-void functions */
#define VERIFYRETFREE(cond, msg, ret) if (!(cond)) { logerror(msg); ataripf_free(); return (ret); }


/* accessors for upper/lower halves of a 32-bit value */
#if LSB_FIRST
#define LOWER_HALF(x) ((data16_t *)&(x))[0]
#define UPPER_HALF(x) ((data16_t *)&(x))[1]
#else
#define LOWER_HALF(x) ((data16_t *)&(x))[1]
#define UPPER_HALF(x) ((data16_t *)&(x))[0]
#endif



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

data16_t *ataripf_0_base;
data16_t *ataripf_0_upper;

data16_t *ataripf_1_base;

data32_t *ataripf_0_base32;



/*##########################################################################
	STATIC VARIABLES
##########################################################################*/

static struct ataripf_data ataripf[ATARIPF_MAX];

static ataripf_overrender_cb overrender_callback;
static struct ataripf_overrender_data overrender_data;



/*##########################################################################
	STATIC FUNCTION DECLARATIONS
##########################################################################*/

static void pf_process(struct ataripf_data *pf, pf_callback callback, void *param, const struct rectangle *clip);
static void pf_usage_callback_1(struct ataripf_data *pf, const struct ataripf_state *state);
static void pf_usage_callback_2(struct ataripf_data *pf, const struct ataripf_state *state);
static void pf_render_callback(struct ataripf_data *pf, const struct ataripf_state *state);
static void pf_overrender_callback(struct ataripf_data *pf, const struct ataripf_state *state);
static void pf_eof_callback(int map);
static void pf_init_gfx(struct ataripf_data *pf, int gfxindex);



/*##########################################################################
	INLINE FUNCTIONS
##########################################################################*/

/*---------------------------------------------------------------
	compute_log: Computes the number of bits necessary to
	hold a given value. The input must be an even power of
	two.
---------------------------------------------------------------*/

INLINE int compute_log(int value)
{
	int log = 0;

	if (value == 0)
		return -1;
	while (!(value & 1))
		log++, value >>= 1;
	if (value != 1)
		return -1;
	return log;
}


/*---------------------------------------------------------------
	round_to_powerof2: Rounds a number up to the nearest
	power of 2. Even powers of 2 are rounded up to the
	next greatest power (e.g., 4 returns 8).
---------------------------------------------------------------*/

INLINE int round_to_powerof2(int value)
{
	int log = 0;

	if (value == 0)
		return 1;
	while ((value >>= 1) != 0)
		log++;
	return 1 << (log + 1);
}


/*---------------------------------------------------------------
	collapse_bits: Moving right-to-left, for each 1 bit in
	the mask, copy the corresponding bit from the input
	value into the result, packing the bits along the way.
---------------------------------------------------------------*/

INLINE int collapse_bits(int value, int mask)
{
	int testmask, ormask;
	int result = 0;

	for (testmask = ormask = 1; testmask != 0; testmask <<= 1)
		if (mask & testmask)
		{
			if (value & testmask)
				result |= ormask;
			ormask <<= 1;
		}
	return result;
}


/*---------------------------------------------------------------
	pf_update_state: Internal routine that updates the
	state list of the playfield with the current parameters.
---------------------------------------------------------------*/

INLINE void pf_update_state(struct ataripf_data *pf, int scanline)
{
	struct ataripf_state *state = &pf->statelist[pf->stateindex];

	/* ignore anything after the bottom of the visible screen */
	if (scanline > Machine->visible_area.max_y)
		return;

	/* ignore anything earlier than the last scanline we entered */
	if (state[-1].scanline > scanline)
	{
		logerror("pf_update_state: Attempted state update on prior scanline (%d vs. %d)\n", scanline, state[-1]);
		return;
	}

	/* if this is the same scanline as last time, overwrite it */
	else if (state[-1].scanline == scanline)
	{
		logerror("pf_update_state: scanlines equal, overwriting\n");
		state--;
	}

	/* otherwise, move forward one entry */
	else
	{
		logerror("pf_update_state: new entry\n");
		pf->stateindex++;
	}

	/* fill in the data */
	*state = pf->curstate;
	state->scanline = scanline;
}



/*##########################################################################
	GLOBAL FUNCTIONS
##########################################################################*/

/*---------------------------------------------------------------
	ataripf_blend_gfx: Takes two GFXElements and blends their
	data together to form one. Then frees the second.
---------------------------------------------------------------*/

void ataripf_blend_gfx(int gfx0, int gfx1, int mask0, int mask1)
{
	struct GfxElement *gx0 = Machine->gfx[gfx0];
	struct GfxElement *gx1 = Machine->gfx[gfx1];
	int c, x, y;

	/* loop over elements */
	for (c = 0; c < gx0->total_elements; c++)
	{
		UINT8 *c0base = gx0->gfxdata + gx0->char_modulo * c;
		UINT8 *c1base = gx1->gfxdata + gx1->char_modulo * c;
		UINT32 usage = 0;

		/* loop over height */
		for (y = 0; y < gx0->height; y++)
		{
			UINT8 *c0 = c0base, *c1 = c1base;

			for (x = 0; x < gx0->width; x++, c0++, c1++)
			{
				*c0 = (*c0 & mask0) | (*c1 & mask1);
				usage |= 1 << *c0;
			}
			c0base += gx0->line_modulo;
			c1base += gx1->line_modulo;
			if (gx0->pen_usage)
				gx0->pen_usage[c] = usage;
		}
	}

	/* free the second graphics element */
	freegfx(gx1);
	Machine->gfx[gfx1] = NULL;
}


/*---------------------------------------------------------------
	ataripf_init: Configures the playfield using the input
	description. Allocates all memory necessary and generates
	the attribute lookup table. If custom_lookup is provided,
	it is used in place of the generated attribute table.
---------------------------------------------------------------*/

int ataripf_init(int map, const struct ataripf_desc *desc)
{
	int lookupcount = round_to_powerof2(desc->tilemask | desc->colormask | desc->hflipmask | desc->vflipmask | desc->prioritymask) >> ATARIPF_LOOKUP_DATABITS;
	struct GfxElement *gfx = Machine->gfx[desc->gfxindex];
	struct ataripf_data *pf = &ataripf[map];
	int i;

	/* sanity checks */
	VERIFYRETFREE(map >= 0 && map < ATARIPF_MAX, "ataripf_init: map out of range", 0)
	VERIFYRETFREE(compute_log(desc->cols) != -1, "ataripf_init: cols must be power of 2", 0)
	VERIFYRETFREE(compute_log(desc->rows) != -1, "ataripf_init: rows must be power of 2", 0)
	VERIFYRETFREE(compute_log(desc->xmult) != -1, "ataripf_init: xmult must be power of 2", 0)
	VERIFYRETFREE(compute_log(desc->ymult) != -1, "ataripf_init: ymult must be power of 2", 0)
	VERIFYRETFREE((desc->tilemask & ATARIPF_LOOKUP_DATAMASK) == ATARIPF_LOOKUP_DATAMASK, "ataripf_init: low bits of tilemask must be 0xff", 0)

	/* copy in the basic data */
	pf->initialized  = 0;
	pf->timerallocated = 0;
	pf->gfxchanged   = 0;

	pf->colshift     = compute_log(desc->xmult);
	pf->rowshift     = compute_log(desc->ymult);
	pf->colmask      = desc->cols - 1;
	pf->rowmask      = desc->rows - 1;
	pf->vrammask     = (pf->colmask << pf->colshift) | (pf->rowmask << pf->rowshift);
	pf->vramsize     = round_to_powerof2(pf->vrammask);

	pf->tilexshift   = compute_log(gfx->width);
	pf->tileyshift   = compute_log(gfx->height);
	pf->tilewidth    = gfx->width;
	pf->tileheight   = gfx->height;
	pf->bitmapwidth  = desc->cols * gfx->width;
	pf->bitmapheight = desc->rows * gfx->height;
	pf->bitmapxmask  = pf->bitmapwidth - 1;
	pf->bitmapymask  = pf->bitmapheight - 1;

	pf->palettebase  = desc->palettebase;
	pf->maxcolors    = desc->maxcolors / ATARIPF_BASE_GRANULARITY;
	pf->shadowxor    = desc->shadowxor;
	pf->transpens    = desc->transpens;
	pf->transpen     = desc->transpens ? compute_log(desc->transpens) : -1;

	pf->lookupmask   = lookupcount - 1;

	pf->latchval     = 0;
	pf->latchdata    = -1;
	pf->latchmask    = desc->latchmask;

	/* allocate the backing bitmap */
	pf->bitmap = bitmap_alloc(pf->bitmapwidth, pf->bitmapheight);
	VERIFYRETFREE(pf->bitmap, "ataripf_init: out of memory for bitmap", 0)

	/* allocate the vram */
	pf->vram = malloc(sizeof(pf->vram[0]) * pf->vramsize);
	VERIFYRETFREE(pf->vram, "ataripf_init: out of memory for vram", 0)

	/* clear it to zero */
	memset(pf->vram, 0, sizeof(pf->vram[0]) * pf->vramsize);

	/* allocate the dirty map */
	pf->dirtymap = malloc(sizeof(pf->dirtymap[0]) * pf->vramsize);
	VERIFYRETFREE(pf->dirtymap, "ataripf_init: out of memory for dirtymap", 0)

	/* mark everything dirty */
	memset(pf->dirtymap, -1, sizeof(pf->dirtymap[0]) * pf->vramsize);

	/* allocate the visitation map */
	pf->visitmap = malloc(sizeof(pf->visitmap[0]) * pf->vramsize);
	VERIFYRETFREE(pf->visitmap, "ataripf_init: out of memory for visitmap", 0)

	/* mark everything non-visited */
	memset(pf->visitmap, 0, sizeof(pf->visitmap[0]) * pf->vramsize);

	/* allocate the attribute lookup */
	pf->lookup = malloc(lookupcount * sizeof(pf->lookup[0]));
	VERIFYRETFREE(pf->lookup, "ataripf_init: out of memory for lookup", 0)

	/* fill in the attribute lookup */
	for (i = 0; i < lookupcount; i++)
	{
		int value    = (i << ATARIPF_LOOKUP_DATABITS);
		int tile     = collapse_bits(value, desc->tilemask);
		int color    = collapse_bits(value, desc->colormask);
		int hflip    = collapse_bits(value, desc->hflipmask);
		int vflip    = collapse_bits(value, desc->vflipmask);
		int priority = collapse_bits(value, desc->prioritymask);

		pf->lookup[i] = ATARIPF_LOOKUP_ENTRY(desc->gfxindex, tile, color, hflip, vflip, priority);
	}

	/* compute the extended usage map */
	pf_init_gfx(pf, desc->gfxindex);
	VERIFYRETFREE(pf->gfxelement[desc->gfxindex].initialized, "ataripf_init: out of memory for extra usage map", 0)

	/* allocate the state list */
	pf->statelist = malloc(pf->bitmapheight * sizeof(pf->statelist[0]));
	VERIFYRETFREE(pf->statelist, "ataripf_init: out of memory for extra state list", 0)

	/* reset the state list */
	memset(&pf->curstate, 0, sizeof(pf->curstate));
	pf->statelist[0] = pf->curstate;
	pf->stateindex = 1;

	pf->initialized = 1;

	logerror("ataripf_init:\n");
	logerror("  width=%d (shift=%d),  height=%d (shift=%d)\n", gfx->width, pf->tilexshift, gfx->height, pf->tileyshift);
	logerror("  cols=%d  (mask=%X),   rows=%d   (mask=%X)\n", desc->cols, pf->colmask, desc->rows, pf->rowmask);
	logerror("  xmult=%d (shift=%d),  ymult=%d  (shift=%d)\n", desc->xmult, pf->colshift, desc->ymult, pf->rowshift);
	logerror("  VRAM mask=%X,  dirtymap size=%d\n", pf->vrammask, pf->vramsize);
	logerror("  bitmap size=%dx%d\n", pf->bitmapwidth, pf->bitmapheight);

	return 1;
}


/*---------------------------------------------------------------
	ataripf_free: Frees any memory allocated for any playfield.
---------------------------------------------------------------*/

void ataripf_free(void)
{
	int i;

	/* free the playfield data */
	for (i = 0; i < ATARIPF_MAX; i++)
	{
		struct ataripf_data *pf = &ataripf[i];

		/* free the backing bitmap */
		if (pf->bitmap)
			bitmap_free(pf->bitmap);
		pf->bitmap = NULL;

		/* free the vram */
		if (pf->vram)
			free(pf->vram);
		pf->vram = NULL;

		/* free the dirty map */
		if (pf->dirtymap)
			free(pf->dirtymap);
		pf->dirtymap = NULL;

		/* free the visitation map */
		if (pf->visitmap)
			free(pf->visitmap);
		pf->visitmap = NULL;

		/* free the attribute lookup */
		if (pf->lookup)
			free(pf->lookup);
		pf->lookup = NULL;

		/* free the state list */
		if (pf->statelist)
			free(pf->statelist);
		pf->statelist = NULL;

		/* free the extended usage maps */
		for (i = 0; i < MAX_GFX_ELEMENTS; i++)
			if (pf->gfxelement[i].usage)
			{
				free(pf->gfxelement[i].usage);
				pf->gfxelement[i].usage = NULL;
				pf->gfxelement[i].initialized = 0;
			}

		pf->initialized = 0;
	}
}


/*---------------------------------------------------------------
	ataripf_get_lookup: Fetches the lookup table so it can
	be modified.
---------------------------------------------------------------*/

UINT32 *ataripf_get_lookup(int map, int *size)
{
	ataripf[map].gfxchanged = 1;
	if (size)
		*size = round_to_powerof2(ataripf[map].lookupmask);
	return ataripf[map].lookup;
}


/*---------------------------------------------------------------
	ataripf_invalidate: Marks all tiles in the playfield
	dirty. This must be called when the palette changes.
---------------------------------------------------------------*/

void ataripf_invalidate(int map)
{
	struct ataripf_data *pf = &ataripf[map];
	if (pf->initialized)
		memset(pf->dirtymap, -1, sizeof(pf->dirtymap[0]) * pf->vramsize);
}


/*---------------------------------------------------------------
	ataripf_render: Render the playfield, updating any dirty
	blocks, and copy it to the destination bitmap.
---------------------------------------------------------------*/

void ataripf_render(int map, struct osd_bitmap *bitmap)
{
	struct ataripf_data *pf = &ataripf[map];

	if (pf->initialized)
	{
		/* render via the standard render callback */
		pf_process(pf, pf_render_callback, bitmap, NULL);

		/* set a timer to call the eof function just before scanline 0 */
		if (!pf->timerallocated)
		{
			timer_set(cpu_getscanlinetime(0), map, pf_eof_callback);
			pf->timerallocated = 1;
		}
	}
}


/*---------------------------------------------------------------
	ataripf_overrender: Overrender the playfield, calling
	the callback for each tile before proceeding.
---------------------------------------------------------------*/

void ataripf_overrender(int map, ataripf_overrender_cb callback, struct ataripf_overrender_data *data)
{
	struct ataripf_data *pf = &ataripf[map];

	if (pf->initialized)
	{
		/* set the globals before processing */
		overrender_callback = callback;
		overrender_data = *data;

		/* render via the standard render callback */
		pf_process(pf, pf_overrender_callback, data->bitmap, &data->clip);
	}
}


/*---------------------------------------------------------------
	ataripf_mark_palette: Mark palette entries used in the
	current playfield.
---------------------------------------------------------------*/

void ataripf_mark_palette(int map)
{
	struct ataripf_data *pf = &ataripf[map];

	if (pf->initialized)
	{
		UINT8 *used_colors = &palette_used_colors[pf->palettebase];
		struct ataripf_usage marked_colors[256];
		int i, j, k;

		/* reset the marked colors */
		memset(marked_colors, 0, pf->maxcolors * sizeof(marked_colors[0]));

		/* mark the colors used */
		if (pf->max_usage_words <= 1)
			pf_process(pf, pf_usage_callback_1, marked_colors, NULL);
		else if (pf->max_usage_words == 2)
			pf_process(pf, pf_usage_callback_2, marked_colors, NULL);
		else
			logerror("ataripf_mark_palette: unsupported max_usage_words = %d\n", pf->max_usage_words);

		/* loop over colors */
		for (i = 0; i < pf->maxcolors; i++)
		{
			for (j = 0; j < pf->max_usage_words; j++)
			{
				UINT32 usage = marked_colors[i].bits[j];

				/* if this entry was marked, loop over bits */
				for (k = 0; usage; k++, usage >>= 1)
					if (usage & 1)
						used_colors[j * 32 + k] = (pf->transpens & (1 << k)) ? PALETTE_COLOR_TRANSPARENT : PALETTE_COLOR_USED;
			}

			/* advance by the color granularity of the gfx */
			used_colors += ATARIPF_BASE_GRANULARITY;
		}

		/* reset the visitation map now that we're done */
		memset(pf->visitmap, 0, sizeof(pf->visitmap[0]) * pf->vramsize);
	}
}


/*---------------------------------------------------------------
	ataripf_set_bankbits: Set the extra banking bits for a
	playfield.
---------------------------------------------------------------*/

void ataripf_set_bankbits(int map, int bankbits, int scanline)
{
	struct ataripf_data *pf = &ataripf[map];

	if (pf->initialized && pf->curstate.bankbits != bankbits)
	{
		pf->curstate.bankbits = bankbits;
		pf_update_state(pf, scanline);
	}
}


/*---------------------------------------------------------------
	ataripf_set_xscroll: Set the horizontal scroll value for
	a playfield.
---------------------------------------------------------------*/

void ataripf_set_xscroll(int map, int xscroll, int scanline)
{
	struct ataripf_data *pf = &ataripf[map];
	if (pf->initialized && pf->curstate.xscroll != xscroll)
	{
		pf->curstate.xscroll = xscroll;
		pf_update_state(pf, scanline);
	}
}


/*---------------------------------------------------------------
	ataripf_set_yscroll: Set the vertical scroll value for
	a playfield.
---------------------------------------------------------------*/

void ataripf_set_yscroll(int map, int yscroll, int scanline)
{
	struct ataripf_data *pf = &ataripf[map];
	if (pf->initialized && pf->curstate.yscroll != yscroll)
	{
		pf->curstate.yscroll = yscroll;
		pf_update_state(pf, scanline);
	}
}


/*---------------------------------------------------------------
	ataripf_set_latch: Set the upper word latch value and mask
	a playfield.
---------------------------------------------------------------*/

void ataripf_set_latch(int map, int latch)
{
	struct ataripf_data *pf = &ataripf[map];
	int mask;

	if (pf->initialized)
	{
		/* -1 means disable the latching */
		if (latch == -1)
			pf->latchdata = -1;
		else
			pf->latchdata = latch & pf->latchmask;

		/* compute the shifted value */
		pf->latchval = latch & pf->latchmask;
		mask = pf->latchmask;
		if (mask)
			for ( ; !(mask & 1); mask >>= 1)
				pf->latchval >>= 1;
	}
}


/*---------------------------------------------------------------
	ataripf_set_latch_lo: Set the latch for any playfield with
	a latchmask in the low byte.
---------------------------------------------------------------*/

void ataripf_set_latch_lo(int latch)
{
	int i;

	for (i = 0; i < ATARIPF_MAX; i++)
		if (ataripf[i].latchmask & 0x00ff)
			ataripf_set_latch(i, latch);
}


/*---------------------------------------------------------------
	ataripf_set_latch_hi: Set the latch for any playfield with
	a latchmask in the high byte.
---------------------------------------------------------------*/

void ataripf_set_latch_hi(int latch)
{
	int i;

	for (i = 0; i < ATARIPF_MAX; i++)
		if (ataripf[i].latchmask & 0xff00)
			ataripf_set_latch(i, latch);
}


/*---------------------------------------------------------------
	ataripf_get_bankbits: Returns the extra banking bits for a
	playfield.
---------------------------------------------------------------*/

int ataripf_get_bankbits(int map)
{
	return ataripf[map].curstate.bankbits;
}


/*---------------------------------------------------------------
	ataripf_get_xscroll: Returns the horizontal scroll value
	for a playfield.
---------------------------------------------------------------*/

int ataripf_get_xscroll(int map)
{
	return ataripf[map].curstate.xscroll;
}


/*---------------------------------------------------------------
	ataripf_get_yscroll: Returns the vertical scroll value for
	a playfield.
---------------------------------------------------------------*/

int ataripf_get_yscroll(int map)
{
	return ataripf[map].curstate.yscroll;
}


/*---------------------------------------------------------------
	ataripf_get_vram: Returns a pointer to video RAM.
---------------------------------------------------------------*/

UINT32 *ataripf_get_vram(int map)
{
	return ataripf[map].vram;
}


/*---------------------------------------------------------------
	ataripf_0_simple_w: Simple write handler for single-word
	playfields.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_simple_w )
{
	int oldword = LOWER_HALF(ataripf[0].vram[offset]);
	int newword = oldword;

	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		LOWER_HALF(ataripf[0].vram[offset]) = newword;
		ataripf[0].dirtymap[offset] = -1;
	}
	COMBINE_DATA(&ataripf_0_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_1_simple_w: Simple write handler for single-word
	playfields.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_1_simple_w )
{
	int oldword = LOWER_HALF(ataripf[1].vram[offset]);
	int newword = oldword;

	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		LOWER_HALF(ataripf[1].vram[offset]) = newword;
		ataripf[1].dirtymap[offset] = -1;
	}
	COMBINE_DATA(&ataripf_1_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_latched_w: Simple write handler for single-word
	playfields that latches additional bits in the upper word.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_latched_w )
{
	int oldword = LOWER_HALF(ataripf[0].vram[offset]);
	int newword = oldword;

	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		LOWER_HALF(ataripf[0].vram[offset]) = newword;
		ataripf[0].dirtymap[offset] = -1;
	}
	if (ataripf[0].latchdata != -1)
	{
		UPPER_HALF(ataripf[0].vram[offset]) = ataripf[0].latchval;
		ataripf_0_upper[offset] = (ataripf_0_upper[offset] & ~ataripf[0].latchmask) | ataripf[0].latchdata;
		ataripf[0].dirtymap[offset] = -1;
	}
	COMBINE_DATA(&ataripf_0_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_1_latched_w: Simple write handler for single-word
	playfields that latches additional bits in the upper word.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_1_latched_w )
{
	int oldword = LOWER_HALF(ataripf[1].vram[offset]);
	int newword = oldword;

	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		LOWER_HALF(ataripf[1].vram[offset]) = newword;
		ataripf[1].dirtymap[offset] = -1;
	}
	if (ataripf[1].latchdata != -1)
	{
		UPPER_HALF(ataripf[1].vram[offset]) = ataripf[1].latchval;
		ataripf_0_upper[offset] = (ataripf_0_upper[offset] & ~ataripf[1].latchmask) | ataripf[1].latchdata;
		ataripf[1].dirtymap[offset] = -1;
	}
	COMBINE_DATA(&ataripf_1_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_upper_msb_w: Simple write handler for the upper
	word of split two-word playfields, where the MSB contains
	the significant data.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_upper_msb_w )
{
	if (ACCESSING_MSB)
	{
		int oldword = UPPER_HALF(ataripf[0].vram[offset]);
		int newword = oldword << 8;

		COMBINE_DATA(&newword);
		newword >>= 8;

		if (oldword != newword)
		{
			UPPER_HALF(ataripf[0].vram[offset]) = newword;
			ataripf[0].dirtymap[offset] = -1;
		}
	}
	COMBINE_DATA(&ataripf_0_upper[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_upper_lsb_w: Simple write handler for the upper
	word of split two-word playfields, where the LSB contains
	the significant data.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_upper_lsb_w )
{
	if (ACCESSING_LSB)
	{
		int oldword = UPPER_HALF(ataripf[0].vram[offset]);
		int newword = oldword;

		COMBINE_DATA(&newword);
		newword &= 0xff;

		if (oldword != newword)
		{
			UPPER_HALF(ataripf[0].vram[offset]) = newword;
			ataripf[0].dirtymap[offset] = -1;
		}
	}
	COMBINE_DATA(&ataripf_0_upper[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_large_w: Simple write handler for double-word
	playfields.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_large_w )
{
	if (!(offset & 1))
	{
		int offs = offset / 2;
		int oldword = UPPER_HALF(ataripf[0].vram[offs]);
		int newword = oldword;

		COMBINE_DATA(&newword);

		if (oldword != newword)
		{
			UPPER_HALF(ataripf[0].vram[offs]) = newword;
			ataripf[0].dirtymap[offs] = -1;
		}
	}
	else
	{
		int offs = offset / 2;
		int oldword = LOWER_HALF(ataripf[0].vram[offs]);
		int newword = oldword;

		COMBINE_DATA(&newword);

		if (oldword != newword)
		{
			LOWER_HALF(ataripf[0].vram[offs]) = newword;
			ataripf[0].dirtymap[offs] = -1;
		}
	}
	COMBINE_DATA(&ataripf_0_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_split_w: Simple write handler for split playfields.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_0_split_w )
{
	int adjusted = (offset & 0x003f) | ((~offset & 0x1000) >> 6) | ((offset & 0x0fc0) << 1);
	int oldword = LOWER_HALF(ataripf[0].vram[adjusted]);
	int newword = oldword;

	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		LOWER_HALF(ataripf[0].vram[adjusted]) = newword;
		ataripf[0].dirtymap[adjusted] = -1;
	}
	COMBINE_DATA(&ataripf_0_base[offset]);
}


/*---------------------------------------------------------------
	ataripf_01_upper_lsb_msb_w: Simple write handler for the
	upper word of dual split two-word playfields, where the LSB
	contains the significant data for playfield 0 and the MSB
	contains the significant data for playfield 1.
---------------------------------------------------------------*/

WRITE16_HANDLER( ataripf_01_upper_lsb_msb_w )
{
	if (ACCESSING_LSB)
	{
		int oldword = UPPER_HALF(ataripf[0].vram[offset]);
		int newword = oldword;

		COMBINE_DATA(&newword);
		newword &= 0xff;

		if (oldword != newword)
		{
			UPPER_HALF(ataripf[0].vram[offset]) = newword;
			ataripf[0].dirtymap[offset] = -1;
		}
	}
	if (ACCESSING_MSB)
	{
		int oldword = UPPER_HALF(ataripf[1].vram[offset]);
		int newword = oldword << 8;

		COMBINE_DATA(&newword);
		newword >>= 8;

		if (oldword != newword)
		{
			UPPER_HALF(ataripf[1].vram[offset]) = newword;
			ataripf[1].dirtymap[offset] = -1;
		}
	}
	COMBINE_DATA(&ataripf_0_upper[offset]);
}


/*---------------------------------------------------------------
	ataripf_0_split32_w: Simple write handler for split playfields.
---------------------------------------------------------------*/

WRITE32_HANDLER( ataripf_0_split32_w )
{
	if (ACCESSING_MSW32)
	{
		int adjusted = ((offset & 0x001f) | ((~offset & 0x0800) >> 6) | ((offset & 0x07e0) << 1)) * 2;
		int oldword = LOWER_HALF(ataripf[0].vram[adjusted]);
		int newword = oldword << 16;

		COMBINE_DATA(&newword);
		newword >>= 16;

		if (oldword != newword)
		{
			LOWER_HALF(ataripf[0].vram[adjusted]) = newword;
			ataripf[0].dirtymap[adjusted] = -1;
		}
	}

	if (ACCESSING_LSW32)
	{
		int adjusted = ((offset & 0x001f) | ((~offset & 0x0800) >> 6) | ((offset & 0x07e0) << 1)) * 2 + 1;
		int oldword = LOWER_HALF(ataripf[0].vram[adjusted]);
		int newword = oldword;

		COMBINE_DATA(&newword);
		newword &= 0xffff;

		if (oldword != newword)
		{
			LOWER_HALF(ataripf[0].vram[adjusted]) = newword;
			ataripf[0].dirtymap[adjusted] = -1;
		}
	}

	COMBINE_DATA(&ataripf_0_base32[offset]);
}


/*---------------------------------------------------------------
	pf_process: Internal routine that loops over chunks of
	the playfield with common parameters and processes them
	via a callback.
---------------------------------------------------------------*/

static void pf_process(struct ataripf_data *pf, pf_callback callback, void *param, const struct rectangle *clip)
{
	struct ataripf_state *state = pf->statelist;
	struct rectangle finalclip;
	int i;

	if (clip)
		finalclip = *clip;
	else
		finalclip = Machine->visible_area;

	/* if the gfx has changed, make sure we have extended usage maps for everyone */
	if (pf->gfxchanged)
	{
		pf->gfxchanged = 0;
		for (i = 0; i < pf->lookupmask + 1; i++)
		{
			int gfxindex = ATARIPF_LOOKUP_GFX(pf->lookup[i]);
			if (!pf->gfxelement[gfxindex].initialized)
			{
				pf_init_gfx(pf, gfxindex);
				if (!pf->gfxelement[gfxindex].initialized)
				{
					logerror("ataripf_init: out of memory for extra usage map\n");
					exit(1);
				}
			}
		}
	}

	/* preinitialization */
	pf->process_clip.min_x = finalclip.min_x;
	pf->process_clip.max_x = finalclip.max_x;

	/* mark the n+1'th entry with a large scanline */
	pf->statelist[pf->stateindex].scanline = 100000;
	pf->process_param = param;

	/* loop over all entries */
	for (i = 0; i < pf->stateindex; i++, state++)
	{
		/* determine the clip rect */
		pf->process_clip.min_y = state[0].scanline;
		pf->process_clip.max_y = state[1].scanline - 1;

		/* skip if we're clipped out */
		if (pf->process_clip.min_y > finalclip.max_y || pf->process_clip.max_y < finalclip.min_y)
			continue;

		/* clip the clipper */
		if (pf->process_clip.min_y < finalclip.min_y)
			pf->process_clip.min_y = finalclip.min_y;
		if (pf->process_clip.max_y > finalclip.max_y)
			pf->process_clip.max_y = finalclip.max_y;

		/* determine the tile rect */
		pf->process_tiles.min_x = ((state->xscroll + pf->process_clip.min_x) >> pf->tilexshift) & pf->colmask;
		pf->process_tiles.max_x = ((state->xscroll + pf->process_clip.max_x + pf->tilewidth) >> pf->tilexshift) & pf->colmask;
		pf->process_tiles.min_y = ((state->yscroll + pf->process_clip.min_y) >> pf->tileyshift) & pf->rowmask;
		pf->process_tiles.max_y = ((state->yscroll + pf->process_clip.max_y + pf->tileheight) >> pf->tileyshift) & pf->rowmask;

		/* call the callback */
		(*callback)(pf, state);
	}
}


/*---------------------------------------------------------------
	pf_usage_callback_1: Internal processing callback that
	marks pens used if the maximum word count is 1.
---------------------------------------------------------------*/

static void pf_usage_callback_1(struct ataripf_data *pf, const struct ataripf_state *state)
{
	struct ataripf_usage *colormap = pf->process_param;
	int x, y, bankbits = state->bankbits;

	/* standard loop over tiles */
	for (y = pf->process_tiles.min_y; y != pf->process_tiles.max_y; y = (y + 1) & pf->rowmask)
		for (x = pf->process_tiles.min_x; x != pf->process_tiles.max_x; x = (x + 1) & pf->colmask)
		{
			int offs = (y << pf->rowshift) + (x << pf->colshift);
			UINT32 data = pf->vram[offs] | bankbits;
			int lookup = pf->lookup[(data >> ATARIPF_LOOKUP_DATABITS) & pf->lookupmask];
			const struct ataripf_gfxelement *gfx = &pf->gfxelement[ATARIPF_LOOKUP_GFX(lookup)];
			int code = ATARIPF_LOOKUP_CODE(lookup, data);
			int color = ATARIPF_LOOKUP_COLOR(lookup);

			/* mark the pens for this color entry */
			colormap[color << gfx->colorshift].bits[0] |= gfx->usage[code].bits[0];

			/* mark the pens for the corresponding shadow */
			colormap[(color ^ pf->shadowxor) << gfx->colorshift].bits[0] |= gfx->usage[code].bits[0];

			/* also mark unvisited tiles dirty */
			if (!pf->visitmap[offs])
				pf->dirtymap[offs] = -1;
		}
}


/*---------------------------------------------------------------
	pf_usage_callback_2: Internal processing callback that
	marks pens used if the maximum word count is 2.
---------------------------------------------------------------*/

static void pf_usage_callback_2(struct ataripf_data *pf, const struct ataripf_state *state)
{
	struct ataripf_usage *colormap = pf->process_param;
	int x, y, bankbits = state->bankbits;

	/* standard loop over tiles */
	for (y = pf->process_tiles.min_y; y != pf->process_tiles.max_y; y = (y + 1) & pf->rowmask)
		for (x = pf->process_tiles.min_x; x != pf->process_tiles.max_x; x = (x + 1) & pf->colmask)
		{
			int offs = (y << pf->rowshift) + (x << pf->colshift);
			UINT32 data = pf->vram[offs] | bankbits;
			int lookup = pf->lookup[(data >> ATARIPF_LOOKUP_DATABITS) & pf->lookupmask];
			const struct ataripf_gfxelement *gfx = &pf->gfxelement[ATARIPF_LOOKUP_GFX(lookup)];
			int code = ATARIPF_LOOKUP_CODE(lookup, data);
			int color = ATARIPF_LOOKUP_COLOR(lookup);

			/* mark the pens for this color entry */
			colormap[color << gfx->colorshift].bits[0] |= gfx->usage[code].bits[0];
			colormap[color << gfx->colorshift].bits[1] |= gfx->usage[code].bits[1];

			/* mark the pens for the corresponding shadow */
			colormap[(color ^ pf->shadowxor) << gfx->colorshift].bits[0] |= gfx->usage[code].bits[0];
			colormap[(color ^ pf->shadowxor) << gfx->colorshift].bits[1] |= gfx->usage[code].bits[1];

			/* also mark unvisited tiles dirty */
			if (!pf->visitmap[offs])
				pf->dirtymap[offs] = -1;
		}
}


/*---------------------------------------------------------------
	pf_render_callback: Internal processing callback that
	renders to the backing bitmap and then copies the result
	to the destination.
---------------------------------------------------------------*/

static void pf_render_callback(struct ataripf_data *pf, const struct ataripf_state *state)
{
	struct osd_bitmap *bitmap = pf->process_param;
	int x, y, bankbits = state->bankbits;

	/* standard loop over tiles */
	for (y = pf->process_tiles.min_y; y != pf->process_tiles.max_y; y = (y + 1) & pf->rowmask)
		for (x = pf->process_tiles.min_x; x != pf->process_tiles.max_x; x = (x + 1) & pf->colmask)
		{
			int offs = (y << pf->rowshift) + (x << pf->colshift);
			UINT32 data = pf->vram[offs] | bankbits;

			/* update only if dirty */
			if (pf->dirtymap[offs] != data)
			{
				int lookup = pf->lookup[(data >> ATARIPF_LOOKUP_DATABITS) & pf->lookupmask];
				const struct ataripf_gfxelement *gfx = &pf->gfxelement[ATARIPF_LOOKUP_GFX(lookup)];
				int code = ATARIPF_LOOKUP_CODE(lookup, data);
				int color = ATARIPF_LOOKUP_COLOR(lookup);
				int hflip = ATARIPF_LOOKUP_HFLIP(lookup);
				int vflip = ATARIPF_LOOKUP_VFLIP(lookup);

				/* draw and reset the dirty value */
				drawgfx(pf->bitmap, &gfx->element, code, color << gfx->colorshift, hflip, vflip,
						x << pf->tilexshift, y << pf->tileyshift,
						0, TRANSPARENCY_NONE, 0);
				pf->dirtymap[offs] = data;
			}

			/* track the tiles we've visited */
			pf->visitmap[offs] = 1;
		}

	/* then blast the result */
	x = -state->xscroll;
	y = -state->yscroll;
	if (!pf->transpens)
		copyscrollbitmap(bitmap, pf->bitmap, 1, &x, 1, &y, &pf->process_clip, TRANSPARENCY_NONE, 0);
	else
		copyscrollbitmap(bitmap, pf->bitmap, 1, &x, 1, &y, &pf->process_clip, TRANSPARENCY_PEN, palette_transparent_pen);
}


/*---------------------------------------------------------------
	pf_overrender_callback: Internal processing callback that
	calls an external function to determine if a tile should
	be drawn again, and if so, how it should be drawn.
---------------------------------------------------------------*/

static void pf_overrender_callback(struct ataripf_data *pf, const struct ataripf_state *state)
{
	int x, y, bankbits = state->bankbits;
	int first_result;

	/* make the first overrender call */
	first_result = (*overrender_callback)(&overrender_data, OVERRENDER_BEGIN);
	if (first_result == OVERRENDER_NONE)
		return;

	/* standard loop over tiles */
	for (y = pf->process_tiles.min_y; y != pf->process_tiles.max_y; y = (y + 1) & pf->rowmask)
	{
		int sy = ((y << pf->tileyshift) - state->yscroll) & pf->bitmapymask;
		if (sy > Machine->visible_area.max_y) sy -= pf->bitmapheight;

		for (x = pf->process_tiles.min_x; x != pf->process_tiles.max_x; x = (x + 1) & pf->colmask)
		{
			int offs = (y << pf->rowshift) + (x << pf->colshift);
			UINT32 data = pf->vram[offs] | bankbits;
			int lookup = pf->lookup[(data >> ATARIPF_LOOKUP_DATABITS) & pf->lookupmask];
			const struct ataripf_gfxelement *gfx = &pf->gfxelement[ATARIPF_LOOKUP_GFX(lookup)];
			int code = ATARIPF_LOOKUP_CODE(lookup, data);

			/* fill in the overrender data that might be needed */
			overrender_data.pfusage = &gfx->usage[code];
			overrender_data.pfcolor = ATARIPF_LOOKUP_COLOR(lookup);
			overrender_data.pfpriority = ATARIPF_LOOKUP_PRIORITY(lookup);

			/* check with the callback to see if we should overrender */
			if (first_result == OVERRENDER_ALL || (*overrender_callback)(&overrender_data, OVERRENDER_QUERY))
			{
				int hflip = ATARIPF_LOOKUP_HFLIP(lookup);
				int vflip = ATARIPF_LOOKUP_VFLIP(lookup);
				int sx = ((x << pf->tilexshift) - state->xscroll) & pf->bitmapxmask;
				if (sx > Machine->visible_area.max_x) sx -= pf->bitmapwidth;

				/* use either mdrawgfx or drawgfx depending on the mask pens */
				if (overrender_data.maskpens != 0)
					mdrawgfx(overrender_data.bitmap, &gfx->element, code, overrender_data.pfcolor << gfx->colorshift, hflip, vflip,
							sx, sy, &pf->process_clip, overrender_data.drawmode, overrender_data.drawpens,
							overrender_data.maskpens);
				else
					drawgfx(overrender_data.bitmap, &gfx->element, code, overrender_data.pfcolor << gfx->colorshift, hflip, vflip,
							sx, sy, &pf->process_clip, overrender_data.drawmode, overrender_data.drawpens);
			}
		}
	}

	/* make the final call */
	(*overrender_callback)(&overrender_data, OVERRENDER_FINISH);
}


/*---------------------------------------------------------------
	pf_eof_callback: This callback is called on scanline 0 to
	reset the playfields.
---------------------------------------------------------------*/

static void pf_eof_callback(int map)
{
	struct ataripf_data *pf = &ataripf[map];

	/* copy the current state to entry 0 and reset the index */
	pf->statelist[0] = pf->curstate;
	pf->statelist[0].scanline = 0;
	pf->stateindex = 1;

	/* go off again same time next frame */
	timer_set(cpu_getscanlinetime(0), map, pf_eof_callback);
}


/*---------------------------------------------------------------
	pf_init_gfx: Initializes our own internal graphics
	representation.
---------------------------------------------------------------*/

static void pf_init_gfx(struct ataripf_data *pf, int gfxindex)
{
	struct ataripf_gfxelement *gfx = &pf->gfxelement[gfxindex];
	struct ataripf_usage *usage;
	int i;

	/* make a copy of the original GfxElement structure */
	gfx->element = *Machine->gfx[gfxindex];

	/* adjust the granularity */
	gfx->colorshift = compute_log(gfx->element.color_granularity / ATARIPF_BASE_GRANULARITY);
	gfx->element.color_granularity = ATARIPF_BASE_GRANULARITY;
	gfx->element.total_colors = pf->maxcolors;
	gfx->element.colortable = &Machine->remapped_colortable[pf->palettebase];

	/* allocate the extended usage map */
	usage = malloc(gfx->element.total_elements * sizeof(usage[0]));
	if (!usage)
		return;

	/* set the pointer and clear the word count */
	gfx->usage = usage;
	gfx->usage_words = 0;

	/* fill in the extended usage map */
	memset(usage, 0, gfx->element.total_elements * sizeof(usage[0]));
	for (i = 0; i < gfx->element.total_elements; i++, usage++)
	{
		UINT8 *src = gfx->element.gfxdata + gfx->element.char_modulo * i;
		int x, y, words;

		/* loop over all pixels, marking pens */
		for (y = 0; y < gfx->element.height; y++)
		{
			for (x = 0; x < gfx->element.width; x++)
				usage->bits[src[x] >> 5] |= 1 << (src[x] & 31);
			src += gfx->element.line_modulo;
		}

		/* count how many words maximum we needed to combine */
		for (words = ATARIPF_USAGE_WORDS; words > 0; words--)
			if (usage->bits[words - 1] != 0)
				break;
		if (words > gfx->usage_words)
			gfx->usage_words = words;
	}

	/* if we're the biggest so far, track it */
	if (gfx->usage_words > pf->max_usage_words)
		pf->max_usage_words = gfx->usage_words;
	gfx->initialized = 1;

	logerror("Finished build external usage map for gfx[%d]: words = %d\n", gfxindex, gfx->usage_words);
	logerror("Color shift = %d (granularity=%d)\n", gfx->colorshift, gfx->element.color_granularity);
	logerror("Current maximum = %d\n", pf->max_usage_words);
}
