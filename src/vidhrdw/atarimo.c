/*##########################################################################

	atarimo.c
	
	Common motion object management functions for Atari raster games.

##########################################################################*/

#include "driver.h"
#include "atarimo.h"
#include "tilemap.h"


/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* internal structure containing a word index, shift and mask */
struct atarimo_mask
{
	int					word;				/* word index */
	int					shift;				/* shift amount */
	int					mask;				/* final mask */
};

/* internal cache entry */
struct atarimo_cache
{
	UINT16				scanline;			/* effective scanline */
	struct atarimo_entry entry;				/* entry data */
};

/* internal structure containing the state of the motion objects */
struct atarimo_data
{
	int					timerallocated;		/* true if we've allocated the timer */
	int					gfxchanged;			/* true if the gfx info has changed */

	int					linked;				/* are the entries linked? */
	int					split;				/* are entries split or together? */
	int					reverse;			/* render in reverse order? */
	int					swapxy;				/* render in swapped X/Y order? */
	UINT8				nextneighbor;		/* does the neighbor bit affect the next object? */
	int					slipshift;			/* log2(pixels_per_SLIP) */
	int					updatescans;		/* number of scanlines per update */

	int					entrycount;			/* number of entries per bank */
	int					entrybits;			/* number of bits needed to represent entrycount */
	int					bankcount;			/* number of banks */

	int					tilexshift;			/* bits to shift X coordinate when drawing */
	int					tileyshift;			/* bits to shift Y coordinate when drawing */
	int					bitmapwidth;		/* width of the full playfield bitmap */
	int					bitmapheight;		/* height of the full playfield bitmap */
	int					bitmapxmask;		/* x coordinate mask for the playfield bitmap */
	int					bitmapymask;		/* y coordinate mask for the playfield bitmap */

	int					spriterammask;		/* combined mask when accessing sprite RAM with raw addresses */
	int					spriteramsize;		/* total size of sprite RAM, in entries */
	int					sliprammask;		/* combined mask when accessing SLIP RAM with raw addresses */
	int					slipramsize;		/* total size of SLIP RAM, in entries */

	int					palettebase;		/* base palette entry */
	int					maxcolors;			/* maximum number of colors */
	int					transpen;			/* transparent pen index */
	
	int					bank;				/* current bank number */
	int					xscroll;			/* current x scroll offset */
	int					yscroll;			/* current y scroll offset */

	struct atarimo_mask	linkmask;			/* mask for the link */
	struct atarimo_mask gfxmask;			/* mask for the graphics bank */
	struct atarimo_mask	codemask;			/* mask for the code index */
	struct atarimo_mask codehighmask;		/* mask for the upper code index */
	struct atarimo_mask	colormask;			/* mask for the color */
	struct atarimo_mask	xposmask;			/* mask for the X position */
	struct atarimo_mask	yposmask;			/* mask for the Y position */
	struct atarimo_mask	widthmask;			/* mask for the width, in tiles*/
	struct atarimo_mask	heightmask;			/* mask for the height, in tiles */
	struct atarimo_mask	hflipmask;			/* mask for the horizontal flip */
	struct atarimo_mask	vflipmask;			/* mask for the vertical flip */
	struct atarimo_mask	prioritymask;		/* mask for the priority */
	struct atarimo_mask	neighbormask;		/* mask for the neighbor */
	struct atarimo_mask absolutemask;		/* mask for absolute coordinates */

	struct atarimo_mask ignoremask;			/* mask for the ignore value */
	int					ignorevalue;		/* resulting value to indicate "ignore" */
	atarimo_special_cb	ignorecb;			/* callback routine for ignored entries */
	int					codehighshift;		/* shift count for the upper code */

	struct atarimo_entry *spriteram;		/* pointer to sprite RAM */
	data16_t **			slipram;			/* pointer to the SLIP RAM pointer */
	UINT16 *			codelookup;			/* lookup table for codes */
	UINT8 *				colorlookup;		/* lookup table for colors */
	UINT8 *				gfxlookup;			/* lookup table for graphics */

	struct atarimo_cache *cache;			/* pointer to the cache data */
	struct atarimo_cache *endcache;			/* end of the cache */
	struct atarimo_cache *curcache;			/* current cache entry */
	struct atarimo_cache *prevcache;		/* previous cache entry */
	
	ataripf_overrender_cb overrender0;		/* overrender callback for PF 0 */
	ataripf_overrender_cb overrender1;		/* overrender callback for PF 1 */
	struct rectangle	process_clip;		/* (during processing) the clip rectangle */
	void *				process_param;		/* (during processing) the callback parameter */
	int					last_xpos;			/* (during processing) the previous X position */
	int					next_xpos;			/* (during processing) the next X position */
	int					process_xscroll;	/* (during processing) the X scroll position */
	int					process_yscroll;	/* (during processing) the Y scroll position */
};


