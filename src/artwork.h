/*********************************************************************

  artwork.h

  Generic backdrop/overlay functions.

  Be sure to include driver.h before including this file.

  Created by Mike Balfour - 10/01/1998
*********************************************************************/

#ifndef ARTWORK_H

#define ARTWORK_H 1

/*********************************************************************
  artwork

  This structure is a generic structure used to hold both backdrops
  and overlays.  At least that's the goal, overlays aren't supported
  yet.
*********************************************************************/
struct artwork
{
    /* Publically accessible */
    struct osd_bitmap *artwork;

    /* Private - don't touch! */
    struct osd_bitmap *orig_artwork;		/* needed for palette recalcs */
	unsigned char orig_palette[256*3];		/* needed for restoring the colors after special effects? */
    int start_pen;
    int num_pens_used;
};

/*********************************************************************
  functions that apply to backdrops AND overlays
*********************************************************************/
void artwork_free(struct artwork *a);

/*********************************************************************
  functions that are backdrop-specific
*********************************************************************/
void backdrop_refresh(struct artwork *a);
void backdrop_set_palette(struct artwork *a, unsigned char *palette);
struct artwork *backdrop_load(char *filename, int start_pen, int max_pens);
int backdrop_black_recalc(void);
void draw_backdrop(struct osd_bitmap *dest,const struct osd_bitmap *src,int sx,int sy,
					const struct rectangle *clip);
void drawgfx_backdrop(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,const struct osd_bitmap *back);

#endif

