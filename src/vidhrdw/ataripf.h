/*##########################################################################

	ataripf.h

	Common playfield management functions for Atari raster games.

##########################################################################*/

#ifndef __ATARIPF__
#define __ATARIPF__

/*##########################################################################
	CONSTANTS
##########################################################################*/

/* maximum number of playfields */
#define ATARIPF_MAX			2

/* overrendering constants */
#define OVERRENDER_BEGIN	0
#define OVERRENDER_QUERY	1
#define OVERRENDER_FINISH	2

/* return results for OVERRENDER_BEGIN case */
#define OVERRENDER_NONE		0
#define OVERRENDER_SOME		1
#define OVERRENDER_ALL		2

/* return results for OVERRENDER_QUERY case */
#define OVERRENDER_NO		0
#define OVERRENDER_YES		1

/* latch masks */
#define LATCHMASK_NONE		0x0000
#define LATCHMASK_MSB		0xff00
#define LATCHMASK_LSB		0x00ff

/* base granularity for all playfield gfx */
#define ATARIPF_BASE_GRANULARITY_SHIFT	3
#define ATARIPF_BASE_GRANULARITY		(1 << ATARIPF_BASE_GRANULARITY_SHIFT)



/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* description of the playfield */
struct ataripf_desc
{
	UINT8				gfxindex;			/* index to which gfx system */
	UINT8				cols, rows;			/* size of the playfield in tiles (x,y) */
	UINT8				xmult, ymult;		/* tile_index = y * ymult + x * xmult (xmult,ymult) */

	UINT16				palettebase;		/* index of palette base */
	UINT16				maxcolors;			/* maximum number of colors */
	UINT8				shadowxor;			/* color XOR for shadow effect (if any) */
	UINT16				latchmask;			/* latch mask */
	UINT32				transpens;			/* transparent pen mask */

	UINT32				tilemask;			/* tile data index mask */
	UINT32				colormask;			/* tile data color mask */
	UINT32				hflipmask;			/* tile data hflip mask */
	UINT32				vflipmask;			/* tile data hflip mask */
	UINT32				prioritymask;		/* tile data priority mask */
};


/* data used for overrendering */
struct ataripf_overrender_data
{
	/* these are passed in to ataripf_overrender */
	struct mame_bitmap *	bitmap;				/* bitmap we're drawing to */
	struct rectangle 	clip;				/* clip region to overrender with */
	UINT32				mousage;			/* motion object pen usage */
	UINT32				mocolor;			/* motion object color */
	UINT32				mopriority;			/* motion object priority */

	/* these are filled in for the callback's usage */
	UINT32				pfcolor;			/* playfield tile color */
	UINT32				pfpriority;			/* playfield tile priority */

	/* these can be modified by the callback, along with pfcolor, above */
	int					drawmode;			/* how should the tile be drawn */
	int					drawpens;			/* which pens? */
	int					maskpens;			/* mask pens */
};


/* overrendering callback function */
typedef int (*ataripf_overrender_cb)(struct ataripf_overrender_data *data, int stage);



/*##########################################################################
	MACROS
##########################################################################*/

/* accessors for the lookup table */
#define ATARIPF_LOOKUP_DATABITS				8
#define ATARIPF_LOOKUP_DATAMASK				((1 << ATARIPF_LOOKUP_DATABITS) - 1)
#define ATARIPF_LOOKUP_CODEMASK				(0xffff ^ ATARIPF_LOOKUP_DATAMASK)

#define ATARIPF_LOOKUP_CODE(lookup,data)	(((lookup) & ATARIPF_LOOKUP_CODEMASK) | ((data) & ATARIPF_LOOKUP_DATAMASK))
#define ATARIPF_LOOKUP_COLOR(lookup)		(((lookup) >> 16) & 0xff)
#define ATARIPF_LOOKUP_HFLIP(lookup)		(((lookup) >> 24) & 1)
#define ATARIPF_LOOKUP_VFLIP(lookup)		(((lookup) >> 25) & 1)
#define ATARIPF_LOOKUP_PRIORITY(lookup)		(((lookup) >> 26) & 7)
#define ATARIPF_LOOKUP_GFX(lookup)			(((lookup) >> 29) & 7)