/* callback function for the internal playfield processing mechanism */
typedef void (*mo_callback)(struct atarimo_data *pf, const struct atarimo_entry *entry);



/*##########################################################################
	MACROS
##########################################################################*/

/* verification macro for void functions */
#define VERIFY(cond, msg) if (!(cond)) { logerror(msg); return; }

/* verification macro for non-void functions */
#define VERIFYRETFREE(cond, msg, ret) if (!(cond)) { logerror(msg); atarimo_free(); return (ret); }


/* data extraction */
#define EXTRACT_DATA(_input, _mask) (((_input)->data[(_mask).word] >> (_mask).shift) & (_mask).mask)



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

data16_t *atarimo_0_spriteram;
data16_t *atarimo_0_slipram;

data16_t *atarimo_1_spriteram;
data16_t *atarimo_1_slipram;



/*##########################################################################
	STATIC VARIABLES
##########################################################################*/

static struct atarimo_data atarimo[ATARIMO_MAX];
static struct GfxElement gfxelement[MAX_GFX_ELEMENTS];



/*##########################################################################
	STATIC FUNCTION DECLARATIONS
##########################################################################*/

static void mo_process(struct atarimo_data *mo, mo_callback callback, void *param, const struct rectangle *clip);
static void mo_update(struct atarimo_data *mo, int scanline);
static void mo_usage_callback(struct atarimo_data *mo, const struct atarimo_entry *entry);
static void mo_render_callback(struct atarimo_data *mo, const struct atarimo_entry *entry);
static void mo_scanline_callback(int scanline);



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
	convert_mask: Converts a 4-word mask into a word index,
	shift, and adjusted mask. Returns 0 if invalid.
---------------------------------------------------------------*/

INLINE int convert_mask(const struct atarimo_entry *input, struct atarimo_mask *result)
{
	int i, temp;
	
	/* determine the word and make sure it's only 1 */
	result->word = -1;
	for (i = 0; i < 4; i++)
		if (input->data[i])
		{
			if (result->word == -1)
				result->word = i;
			else
				return 0;
		}
	
	/* if all-zero, it's valid */
	if (result->word == -1)
	{
		result->word = result->shift = result->mask = 0;
		return 1;
	}
	
	/* determine the shift and final mask */
	result->shift = 0;
	temp = input->data[result->word];
	while (!(temp & 1))
	{
		result->shift++;
		temp >>= 1;
	}
	result->mask = temp;
	return 1;
}



/*##########################################################################
	GLOBAL FUNCTIONS
##########################################################################*/

/*---------------------------------------------------------------
	atarimo_init: Configures the motion objects using the input
	description. Allocates all memory necessary and generates
	the attribute lookup table.
---------------------------------------------------------------*/

