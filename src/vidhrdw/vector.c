/******************************************************************************
 *
 * vector.c
 *
 *
 * Copyright 1997,1998 by the M.A.M.E. Project
 *
 *        anti-alias code by Andrew Caldwell
 *        (still more to add)
 *
 * 980611 use translucent vectors. Thanks to Peter Hirschberg
 *        and Neil Bradley for the inspiration. BW
 * 980307 added cleverer dirty handling. BW, ASG
 *        fixed antialias table .ac
 * 980221 rewrote anti-alias line draw routine
 *        added inline assembly multiply fuction for 8086 based machines
 *        beam diameter added to draw routine
 *        beam diameter is accurate in anti-alias line draw (Tcosin)
 *        flicker added .ac
 * 980203 moved LBO's routines for drawing into a buffer of vertices
 *        from avgdvg.c to this location. Scaling is now initialized
 *        by calling vector_init(...). BW
 * 980202 moved out of msdos.c ASG
 * 980124 added anti-alias line draw routine
 *        modified avgdvg.c and sega.c to support new line draw routine
 *        added two new tables Tinten and Tmerge (for 256 color support)
 *        added find_color routine to build above tables .ac
 * 010903 added support for direct RGB modes MLR
 *
 **************************************************************************** */

/* GLmame and FXmame provide their own vector implementations */
#if !(defined xgl) && !(defined xfx) && !(defined svgafx)

#include <math.h>
#include "osinline.h"
#include "driver.h"
#include "osdepend.h"
#include "vector.h"
#include "artwork.h"

#define VCLEAN  0
#define VDIRTY  1
#define VCLIP   2

unsigned char *vectorram;
size_t vectorram_size;

static int vector_orientation;

static int antialias;                            /* flag for anti-aliasing */
static int beam;                                 /* size of vector beam    */
static int flicker;                              /* beam flicker value     */
int translucency;

static int beam_diameter_is_one;		  /* flag that beam is one pixel wide */

static int vector_scale_x;                /* scaling to screen */
static int vector_scale_y;                /* scaling to screen */

static float gamma_correction = 1.2;
static float flicker_correction = 0.0;
static float intensity_correction = 1.5;

/* The vectices are buffered here */
typedef struct
{
	int x; int y;
	int col;
	int intensity;
	int arg1; int arg2; /* start/end in pixel array or clipping info */
	int status;         /* for dirty and clipping handling */
} point;

static point *new_list;
static point *old_list;
static int new_index;
static int old_index;

/* coordinates of pixels are stored here for faster removal */
static unsigned int *pixel;
static int p_index=0;

static UINT32 *pTcosin;            /* adjust line width */

#define Tcosin(x)   pTcosin[(x)]          /* adjust line width */

#define ANTIALIAS_GUNBIT  6             /* 6 bits per gun in vga (1-8 valid) */
#define ANTIALIAS_GUNNUM  (1<<ANTIALIAS_GUNBIT)

static UINT8 Tgamma[256];         /* quick gamma anti-alias table  */
static UINT8 Tgammar[256];        /* same as above, reversed order */

static struct mame_bitmap *vecbitmap;
static int vecwidth, vecheight;
static int vecshift;
static int xmin, ymin, xmax, ymax; /* clipping area */

static int vector_runs;	/* vector runs per refresh */

static void (*vector_draw_aa_pixel)(int x, int y, int col, int dirty);

static void vector_draw_aa_pixel_15 (int x, int y, int col, int dirty);
static void vector_draw_aa_pixel_32 (int x, int y, int col, int dirty);

/*
 * multiply and divide routines for drawing lines
 * can be be replaced by an assembly routine in osinline.h
 */
#ifndef vec_mult
INLINE int vec_mult(int parm1, int parm2)
{
	int temp,result;

	temp     = abs(parm1);
	result   = (temp&0x0000ffff) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp&0x0000ffff) * (parm2>>16       );
	result  += (temp>>16       ) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp>>16       ) * (parm2>>16       );

	if( parm1 < 0 )
		return(-result);
	else
		return( result);
}
#endif

