/*##########################################################################

	atarirle.h
	
	Common RLE-based motion object management functions for early 90's
	Atari raster games.

##########################################################################*/

#ifndef __ATARIRLE__
#define __ATARIRLE__

#include "ataripf.h"


/*##########################################################################
	CONSTANTS
##########################################################################*/

/* maximum number of motion object processors */
#define ATARIRLE_MAX		1



/*##########################################################################
	TYPES & STRUCTURES
##########################################################################*/

/* description for an eight-word mask */
struct atarirle_entry
{
	data16_t			data[8];
};

/* description of the motion objects */
struct atarirle_desc
{
	UINT8				region;				/* region where the GFX data lives */
	UINT16				spriteramentries;	/* number of entries in sprite RAM */
	UINT16				leftclip;			/* left clip coordinate */
	UINT16				rightclip;			/* right clip coordinate */
	
	UINT16				palettebase;		/* base palette entry */
	UINT16				maxcolors;			/* maximum number of colors */

	struct atarirle_entry codemask;			/* mask for the code index */
	struct atarirle_entry colormask;		/* mask for the color */
	struct atarirle_entry xposmask;			/* mask for the X position */
	struct atarirle_entry yposmask;			/* mask for the Y position */
	struct atarirle_entry scalemask;		/* mask for the scale factor */
	struct atarirle_entry hflipmask;		/* mask for the horizontal flip */
	struct atarirle_entry vflipmask;		/* mask for the vertical flip */
	struct atarirle_entry prioritymask;		/* mask for the priority */
};



/*##########################################################################
	FUNCTION PROTOTYPES
##########################################################################*/

/* setup/shutdown */
int atarirle_init(int map, const struct atarirle_desc *desc);
void atarirle_free(void);

/* core processing */
void atarirle_mark_palette(int map);
void atarirle_render(int map, struct osd_bitmap *bitmap, ataripf_overrender_cb callback);

/* attribute setters */
void atarirle_set_xscroll(int map, int xscroll, int scanline);
void atarirle_set_yscroll(int map, int xscroll, int scanline);

/* attribute getters */
int atarirle_get_xscroll(int map);
int atarirle_get_yscroll(int map);

/* write handlers */
WRITE16_HANDLER( atarirle_0_spriteram_w );

WRITE32_HANDLER( atarirle_0_spriteram32_w );



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

extern data16_t *atarirle_0_spriteram;

extern data32_t *atarirle_0_spriteram32;


#endif