int atarimo_init(int map, const struct atarimo_desc *desc)
{
	struct GfxElement *gfx = Machine->gfx[desc->gfxindex];
	struct atarimo_data *mo = &atarimo[map];
	int i;

	VERIFYRETFREE(map >= 0 && map < ATARIMO_MAX, "atarimo_init: map out of range", 0)

	/* determine the masks first */
	convert_mask(&desc->linkmask,     &mo->linkmask);
	convert_mask(&desc->gfxmask,      &mo->gfxmask);
	convert_mask(&desc->codemask,     &mo->codemask);
	convert_mask(&desc->codehighmask, &mo->codehighmask);
	convert_mask(&desc->colormask,    &mo->colormask);
	convert_mask(&desc->xposmask,     &mo->xposmask);
	convert_mask(&desc->yposmask,     &mo->yposmask);
	convert_mask(&desc->widthmask,    &mo->widthmask);
	convert_mask(&desc->heightmask,   &mo->heightmask);
	convert_mask(&desc->hflipmask,    &mo->hflipmask);
	convert_mask(&desc->vflipmask,    &mo->vflipmask);
	convert_mask(&desc->prioritymask, &mo->prioritymask);
	convert_mask(&desc->neighbormask, &mo->neighbormask);
	convert_mask(&desc->absolutemask, &mo->absolutemask);

	/* copy in the basic data */
	mo->timerallocated = 0;
	mo->gfxchanged    = 0;

	mo->linked        = desc->linked;
	mo->split         = desc->split;
	mo->reverse       = desc->reverse;
	mo->swapxy        = desc->swapxy;
	mo->nextneighbor  = desc->nextneighbor;
	mo->slipshift     = desc->slipheight ? compute_log(desc->slipheight) : 0;
	mo->updatescans   = desc->updatescans;

	mo->entrycount    = round_to_powerof2(mo->linkmask.mask);
	mo->entrybits     = compute_log(mo->entrycount);
	mo->bankcount     = desc->banks;

	mo->tilexshift    = compute_log(gfx->width);
	mo->tileyshift    = compute_log(gfx->height);
	mo->bitmapwidth   = round_to_powerof2(mo->xposmask.mask);
	mo->bitmapheight  = round_to_powerof2(mo->yposmask.mask);
	mo->bitmapxmask   = mo->bitmapwidth - 1;
	mo->bitmapymask   = mo->bitmapheight - 1;

	mo->spriteramsize = mo->bankcount * mo->entrycount;
	mo->spriterammask = mo->spriteramsize - 1;
	mo->slipramsize   = mo->bitmapheight >> mo->tileyshift;
	mo->sliprammask   = mo->slipramsize - 1;
	
	mo->palettebase   = desc->palettebase;
	mo->maxcolors     = desc->maxcolors / gfx->color_granularity;
	mo->transpen      = desc->transpen;
	
	mo->bank          = 0;
	mo->xscroll       = 0;
	mo->yscroll       = 0;

	convert_mask(&desc->ignoremask, &mo->ignoremask);
	mo->ignorevalue   = desc->ignorevalue;
	mo->ignorecb      = desc->ignorecb;
	mo->codehighshift = compute_log(round_to_powerof2(mo->codemask.mask));
	
	mo->slipram       = (map == 0) ? &atarimo_0_slipram : &atarimo_1_slipram;
	
	/* allocate the priority bitmap */
	priority_bitmap = bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 8);
	VERIFYRETFREE(priority_bitmap, "atarimo_init: out of memory for priority bitmap", 0)

	/* allocate the spriteram */
	mo->spriteram = malloc(sizeof(mo->spriteram[0]) * mo->spriteramsize);
	VERIFYRETFREE(mo->spriteram, "atarimo_init: out of memory for spriteram", 0)
	
	/* clear it to zero */
	memset(mo->spriteram, 0, sizeof(mo->spriteram[0]) * mo->spriteramsize);

	/* allocate the code lookup */
	mo->codelookup = malloc(sizeof(mo->codelookup[0]) * round_to_powerof2(mo->codemask.mask));
	VERIFYRETFREE(mo->codelookup, "atarimo_init: out of memory for code lookup", 0)
	
	/* initialize it 1:1 */
	for (i = 0; i < round_to_powerof2(mo->codemask.mask); i++)
		mo->codelookup[i] = i;

	/* allocate the color lookup */
	mo->colorlookup = malloc(sizeof(mo->colorlookup[0]) * round_to_powerof2(mo->colormask.mask));
	VERIFYRETFREE(mo->colorlookup, "atarimo_init: out of memory for color lookup", 0)
	
	/* initialize it 1:1 */
	for (i = 0; i < round_to_powerof2(mo->colormask.mask); i++)
		mo->colorlookup[i] = i;

	/* allocate the gfx lookup */
	mo->gfxlookup = malloc(sizeof(mo->gfxlookup[0]) * round_to_powerof2(mo->gfxmask.mask));
	VERIFYRETFREE(mo->gfxlookup, "atarimo_init: out of memory for gfx lookup", 0)
	
	/* initialize it with the gfxindex we were passed in */
	for (i = 0; i < round_to_powerof2(mo->gfxmask.mask); i++)
		mo->gfxlookup[i] = desc->gfxindex;

	/* allocate the cache */
	mo->cache = malloc(mo->entrycount * Machine->drv->screen_height * sizeof(mo->cache[0]));
	VERIFYRETFREE(mo->cache, "atarimo_init: out of memory for cache", 0)
	mo->endcache = mo->cache + mo->entrycount * Machine->drv->screen_height;

	/* initialize the end/last pointers */
	mo->curcache = mo->cache;
	mo->prevcache = NULL;
	
	/* initialize the gfx elements */
	gfxelement[desc->gfxindex] = *Machine->gfx[desc->gfxindex];
	gfxelement[desc->gfxindex].colortable = &Machine->remapped_colortable[mo->palettebase];

	logerror("atarimo_init:\n");
	logerror("  width=%d (shift=%d),  height=%d (shift=%d)\n", gfx->width, mo->tilexshift, gfx->height, mo->tileyshift);
	logerror("  spriteram mask=%X, size=%d\n", mo->spriterammask, mo->spriteramsize);
	logerror("  slipram mask=%X, size=%d\n", mo->sliprammask, mo->slipramsize);
	logerror("  bitmap size=%dx%d\n", mo->bitmapwidth, mo->bitmapheight);
	
	return 1;
}


/*---------------------------------------------------------------
	atarimo_free: Frees any memory allocated for motion objects.
---------------------------------------------------------------*/

