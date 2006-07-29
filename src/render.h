/***************************************************************************

    render.h

    Core rendering routines for MAME.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __RENDER_H__
#define __RENDER_H__

#include "driver.h"
#include "osdepend.h"

#include <math.h>



/***************************************************************************

    Theory of operation
    -------------------

    A render "target" is described by 5 parameters:

        - width = width, in pixels
        - height = height, in pixels
        - bpp = depth, in bits per pixel
        - orientation = orientation of the target
        - pixel_aspect = aspect ratio of the pixels

    Width, height, and bpp are self-explanatory. The remaining parameters
    need some additional explanation.

    Regarding orientation, there are three orientations that need to be
    dealt with: target orientation, UI orientation, and game orientation.
    In the current model, the UI orientation tracks the target orientation
    so that the UI is (in theory) facing the correct direction. The game
    orientation is specified by the game driver and indicates how the
    game and artwork are rotated.

    Regarding pixel_aspect, this is the aspect ratio of the individual
    pixels, not the aspect ratio of the screen. You can determine this by
    dividing the aspect ratio of the screen by the aspect ratio of the
    resolution. For example, a 4:3 screen displaying 640x480 gives a
    pixel aspect ratio of (4/3)/(640/480) = 1.0, meaning the pixels are
    square. That same screen displaying 1280x1024 would have a pixel
    aspect ratio of (4/3)/(1280/1024) = 1.06666, meaning the pixels are
    slightly wider than they are tall.

    Artwork is always assumed to be a 1.0 pixel aspect ratio. The game
    screens themselves can be variable aspect ratios.

***************************************************************************/


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* render primitive types */
enum
{
	RENDER_PRIMITIVE_LINE,							/* a single line */
	RENDER_PRIMITIVE_QUAD							/* a rectilinear quad */
};


/* layer config masks */
#define LAYER_CONFIG_ENABLE_BACKDROP 0x01			/* enable backdrop layers */
#define LAYER_CONFIG_ENABLE_OVERLAY	0x02			/* enable overlay layers */
#define LAYER_CONFIG_ENABLE_BEZEL	0x04			/* enable bezel layers */
#define LAYER_CONFIG_ZOOM_TO_SCREEN	0x08			/* zoom to screen area by default */


/* texture formats */
#define TEXFORMAT_UNDEFINED			0				/* require a format to be specified */
#define TEXFORMAT_PALETTE16			1				/* 16bpp palettized */
#define TEXFORMAT_RGB15				2				/* 5-5-5 RGB */
#define TEXFORMAT_RGB32				3				/* 8-8-8 RGB */
#define TEXFORMAT_ARGB32			4				/* 8-8-8-8 ARGB */

/* blending modes */
#define	BLENDMODE_NONE				0				/* no blending */
#define	BLENDMODE_ALPHA				1				/* standard alpha blend */
#define BLENDMODE_RGB_MULTIPLY		2				/* apply source alpha to source pix, then multiply RGB values */
#define BLENDMODE_ADD				3				/* apply source alpha to source pix, then add to destination */


/* flags for primitives */
#define PRIMFLAG_TEXORIENT_SHIFT	0
#define PRIMFLAG_TEXORIENT_MASK		(7 << PRIMFLAG_TEXORIENT_SHIFT)
#define PRIMFLAG_TEXORIENT(x)		((x) << PRIMFLAG_TEXORIENT_SHIFT)
#define PRIMFLAG_GET_TEXORIENT(x)	(((x) & PRIMFLAG_TEXORIENT_MASK) >> PRIMFLAG_TEXORIENT_SHIFT)

#define PRIMFLAG_TEXFORMAT_SHIFT	4
#define PRIMFLAG_TEXFORMAT_MASK		(7 << PRIMFLAG_TEXFORMAT_SHIFT)
#define PRIMFLAG_TEXFORMAT(x)		((x) << PRIMFLAG_TEXFORMAT_SHIFT)
#define PRIMFLAG_GET_TEXFORMAT(x)	(((x) & PRIMFLAG_TEXFORMAT_MASK) >> PRIMFLAG_TEXFORMAT_SHIFT)

#define PRIMFLAG_BLENDMODE_SHIFT	8
#define PRIMFLAG_BLENDMODE_MASK		(7 << PRIMFLAG_BLENDMODE_SHIFT)
#define PRIMFLAG_BLENDMODE(x)		((x) << PRIMFLAG_BLENDMODE_SHIFT)
#define PRIMFLAG_GET_BLENDMODE(x)	(((x) & PRIMFLAG_BLENDMODE_MASK) >> PRIMFLAG_BLENDMODE_SHIFT)

