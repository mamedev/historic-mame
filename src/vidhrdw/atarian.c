/*##########################################################################

	atarian.c

	Common alphanumerics management functions for Atari raster games.

##########################################################################*/

#include "driver.h"
#include "atarian.h"


/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* internal structure containing the state of the alphanumerics */
struct atarian_data
{
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

	int					xtiles;				/* number of visible X tiles */
	int					ytiles;				/* number of visible Y tiles */

	int					palettebase;		/* base palette entry */
	int					maxcolors;			/* maximum number of colors */

	int					lookupmask;			/* mask for the lookup table */

	UINT32 *			lookup;				/* pointer to lookup table */
	data16_t **			vram;				/* pointer to the VRAM pointer */

	int					bankbits;			/* current extra banking bits */

	struct GfxElement 	gfxelement;			/* copy of the GfxElement we're using */
};



/*##########################################################################
	MACROS
##########################################################################*/

/* verification macro for void functions */
#define VERIFY(cond, msg) if (!(cond)) { logerror(msg); return; }

/* verification macro for non-void functions */
#define VERIFYRETFREE(cond, msg, ret) if (!(cond)) { logerror(msg); atarian_free(); return (ret); }



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

data16_t *atarian_0_base;
data16_t *atarian_1_base;

data32_t *atarian_0_base32;



/*##########################################################################
	STATIC VARIABLES
##########################################################################*/

static struct atarian_data atarian[ATARIAN_MAX];
static offs_t address_xor;



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



/*##########################################################################
	GLOBAL FUNCTIONS
##########################################################################*/

/*---------------------------------------------------------------
	atarian_init: Configures the alphanumerics using the input
	description. Allocates all memory necessary.
---------------------------------------------------------------*/

int atarian_init(int map, const struct atarian_desc *desc)
{
	int lookupcount = round_to_powerof2(desc->tilemask | desc->colormask | desc->hflipmask | desc->opaquemask) >> ATARIAN_LOOKUP_DATABITS;
	struct GfxElement *gfx = Machine->gfx[desc->gfxindex];
	struct atarian_data *an = &atarian[map];
	int i;

	/* sanity checks */
	VERIFYRETFREE(map >= 0 && map < ATARIAN_MAX, "atarian_init: map out of range", 0)
	VERIFYRETFREE(compute_log(desc->cols) != -1, "atarian_init: cols must be power of 2", 0)
	VERIFYRETFREE(compute_log(desc->rows) != -1, "atarian_init: rows must be power of 2", 0)

	/* copy in the basic data */
	an->colshift     = compute_log(desc->cols);
	an->rowshift     = compute_log(desc->rows);
	an->colmask      = desc->cols - 1;
	an->rowmask      = desc->rows - 1;
	an->vrammask     = (an->colmask << an->colshift) | (an->rowmask << an->rowshift);
	an->vramsize     = round_to_powerof2(an->vrammask);

	an->tilexshift   = compute_log(gfx->width);
	an->tileyshift   = compute_log(gfx->height);
	an->tilewidth    = gfx->width;
	an->tileheight   = gfx->height;

	an->xtiles       = Machine->drv->screen_width >> an->tilexshift;
	an->ytiles       = Machine->drv->screen_height >> an->tileyshift;

	an->palettebase  = desc->palettebase;
	an->maxcolors    = desc->maxcolors / gfx->color_granularity;

	an->lookupmask   = lookupcount - 1;
	an->vram         = (map == 0) ? &atarian_0_base : &atarian_1_base;

	/* allocate the attribute lookup */
	an->lookup = malloc(lookupcount * sizeof(an->lookup[0]));
	VERIFYRETFREE(an->lookup, "atarian_init: out of memory for lookup", 0)

	/* fill in the attribute lookup */
	for (i = 0; i < lookupcount; i++)
	{
		int value    = (i << ATARIAN_LOOKUP_DATABITS);
		int tile     = collapse_bits(value, desc->tilemask);
		int color    = collapse_bits(value, desc->colormask);
		int flip     = collapse_bits(value, desc->hflipmask);
		int opaque   = collapse_bits(value, desc->opaquemask);

		if (desc->palettesplit)
			color = (color & desc->palettesplit) | ((color & ~desc->palettesplit) << 1);
		an->lookup[i] = ATARIAN_LOOKUP_ENTRY(desc->gfxindex, tile, color, flip, opaque);
	}

	/* make a copy of the original GfxElement structure */
	an->gfxelement = *Machine->gfx[desc->gfxindex];

	/* adjust the color base */
	an->gfxelement.colortable = &Machine->remapped_colortable[an->palettebase];

	/* by default we don't need to swap */
	address_xor = 0;

	/* copy the 32-bit base */
	if (cpunum_databus_width(0) == 32)
	{
		address_xor = 1;
		atarian_0_base = (data16_t *)atarian_0_base32;
	}

	logerror("atarian_init:\n");
	logerror("  width=%d (shift=%d),  height=%d (shift=%d)\n", gfx->width, an->tilexshift, gfx->height, an->tileyshift);
	logerror("  cols=%d  (mask=%X),   rows=%d   (mask=%X)\n", desc->cols, an->colmask, desc->rows, an->rowmask);
	logerror("  VRAM mask=%X\n", an->vrammask);

	return 1;
}