void atarimo_free(void)
{
	int i;
	
	/* free the motion object data */
	for (i = 0; i < ATARIMO_MAX; i++)
	{
		struct atarimo_data *mo = &atarimo[i];
	
		/* free the priority bitmap */
		if (priority_bitmap)
			free(priority_bitmap);
		priority_bitmap = NULL;
	
		/* free the spriteram */
		if (mo->spriteram)
			free(mo->spriteram);
		mo->spriteram = NULL;
		
		/* free the codelookup */
		if (mo->codelookup)
			free(mo->codelookup);
		mo->codelookup = NULL;
	
		/* free the codelookup */
		if (mo->codelookup)
			free(mo->codelookup);
		mo->codelookup = NULL;
	
		/* free the colorlookup */
		if (mo->colorlookup)
			free(mo->colorlookup);
		mo->colorlookup = NULL;
	
		/* free the gfxlookup */
		if (mo->gfxlookup)
			free(mo->gfxlookup);
		mo->gfxlookup = NULL;
	
		/* free the cache */
		if (mo->cache)
			free(mo->cache);
		mo->cache = NULL;
	}
}


/*---------------------------------------------------------------
	atarimo_get_code_lookup: Returns a pointer to the code
	lookup table.
---------------------------------------------------------------*/

UINT16 *atarimo_get_code_lookup(int map, int *size)
{
	struct atarimo_data *mo = &atarimo[map];

	if (size)
		*size = round_to_powerof2(mo->codemask.mask);
	return mo->codelookup;
}


/*---------------------------------------------------------------
	atarimo_get_code_lookup: Returns a pointer to the code
	lookup table.
---------------------------------------------------------------*/

UINT8 *atarimo_get_color_lookup(int map, int *size)
{
	struct atarimo_data *mo = &atarimo[map];

	if (size)
		*size = round_to_powerof2(mo->colormask.mask);
	return mo->colorlookup;
}


/*---------------------------------------------------------------
	atarimo_get_code_lookup: Returns a pointer to the code
	lookup table.
---------------------------------------------------------------*/

UINT8 *atarimo_get_gfx_lookup(int map, int *size)
{
	struct atarimo_data *mo = &atarimo[map];

	mo->gfxchanged = 1;
	if (size)
		*size = round_to_powerof2(mo->gfxmask.mask);
	return mo->gfxlookup;
}


/*---------------------------------------------------------------
	atarimo_render: Render the motion objects to the 
	destination bitmap.
---------------------------------------------------------------*/

void atarimo_render(int map, struct osd_bitmap *bitmap, ataripf_overrender_cb callback1, ataripf_overrender_cb callback2)
{
	struct atarimo_data *mo = &atarimo[map];

	/* render via the standard render callback */
	mo->overrender0 = callback1;
	mo->overrender1 = callback2;
	mo_process(mo, mo_render_callback, bitmap, NULL);

	/* set a timer to call the eof function on scanline 0 */
	if (!mo->timerallocated)
	{
		timer_set(cpu_getscanlinetime(0), 0 | (map << 16), mo_scanline_callback);
		mo->timerallocated = 1;
	}
}


/*---------------------------------------------------------------
	atarimo_mark_palette: Mark palette entries used in the
	current set of motion objects.
---------------------------------------------------------------*/

void atarimo_mark_palette(int map)
{
	struct atarimo_data *mo = &atarimo[map];
	UINT8 *used_colors = &palette_used_colors[mo->palettebase];
	UINT32 marked_colors[256];
	int i, j;
	
	/* reset the marked colors */
	memset(marked_colors, 0, mo->maxcolors * sizeof(UINT32));
	
	/* mark the colors used */
	mo_process(mo, mo_usage_callback, marked_colors, NULL);
	
	/* loop over colors */
	for (i = 0; i < mo->maxcolors; i++)
	{
		int usage = marked_colors[i];
		
		/* if this entry was marked, loop over bits */
		if (usage)
		{
			for (j = 0; j < 32; j++, usage >>= 1)
				if (usage & 1)
					used_colors[j] = PALETTE_COLOR_USED;
			used_colors[mo->transpen] = PALETTE_COLOR_TRANSPARENT;
		}
		
		/* advance by the color granularity of the gfx */
		used_colors += gfxelement[mo->gfxlookup[0]].color_granularity;
	}
}


/*---------------------------------------------------------------
	atarimo_force_update: Force an update for the given
	scanline.
---------------------------------------------------------------*/

void atarimo_force_update(int map, int scanline)
{
	mo_update(&atarimo[map], scanline);
}


/*---------------------------------------------------------------
	atarimo_set_bank: Set the banking value for
	the motion objects.
---------------------------------------------------------------*/

void atarimo_set_bank(int map, int bank, int scanline)
{
	struct atarimo_data *mo = &atarimo[map];

	if (mo->bank != bank)
	{
		mo->bank = bank;
		mo_update(mo, scanline);
	}
}


/*---------------------------------------------------------------
	atarimo_set_palettebase: Set the palette base for
	the motion objects.
---------------------------------------------------------------*/

void atarimo_set_palettebase(int map, int base, int scanline)
{
	struct atarimo_data *mo = &atarimo[map];
	int i;
	
	mo->palettebase = base;
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		gfxelement[i].colortable = &Machine->remapped_colortable[base];
}