#define PRIMFLAG_ANTIALIAS_SHIFT	12
#define PRIMFLAG_ANTIALIAS_MASK		(1 << PRIMFLAG_ANTIALIAS_SHIFT)
#define PRIMFLAG_ANTIALIAS(x)		((x) << PRIMFLAG_ANTIALIAS_SHIFT)
#define PRIMFLAG_GET_ANTIALIAS(x)	(((x) & PRIMFLAG_ANTIALIAS_MASK) >> PRIMFLAG_ANTIALIAS_SHIFT)

#define PRIMFLAG_SCREENTEX_SHIFT	13
#define PRIMFLAG_SCREENTEX_MASK		(1 << PRIMFLAG_SCREENTEX_SHIFT)
#define PRIMFLAG_SCREENTEX(x)		((x) << PRIMFLAG_SCREENTEX_SHIFT)
#define PRIMFLAG_GET_SCREENTEX(x)	(((x) & PRIMFLAG_SCREENTEX_MASK) >> PRIMFLAG_SCREENTEX_SHIFT)

#define PRIMFLAG_TEXWRAP_SHIFT		14
#define PRIMFLAG_TEXWRAP_MASK		(1 << PRIMFLAG_TEXWRAP_SHIFT)
#define PRIMFLAG_TEXWRAP(x)			((x) << PRIMFLAG_TEXWRAP_SHIFT)
#define PRIMFLAG_GET_TEXWRAP(x)		(((x) & PRIMFLAG_TEXWRAP_MASK) >> PRIMFLAG_TEXWRAP_SHIFT)



/***************************************************************************
    MACROS
***************************************************************************/

/* convenience macros for adding items to the UI container */
#define render_ui_add_point(x0,y0,diam,argb,flags)				render_container_add_line(render_container_get_ui(), x0, y0, x0, y0, diam, argb, flags)
#define render_ui_add_line(x0,y0,x1,y1,diam,argb,flags)			render_container_add_line(render_container_get_ui(), x0, y0, x1, y1, diam, argb, flags)
#define render_ui_add_rect(x0,y0,x1,y1,argb,flags)				render_container_add_quad(render_container_get_ui(), x0, y0, x1, y1, argb, NULL, flags)
#define render_ui_add_quad(x0,y0,x1,y1,argb,tex,flags)			render_container_add_quad(render_container_get_ui(), x0, y0, x1, y1, argb, tex, flags)
#define render_ui_add_char(x0,y0,ht,asp,argb,font,ch)			render_container_add_char(render_container_get_ui(), x0, y0, ht, asp, argb, font, ch)

/* convenience macros for adding items to a screen container */
#define render_screen_add_point(scr,x0,y0,diam,argb,flags)		render_container_add_line(render_container_get_screen(scr), x0, y0, x0, y0, diam, argb, flags)
#define render_screen_add_line(scr,x0,y0,x1,y1,diam,argb,flags)	render_container_add_line(render_container_get_screen(scr), x0, y0, x1, y1, diam, argb, flags)
#define render_screen_add_rect(scr,x0,y0,x1,y1,argb,flags)		render_container_add_quad(render_container_get_screen(scr), x0, y0, x1, y1, argb, NULL, flags)
#define render_screen_add_quad(scr,x0,y0,x1,y1,argb,tex,flags)	render_container_add_quad(render_container_get_screen(scr), x0, y0, x1, y1, argb, tex, flags)
#define render_screen_add_char(scr,x0,y0,ht,asp,argb,font,ch)	render_container_add_char(render_container_get_screen(scr), x0, y0, ht, asp, argb, font, ch)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/*-------------------------------------------------
    callbacks
-------------------------------------------------*/

/* texture scaling callback */
typedef void (*texture_scaler)(mame_bitmap *dest, const mame_bitmap *source, const rectangle *sbounds, void *param);



/*-------------------------------------------------
    opaque types
-------------------------------------------------*/

typedef struct _render_container render_container;
typedef struct _render_target render_target;
typedef struct _render_texture render_texture;
typedef struct _render_font render_font;
typedef struct _render_ref render_ref;



/*-------------------------------------------------
    render_bounds - floating point bounding
    rectangle
-------------------------------------------------*/