/* can be be replaced by an assembly routine in osinline.h */
#ifndef vec_div
INLINE int vec_div(int parm1, int parm2)
{
	if( (parm2>>12) )
	{
		parm1 = (parm1<<4) / (parm2>>12);
		if( parm1 > 0x00010000 )
			return( 0x00010000 );
		if( parm1 < -0x00010000 )
			return( -0x00010000 );
		return( parm1 );
	}
	return( 0x00010000 );
}
#endif

#define Tinten(intensity, col) \
((((col) & 0xff) * (intensity)) >> 8) \
	| (((((col) >> 8) & 0xff) * (intensity)) & 0xff00) \
	| (((((col) >> 16) * (intensity)) >> 8) << 16)

/* MLR 990316 new gamma handling added */
void vector_set_gamma(float _gamma)
{
	int i, h;

	gamma_correction = _gamma;

	for (i = 0; i < 256; i++)
	{
		h = 255.0*pow(i/255.0, 1.0/gamma_correction);
		if( h > 255) h = 255;
		Tgamma[i] = Tgammar[255-i] = h;
	}
}

float vector_get_gamma(void)
{
	return gamma_correction;
}

void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	flicker = (int)(flicker_correction * 2.55);
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

void vector_set_intensity(float _intensity)
{
	intensity_correction = _intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}

/*
 * Initializes vector game video emulation
 */

int vector_vh_start (void)
{
	int i;

	/* Grab the settings for this session */
	antialias = options.antialias;
	translucency = options.translucency;
	vector_set_flicker(options.vector_flicker);
	beam = options.beam;


	if (beam == 0x00010000)
		beam_diameter_is_one = 1;
	else
		beam_diameter_is_one = 0;

	p_index = 0;

	new_index = 0;
	old_index = 0;
	vector_runs = 0;

	switch(Machine->color_depth)
	{
	case 15:
		vector_draw_aa_pixel = vector_draw_aa_pixel_15;
		break;
	case 32:
		vector_draw_aa_pixel = vector_draw_aa_pixel_32;
		break;
	default:
		logerror ("Vector games have to use direct RGB modes!\n");
		return 1;
		break;
	}

	/* allocate memory for tables */
	pTcosin = malloc ( (2048+1) * sizeof(INT32));   /* yes! 2049 is correct */
	pixel = malloc (MAX_PIXELS * sizeof (UINT32));
	old_list = malloc (MAX_POINTS * sizeof (point));
	new_list = malloc (MAX_POINTS * sizeof (point));

	/* did we get the requested memory? */
	if (!(pTcosin && pixel && old_list && new_list))
	{
		/* vector_vh_stop should better be called by the main engine */
		/* if vector_vh_start fails */
		vector_vh_stop();
		return 1;
	}

	/* build cosine table for fixing line width in antialias */
	for (i=0; i<=2048; i++)
	{
		Tcosin(i) = (int)((double)(1.0/cos(atan((double)(i)/2048.0)))*0x10000000 + 0.5);
	}

	vector_set_flip_x(0);
	vector_set_flip_y(0);
	vector_set_swap_xy(0);

	/* build gamma correction table */
	vector_set_gamma (gamma_correction);

	return 0;
}



void vector_set_flip_x (int flip)
{
	if (flip)
		vector_orientation |=  ORIENTATION_FLIP_X;
	else
		vector_orientation &= ~ORIENTATION_FLIP_X;
}

void vector_set_flip_y (int flip)
{
	if (flip)
		vector_orientation |=  ORIENTATION_FLIP_Y;
	else
		vector_orientation &= ~ORIENTATION_FLIP_Y;
}

void vector_set_swap_xy (int swap)
{
	if (swap)
		vector_orientation |=  ORIENTATION_SWAP_XY;
	else
		vector_orientation &= ~ORIENTATION_SWAP_XY;
}


/*
 * Setup scaling. Currently the Sega games are stuck at VECSHIFT 15
 * and the the AVG games at VECSHIFT 16
 */
void vector_set_shift (int shift)
{
	vecshift = shift;
}

/*
 * Clear the old bitmap. Delete pixel for pixel, this is faster than memset.
 */