/*---------------------------------------------------------------
	atarimo_set_xscroll: Set the horizontal scroll value for
	the motion objects.
---------------------------------------------------------------*/

void atarimo_set_xscroll(int map, int xscroll, int scanline)
{
	struct atarimo_data *mo = &atarimo[map];

	if (mo->xscroll != xscroll)
	{
		mo->xscroll = xscroll;
		mo_update(mo, scanline);
	}
}


/*---------------------------------------------------------------
	atarimo_set_yscroll: Set the vertical scroll value for
	the motion objects.
---------------------------------------------------------------*/

void atarimo_set_yscroll(int map, int yscroll, int scanline)
{
	struct atarimo_data *mo = &atarimo[map];

	if (mo->yscroll != yscroll)
	{
		mo->yscroll = yscroll;
		mo_update(mo, scanline);
	}
}


/*---------------------------------------------------------------
	atarimo_get_bank: Returns the banking value 
	for the motion objects.
---------------------------------------------------------------*/

int atarimo_get_bank(int map)
{
	return atarimo[map].bank;
}


/*---------------------------------------------------------------
	atarimo_get_palettebase: Returns the palette base 
	for the motion objects.
---------------------------------------------------------------*/

int atarimo_get_palettebase(int map)
{
	return atarimo[map].palettebase;
}


/*---------------------------------------------------------------
	atarimo_get_xscroll: Returns the horizontal scroll value 
	for the motion objects.
---------------------------------------------------------------*/

int atarimo_get_xscroll(int map)
{
	return atarimo[map].xscroll;
}


/*---------------------------------------------------------------
	atarimo_get_yscroll: Returns the vertical scroll value for
	the motion objects.
---------------------------------------------------------------*/

int atarimo_get_yscroll(int map)
{
	return atarimo[map].yscroll;
}


/*---------------------------------------------------------------
	atarimo_0_spriteram_w: Write handler for the spriteram.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarimo_0_spriteram_w )
{
	int entry, idx, bank;
	
	COMBINE_DATA(&atarimo_0_spriteram[offset]);
	if (atarimo[0].split)
	{
		entry = offset & atarimo[0].linkmask.mask;
		idx = (offset >> atarimo[0].entrybits) & 3;
	}
	else
	{
		entry = (offset >> 2) & atarimo[0].linkmask.mask;
		idx = offset & 3;
	}
	bank = offset >> (2 + atarimo[0].entrybits);
	COMBINE_DATA(&atarimo[0].spriteram[(bank << atarimo[0].entrybits) + entry].data[idx]);
}


/*---------------------------------------------------------------
	atarimo_1_spriteram_w: Write handler for the spriteram.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarimo_1_spriteram_w )
{
	int entry, idx, bank;
	
	COMBINE_DATA(&atarimo_1_spriteram[offset]);
	if (atarimo[1].split)
	{
		entry = offset & atarimo[1].linkmask.mask;
		idx = (offset >> atarimo[1].entrybits) & 3;
	}
	else
	{
		entry = (offset >> 2) & atarimo[1].linkmask.mask;
		idx = offset & 3;
	}
	bank = offset >> (2 + atarimo[1].entrybits);
	COMBINE_DATA(&atarimo[1].spriteram[(bank << atarimo[1].entrybits) + entry].data[idx]);
}


/*---------------------------------------------------------------
	atarimo_0_spriteram_expanded_w: Write handler for the 
	expanded form of spriteram.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarimo_0_spriteram_expanded_w )
{
	int entry, idx, bank;
	
	COMBINE_DATA(&atarimo_0_spriteram[offset]);
	if (!(offset & 1))
	{
		offset >>= 1;
		if (atarimo[0].split)
		{
			entry = offset & atarimo[0].linkmask.mask;
			idx = (offset >> atarimo[0].entrybits) & 3;
		}
		else
		{
			entry = (offset >> 2) & atarimo[0].linkmask.mask;
			idx = offset & 3;
		}
		bank = offset >> (2 + atarimo[0].entrybits);
		COMBINE_DATA(&atarimo[0].spriteram[(bank << atarimo[0].entrybits) + entry].data[idx]);
	}
}


/*---------------------------------------------------------------
	atarimo_0_slipram_w: Write handler for the slipram.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarimo_0_slipram_w )
{
	COMBINE_DATA(&atarimo_0_slipram[offset]);
}


/*---------------------------------------------------------------
	atarimo_1_slipram_w: Write handler for the slipram.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarimo_1_slipram_w )
{
	COMBINE_DATA(&atarimo_1_slipram[offset]);
}


/*---------------------------------------------------------------
	mo_process: Internal routine that loops over chunks of
	the playfield with common parameters and processes them
	via a callback.
---------------------------------------------------------------*/

