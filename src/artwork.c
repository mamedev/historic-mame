/*********************************************************************

  artwork.c

  Generic backdrop/overlay functions.

  Created by Mike Balfour - 10/01/1998
*********************************************************************/

#include "driver.h"
#include "artwork.h"

extern int png_read_backdrop(char *file_name, struct osd_bitmap **ovl_bitmap, unsigned char *palette, int *num_palette);
extern int png_read_overlay(char *file_name, struct osd_bitmap **ovl_bitmap, unsigned char *palette, int *num_palette, unsigned char *trans, int *num_trans);

/* Local variables */
static unsigned char isblack[256];

/*********************************************************************
  backdrop_refresh

  This remaps the "original" palette indexes to the abstract OS indexes
  used by MAME.  This needs to be called every time palette_recalc
  returns a non-zero value, since the mappings will have changed.
 *********************************************************************/

void backdrop_refresh(struct artwork *a)
{
    int i,j;
	int height,width;
	struct osd_bitmap *back = NULL;
	struct osd_bitmap *orig = NULL;
	int offset;

	offset = a->start_pen;
	back = a->artwork;
	orig = a->orig_artwork;
	height = a->artwork->height;
	width = a->artwork->width;

	for ( j=0; j<height; j++)
		for (i=0; i<width; i++)
			back->line[j][i] = Machine->pens[orig->line[j][i]+offset];
}

/*********************************************************************
  backdrop_set_palette

  This sets the palette colors used by the backdrop to the new colors
  passed in as palette.  The values should be stored as one byte of red,
  one byte of blue, one byte of green.  This could hopefully be used
  for special effects, like lightening and darkening the backdrop.
 *********************************************************************/

void backdrop_set_palette(struct artwork *a, unsigned char *palette)
{
	int i;

    /* Load colors into the palette */
	for (i = 0; i < a->num_pens_used; i++)
		palette_change_color(i + a->start_pen, palette[i*3], palette[i*3+1], palette[i*3+2]);

    palette_recalc();

	backdrop_refresh(a);
}


/*********************************************************************
  backdrop_load

  This is what loads your backdrop in from disk.
  start_pen = the first pen available for the backdrop to use
  max_pens = the number of pens the backdrop can use
  So, for example, suppose you want to load "dotron.png", have it
  start with color 192, and only use 48 pens.  You would call
  backdrop = backdrop_load("dotron.png",192,48);
 *********************************************************************/

struct artwork *backdrop_load(char *filename, int start_pen, int max_pens)
{
	struct osd_bitmap *picture = NULL;
    struct artwork *b = NULL;
	int scalex, scaley;

    b = (struct artwork *)malloc(sizeof(struct artwork));
    if (b == NULL)
    {
        if (errorlog) fprintf(errorlog,"Not enough memory for backdrop!\n");
        return NULL;
    }

	b->start_pen = start_pen;

	/* Get original picture */
	if (png_read_backdrop(filename, &picture, b->orig_palette, &(b->num_pens_used)) == 0)
    {
		free(b);
        return NULL;
    }

    /* Make sure we don't have too many colors */
	if (b->num_pens_used > max_pens)
	{
        if (errorlog) fprintf(errorlog,"Too many colors in backdrop.\n");
        if (errorlog) fprintf(errorlog,"Colors found: %d  Max Allowed: %d\n",b->num_pens_used,max_pens);
    	osd_free_bitmap(picture);
		free(b);
		return NULL;
	}

    /* Scale the original picture to be the same size as the visible area */
	if ((b->orig_artwork = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
        if (errorlog) fprintf(errorlog,"Not enough memory for backdrop!\n");
    	osd_free_bitmap(picture);
		free(b);
		return NULL;
	}
	fillbitmap(b->orig_artwork,0,0);

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		scalex=(Machine->drv->screen_width<<16)/(picture->height);
		scaley=(Machine->drv->screen_height<<16)/(picture->width);
	}
	else
	{
		scalex=(Machine->drv->screen_width<<16)/(picture->width);
		scaley=(Machine->drv->screen_height<<16)/(picture->height);
	}
	copybitmapzoom(b->orig_artwork,picture,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0,scalex,scaley);

