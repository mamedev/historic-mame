/*##########################################################################

	atarian.h

	Common alphanumerics management functions for Atari raster games.

##########################################################################*/

#ifndef __ATARIAN__
#define __ATARIAN__


/*##########################################################################
	CONSTANTS
##########################################################################*/

#define ATARIAN_MAX		2


/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* description of the alphanumerics */
struct atarian_desc
{
	UINT8				gfxindex;			/* index to which gfx system */
	UINT8				cols, rows;			/* size of the alpha RAM in tiles (x,y) */

	UINT16				palettebase;		/* index of palette base */
	UINT16				maxcolors;			/* maximum number of colors */
	UINT16				palettesplit;		/* mask of the palette split */

	UINT32				tilemask;			/* tile data index mask */
	UINT32				colormask;			/* tile data color mask */
	UINT32				hflipmask;			/* tile data hflip mask */
	UINT32				opaquemask;			/* tile data opacity mask */
};



/*##########################################################################
	MACROS
##########################################################################*/

/* accessors for the lookup table */
#define ATARIAN_LOOKUP_DATABITS				9
#define ATARIAN_LOOKUP_DATAMASK				((1 << ATARIAN_LOOKUP_DATABITS) - 1)
#define ATARIAN_LOOKUP_CODEMASK				(0xffff ^ ATARIAN_LOOKUP_DATAMASK)

#define ATARIAN_LOOKUP_CODE(lookup,data)	(((lookup) & ATARIAN_LOOKUP_CODEMASK) | ((data) & ATARIAN_LOOKUP_DATAMASK))
#define ATARIAN_LOOKUP_COLOR(lookup)		(((lookup) >> 16) & 0x7ff)
#define ATARIAN_LOOKUP_HFLIP(lookup)		(((lookup) >> 27) & 1)
#define ATARIAN_LOOKUP_OPAQUE(lookup)		(((lookup) >> 28) & 1)
#define ATARIAN_LOOKUP_GFX(lookup)			(((lookup) >> 29) & 7)

#define ATARIAN_LOOKUP_ENTRY(gfxindex, code, color, hflip, opaque)		\
			(((gfxindex) & 7) << 29) |									\
			(((opaque) & 1) << 28) |									\
			(((hflip) & 1) << 27) |										\
			(((color) & 0x7ff) << 16) |									\
			(((code) % Machine->gfx[gfxindex]->total_elements) & ATARIAN_LOOKUP_CODEMASK)

#define ATARIAN_LOOKUP_SET_CODE(data,code)		((data) = ((data) & ~ATARIAN_LOOKUP_CODEMASK) | ((code) & ATARIAN_LOOKUP_CODEMASK))
#define ATARIAN_LOOKUP_SET_COLOR(data,color)	((data) = ((data) & ~0x07ff0000) | (((color) << 16) & 0x07ff0000))
#define ATARIAN_LOOKUP_SET_HFLIP(data,hflip)	((data) = ((data) & ~0x08000000) | (((hflip) << 27) & 0x08000000))
#define ATARIAN_LOOKUP_SET_OPAQUE(data,opq)		((data) = ((data) & ~0x10000000) | (((opq) << 28) & 0x10000000))
#define ATARIAN_LOOKUP_SET_GFX(data,gfx)		((data) = ((data) & ~0xe0000000) | (((gfx) << 29) & 0xe0000000))


/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

/* setup/shutdown */
int atarian_init(int map, const struct atarian_desc *desc);
void atarian_free(void);
UINT32 *atarian_get_lookup(int map, int *size);

/* core processing */
void atarian_render(int map, struct osd_bitmap *bitmap);

/* atrribute setters */
void atarian_set_bankbits(int map, int bankbits);

/* atrribute getters */
int atarian_get_bankbits(int map);
data16_t *atarian_get_vram(int map);

/* write handlers */
WRITE16_HANDLER( atarian_0_vram_w );
WRITE16_HANDLER( atarian_1_vram_w );

WRITE32_HANDLER( atarian_0_vram32_w );



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

extern data16_t *atarian_0_base;
extern data16_t *atarian_1_base;

extern data32_t *atarian_0_base32;


#endif