static void mo_process(struct atarimo_data *mo, mo_callback callback, void *param, const struct rectangle *clip)
{
	struct rectangle finalclip = clip ? *clip : Machine->visible_area;
	struct atarimo_cache *base = mo->cache;

	/* if the graphics info has changed, recompute */
	if (mo->gfxchanged)
	{
		int i;
		
		mo->gfxchanged = 0;
		for (i = 0; i < round_to_powerof2(mo->gfxmask.mask); i++)
		{
			int idx = mo->gfxlookup[i];
			gfxelement[idx] = *Machine->gfx[idx];
			gfxelement[idx].colortable = &Machine->remapped_colortable[mo->palettebase];
		}
	}

	/* create a clipping rectangle so that only partial sections are updated at a time */
	mo->process_clip.min_x = finalclip.min_x;
	mo->process_clip.max_x = finalclip.max_x;
	mo->process_param = param;
	mo->next_xpos = 123456;

	/* loop over the list until the end */
	while (base < mo->curcache)
	{
		struct atarimo_cache *current, *first, *last;
		int step;

		/* set the upper clip bound and a maximum lower bound */
		mo->process_clip.min_y = base->scanline;
		mo->process_clip.max_y = 100000;
		
		/* import the X and Y scroll values */
		mo->process_xscroll = base->entry.data[0];
		mo->process_yscroll = base->entry.data[1];
		base++;

		/* look for an entry whose scanline start is different from ours; that's our bottom */
		for (current = base; current < mo->curcache; current++)
			if (current->scanline != mo->process_clip.min_y)
			{
				mo->process_clip.max_y = current->scanline;
				break;
			}

		/* clip the clipper */
		if (mo->process_clip.min_y < finalclip.min_y)
			mo->process_clip.min_y = finalclip.min_y;
		if (mo->process_clip.max_y > finalclip.max_y)
			mo->process_clip.max_y = finalclip.max_y;

		/* set the start and end points */
		if (mo->reverse)
		{
			first = current - 1;
			last = base - 1;
			step = -1;
		}
		else
		{
			first = base;
			last = current;
			step = 1;
		}

		/* update the base */
		base = current;

		/* render the mos */
		for (current = first; current != last; current += step)
			(*callback)(mo, &current->entry);
	}
}


/*---------------------------------------------------------------
	mo_update: Parses the current motion object list, caching 
	all entries.
---------------------------------------------------------------*/

static void mo_update(struct atarimo_data *mo, int scanline)
{
	struct atarimo_cache *current = mo->curcache;
	struct atarimo_cache *previous = mo->prevcache;
	struct atarimo_cache *new_previous = current;
	UINT8 spritevisit[ATARIMO_MAXPERBANK];
	int match = 0, link;
	
	/* skip if the scanline is past the bottom of the screen */
	if (scanline > Machine->visible_area.max_y)
		return;

	/* if we don't use SLIPs, just recapture from 0 */
	if (!mo->slipshift)
		link = 0;

	/* otherwise, grab the SLIP */
	else
	{
		int slipentry = ((scanline + mo->yscroll) & mo->bitmapymask) >> mo->slipshift;
		link = ((*mo->slipram)[slipentry] >> mo->linkmask.shift) & mo->linkmask.mask;
	}

	/* if the last list entries were on the same scanline, overwrite them */
	if (previous)
	{
		if (previous->scanline == scanline)
			current = new_previous = previous;
		else
			match = 1;
	}
	
	/* set up the first entry with scroll and banking information */
	current->scanline = scanline;
	current->entry.data[0] = mo->xscroll;
	current->entry.data[1] = mo->yscroll;
	
	/* look for a match with the previous entry */
	if (match)
	{
		if (previous->entry.data[0] != current->entry.data[0] ||
			previous->entry.data[1] != current->entry.data[1])
			match = 0;
		previous++;
	}
	current++;

	/* visit all the sprites and copy their data into the display list */
	memset(spritevisit, 0, mo->entrycount);
	while (!spritevisit[link])
	{
		struct atarimo_entry *modata = &mo->spriteram[link + (mo->bank << mo->entrybits)];

		/* bounds checking */
		if (current >= mo->endcache)
		{
			logerror("Motion object list exceeded maximum\n");
			break;
		}

		/* start with the scanline */
		current->scanline = scanline;
		current->entry = *modata;

		/* update our match status */
		if (match)
		{
			if (previous->entry.data[0] != current->entry.data[0] ||
				previous->entry.data[1] != current->entry.data[1] ||
				previous->entry.data[2] != current->entry.data[2] ||
				previous->entry.data[3] != current->entry.data[3])
				match = 0;
			previous++;
		}
		current++;

		/* link to the next object */
		spritevisit[link] = 1;
		if (mo->linked)
			link = EXTRACT_DATA(modata, mo->linkmask);
		else
			link = (link + 1) & mo->linkmask.mask;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		mo->prevcache = new_previous;
		mo->curcache = current;
	}
}


/*---------------------------------------------------------------
	mo_usage_callback: Internal processing callback that
	marks pens used.
---------------------------------------------------------------*/