    /* Create a second bitmap for public use */
	if ((b->artwork = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
        if (errorlog) fprintf(errorlog,"Not enough memory for backdrop!\n");
    	osd_free_bitmap(b->orig_artwork);
    	osd_free_bitmap(picture);
		free(b);
		return NULL;
	}

	backdrop_set_palette(b,b->orig_palette);

    /* We don't need the original any more */
	osd_free_bitmap(picture);

	return b;
}

/*********************************************************************
  artwork_free

  Don't forget to clean up when you're done with the backdrop!!!
 *********************************************************************/

void artwork_free(struct artwork *a)
{
	osd_free_bitmap(a->artwork);
	osd_free_bitmap(a->orig_artwork);
	free(a);
}

/*********************************************************************
  backdrop_black_recalc

  If you use any of the experimental backdrop draw* blitters below,
  call this once per frame.  It will catch palette changes and mark
  every black as transparent.  If it returns a non-zero value, redraw
  the whole background.
 *********************************************************************/

int backdrop_black_recalc(void)
{
    unsigned char r,g,b;
    int i;
	int redraw = 0;

    /* Determine what colors can be overwritten */
    for (i=0; i<256; i++)
    {
        osd_get_pen(i,&r,&g,&b);
        if ((r==0) && (g==0) && (b==0))
		{
			if (isblack[i] != 1)
				redraw = 1;
            isblack[i] = 1;
		}
        else
		{
			if (isblack[i] != 0)
				redraw = 1;
            isblack[i] = 0;
		}
    }

	return redraw;
}

/*********************************************************************
  draw_backdrop

  This is an experimental backdrop blitter.  How to use:
  1)  Draw the dirty background video game graphic with no transparency.
  2)  Call draw_backdrop with a clipping rectangle containing the location
      of the dirty_graphic.

  draw_backdrop will fill in everything that's colored black with the
  backdrop.
 *********************************************************************/

void draw_backdrop(struct osd_bitmap *dest,const struct osd_bitmap *src,int sx,int sy,
					const struct rectangle *clip)
{
	int ox,oy,ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	int col;
//	int *sd4;
//	int trans4,col4;
	struct rectangle myclip;

	if (!src) return;
	if (!dest) return;

    /* Rotate and swap as necessary... */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - src->width - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - src->height - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + src->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + src->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

    /* VERY IMPORTANT to mark this rectangle as dirty! :) - MAB */
	osd_mark_dirty (sx,sy,ex,ey,0);

	start = sy-oy;
	dy = 1;

	for (y = sy;y <= ey;y++)
	{
		bm = dest->line[y];
		bme = bm + ex;
		sd = src->line[start] + (sx-ox);
		for( bm = bm+sx ; bm <= bme ; bm++ )
		{
			if (isblack[*bm])
				*bm = *sd;
			sd++;
		}
		start+=dy;
	}
}


/*********************************************************************
  drawgfx_backdrop

  This is an experimental backdrop blitter.  How to use:

  Every time you want to draw a background tile, instead of calling
  drawgfx, call this and pass in the backdrop bitmap.  Wherever the
  tile is black, the backdrop will be drawn.
 *********************************************************************/
void drawgfx_backdrop(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,const struct osd_bitmap *back)
{
	int ox,oy,ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	int col;
//	int *sd4;
//	int trans4,col4;
	struct rectangle myclip;

	const unsigned char *sb;

	if (!gfx) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - gfx->width - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - gfx->height - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	/* start = code * gfx->height; */
	if (flipy)	/* Y flop */
	{
		start = code * gfx->height + gfx->height-1 - (sy-oy);
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height + (sy-oy);
		dy = 1;
	}


	if (gfx->colortable)	/* remap colors */
	{
		const unsigned short *paldata;	/* ASG 980209 */

		paldata = &gfx->colortable[gfx->color_granularity * color];

		if (flipx)	/* X flip */
		{
			for (y = sy;y <= ey;y++)
			{
				bm  = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
                sb = back->line[y] + sx;
				for( bm += sx ; bm <= bme ; bm++ )
				{
					if (isblack[paldata[*sd]])
                        *bm = *sb;
                    else
						*bm = paldata[*sd];
					sd--;
                    sb++;
				}
				start+=dy;
			}
		}
		else		/* normal */
		{
			for (y = sy;y <= ey;y++)
			{
				bm  = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + (sx-ox);
                sb = back->line[y] + sx;
				for( bm += sx ; bm <= bme ; bm++ )
				{
					if (isblack[paldata[*sd]])
                        *bm = *sb;
                    else
						*bm = paldata[*sd];
					sd++;
                    sb++;
				}
				start+=dy;
			}
		}
	}
	else
	{
		if (flipx)	/* X flip */
		{
			for (y = sy;y <= ey;y++)
			{
				bm = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
                sb = back->line[y] + sx;
				for( bm = bm+sx ; bm <= bme ; bm++ )
				{
					if (isblack[*sd])
                        *bm = *sb;
                    else
						*bm = *sd;
					sd--;
                    sb++;
				}
				start+=dy;
			}
		}
		else		/* normal */
		{
			for (y = sy;y <= ey;y++)
			{
				bm = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + (sx-ox);
                sb = back->line[y] + sx;
				for( bm = bm+sx ; bm <= bme ; bm++ )
				{
					if (isblack[*sb])
                        *bm = *sb;
                    else
						*bm = *sd;
					sd++;
                    sb++;
				}
				start+=dy;
			}
		}
	}
}