static void vector_clear_pixels (void)
{
	int i;
	int coords;

	if (Machine->color_depth == 32)
	{
		for (i=p_index-1; i>=0; i--)
		{
			coords = pixel[i];
			((UINT32 *)vecbitmap->line[coords & 0xffff])[coords >> 16] = 0;
		}
	}
	else
	{
		for (i=p_index-1; i>=0; i--)
		{
			coords = pixel[i];
			((UINT16 *)vecbitmap->line[coords & 0xffff])[coords >> 16] = 0;
		}
	}
	p_index=0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */
void vector_vh_stop (void)
{
	if (pTcosin)
		free (pTcosin);
	pTcosin = NULL;
	if (pixel)
		free (pixel);
	pixel = NULL;
	if (old_list)
		free (old_list);
	old_list = NULL;
	if (new_list)
		free (new_list);
	new_list = NULL;
}

/*
 * draws an anti-aliased pixel (blends pixel with background)
 */
#define LIMIT5(x) ((x < 0x1f)? x : 0x1f)
#define LIMIT8(x) ((x < 0xff)? x : 0xff)

static void vector_draw_aa_pixel_15 (int x, int y, int col, int dirty)
{
	UINT32 dst;

	if (x < xmin || x >= xmax)
		return;
	if (y < ymin || y >= ymax)
		return;

	dst = ((UINT16 *)vecbitmap->line[y])[x];
	((UINT16 *)vecbitmap->line[y])[x] = LIMIT5(((col>>3) & 0x1f) + (dst & 0x1f))
		| (LIMIT5(((col >> 11) & 0x1f) + ((dst >> 5) & 0x1f)) << 5)
		| (LIMIT5((col >> 19) + (dst >> 10)) << 10);

	if (p_index<MAX_PIXELS)
	{
		pixel[p_index] = y | (x << 16);
		p_index++;
	}

	/* Mark this pixel as dirty */
	if (dirty)
		osd_mark_vector_dirty (x, y);
}

static void vector_draw_aa_pixel_32 (int x, int y, int col, int dirty)
{
	UINT32 dst;

	if (x < xmin || x >= xmax)
		return;
	if (y < ymin || y >= ymax)
		return;

	dst = ((UINT32 *)vecbitmap->line[y])[x];
	((UINT32 *)vecbitmap->line[y])[x] = LIMIT8((col & 0xff) + (dst & 0xff))
		| (LIMIT8(((col >> 8) & 0xff) + ((dst >> 8) & 0xff)) << 8)
		| (LIMIT8((col >> 16) + (dst >> 16)) << 16);

	if (p_index<MAX_PIXELS)
	{
		pixel[p_index] = y | (x << 16);
		p_index++;
	}

	/* Mark this pixel as dirty */
	if (dirty)
		osd_mark_vector_dirty (x, y);
}


/*
 * draws a line
 *
 * input:   x2  16.16 fixed point
 *          y2  16.16 fixed point
 *         col  0-255 indexed color (8 bit)
 *   intensity  0-255 intensity
 *       dirty  bool  mark the pixels as dirty while plotting them
 *
 * written by Andrew Caldwell
 */

void vector_draw_to (int x2, int y2, int col, int intensity, int dirty)
{
	unsigned char a1;
	int dx,dy,sx,sy,cx,cy,width;
	static int x1,yy1;
	int xx,yy;
	int xy_swap;

	/* [1] scale coordinates to display */

	x2 = vec_mult(x2<<4,vector_scale_x);
	y2 = vec_mult(y2<<4,vector_scale_y);

	/* [2] fix display orientation */

	if ((Machine->orientation ^ vector_orientation) & ORIENTATION_SWAP_XY)
		xy_swap = 1;
	else
		xy_swap = 0;

	if (vector_orientation & ORIENTATION_FLIP_X)
	{
		if (xy_swap)
			x2 = ((vecheight-1)<<16)-x2;
		else
			x2 = ((vecwidth-1)<<16)-x2;
	}
	if (vector_orientation & ORIENTATION_FLIP_Y)
	{
		if (xy_swap)
			y2 = ((vecwidth-1)<<16)-y2;
		else
			y2 = ((vecheight-1)<<16)-y2;
	}

	if ((Machine->orientation ^ vector_orientation) & ORIENTATION_SWAP_XY)
	{
		int temp;
		temp = x2;
		x2 = y2;
		y2 = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x2 = ((vecwidth-1)<<16)-x2;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y2 = ((vecheight-1)<<16)-y2;

	/* [3] adjust cords if needed */

	if (antialias)
	{
		if(beam_diameter_is_one)
		{
			x2 = (x2+0x8000)&0xffff0000;
			y2 = (y2+0x8000)&0xffff0000;
		}
	}
	else /* noantialiasing */
	{
		x2 >>= 16;
		y2 >>= 16;
	}

	/* [4] handle color and intensity */

	if (intensity == 0) goto end_draw;

	col = Tinten(intensity,col);

	/* [5] draw line */

	if (antialias)
	{
		/* draw an anti-aliased line */
		dx=abs(x1-x2);
		dy=abs(yy1-y2);
		if (dx>=dy)
		{
			sx = ((x1 <= x2) ? 1:-1);
			sy = vec_div(y2-yy1,dx);
			if (sy<0)
				dy--;
			x1 >>= 16;
			xx = x2>>16;
			width = vec_mult(beam<<4,Tcosin(abs(sy)>>5));
			if (!beam_diameter_is_one)
				yy1-= width>>1; /* start back half the diameter */
			for (;;)
			{
				dx = width;    /* init diameter of beam */
				dy = yy1>>16;
				vector_draw_aa_pixel(x1,dy++,Tinten(Tgammar[0xff&(yy1>>8)],col), dirty);
				dx -= 0x10000-(0xffff & yy1); /* take off amount plotted */
				a1 = Tgamma[(dx>>8)&0xff];   /* calc remainder pixel */
				dx >>= 16;                   /* adjust to pixel (solid) count */
				while (dx--)                 /* plot rest of pixels */
					vector_draw_aa_pixel(x1,dy++,col, dirty);
				vector_draw_aa_pixel(x1,dy,Tinten(a1,col), dirty);
				if (x1 == xx) break;
				x1+=sx;
				yy1+=sy;
			}
		}
		else
		{
			sy = ((yy1 <= y2) ? 1:-1);
			sx = vec_div(x2-x1,dy);
			if (sx<0)
				dx--;
			yy1 >>= 16;
			yy = y2>>16;
			width = vec_mult(beam<<4,Tcosin(abs(sx)>>5));
			if( !beam_diameter_is_one )
				x1-= width>>1; /* start back half the width */
			for (;;)
			{
				dy = width;    /* calc diameter of beam */
				dx = x1>>16;
				vector_draw_aa_pixel(dx++,yy1,Tinten(Tgammar[0xff&(x1>>8)],col), dirty);
				dy -= 0x10000-(0xffff & x1); /* take off amount plotted */
				a1 = Tgamma[(dy>>8)&0xff];   /* remainder pixel */
				dy >>= 16;                   /* adjust to pixel (solid) count */
				while (dy--)                 /* plot rest of pixels */
					vector_draw_aa_pixel(dx++,yy1,col, dirty);
				vector_draw_aa_pixel(dx,yy1,Tinten(a1,col), dirty);
				if (yy1 == yy) break;
				yy1+=sy;
				x1+=sx;
			}
		}
	}
	else /* use good old Bresenham for non-antialiasing 980317 BW */
	{
		dx = abs(x1-x2);
		dy = abs(yy1-y2);
		sx = (x1 <= x2) ? 1: -1;
		sy = (yy1 <= y2) ? 1: -1;
		cx = dx/2;
		cy = dy/2;

		if (dx>=dy)
		{
			for (;;)
			{
				vector_draw_aa_pixel (x1, yy1, col, dirty);
				if (x1 == x2) break;
				x1 += sx;
				cx -= dy;
				if (cx < 0)
				{
					yy1 += sy;
					cx += dx;
				}
			}
		}
		else
		{
			for (;;)
			{
				vector_draw_aa_pixel (x1, yy1, col, dirty);
				if (yy1 == y2) break;
				yy1 += sy;
				cy -= dx;
				if (cy < 0)
				{
					x1 += sx;
					cy += dy;
				}
			}
		}
	}

end_draw:

	x1=x2;
	yy1=y2;
}


/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
void vector_add_point (int x, int y, int color, int intensity)
{
	point *new;

	intensity *= intensity_correction;
	if (intensity > 0xff)
		intensity = 0xff;

	if (flicker && (intensity > 0))
	{
		intensity += (intensity * (0x80-(rand()&0xff)) * flicker)>>16;
		if (intensity < 0)
			intensity = 0;
		if (intensity > 0xff)
			intensity = 0xff;
	}
	new = &new_list[new_index];
	new->x = x;
	new->y = y;
	new->col = color;
	new->intensity = intensity;
	new->status = VDIRTY; /* mark identical lines as clean later */

	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}

/*
 * Add new clipping info to the list
 */
void vector_add_clip (int x1, int yy1, int x2, int y2)
{
	point *new;

	new = &new_list[new_index];
	new->x = x1;
	new->y = yy1;
	new->arg1 = x2;
	new->arg2 = y2;
	new->status = VCLIP;

	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}


/*
 * Set the clipping area
 */
void vector_set_clip (int x1, int yy1, int x2, int y2)
{
	int tmp;

	/* failsafe */
	if ((x1 >= x2) || (yy1 >= y2))
	{
		logerror("Error in clipping parameters.\n");
		xmin = 0;
		ymin = 0;
		xmax = vecwidth;
		ymax = vecheight;
		return;
	}

	/* scale coordinates to display */
	x1 = vec_mult(x1<<4,vector_scale_x);
	yy1 = vec_mult(yy1<<4,vector_scale_y);
	x2 = vec_mult(x2<<4,vector_scale_x);
	y2 = vec_mult(y2<<4,vector_scale_y);

	/* fix orientation */

	/* don't forget to swap x1,x2, since x2 becomes the minimum */
	if (vector_orientation & ORIENTATION_FLIP_X)
	{
		x1 = ((vecwidth-1)<<16)-x1;
		x2 = ((vecwidth-1)<<16)-x2;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	/* don't forget to swap yy1,y2, since y2 becomes the minimum */
	if (vector_orientation & ORIENTATION_FLIP_Y)
	{
		yy1 = ((vecheight-1)<<16)-yy1;
		y2 = ((vecheight-1)<<16)-y2;
		tmp = yy1; yy1 = y2; y2 = tmp;
	}
	/* swapping x/y coordinates will still have the minima in x1,yy1 */
	if ((Machine->orientation ^ vector_orientation) & ORIENTATION_SWAP_XY)
	{
		tmp = x1; x1 = yy1; yy1 = tmp;
		tmp = x2; x2 = y2; y2 = tmp;
	}
	/* don't forget to swap x1,x2, since x2 becomes the minimum */
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		x1 = ((vecwidth-1)<<16)-x1;
		x2 = ((vecwidth-1)<<16)-x2;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	/* don't forget to swap yy1,y2, since y2 becomes the minimum */
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		yy1 = ((vecheight-1)<<16)-yy1;
		y2 = ((vecheight-1)<<16)-y2;
		tmp = yy1; yy1 = y2; y2 = tmp;
	}

	xmin = x1 >> 16;
	ymin = yy1 >> 16;
	xmax = x2 >> 16;
	ymax = y2 >> 16;

	/* Make it foolproof by trapping rounding errors */
	if (xmin < 0) xmin = 0;
	if (ymin < 0) ymin = 0;
	if (xmax > vecwidth) xmax = vecwidth;
	if (ymax > vecheight) ymax = vecheight;
}


/*
 * The vector CPU creates a new display list. We save the old display list,
 * but only once per refresh.
 */
void vector_clear_list (void)
{
	point *tmp;

	if (vector_runs == 0)
	{
		old_index = new_index;
		tmp = old_list; old_list = new_list; new_list = tmp;
	}

	new_index = 0;
	vector_runs++;
}


/*
 * By comparing with the last drawn list, we can prevent that identical
 * vectors are marked dirty which appeared at the same list index in the
 * previous frame. BW 19980307
 */
static void clever_mark_dirty (void)
{
	int i, j, min_index, last_match = 0;
	unsigned int *coords;
	point *new, *old;
	point newclip, oldclip;
	int clips_match = 1;

	if (old_index < new_index)
		min_index = old_index;
	else
		min_index = new_index;

	/* Reset the active clips to invalid values */
	memset (&newclip, 0, sizeof (newclip));
	memset (&oldclip, 0, sizeof (oldclip));

	/* Mark vectors which are not the same in both lists as dirty */
	new = new_list;
	old = old_list;

	for (i = min_index; i > 0; i--, old++, new++)
	{
		/* If this is a clip, we need to determine if the clip regions still match */
		if (old->status == VCLIP || new->status == VCLIP)
		{
			if (old->status == VCLIP)
				oldclip = *old;
			if (new->status == VCLIP)
				newclip = *new;
			clips_match = (newclip.x == oldclip.x) && (newclip.y == oldclip.y) && (newclip.arg1 == oldclip.arg1) && (newclip.arg2 == oldclip.arg2);
			if (!clips_match)
				last_match = 0;

			/* fall through to erase the old line if this is not a clip */
			if (old->status == VCLIP)
				continue;
		}

		/* If the clips match and the vectors match, update */
		else if (clips_match && (new->x == old->x) && (new->y == old->y) &&
			(new->col == old->col) && (new->intensity == old->intensity))
		{
			if (last_match)
			{
				new->status = VCLEAN;
				continue;
			}
			last_match = 1;
		}

		/* If nothing matches, remember it */
		else
			last_match = 0;

		/* mark the pixels of the old vector dirty */
		coords = &pixel[old->arg1];
		for (j = (old->arg2 - old->arg1); j > 0; j--)
		{
			osd_mark_vector_dirty (*coords >> 16, *coords & 0x0000ffff);
			coords++;
		}
	}

	/* all old vector with index greater new_index are dirty */
	/* old = &old_list[min_index] here! */
	for (i = (old_index-min_index); i > 0; i--, old++)
	{
		/* skip old clips */
		if (old->status == VCLIP)
			continue;

		/* mark the pixels of the old vector dirty */
		coords = &pixel[old->arg1];
		for (j = (old->arg2 - old->arg1); j > 0; j--)
		{
			osd_mark_vector_dirty (*coords >> 16, *coords & 0x0000ffff);
			coords++;
		}
	}
}

void vector_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int i;
	int temp_x, temp_y;
	point *new;


	if (full_refresh)
		fillbitmap(bitmap,0,NULL);


	/* copy parameters */
	vecbitmap = bitmap;
	vecwidth  = bitmap->width;
	vecheight = bitmap->height;

	/* setup scaling */
	temp_x = (1<<(44-vecshift)) / (Machine->visible_area.max_x - Machine->visible_area.min_x);
	temp_y = (1<<(44-vecshift)) / (Machine->visible_area.max_y - Machine->visible_area.min_y);

	if ((Machine->orientation ^ vector_orientation) & ORIENTATION_SWAP_XY)
	{
		vector_scale_x = temp_x * vecheight;
		vector_scale_y = temp_y * vecwidth;
	}
	else
	{
		vector_scale_x = temp_x * vecwidth;
		vector_scale_y = temp_y * vecheight;
	}
	/* reset clipping area */
	xmin = 0; xmax = vecwidth; ymin = 0; ymax = vecheight;

	/* next call to vector_clear_list() is allowed to swap the lists */
	vector_runs = 0;

	/* mark pixels which are not idential in newlist and oldlist dirty */
	/* the old pixels which get removed are marked dirty immediately,  */
	/* new pixels are recognized by setting new->dirty                 */
	clever_mark_dirty();

	/* clear ALL pixels in the hidden map */
	vector_clear_pixels();

	/* Draw ALL lines into the hidden map. Mark only those lines with */
	/* new->dirty = 1 as dirty. Remember the pixel start/end indices  */
	new = new_list;
	for (i = 0; i < new_index; i++)
	{
		if (new->status == VCLIP)
			vector_set_clip (new->x, new->y, new->arg1, new->arg2);
		else
		{
			new->arg1 = p_index;
			vector_draw_to (new->x, new->y, new->col, Tgamma[new->intensity], new->status);

			new->arg2 = p_index;
		}
		new++;
	}
}

#endif /* if !(defined xgl) && !(defined xfx) && !(defined svgafx) */