static void mo_usage_callback(struct atarimo_data *mo, const struct atarimo_entry *entry)
{
	int gfxindex = mo->gfxlookup[EXTRACT_DATA(entry, mo->gfxmask)];
	const unsigned int *usage = gfxelement[gfxindex].pen_usage;
	UINT32 *colormap = mo->process_param;
	int code = mo->codelookup[EXTRACT_DATA(entry, mo->codemask)] | (EXTRACT_DATA(entry, mo->codehighmask) << mo->codehighshift);
	int width = EXTRACT_DATA(entry, mo->widthmask) + 1;
	int height = EXTRACT_DATA(entry, mo->heightmask) + 1;
	int color = mo->colorlookup[EXTRACT_DATA(entry, mo->colormask)];
	int tiles = width * height;
	UINT32 temp = 0;
	int i;

	/* is this one to ignore? */
	if (mo->ignoremask.mask != 0 && EXTRACT_DATA(entry, mo->ignoremask) == mo->ignorevalue)
		return;

	for (i = 0; i < tiles; i++)
		temp |= usage[code++];
	colormap[color] |= temp;
}


/*---------------------------------------------------------------
	mo_render_callback: Internal processing callback that
	renders to the backing bitmap and then copies the result
	to the destination.
---------------------------------------------------------------*/