#define ATARIPF_LOOKUP_ENTRY(gfxindex, code, color, hflip, vflip, priority)	\
			(((gfxindex) & 7) << 29) |										\
			(((priority) & 7) << 26) |										\
			(((vflip) & 1) << 25) |											\
			(((hflip) & 1) << 24) |											\
			(((color) & 0xff) << 16) |										\
			(((code) % Machine->gfx[gfxindex]->total_elements) & ATARIPF_LOOKUP_CODEMASK)

#define ATARIPF_LOOKUP_SET_CODE(data,code)		((data) = ((data) & ~ATARIPF_LOOKUP_CODEMASK) | ((code) & ATARIPF_LOOKUP_CODEMASK))
#define ATARIPF_LOOKUP_SET_COLOR(data,color)	((data) = ((data) & ~0x00ff0000) | (((color) << 16) & 0x00ff0000))
#define ATARIPF_LOOKUP_SET_HFLIP(data,hflip)	((data) = ((data) & ~0x01000000) | (((hflip) << 24) & 0x01000000))
#define ATARIPF_LOOKUP_SET_VFLIP(data,vflip)	((data) = ((data) & ~0x02000000) | (((vflip) << 25) & 0x02000000))
#define ATARIPF_LOOKUP_SET_PRIORITY(data,pri)	((data) = ((data) & ~0x1c000000) | (((pri) << 26) & 0x1c000000))
#define ATARIPF_LOOKUP_SET_GFX(data,gfx)		((data) = ((data) & ~0xe0000000) | (((gfx) << 29) & 0xe0000000))



/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

/* preinitialization */
void ataripf_blend_gfx(int gfx0, int gfx1, int mask0, int mask1);

/* setup/shutdown */
int ataripf_init(int map, const struct ataripf_desc *desc);
void ataripf_free(void);
UINT32 *ataripf_get_lookup(int map, int *size);

/* core processing */
void ataripf_render(int map, struct mame_bitmap *bitmap);
void ataripf_overrender(int map, ataripf_overrender_cb callback, struct ataripf_overrender_data *data);

/* atrribute setters */
void ataripf_set_bankbits(int map, int bankbits, int scanline);
void ataripf_set_xscroll(int map, int xscroll, int scanline);
void ataripf_set_yscroll(int map, int yscroll, int scanline);
void ataripf_set_latch(int map, int latch);
void ataripf_set_latch_lo(int latch);
void ataripf_set_latch_hi(int latch);
void ataripf_set_transparent_pens(int map, int pens);

/* atrribute getters */
int ataripf_get_bankbits(int map);
int ataripf_get_xscroll(int map);
int ataripf_get_yscroll(int map);
UINT32 *ataripf_get_vram(int map);

/* write handlers */
WRITE16_HANDLER( ataripf_0_simple_w );
WRITE16_HANDLER( ataripf_0_latched_w );
WRITE16_HANDLER( ataripf_0_upper_msb_w );
WRITE16_HANDLER( ataripf_0_upper_lsb_w );
WRITE16_HANDLER( ataripf_0_large_w );
WRITE16_HANDLER( ataripf_0_split_w );

WRITE16_HANDLER( ataripf_1_simple_w );
WRITE16_HANDLER( ataripf_1_latched_w );

WRITE16_HANDLER( ataripf_01_upper_lsb_msb_w );

WRITE32_HANDLER( ataripf_0_split32_w );



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

extern data16_t *ataripf_0_base;
extern data16_t *ataripf_0_upper;
extern data16_t *ataripf_1_base;
extern data16_t *ataripf_1_upper;

extern data32_t *ataripf_0_base32;


#endif