/*---------------------------------------------------------------
	atarian_free: Frees any memory allocated for the alphanumerics.
---------------------------------------------------------------*/

void atarian_free(void)
{
	int i;

	for (i = 0; i < ATARIAN_MAX; i++)
	{
		struct atarian_data *an = &atarian[i];

		/* free the lookup */
		if (an->lookup)
			free(an->lookup);
		an->lookup = NULL;
	}
}


/*---------------------------------------------------------------
	atarian_get_lookup: Fetches the lookup table so it can
	be modified.
---------------------------------------------------------------*/

UINT32 *atarian_get_lookup(int map, int *size)
{
	struct atarian_data *an = &atarian[map];

	if (size)
		*size = round_to_powerof2(an->lookupmask);
	return an->lookup;
}


/*---------------------------------------------------------------
	atarian_render: Render the alphanumerics to the
	destination bitmap.
---------------------------------------------------------------*/

void atarian_render(int map, struct osd_bitmap *bitmap)
{
	const struct rectangle *clip = &Machine->visible_area;
	struct atarian_data *an = &atarian[map];
	data16_t *base = *an->vram;
	int x, y;

	/* loop over rows */
	for (y = 0; y < an->ytiles; y++)
	{
		int offs = y << an->colshift;

		/* loop over columns */
		for (x = 0; x < an->xtiles; x++, offs++)
		{
			int data = base[offs ^ address_xor] | an->bankbits;
			UINT32 lookup = an->lookup[(data >> ATARIAN_LOOKUP_DATABITS) & an->lookupmask];
			int code = ATARIAN_LOOKUP_CODE(lookup, data);
			int opaque = ATARIAN_LOOKUP_OPAQUE(lookup);

			/* only process opaque tiles or non-zero tiles */
			if (code || opaque)
			{
				int color = ATARIAN_LOOKUP_COLOR(lookup);
				int hflip = ATARIAN_LOOKUP_HFLIP(lookup);

				drawgfx(bitmap, &an->gfxelement, code, color, hflip, 0,
						x << an->tilexshift, y << an->tileyshift, clip,
						opaque ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}
}


/*---------------------------------------------------------------
	atarian_set_bankbits: Set the extra banking bits for the
	alphanumerics.
---------------------------------------------------------------*/

void atarian_set_bankbits(int map, int bankbits)
{
	struct atarian_data *an = &atarian[map];
	an->bankbits = bankbits;
}


/*---------------------------------------------------------------
	atarian_get_bankbits: Returns the extra banking bits for
	the alphanumerics.
---------------------------------------------------------------*/

int atarian_get_bankbits(int map)
{
	return atarian[map].bankbits;
}


/*---------------------------------------------------------------
	atarian_get_vram: Returns a pointer to video RAM.
---------------------------------------------------------------*/

data16_t *atarian_get_vram(int map)
{
	return (map == 0) ? atarian_0_base : atarian_1_base;
}


/*---------------------------------------------------------------
	atarian_vram_w: Write handler for the alphanumerics RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarian_0_vram_w )
{
	COMBINE_DATA(&atarian_0_base[offset]);
}


/*---------------------------------------------------------------
	atarian_vram_w: Write handler for the alphanumerics RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarian_1_vram_w )
{
	COMBINE_DATA(&atarian_1_base[offset]);
}


/*---------------------------------------------------------------
	atarian_vram_w: Write handler for the alphanumerics RAM.
---------------------------------------------------------------*/

WRITE32_HANDLER( atarian_0_vram32_w )
{
	COMBINE_DATA(&atarian_0_base32[offset]);
}