static void mo_render_callback(struct atarimo_data *mo, const struct atarimo_entry *entry)
{
	int gfxindex = mo->gfxlookup[EXTRACT_DATA(entry, mo->gfxmask)];
	const struct GfxElement *gfx = &gfxelement[gfxindex];
	const unsigned int *usage = gfx->pen_usage;
	struct osd_bitmap *bitmap = mo->process_param;
	struct ataripf_overrender_data overrender_data;
	UINT32 total_usage = 0;
	int x, y, sx, sy;

	/* extract data from the various words */
	int code = mo->codelookup[EXTRACT_DATA(entry, mo->codemask)] | (EXTRACT_DATA(entry, mo->codehighmask) << mo->codehighshift);
	int color = mo->colorlookup[EXTRACT_DATA(entry, mo->colormask)];
	int xpos = EXTRACT_DATA(entry, mo->xposmask);
	int ypos = -EXTRACT_DATA(entry, mo->yposmask);
	int hflip = EXTRACT_DATA(entry, mo->hflipmask);
	int vflip = EXTRACT_DATA(entry, mo->vflipmask);
	int width = EXTRACT_DATA(entry, mo->widthmask) + 1;
	int height = EXTRACT_DATA(entry, mo->heightmask) + 1;
	int xadv, yadv;
	
	/* is this one to ignore? */
	if (mo->ignoremask.mask != 0 && EXTRACT_DATA(entry, mo->ignoremask) == mo->ignorevalue)
	{
		if (mo->ignorecb)
			(*mo->ignorecb)(bitmap, &mo->process_clip, code, color, xpos, ypos);
		return;
	}

	/* add in the scroll positions if we're not in absolute coordinates */
	if (!EXTRACT_DATA(entry, mo->absolutemask))
	{
		xpos -= mo->process_xscroll;
		ypos -= mo->process_yscroll;
	}

	/* adjust for height */
	ypos -= height << mo->tileyshift;

	/* handle previous hold bits */
	if (mo->next_xpos != 123456)
		xpos = mo->next_xpos;
	mo->next_xpos = 123456;

	/* check for the hold bit */
	if (EXTRACT_DATA(entry, mo->neighbormask))
	{
		if (!mo->nextneighbor)
			xpos = mo->last_xpos + gfx->width;
		else
			mo->next_xpos = xpos + gfx->width;
	}
	mo->last_xpos = xpos;

	/* adjust the final coordinates */
	xpos &= mo->bitmapxmask;
	ypos &= mo->bitmapymask;
	if (xpos > Machine->visible_area.max_x) xpos -= mo->bitmapwidth;
	if (ypos > Machine->visible_area.max_y) ypos -= mo->bitmapheight;

	/* compute the overrendering clip rect */
	overrender_data.clip.min_x = xpos;
	overrender_data.clip.min_y = ypos;
	overrender_data.clip.max_x = xpos + width * gfx->width - 1;
	overrender_data.clip.max_y = ypos + height * gfx->height - 1;

	/* adjust for h flip */
	xadv = gfx->width;
	if (hflip)
	{
		xpos += (width - 1) << mo->tilexshift;
		xadv = -xadv;
	}

	/* adjust for v flip */
	yadv = gfx->height;
	if (vflip)
	{
		ypos += (height - 1) << mo->tileyshift;
		yadv = -yadv;
	}

	/* standard order is: loop over Y first, then X */
	if (!mo->swapxy)
	{
		/* loop over the height */
		for (y = 0, sy = ypos; y < height; y++, sy += yadv)
		{
			/* clip the Y coordinate */
			if (sy <= mo->process_clip.min_y - gfx->height)
			{
				code += width;
				continue;
			}
			else if (sy > mo->process_clip.max_y)
				break;
	
			/* loop over the width */
			for (x = 0, sx = xpos; x < width; x++, sx += xadv, code++)
			{
				/* clip the X coordinate */
				if (sx <= -mo->process_clip.min_x - gfx->width || sx > mo->process_clip.max_x)
					continue;
	
				/* draw the sprite */
				drawgfx(bitmap, gfx, code, color, hflip, vflip, sx, sy, &mo->process_clip, TRANSPARENCY_PEN, mo->transpen);
				
				/* also draw the raw version to the priority bitmap */
				if (mo->overrender0 || mo->overrender1)
					drawgfx(priority_bitmap, gfx, code, 0, hflip, vflip, sx, sy, &mo->process_clip, TRANSPARENCY_NONE_RAW, mo->transpen);
				
				/* track the total usage */
				total_usage |= usage[code];
			}
		}
	}
	
	/* alternative order is swapped */
	else
	{
		/* loop over the width */
		for (x = 0, sx = xpos; x < width; x++, sx += xadv)
		{
			/* clip the X coordinate */
			if (sx <= mo->process_clip.min_x - gfx->width)
			{
				code += height;
				continue;
			}
			else if (sx > mo->process_clip.max_x)
				break;
	
			/* loop over the height */
			for (y = 0, sy = ypos; y < height; y++, sy += yadv, code++)
			{
				/* clip the X coordinate */
				if (sy <= -mo->process_clip.min_y - gfx->height || sy > mo->process_clip.max_y)
					continue;
	
				/* draw the sprite */
				drawgfx(bitmap, gfx, code, color, hflip, vflip, sx, sy, &mo->process_clip, TRANSPARENCY_PEN, mo->transpen);
				
				/* also draw the raw version to the priority bitmap */
				if (mo->overrender0 || mo->overrender1)
					drawgfx(priority_bitmap, gfx, code, 0, hflip, vflip, sx, sy, &mo->process_clip, TRANSPARENCY_NONE_RAW, mo->transpen);
				
				/* track the total usage */
				total_usage |= usage[code];
			}
		}
	}

	/* handle overrendering */
	if (mo->overrender0 || mo->overrender1)
	{
		/* clip to the display */
		if (overrender_data.clip.min_x < mo->process_clip.min_x)
			overrender_data.clip.min_x = mo->process_clip.min_x;
		else if (overrender_data.clip.min_x > mo->process_clip.max_x)
			overrender_data.clip.min_x = mo->process_clip.max_x;
		if (overrender_data.clip.max_x < mo->process_clip.min_x)
			overrender_data.clip.max_x = mo->process_clip.min_x;
		else if (overrender_data.clip.max_x > mo->process_clip.max_x)
			overrender_data.clip.max_x = mo->process_clip.max_x;
		if (overrender_data.clip.min_y < mo->process_clip.min_y)
			overrender_data.clip.min_y = mo->process_clip.min_y;
		else if (overrender_data.clip.min_y > mo->process_clip.max_y)
			overrender_data.clip.min_y = mo->process_clip.max_y;
		if (overrender_data.clip.max_y < mo->process_clip.min_y)
			overrender_data.clip.max_y = mo->process_clip.min_y;
		else if (overrender_data.clip.max_y > mo->process_clip.max_y)
			overrender_data.clip.max_y = mo->process_clip.max_y;

		/* overrender the playfield */
		overrender_data.bitmap = bitmap;
		overrender_data.mousage = total_usage;
		overrender_data.mocolor = color;
		overrender_data.mopriority = EXTRACT_DATA(entry, mo->prioritymask);
		if (mo->overrender0)
			ataripf_overrender(0, mo->overrender0, &overrender_data);
		if (mo->overrender1)
			ataripf_overrender(1, mo->overrender1, &overrender_data);
	}
}


/*---------------------------------------------------------------
	mo_scanline_callback: This callback is called on SLIP
	boundaries to update the current set of motion objects.
---------------------------------------------------------------*/

static void mo_scanline_callback(int param)
{
	struct atarimo_data *mo = &atarimo[param >> 16];
	int scanline = param & 0xffff;
	int nextscanline = scanline + mo->updatescans;
	
	/* if this is scanline 0, reset things */
	/* also, adjust where we will next break */
	if (scanline == 0)
	{
		mo->curcache = mo->cache;
		mo->prevcache = NULL;
	}
	
	/* do the update */
	mo_update(mo, scanline);
	
	/* don't bother updating in the VBLANK area, just start back at 0 */
	if (nextscanline > Machine->visible_area.max_y)
		nextscanline = 0;
	timer_set(cpu_getscanlinetime(nextscanline), nextscanline | (param & ~0xffff), mo_scanline_callback);
}