typedef struct _render_bounds render_bounds;
struct _render_bounds
{
	float				x0;					/* leftmost X coordinate */
	float				y0;					/* topmost Y coordinate */
	float				x1;					/* rightmost X coordinate */
	float				y1;					/* bottommost Y coordinate */
};


/*-------------------------------------------------
    render_color - floating point set of ARGB
    values
-------------------------------------------------*/

typedef struct _render_color render_color;
struct _render_color
{
	float				a;					/* alpha component (0.0 = transparent, 1.0 = opaque) */
	float				r;					/* red component (0.0 = none, 1.0 = max) */
	float				g;					/* green component (0.0 = none, 1.0 = max) */
	float				b;					/* blue component (0.0 = none, 1.0 = max) */
};


/*-------------------------------------------------
    render_texuv - floating point set of UV
    texture coordinates
-------------------------------------------------*/

typedef struct _render_texuv render_texuv;
struct _render_texuv
{
	float				u;					/* U coodinate (0.0-1.0) */
	float				v;					/* V coordinate (0.0-1.0) */
};


/*-------------------------------------------------
    render_quad_texuv - floating point set of UV
    texture coordinates
-------------------------------------------------*/

typedef struct _render_quad_texuv render_quad_texuv;
struct _render_quad_texuv
{
	render_texuv		tl;					/* top-left UV coordinate */
	render_texuv		tr;					/* top-right UV coordinate */
	render_texuv		bl;					/* bottom-left UV coordinate */
	render_texuv		br;					/* bottom-right UV coordinate */
};


/*-------------------------------------------------
    render_texinfo - texture information
-------------------------------------------------*/

typedef struct _render_texinfo render_texinfo;
struct _render_texinfo
{
	void *				base;				/* base of the data */
	UINT32				rowpixels;			/* pixels per row */
	UINT32				width;				/* width of the image */
	UINT32				height;				/* height of the image */
	const rgb_t *		palette;			/* palette for PALETTE16 textures, LUTs for RGB15/RGB32 */
	UINT32				seqid;				/* sequence ID */
};


/*-------------------------------------------------
    render_primitive - a single low-level
    primitive for the rendering engine
-------------------------------------------------*/

typedef struct _render_primitive render_primitive;
struct _render_primitive
{
	render_primitive *	next;				/* pointer to next element */
	int					type;				/* type of primitive */
	render_bounds		bounds;				/* bounds or positions */
	render_color		color;				/* RGBA values */
	UINT32				flags;				/* flags */
	float				width;				/* width (for line primitives) */
	render_texinfo		texture;			/* texture info (for quad primitives) */
	render_quad_texuv	texcoords;			/* texture coordinates (for quad primitives) */
};


/*-------------------------------------------------
    render_primitive_list - an object containing
    a list head plus a lock
-------------------------------------------------*/

typedef struct _render_primitive_list render_primitive_list;
struct _render_primitive_list
{
	render_primitive *	head;				/* head of the list */
	render_primitive **	nextptr;			/* pointer to the next tail pointer */
	osd_lock *			lock;				/* should only should be accessed under this lock */
	render_ref *		reflist;			/* list of references */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void render_init(void);
UINT32 render_get_live_screens_mask(void);
float render_get_ui_aspect(void);
void render_set_ui_target(render_target *target);
render_target *render_get_ui_target(void);


/* ----- render target management ----- */

render_target *render_target_alloc(const char *layout, int singleview);
void render_target_free(render_target *target);
render_target *render_target_get_indexed(int index);
const char *render_target_get_view_name(render_target *target, int viewindex);
UINT32 render_target_get_view_screens(render_target *target, int viewindex);
void render_target_get_bounds(render_target *target, INT32 *width, INT32 *height, float *pixel_aspect);
void render_target_set_bounds(render_target *target, INT32 width, INT32 height, float pixel_aspect);
int render_target_get_orientation(render_target *target);
void render_target_set_orientation(render_target *target, int orientation);
int render_target_get_layer_config(render_target *target);
void render_target_set_layer_config(render_target *target, int layerconfig);
int render_target_get_view(render_target *target);
void render_target_set_view(render_target *target, int viewindex);
void render_target_set_max_texture_size(render_target *target, int maxwidth, int maxheight);
void render_target_compute_visible_area(render_target *target, INT32 target_width, INT32 target_height, float target_pixel_aspect, UINT8 target_orientation, INT32 *visible_width, INT32 *visible_height);
void render_target_get_minimum_size(render_target *target, INT32 *minwidth, INT32 *minheight);
const render_primitive_list *render_target_get_primitives(render_target *target);


/* ----- runtime controls ----- */

int render_view_item_get_state(const char *itemname);
void render_view_item_set_state(const char *itemname, int state);


/* ----- render texture management ----- */

render_texture *render_texture_alloc(mame_bitmap *bitmap, const rectangle *sbounds, rgb_t *palette, int format, texture_scaler scaler, void *param);
void render_texture_free(render_texture *texture);
void render_texture_set_bitmap(render_texture *texture, mame_bitmap *bitmap, const rectangle *sbounds, rgb_t *palette, int format);
void render_texture_hq_scale(mame_bitmap *dest, const mame_bitmap *source, const rectangle *sbounds, void *param);


/* ----- render containers ----- */

void render_container_empty(render_container *container);
int render_container_is_empty(render_container *container);
int render_container_get_orientation(render_container *container);
void render_container_set_orientation(render_container *container, int orientation);
float render_container_get_brightness(render_container *container);
void render_container_set_brightness(render_container *container, float brightness);
float render_container_get_contrast(render_container *container);
void render_container_set_contrast(render_container *container, float contrast);
float render_container_get_gamma(render_container *container);
void render_container_set_gamma(render_container *container, float gamma);
float render_container_get_xscale(render_container *container);
void render_container_set_xscale(render_container *container, float xscale);
float render_container_get_yscale(render_container *container);
void render_container_set_yscale(render_container *container, float yscale);
float render_container_get_xoffset(render_container *container);
void render_container_set_xoffset(render_container *container, float xoffset);
float render_container_get_yoffset(render_container *container);
void render_container_set_yoffset(render_container *container, float yoffset);
void render_container_set_overlay(render_container *container, mame_bitmap *bitmap);
render_container *render_container_get_ui(void);
render_container *render_container_get_screen(int screen);
void render_container_add_line(render_container *container, float x0, float y0, float x1, float y1, float width, rgb_t argb, UINT32 flags);
void render_container_add_quad(render_container *container, float x0, float y0, float x1, float y1, rgb_t argb, render_texture *texture, UINT32 flags);
void render_container_add_char(render_container *container, float x0, float y0, float height, float aspect, rgb_t argb, render_font *font, UINT16 ch);


/* ----- render utilities ----- */

void render_resample_argb_bitmap_hq(void *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const mame_bitmap *source, const rectangle *sbounds, const render_color *color);
int render_clip_line(render_bounds *bounds, const render_bounds *clip);
int render_clip_quad(render_bounds *bounds, const render_bounds *clip, render_quad_texuv *texcoords);
void render_line_to_quad(const render_bounds *bounds, float width, render_bounds *bounds0, render_bounds *bounds1);
mame_bitmap *render_load_png(const char *dirname, const char *filename, mame_bitmap *alphadest, int *hasalpha);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* single screen layouts */
extern const char layout_horizont[];	/* horizontal 4:3 screens */
extern const char layout_vertical[];	/* vertical 4:3 screens */

/* dual screen layouts */
extern const char layout_dualhsxs[];	/* dual 4:3 screens side-by-side */
extern const char layout_dualhovu[];	/* dual 4:3 screens above and below */
extern const char layout_dualhuov[];	/* dual 4:3 screens below and above */

/* triple screen layouts */
extern const char layout_triphsxs[];	/* triple 4:3 screens side-by-side */

/* generic color overlay layouts */
extern const char layout_ho20ffff[];	/* horizontal 4:3 with 20,FF,FF color overlay */
extern const char layout_ho2eff2e[];	/* horizontal 4:3 with 2E,FF,2E color overlay */
extern const char layout_ho88ffff[];	/* horizontal 4:3 with 88,FF,FF color overlay */
extern const char layout_hoa0a0ff[];	/* horizontal 4:3 with A0,A0,FF color overlay */
extern const char layout_hoffe457[];	/* horizontal 4:3 with FF,E4,57 color overlay */
extern const char layout_hoffff20[];	/* horizontal 4:3 with FF,FF,20 color overlay */
extern const char layout_voffff20[];	/* vertical 4:3 with FF,FF,20 color overlay */



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    set_render_bounds_xy - cleaner way to set the
    bounds
-------------------------------------------------*/

INLINE void set_render_bounds_xy(render_bounds *bounds, float x0, float y0, float x1, float y1)
{
	bounds->x0 = x0;
	bounds->y0 = y0;
	bounds->x1 = x1;
	bounds->y1 = y1;
}


/*-------------------------------------------------
    set_render_bounds_wh - cleaner way to set the
    bounds
-------------------------------------------------*/

INLINE void set_render_bounds_wh(render_bounds *bounds, float x0, float y0, float width, float height)
{
	bounds->x0 = x0;
	bounds->y0 = y0;
	bounds->x1 = x0 + width;
	bounds->y1 = y0 + height;
}


/*-------------------------------------------------
    sect_render_bounds - compute the intersection
    of two render_bounds
-------------------------------------------------*/

INLINE void sect_render_bounds(render_bounds *dest, const render_bounds *src)
{
	dest->x0 = (dest->x0 > src->x0) ? dest->x0 : src->x0;
	dest->x1 = (dest->x1 < src->x1) ? dest->x1 : src->x1;
	dest->y0 = (dest->y0 > src->y0) ? dest->y0 : src->y0;
	dest->y1 = (dest->y1 < src->y1) ? dest->y1 : src->y1;
}


/*-------------------------------------------------
    union_render_bounds - compute the union of two
    render_bounds
-------------------------------------------------*/

INLINE void union_render_bounds(render_bounds *dest, const render_bounds *src)
{
	dest->x0 = (dest->x0 < src->x0) ? dest->x0 : src->x0;
	dest->x1 = (dest->x1 > src->x1) ? dest->x1 : src->x1;
	dest->y0 = (dest->y0 < src->y0) ? dest->y0 : src->y0;
	dest->y1 = (dest->y1 > src->y1) ? dest->y1 : src->y1;
}


/*-------------------------------------------------
    set_render_color - cleaner way to set a color
-------------------------------------------------*/

INLINE void set_render_color(render_color *color, float a, float r, float g, float b)
{
	color->a = a;
	color->r = r;
	color->g = g;
	color->b = b;
}


/*-------------------------------------------------
    orientation_swap_flips - swap the X and Y
    flip flags
-------------------------------------------------*/

INLINE int orientation_swap_flips(int orientation)
{
	return (orientation & ORIENTATION_SWAP_XY) |
	       ((orientation & ORIENTATION_FLIP_X) ? ORIENTATION_FLIP_Y : 0) |
	       ((orientation & ORIENTATION_FLIP_Y) ? ORIENTATION_FLIP_X : 0);
}


/*-------------------------------------------------
    orientation_reverse - compute the orientation
    that will undo another orientation
-------------------------------------------------*/

INLINE int orientation_reverse(int orientation)
{
	/* if not swapping X/Y, then just apply the same transform to reverse */
	if (!(orientation & ORIENTATION_SWAP_XY))
		return orientation;

	/* if swapping X/Y, then swap X/Y flip bits to get the reverse */
	else
		return orientation_swap_flips(orientation);
}


/*-------------------------------------------------
    orientation_add - compute effective orientation
    after applying two subsequent orientations
-------------------------------------------------*/

INLINE int orientation_add(int orientation1, int orientation2)
{
	/* if the 2nd transform doesn't swap, just XOR together */
	if (!(orientation2 & ORIENTATION_SWAP_XY))
		return orientation1 ^ orientation2;

	/* otherwise, we need to effectively swap the flip bits on the first transform */
	else
		return orientation_swap_flips(orientation1) ^ orientation2;
}


/*-------------------------------------------------
    apply_brightness_contrast_gamma_fp - apply
    brightness, contrast, and gamma controls to
    a single RGB component
-------------------------------------------------*/

INLINE float apply_brightness_contrast_gamma_fp(float srcval, float brightness, float contrast, float gamma)
{
	/* first apply gamma */
	srcval = pow(srcval, 1.0f / gamma);

	/* then contrast/brightness */
	srcval = (srcval * contrast) + brightness - 1.0f;

	/* clamp and return */
	if (srcval < 0.0f)
		srcval = 0.0f;
	if (srcval > 1.0f)
		srcval = 1.0f;
	return srcval;
}


/*-------------------------------------------------
    apply_brightness_contrast_gamma - apply
    brightness, contrast, and gamma controls to
    a single RGB component
-------------------------------------------------*/

INLINE UINT8 apply_brightness_contrast_gamma(UINT8 src, float brightness, float contrast, float gamma)
{
	float srcval = (float)src * (1.0f / 255.0f);
	float result = apply_brightness_contrast_gamma_fp(srcval, brightness, contrast, gamma);
	return (UINT8)(result * 255.0f);
}


#endif	/* __RENDER_H__ */
