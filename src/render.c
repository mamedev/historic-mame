/***************************************************************************

    render.c

    Core rendering system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Still to-do:
        * controls for which screen the UI is displayed on
        * positioning of game screens
        * brightness/contrast/gamma controls
        * rewrite usrintrf to work in floating point once NEW_RENDER is gone

    Windows-specific to-do:
        * no fallback if we run out of video memory
        * multiple windows on screen produces odd order and UI is not visible

****************************************************************************

    Notes:

        Unlike the old system, the artwork is not rotated with the game
        orientation. This is to support odd configurations like two
        monitors in different orientations. You can specify an orientation
        for a backdrop/screen/overlay/bezel element, but it only applies
        to the artwork itself, and does not affect coordinates in any way.


    Overview of objects:

        render_target -- This represents a final rendering target. It
            is specified using integer width/height values, can have
            non-square pixels, and you can specify its rotation. It is
            what really determines the final rendering details. The OSD
            layer creates one or more of these to encapsulate the
            rendering process. Each render_target holds a list of
            layout_files that it can use for drawing. When rendering, it
            makes use of both layout_files and render_containers.

        layout_file -- A layout_file comprises a list of elements and a
            list of views. The elements are reusable items that the views
            reference.

        layout_view -- A layout_view describes a single view within a
            layout_file. The view is described using arbitrary coordinates
            that are scaled to fit within the render target. Pixels within
            a view are assumed to be square.

        view_item -- Each view has four lists of view_items, one for each
            "layer." Each view item is specified using floating point
            coordinates in arbitrary units, and is assumed to have square
            pixels. Each view item can control its orientation independently.
            Each item can also have an optional name, and can be set at
            runtime into different "states", which control how the embedded
            elements are displayed.

        layout_element -- A layout_element is a description of a piece of
            visible artwork. Most view_items (except for those in the screen
            layer) have exactly one layout_element which describes the
            contents of the item. Elements are separate from items because
            they can be re-used multiple times within a layout. Even though
            an element can contain a number of components, they are treated
            as if they were a single bitmap.

        element_component -- Each layout_element contains one or more
            components. Each component can describe either an image or
            a rectangle/disk primitive. Each component also has a "state"
            associated with it, which controls whether or not the component
            is visible (if the owning item has the same state, it is
            visible).

        render_container -- Containers are the top of a hierarchy that is
            not directly related to the objects above. Containers hold
            high level primitives that are generated at runtime by the
            video system. They are used currently for each screen and
            the user interface. These high-level primitives are broken down
            into low-level primitives at render time.

****************************************************************************

    Layout file format:

    bounds: left/top/right/bottom or x/y/width/height
    color: red/green/blue/alpha
    orientation: rotate/xflip/yflip/swapxy

    items in an element are only displayed if state matches state value in XML
    if no state in XML, state="0", which is the default state

    <?xml version="1.0"?>
    <mamelayout version="2">

        <element name="backdrop">
            <image file="cb-back.png" alphafile="cb-back-mask.png">
                <color red="1.0" green="1.0" blue="1.0" alpha="1.0" />
            </image>
        </element>

        <element name="led_digit" defstate="10">
            <image state="0" file="digit0.png" />
            <image state="1" file="digit1.png" />
            <image state="2" file="digit2.png" />
            <image state="3" file="digit3.png" />
            <image state="4" file="digit4.png" />
            <image state="5" file="digit5.png" />
            <image state="6" file="digit6.png" />
            <image state="7" file="digit7.png" />
            <image state="8" file="digit8.png" />
            <image state="9" file="digit9.png" />
            <image state="10" file="digitoff.png" />
        </element>

        <element name="sundance_overlay">
            <rect>
                <bounds left="0.0" top="0.0" right="1.0" bottom="1.0" />
                <color red="1.0" green="1.0" blue="0.125" />
            </rect>
        </element>

        <element name="starcas_overlay">
            <rect>
                <bounds left="0.0" top="0.0" right="1.0" bottom="1.0" />
                <color red="0.0" green="0.235" blue="1.0" />
            </rect>
            <disk>
                <bounds x="0.5" y="0.5" width="0.1225" height="0.1225" />
                <color red="1.0" green="0.125" blue="0.125" />
            </disk>
            <disk>
                <bounds x="0.5" y="0.5" width="0.0950" height="0.0950" />
                <color red="1.0" green="0.5" blue="0.0627" />
            </disk>
            <disk>
                <bounds x="0.5" y="0.5" width="0.0725" height="0.0725" />
                <color red="1.0" green="1.0" blue="0.125" />
            </disk>
        </element>

        <view name="dual screen">
            <screen index="0">
                <bounds left="0" top="0" right="1000" bottom="750" />
                <color red="1.0" green="1.0" blue="1.0" alpha="1.0" />
                <orientation rotate="90" />
            </screen>

            <screen index="1">
                <bounds left="1100" top="0" right="2100" bottom="750" />
                <color red="1.0" green="1.0" blue="1.0" alpha="1.0" />
                <orientation xflip="yes" yflip="yes" swapxy="yes" />
            </screen>

            <backdrop element="backdrop" state="0">
                <bounds left="-75" top="-75" right="1075" bottom="900" />
            </backdrop>

            <overlay element="starcas_overlay" state="0">
                <bounds left="0" top="0" right="1000" bottom="750" />
            </overlay>

            <bezel name="score0" element="led_digit" state="0">
                <bounds left="0" top="0" right="1000" bottom="750" />
            </bezel>
        </view>

    </mamelayout>

***************************************************************************/

#include "render.h"
#include "rendfont.h"
#include "config.h"
#include "xmlfile.h"
#include "png.h"
#include <math.h>



/***************************************************************************
    STANDARD LAYOUTS
***************************************************************************/

/* single screen layouts */
#include "horizont.lh"
#include "vertical.lh"

/* dual screen layouts */
#include "dualhsxs.lh"
#include "dualhovu.lh"
#include "dualhuov.lh"

/* triple screen layouts */
#include "triphsxs.lh"

/* generic color overlay layouts */
#include "ho20ffff.lh"
#include "ho2eff2e.lh"
#include "ho88ffff.lh"
#include "hoa0a0ff.lh"
#include "hoffe457.lh"
#include "voffff20.lh"
#include "hoffff20.lh"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LAYOUT_VERSION			2

#define MAX_TEXTURE_SCALES		8

#define NUM_PRIMLISTS			2

#define MAX_CLEAR_EXTENTS		1000

#define INTERNAL_FLAG_CHAR		0x00000001

enum
{
	COMPONENT_TYPE_IMAGE = 0,
	COMPONENT_TYPE_RECT,
	COMPONENT_TYPE_DISK,
	COMPONENT_TYPE_MAX
};


enum
{
	ITEM_LAYER_BACKDROP = 0,
	ITEM_LAYER_SCREEN,
	ITEM_LAYER_OVERLAY,
	ITEM_LAYER_BEZEL,
	ITEM_LAYER_MAX
};


enum
{
	CONTAINER_ITEM_LINE = 0,
	CONTAINER_ITEM_QUAD,
	CONTAINER_ITEM_MAX
};



/***************************************************************************
    MACROS
***************************************************************************/

#define ISWAP(var1, var2) do { int temp = var1; var1 = var2; var2 = temp; } while (0)
#define FSWAP(var1, var2) do { float temp = var1; var1 = var2; var2 = temp; } while (0)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* typedef struct _render_texture render_texture; -- defined in render.h */
/* typedef struct _render_target render_target; -- defined in render.h */
/* typedef struct _render_container render_container; -- defined in render.h */
typedef struct _object_transform object_transform;
typedef struct _scaled_texture scaled_texture;
typedef struct _element_component element_component;
typedef struct _element_texture element_texture;
typedef struct _layout_element layout_element;
typedef struct _view_item_state view_item_state;
typedef struct _view_item view_item;
typedef struct _layout_view layout_view;
typedef struct _layout_file layout_file;
typedef struct _container_item container_item;


/* a render_ref is an abstract reference to an internal object of some sort */
struct _render_ref
{
	render_ref *		next;				/* link to the next reference */
	void *				refptr;				/* reference pointer */
};


/* an object_transform is used to track transformations when building an object list */
struct _object_transform
{
	float				xoffs, yoffs;		/* offset transforms */
	float				xscale, yscale;		/* scale transforms */
	render_color		color;				/* color transform */
	int					orientation;		/* orientation transform */
};


/* a scaled_texture contains a single scaled entry for a texture */
struct _scaled_texture
{
	mame_bitmap *		bitmap;				/* final bitmap */
	UINT32				seqid;				/* sequence number */
};


/* a render_texture is used to track transformations when building an object list */
struct _render_texture
{
	mame_bitmap *		bitmap;				/* pointer to the original bitmap */
	rectangle			sbounds;			/* source bounds within the bitmap */
	rgb_t *				palette;			/* pointer to the palette (if present) */
	int					format;				/* format of the texture data */
	texture_scaler		scaler;				/* scaling callback */
	void *				param;				/* scaling callback parameter */
	UINT32				curseq;				/* current sequence number */
	scaled_texture		scaled[MAX_TEXTURE_SCALES];	/* array of scaled variants of this texture */
};


/* an element_component represents an image, rectangle, or disk in an element */
struct _element_component
{
	element_component *	next;				/* link to next component */
	int					type;				/* type of component */
	int					state;				/* state where this component is visible (-1 means all states) */
	render_bounds		bounds;				/* bounds of the element */
	render_color		color;				/* color of the element */
	mame_bitmap *		bitmap;				/* source bitmap for images */
	int					hasalpha;			/* is there any alpha component present? */
};


/* an element_texture encapsulates a texture for a given element in a given state */
struct _element_texture
{
	layout_element *	element;			/* pointer back to the element */
	render_texture *	texture;			/* texture for this state */
	int					state;				/* associated state number */
};


/* a layout_element is a single named element, which may have multiple components */
struct _layout_element
{
	layout_element *	next;				/* link to next element */
	const char *		name;				/* name of this element */
	element_component *	complist;			/* head of the list of components */
	int					defstate;			/* default state of this element */
	int					maxstate;			/* maximum state value for all components */
	element_texture *	elemtex;			/* array of textures used for managing the scaled bitmaps */
};


/* a view_item_state contains the string-tagged state of a view item */
struct _view_item_state
{
	view_item_state *	next;				/* pointer to the next one */
	const char *		name;				/* string that was set */
	int					curstate;			/* current state */
};


/* a view_item is a single backdrop, screen, overlay, or bezel item */
struct _view_item
{
	view_item *			next;				/* link to next item */
	layout_element *	element;			/* pointer to the associated element (non-screens only) */
	const char *		name;				/* name of this item */
	int					index;				/* index for this item (screens only) */
	view_item_state *	state;				/* pointer to the state */
	int					orientation;		/* orientation of this item */
	render_bounds		bounds;				/* bounds of the item */
	render_bounds		rawbounds;			/* raw (original) bounds of the item */
	render_color		color;				/* color of the item */
};


/* a layout_view encapsulates a named list of items */
struct _layout_view
{
	layout_view *		next;				/* pointer to next layout in the list */
	const char *		name;				/* name of the layout */
	float				aspect;				/* X/Y of the layout */
	float				scraspect;			/* X/Y of the screen areas */
	UINT32				screens;			/* bitmask of screens used */
	render_bounds		bounds;				/* computed bounds of the view */
	render_bounds		scrbounds;			/* computed bounds of the screens within the view */
	render_bounds		expbounds;			/* explicit bounds of the view */
	UINT8				layenabled[ITEM_LAYER_MAX]; /* is this layer enabled? */
	view_item *			itemlist[ITEM_LAYER_MAX]; /* list of layout items for each layer */
};


/* a layout_file consists of a list of elements and a list of views */
struct _layout_file
{
	layout_file *		next;				/* pointer to the next file in the list */
	layout_element *	elemlist;			/* list of shared layout elements */
	layout_view *		viewlist;			/* list of views */
};


/* a render_target describes a surface that is being rendered to */
struct _render_target
{
	render_target *		next;				/* keep a linked list of targets */
	layout_view *		curview;			/* current view */
	layout_file *		filelist;			/* list of layout files */
	render_primitive_list primlist[NUM_PRIMLISTS];/* list of primitives */
	INT32				width;				/* width in pixels */
	INT32				height;				/* height in pixels */
	float				pixel_aspect;		/* aspect ratio of individual pixels */
	int					orientation;		/* orientation */
	int					layerconfig;		/* layer configuration */
	layout_view *		base_view;			/* the view at the time of first frame */
	int					base_orientation;	/* the orientation at the time of first frame */
	int					base_layerconfig;	/* the layer configuration at the time of first frame */
	int					maxtexwidth;		/* maximum width of a texture */
	int					maxtexheight;		/* maximum height of a texture */
};


/* a container_item describes a high level primitive that is added to a container */
struct _container_item
{
	container_item *	next;				/* pointer to the next element in the list */
	UINT8				type;				/* type of element */
	render_bounds		bounds;				/* bounds of the element */
	render_color		color;				/* RGBA factors */
	UINT32				flags;				/* option flags */
	UINT32				internal;			/* internal flags */
	float				width;				/* width of the line (lines only) */
	render_texture *	texture;			/* pointer to the source texture (quads only) */
};


/* a render_container holds a list of items and an orientation for the entire collection */
struct _render_container
{
	container_item *	itemlist;			/* head of the item list */
	container_item **	nextitem;			/* pointer to the next item to add */
	int					orientation;		/* orientation of the container */
};



/***************************************************************************
    GLOBALS
***************************************************************************/

/* array of live targets */
static render_target *targetlist;

/* free lists */
static render_primitive *render_primitive_free_list;
static container_item *container_item_free_list;
static render_ref *render_ref_free_list;

/* containers for the UI and for screens */
static render_container *ui_container;
static render_container *screen_container[MAX_SCREENS];

/* list of view item states */
static view_item_state *item_statelist;

/* variables for tracking extents to clear */
static INT32 clear_extents[MAX_CLEAR_EXTENTS];
static INT32 clear_extent_count;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* core system */
static void render_exit(void);
static void render_load(int config_type, xml_data_node *parentnode);
static void render_save(int config_type, xml_data_node *parentnode);

/* render targets */
static void release_render_list(render_primitive_list *list);
static void add_container_primitives(render_target *target, render_primitive_list *list, const object_transform *xform, const render_container *container, int blendmode);
static void add_element_primitives(render_target *target, render_primitive_list *list, const object_transform *xform, const layout_element *element, int state, int blendmode);
static void add_clear_and_optimize_primitive_list(render_target *target, render_primitive_list *list);

/* render references */
static void invalidate_all_render_ref(void *refptr);

/* render textures */
static void render_texture_get_scaled(render_texture *texture, UINT32 dwidth, UINT32 dheight, render_texinfo *texinfo, render_ref **reflist);

/* render containers */
static render_container *render_container_alloc(void);
static void render_container_free(render_container *container);
static container_item *render_container_item_add_generic(render_container *container, UINT8 type, float x0, float y0, float x1, float y1, rgb_t argb);

/* resampler */
static void resample_argb_bitmap_average(UINT32 *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const UINT32 *source, UINT32 srowpixels, const render_color *color, UINT32 dx, UINT32 dy);
static void resample_argb_bitmap_bilinear(UINT32 *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const UINT32 *source, UINT32 srowpixels, const render_color *color, UINT32 dx, UINT32 dy);

/* layout views */
static void layout_view_recompute(layout_view *view, int layerconfig);

/* layout elements */
static void layout_element_scale(mame_bitmap *dest, const mame_bitmap *source, const rectangle *sbounds, void *param);
static void layout_element_draw_rect(mame_bitmap *dest, const rectangle *bounds, const render_color *color);
static void layout_element_draw_disk(mame_bitmap *dest, const rectangle *bounds, const render_color *color);

/* layout file parsing */
static layout_file *load_layout_file(const char *dirname, const char *filename);
static layout_element *load_layout_element(xml_data_node *elemnode, const char *dirname);
static element_component *load_element_component(xml_data_node *compnode, const char *dirname);
static layout_view *load_layout_view(xml_data_node *viewnode, layout_element *elemlist);
static view_item *load_view_item(xml_data_node *itemnode, layout_element *elemlist);
static mame_bitmap *load_component_bitmap(const char *dirname, const char *file, const char *alphafile, int *hasalpha);
static int load_alpha_bitmap(const char *dirname, const char *alphafile, mame_bitmap *bitmap, const png_info *original);
static int load_bounds(xml_data_node *boundsnode, render_bounds *bounds);
static int load_color(xml_data_node *colornode, render_color *color);
static int load_orientation(xml_data_node *orientnode, int *orientation);
static int open_and_read_png(const char *dirname, const char *filename, png_info *png);
static void layout_file_free(layout_file *file);
static void layout_view_free(layout_view *view);
static void layout_element_free(layout_element *element);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    round_nearest - floating point
    round-to-nearest
-------------------------------------------------*/

INLINE float round_nearest(float f)
{
	return floor(f + 0.5f);
}


/*-------------------------------------------------
    decimal_to_fraction - convert a (small) fp
    value to a numerator/denominator
-------------------------------------------------*/

INLINE void decimal_to_fraction(float decimal, int *num, int *den)
{
	/* find integral aspect ratios */
	for (*den = 1; *den < 1000; *den += 1)
	{
		float testnum = *den * decimal;
		float intnum = round_nearest(testnum);
		if (fabs(testnum - intnum) < 0.001f)
			break;
	}
	*num = round_nearest(*den * decimal);
}


/*-------------------------------------------------
    reduce_fraction - reduce a fraction by
    dividing out common factors
-------------------------------------------------*/

INLINE void reduce_fraction(int *num, int *den)
{
	int div;

	/* keep trying to divide down the fraction as much as possible */
	for (div = MAX(*num,*den) / 2; div > 1; div--)
		if (((*num / div) * div) == *num && ((*den / div) * div) == *den)
		{
			*num /= div;
			*den /= div;
		}
}


/*-------------------------------------------------
    apply_orientation - apply orientation to a
    set of bounds
-------------------------------------------------*/

INLINE void apply_orientation(render_bounds *bounds, int orientation)
{
	/* swap first */
	if (orientation & ORIENTATION_SWAP_XY)
	{
		FSWAP(bounds->x0, bounds->y0);
		FSWAP(bounds->x1, bounds->y1);
	}

	/* apply X flip */
	if (orientation & ORIENTATION_FLIP_X)
	{
		bounds->x0 = 1.0f - bounds->x0;
		bounds->x1 = 1.0f - bounds->x1;
	}

	/* apply Y flip */
	if (orientation & ORIENTATION_FLIP_Y)
	{
		bounds->y0 = 1.0f - bounds->y0;
		bounds->y1 = 1.0f - bounds->y1;
	}
}


/*-------------------------------------------------
    set_bounds_xy - cleaner way to set the bounds
-------------------------------------------------*/

INLINE void set_bounds_xy(render_bounds *bounds, float x0, float y0, float x1, float y1)
{
	bounds->x0 = x0;
	bounds->y0 = y0;
	bounds->x1 = x1;
	bounds->y1 = y1;
}


/*-------------------------------------------------
    set_bounds_wh - cleaner way to set the bounds
-------------------------------------------------*/

INLINE void set_bounds_wh(render_bounds *bounds, float x0, float y0, float width, float height)
{
	bounds->x0 = x0;
	bounds->y0 = y0;
	bounds->x1 = x0 + width;
	bounds->y1 = y0 + height;
}


/*-------------------------------------------------
    normalize_bounds - normalize bounds so that
    x0/y0 are less than x1/y1
-------------------------------------------------*/

INLINE void normalize_bounds(render_bounds *bounds)
{
	if (bounds->x0 > bounds->x1)
		FSWAP(bounds->x0, bounds->x1);
	if (bounds->y0 > bounds->y1)
		FSWAP(bounds->y0, bounds->y1);
}


/*-------------------------------------------------
    sect_bounds - compute the intersection of two
    render_bounds
-------------------------------------------------*/

INLINE void sect_bounds(render_bounds *dest, const render_bounds *src)
{
	dest->x0 = (dest->x0 > src->x0) ? dest->x0 : src->x0;
	dest->x1 = (dest->x1 < src->x1) ? dest->x1 : src->x1;
	dest->y0 = (dest->y0 > src->y0) ? dest->y0 : src->y0;
	dest->y1 = (dest->y1 < src->y1) ? dest->y1 : src->y1;
}


/*-------------------------------------------------
    union_bounds - compute the union of two
    render_bounds
-------------------------------------------------*/

INLINE void union_bounds(render_bounds *dest, const render_bounds *src)
{
	dest->x0 = (dest->x0 < src->x0) ? dest->x0 : src->x0;
	dest->x1 = (dest->x1 > src->x1) ? dest->x1 : src->x1;
	dest->y0 = (dest->y0 < src->y0) ? dest->y0 : src->y0;
	dest->y1 = (dest->y1 > src->y1) ? dest->y1 : src->y1;
}


/*-------------------------------------------------
    set_color - cleaner way to set a color
-------------------------------------------------*/

INLINE void set_color(render_color *color, float a, float r, float g, float b)
{
	color->a = a;
	color->r = r;
	color->g = g;
	color->b = b;
}


/*-------------------------------------------------
    compute_brightness - compute the effective
    brightness for an RGB pixel
-------------------------------------------------*/

INLINE UINT8 compute_brightness(rgb_t rgb)
{
	return (RGB_RED(rgb) * 222 + RGB_GREEN(rgb) * 707 + RGB_BLUE(rgb) * 71) / 1000;
}


/*-------------------------------------------------
    copy_string - make a copy of a string
-------------------------------------------------*/

INLINE const char *copy_string(const char *string)
{
	char *newstring = malloc_or_die(strlen(string) + 1);
	strcpy(newstring, string);
	return newstring;
}


/*-------------------------------------------------
    alloc_container_item - allocate a new
    container item object
--------------------------------------------------*/

INLINE container_item *alloc_container_item(void)
{
	container_item *result = container_item_free_list;

	/* allocate from the free list if we can; otherwise, malloc a new item */
	if (result != NULL)
		container_item_free_list = result->next;
	else
		result = malloc_or_die(sizeof(*result));

	memset(result, 0, sizeof(*result));
	return result;
}


/*-------------------------------------------------
    container_item_free - free a previously
    allocated render element object
-------------------------------------------------*/

INLINE void free_container_item(container_item *item)
{
	item->next = container_item_free_list;
	container_item_free_list = item;
}


/*-------------------------------------------------
    alloc_render_primitive - allocate a new empty
    element object
-------------------------------------------------*/

INLINE render_primitive *alloc_render_primitive(int type)
{
	render_primitive *result = render_primitive_free_list;

	/* allocate from the free list if we can; otherwise, malloc a new item */
	if (result != NULL)
		render_primitive_free_list = result->next;
	else
		result = malloc_or_die(sizeof(*result));

	/* clear to 0 */
	memset(result, 0, sizeof(*result));
	result->type = type;
	return result;
}


/*-------------------------------------------------
    append_render_primitive - append a primitive
    to the end of the list
-------------------------------------------------*/

INLINE void append_render_primitive(render_primitive_list *list, render_primitive *prim)
{
	*list->nextptr = prim;
	list->nextptr = &prim->next;
}


/*-------------------------------------------------
    free_render_primitive - free a previously
    allocated render element object
-------------------------------------------------*/

INLINE void free_render_primitive(render_primitive *element)
{
	element->next = render_primitive_free_list;
	render_primitive_free_list = element;
}


/*-------------------------------------------------
    add_render_ref - add a new reference
-------------------------------------------------*/

INLINE void add_render_ref(render_ref **list, void *refptr)
{
	render_ref *ref;

	/* skip if we already have one */
	for (ref = *list; ref != NULL; ref = ref->next)
		if (ref->refptr == refptr)
			return;

	/* allocate from the free list if we can; otherwise, malloc a new item */
	ref = render_ref_free_list;
	if (ref != NULL)
		render_ref_free_list = ref->next;
	else
		ref = malloc_or_die(sizeof(*ref));

	/* set the refptr and link us into the list */
	ref->refptr = refptr;
	ref->next = *list;
	*list = ref;
}


/*-------------------------------------------------
    has_render_ref - find a refptr in a reference
    list
-------------------------------------------------*/

INLINE int has_render_ref(render_ref *list, void *refptr)
{
	render_ref *ref;

	/* skip if we already have one */
	for (ref = list; ref != NULL; ref = ref->next)
		if (ref->refptr == refptr)
			return TRUE;
	return FALSE;
}


/*-------------------------------------------------
    free_render_ref - free a previously
    allocated render reference
-------------------------------------------------*/

INLINE void free_render_ref(render_ref *ref)
{
	ref->next = render_ref_free_list;
	render_ref_free_list = ref;
}



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    render_init - allocate base structures for
    the rendering system
-------------------------------------------------*/

void render_init(void)
{
	int screen;

	/* make sure we clean up after ourselves */
	add_exit_callback(render_exit);

	/* set up the list of render targets */
	targetlist = NULL;

	/* zap the free lists */
	render_primitive_free_list = NULL;
	container_item_free_list = NULL;

	/* create a UI container */
	ui_container = render_container_alloc();

	/* create a container for each screen and determine its orientation */
	for (screen = 0; screen < MAX_SCREENS; screen++)
	{
		screen_container[screen] = render_container_alloc();
		screen_container[screen]->orientation = Machine->gamedrv->flags & ORIENTATION_MASK;
	}

	/* register callbacks */
	config_register("video", render_load, render_save);
}


/*-------------------------------------------------
    render_exit - free all rendering data
-------------------------------------------------*/

static void render_exit(void)
{
	int screen;

	/* free the UI container */
	if (ui_container != NULL)
		render_container_free(ui_container);

	/* free the screen container */
	for (screen = 0; screen < MAX_SCREENS; screen++)
		if (screen_container[screen] != NULL)
			render_container_free(screen_container[screen]);

	/* free the render primitives */
	while (render_primitive_free_list != NULL)
	{
		render_primitive *temp = render_primitive_free_list;
		render_primitive_free_list = temp->next;
		free(temp);
	}

	/* free the render refs */
	while (render_ref_free_list != NULL)
	{
		render_ref *temp = render_ref_free_list;
		render_ref_free_list = temp->next;
		free(temp);
	}

	/* free the container items */
	while (container_item_free_list != NULL)
	{
		container_item *temp = container_item_free_list;
		container_item_free_list = temp->next;
		free(temp);
	}

	/* free the targets */
	while (targetlist != NULL)
		render_target_free(targetlist);

	/* free the item states */
	while (item_statelist != NULL)
	{
		view_item_state *temp = item_statelist;
		item_statelist = temp->next;
		if (temp->name)
			free((void *)temp->name);
		free(temp);
	}
}


/*-------------------------------------------------
    render_load - read and apply data from the
    configuration file
-------------------------------------------------*/

static void render_load(int config_type, xml_data_node *parentnode)
{
	xml_data_node *targetnode;

	/* we only care about game files */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* might not have any data */
	if (parentnode == NULL)
		return;

	/* iterate over target nodes */
	for (targetnode = xml_get_sibling(parentnode->child, "target"); targetnode; targetnode = xml_get_sibling(targetnode->next, "channel"))
	{
		render_target *target = render_target_get_indexed(xml_get_attribute_int(targetnode, "index", -1));
		if (target != NULL)
		{
			const char *viewname = xml_get_attribute_string(targetnode, "view", NULL);
			int viewnum, tmpint;

			/* find the view */
			if (viewname != NULL)
				for (viewnum = 0; viewnum < 1000; viewnum++)
				{
					const char *testname = render_target_get_view_name(target, viewnum);
					if (testname == NULL)
						break;
					if (!strcmp(viewname, testname))
					{
						render_target_set_view(target, viewnum);
						break;
					}
				}

			/* modify the artwork config */
			tmpint = xml_get_attribute_int(targetnode, "backdrops", -1);
			if (tmpint == 0)
				render_target_set_layer_config(target, target->layerconfig & ~LAYER_CONFIG_ENABLE_BACKDROP);
			else if (tmpint == 1)
				render_target_set_layer_config(target, target->layerconfig | LAYER_CONFIG_ENABLE_BACKDROP);

			tmpint = xml_get_attribute_int(targetnode, "overlays", -1);
			if (tmpint == 0)
				render_target_set_layer_config(target, target->layerconfig & ~LAYER_CONFIG_ENABLE_OVERLAY);
			else if (tmpint == 1)
				render_target_set_layer_config(target, target->layerconfig | LAYER_CONFIG_ENABLE_OVERLAY);

			tmpint = xml_get_attribute_int(targetnode, "bezels", -1);
			if (tmpint == 0)
				render_target_set_layer_config(target, target->layerconfig & ~LAYER_CONFIG_ENABLE_BEZEL);
			else if (tmpint == 1)
				render_target_set_layer_config(target, target->layerconfig | LAYER_CONFIG_ENABLE_BEZEL);

			tmpint = xml_get_attribute_int(targetnode, "zoom", -1);
			if (tmpint == 0)
				render_target_set_layer_config(target, target->layerconfig & ~LAYER_CONFIG_ZOOM_TO_SCREEN);
			else if (tmpint == 1)
				render_target_set_layer_config(target, target->layerconfig | LAYER_CONFIG_ZOOM_TO_SCREEN);

			/* apply orientation */
			tmpint = xml_get_attribute_int(targetnode, "rotate", -1);
			if (tmpint != -1)
			{
				if (tmpint == 90)
					tmpint = ROT90;
				else if (tmpint == 180)
					tmpint = ROT180;
				else if (tmpint == 270)
					tmpint = ROT270;
				else
					tmpint = ROT0;
				render_target_set_orientation(target, orientation_add(tmpint, target->orientation));

				/* apply the opposite orientation to the UI */
				if (target == render_get_ui_target())
				{
					if (tmpint == ROT90)
						tmpint = ROT270;
					else if (tmpint == ROT270)
						tmpint = ROT90;
					render_container_set_orientation(ui_container, orientation_add(tmpint, ui_container->orientation));
				}
			}
		}
	}
}


/*-------------------------------------------------
    render_save - save data to the configuration
    file
-------------------------------------------------*/

static void render_save(int config_type, xml_data_node *parentnode)
{
	render_target *target;
	int targetnum = 0;

	/* we only care about game files */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* iterate over targets */
	for (target = targetlist; target != NULL; target = target->next)
	{
		xml_data_node *targetnode;

		/* create a node */
		targetnode = xml_add_child(parentnode, "target", NULL);
		if (targetnode != NULL)
		{
			/* output the basics */
			xml_set_attribute_int(targetnode, "index", targetnum);

			/* output the view */
			if (target->curview != target->base_view)
				xml_set_attribute(targetnode, "view",  target->curview->name);

			/* output the layer config */
			if (target->layerconfig != target->base_layerconfig)
			{
				xml_set_attribute_int(targetnode, "backdrops", (target->layerconfig & LAYER_CONFIG_ENABLE_BACKDROP) != 0);
				xml_set_attribute_int(targetnode, "overlays", (target->layerconfig & LAYER_CONFIG_ENABLE_OVERLAY) != 0);
				xml_set_attribute_int(targetnode, "bezels", (target->layerconfig & LAYER_CONFIG_ENABLE_BEZEL) != 0);
				xml_set_attribute_int(targetnode, "zoom", (target->layerconfig & LAYER_CONFIG_ZOOM_TO_SCREEN) != 0);
			}

			/* output rotation */
			if (target->orientation != target->base_orientation)
			{
				int rotate = 0;
				if (orientation_add(ROT90, target->base_orientation) == target->orientation)
					rotate = 90;
				else if (orientation_add(ROT180, target->base_orientation) == target->orientation)
					rotate = 180;
				else if (orientation_add(ROT270, target->base_orientation) == target->orientation)
					rotate = 270;
				assert(rotate != 0);
				xml_set_attribute_int(targetnode, "rotate", rotate);
			}
		}

		/* bump the targetnum */
		targetnum++;
	}
}


/*-------------------------------------------------
    render_get_live_screens_mask - return a
    bitmask indicating the live screens
-------------------------------------------------*/

UINT32 render_get_live_screens_mask(void)
{
	render_target *target;
	UINT32 bitmask = 0;

	/* iterate over all live targets and or together their screen masks */
	for (target = targetlist; target != NULL; target = target->next)
		bitmask |= target->curview->screens;

	return bitmask;
}


/*-------------------------------------------------
    render_get_ui_target - return the UI target
-------------------------------------------------*/

render_target *render_get_ui_target(void)
{
	/* this is a bit of a kludge; right now the UI target is always the first */
	return targetlist;
}


/*-------------------------------------------------
    render_get_ui_aspect - return the aspect
    ratio for UI fonts
-------------------------------------------------*/

float render_get_ui_aspect(void)
{
	render_target *target = render_get_ui_target();
	if (target != NULL)
	{
		int orient = orientation_add(target->orientation, ui_container->orientation);
		float aspect;

		/* based on the orientation of the target, compute height/width or width/height */
		if (!(orient & ORIENTATION_SWAP_XY))
			aspect = (float)target->height / (float)target->width;
		else
			aspect = (float)target->width / (float)target->height;

		/* if we have a valid pixel aspect, apply that and return */
		if (target->pixel_aspect != 0.0f)
			return aspect * target->pixel_aspect;

		/* if not, clamp for extreme proportions */
		if (aspect < 0.66f)
			aspect = 0.66f;
		if (aspect > 1.5f)
			aspect = 1.5f;
		return aspect;
	}

	return 1.0f;
}




/***************************************************************************

    Render targets

***************************************************************************/

/*-------------------------------------------------
    render_target_alloc - allocate a new render
    target
-------------------------------------------------*/

render_target *render_target_alloc(const char *layoutfile, int singleview)
{
	layout_file **nextfile;
	render_target *target;
	render_target **nextptr;
	int scrnum, listnum, scrcount;

	/* allocate memory for the target */
	target = malloc_or_die(sizeof(*target));
	memset(target, 0, sizeof(*target));

	/* add it to the end of the list */
	for (nextptr = &targetlist; *nextptr != NULL; nextptr = &(*nextptr)->next) ;
	*nextptr = target;

	/* fill in the basics with reasonable defaults */
	target->width = 640;
	target->height = 480;
	target->pixel_aspect = 0.0f;
	target->orientation = ROT0;
	target->layerconfig = LAYER_CONFIG_ENABLE_BACKDROP | LAYER_CONFIG_ENABLE_OVERLAY | LAYER_CONFIG_ENABLE_BEZEL;
	target->base_orientation = -1;
	target->base_layerconfig = -1;
	target->maxtexwidth = 65536;
	target->maxtexheight = 65536;

	/* allocate a lock for the primitive list */
	for (listnum = 0; listnum < NUM_PRIMLISTS; listnum++)
		target->primlist[listnum].lock = osd_lock_alloc();

	/* count the screens */
	for (scrnum = 0; Machine->drv->screen[scrnum].tag != NULL; scrnum++) ;
	scrcount = scrnum;

	/* start the file list */
	nextfile = &target->filelist;

	/* if there's an explicit file, load that first */
	if (layoutfile != NULL)
	{
		*nextfile = load_layout_file(Machine->gamedrv->name, layoutfile);
		if (*nextfile != NULL)
			nextfile = &(*nextfile)->next;
	}

	/* try to load a file based on the driver name */
	if (!singleview || target->filelist == NULL)
	{
		*nextfile = load_layout_file(Machine->gamedrv->name, Machine->gamedrv->name);
		if (*nextfile != NULL)
			nextfile = &(*nextfile)->next;
	}

	/* if a default view has been specified, use that as a fallback */
	if (!singleview || target->filelist == NULL)
	{
		if (Machine->gamedrv->default_layout != NULL)
		{
			*nextfile = load_layout_file(NULL, Machine->gamedrv->default_layout);
			if (*nextfile != NULL)
				nextfile = &(*nextfile)->next;
		}
	}
	if (!singleview || target->filelist == NULL)
	{
		if (Machine->drv->default_layout != NULL)
		{
			*nextfile = load_layout_file(NULL, Machine->drv->default_layout);
			if (*nextfile != NULL)
				nextfile = &(*nextfile)->next;
		}
	}

	/* try to load another file based on the parent driver name */
	if (!singleview || target->filelist == NULL)
	{
		const game_driver *cloneof = driver_get_clone(Machine->gamedrv);
		if (cloneof != NULL)
		{
			*nextfile = load_layout_file(cloneof->name, cloneof->name);
			if (*nextfile != NULL)
				nextfile = &(*nextfile)->next;
		}
	}

	/* now do the built-in layouts for single-screen games */
	if ((!singleview || target->filelist == NULL) && Machine->drv->screen[1].tag == NULL)
	{
		if (Machine->gamedrv->flags & ORIENTATION_SWAP_XY)
			*nextfile = load_layout_file(NULL, layout_vertical);
		else
			*nextfile = load_layout_file(NULL, layout_horizont);
		assert_always(*nextfile != NULL, "Couldn't parse default layout??");
		nextfile = &(*nextfile)->next;
	}

	/* set the current view to the first one */
	render_target_set_view(target, 0);
	return target;
}


/*-------------------------------------------------
    render_target_free - free memory for a render
    target
-------------------------------------------------*/

void render_target_free(render_target *target)
{
	render_target **curr;
	int listnum;

	/* remove us from the list */
	for (curr = &targetlist; *curr != target; curr = &(*curr)->next) ;
	*curr = target->next;

	/* free any primitives */
	for (listnum = 0; listnum < NUM_PRIMLISTS; listnum++)
	{
		release_render_list(&target->primlist[listnum]);
		osd_lock_free(target->primlist[listnum].lock);
	}

	/* free the layout files */
	while (target->filelist != NULL)
	{
		layout_file *temp = target->filelist;
		target->filelist = temp->next;
		layout_file_free(temp);
	}

	/* free the target itself */
	free(target);
}


/*-------------------------------------------------
    render_target_get_indexed - get a render_target
    by index
-------------------------------------------------*/

render_target *render_target_get_indexed(int index)
{
	render_target *target;

	/* count up the targets until we hit the requested index */
	for (target = targetlist; target != NULL; target = target->next)
		if (index-- == 0)
			return target;
	return NULL;
}


/*-------------------------------------------------
    render_target_get_view_name - return the
    name of the indexed view, or NULL if it
    doesn't exist
-------------------------------------------------*/

const char *render_target_get_view_name(render_target *target, int viewindex)
{
	layout_file *file;
	layout_view *view;

	/* return the name from the indexed view */
	for (file = target->filelist; file != NULL; file = file->next)
		for (view = file->viewlist; view != NULL; view = view->next)
			if (viewindex-- == 0)
				return view->name;

	return NULL;
}


/*-------------------------------------------------
    render_target_get_view_screens - return a
    bitmask of which screens are visible on a
    given view
-------------------------------------------------*/

UINT32 render_target_get_view_screens(render_target *target, int viewindex)
{
	layout_file *file;
	layout_view *view;

	/* return the name from the indexed view */
	for (file = target->filelist; file != NULL; file = file->next)
		for (view = file->viewlist; view != NULL; view = view->next)
			if (viewindex-- == 0)
				return view->screens;

	return 0;
}


/*-------------------------------------------------
    render_target_get_bounds - get the bounds and
    pixel aspect of a target
-------------------------------------------------*/

void render_target_get_bounds(render_target *target, INT32 *width, INT32 *height, float *pixel_aspect)
{
	if (width != NULL)
		*width = target->width;
	if (height != NULL)
		*height = target->height;
	if (pixel_aspect != NULL)
		*pixel_aspect = target->pixel_aspect;
}


/*-------------------------------------------------
    render_target_set_bounds - set the bounds and
    pixel aspect of a target
-------------------------------------------------*/

void render_target_set_bounds(render_target *target, INT32 width, INT32 height, float pixel_aspect)
{
	target->width = width;
	target->height = height;
	target->pixel_aspect = pixel_aspect;
}


/*-------------------------------------------------
    render_target_get_orientation - get the
    orientation of a target
-------------------------------------------------*/

int render_target_get_orientation(render_target *target)
{
	return target->orientation;
}


/*-------------------------------------------------
    render_target_set_orientation - set the
    orientation of a target
-------------------------------------------------*/

void render_target_set_orientation(render_target *target, int orientation)
{
	target->orientation = orientation;
}


/*-------------------------------------------------
    render_target_get_layer_config - get the
    layer config of a target
-------------------------------------------------*/

int render_target_get_layer_config(render_target *target)
{
	return target->layerconfig;
}


/*-------------------------------------------------
    render_target_set_layer_config - set the
    layer config of a target
-------------------------------------------------*/

void render_target_set_layer_config(render_target *target, int layerconfig)
{
	target->layerconfig = layerconfig;
	layout_view_recompute(target->curview, layerconfig);
}


/*-------------------------------------------------
    render_target_get_view - return the currently
    selected view index
-------------------------------------------------*/

int render_target_get_view(render_target *target)
{
	layout_file *file;
	layout_view *view;
	int index = 0;

	/* find the first named match */
	for (file = target->filelist; file != NULL; file = file->next)
		for (view = file->viewlist; view != NULL; view = view->next)
		{
			if (target->curview == view)
				return index;
			index++;
		}
	return 0;
}


/*-------------------------------------------------
    render_target_set_view - dynamically change
    the view for a target
-------------------------------------------------*/

void render_target_set_view(render_target *target, int viewindex)
{
	layout_file *file;
	layout_view *view;

	/* find the first named match */
	for (file = target->filelist; file != NULL; file = file->next)
		for (view = file->viewlist; view != NULL; view = view->next)
			if (viewindex-- == 0)
			{
				target->curview = view;
				layout_view_recompute(view, target->layerconfig);
				break;
			}
}


/*-------------------------------------------------
    render_target_set_max_texture_size - set the
    upper bound on the texture size
-------------------------------------------------*/

void render_target_set_max_texture_size(render_target *target, int maxwidth, int maxheight)
{
	target->maxtexwidth = maxwidth;
	target->maxtexheight = maxheight;
}


/*-------------------------------------------------
    render_target_compute_visible_area - compute
    the visible area for the given target with
    the current layout and proposed new parameters
-------------------------------------------------*/

void render_target_compute_visible_area(render_target *target, INT32 target_width, INT32 target_height, float target_pixel_aspect, UINT8 target_orientation, INT32 *visible_width, INT32 *visible_height)
{
	float width, height;
	float scale;

	/* constrained case */
	if (target_pixel_aspect != 0.0f)
	{
		/* start with the aspect ratio of the square pixel layout */
		width = (target->layerconfig & LAYER_CONFIG_ZOOM_TO_SCREEN) ? target->curview->scraspect : target->curview->aspect;
		height = 1.0f;

		/* first apply target orientation */
		if (target_orientation & ORIENTATION_SWAP_XY)
			FSWAP(width, height);

		/* apply the target pixel aspect ratio */
		height *= target_pixel_aspect;

		/* based on the height/width ratio of the source and target, compute the scale factor */
		if (width / height > (float)target_width / (float)target_height)
			scale = (float)target_width / width;
		else
			scale = (float)target_height / height;
	}

	/* stretch-to-fit case */
	else
	{
		width = (float)target_width;
		height = (float)target_height;
		scale = 1.0f;
	}

	/* set the final width/height */
	if (visible_width != NULL)
		*visible_width = round_nearest(width * scale);
	if (visible_height != NULL)
		*visible_height = round_nearest(height * scale);
}


/*-------------------------------------------------
    render_target_get_minimum_size - get the
    "minimum" size of a target, which is the
    smallest bounds that will ensure at least
    1 target pixel per source pixel for all
    included screens
-------------------------------------------------*/

void render_target_get_minimum_size(render_target *target, INT32 *minwidth, INT32 *minheight)
{
	float maxxscale = 1.0f, maxyscale = 1.0f;
	int layer;

	/* scan the current view for all screens */
	for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
	{
		view_item *item;

		/* iterate over items in the layer */
		for (item = target->curview->itemlist[layer]; item != NULL; item = item->next)
			if (item->element == NULL)
			{
				const rectangle vectorvis = { 0, 639, 0, 479 };
				const rectangle *visarea;
				render_container *container = screen_container[item->index];
				render_bounds bounds;
				float xscale, yscale;

				/* we may be called very early, before Machine->visible_area is initialized; handle that case */
				if ((Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) != 0)
					visarea = &vectorvis;
				else if (Machine->visible_area[item->index].max_x > Machine->visible_area[item->index].min_x)
					visarea = &Machine->visible_area[item->index];
				else
					visarea = &Machine->drv->screen[item->index].default_visible_area;

				/* apply target orientation to the bounds */
				bounds = item->bounds;
				apply_orientation(&bounds, target->orientation);
				normalize_bounds(&bounds);

				/* based on the orientation of the screen container, check the bitmap */
				if (!(orientation_add(target->orientation, container->orientation) & ORIENTATION_SWAP_XY))
				{
					xscale = (float)(visarea->max_x + 1 - visarea->min_x) / (bounds.x1 - bounds.x0);
					yscale = (float)(visarea->max_y + 1 - visarea->min_y) / (bounds.y1 - bounds.y0);
				}
				else
				{
					xscale = (float)(visarea->max_y + 1 - visarea->min_y) / (bounds.x1 - bounds.x0);
					yscale = (float)(visarea->max_x + 1 - visarea->min_x) / (bounds.y1 - bounds.y0);
				}

				/* pick the greater */
				if (xscale > maxxscale)
					maxxscale = xscale;
				if (yscale > maxyscale)
					maxyscale = yscale;
			}
	}

	/* round up */
	if (minwidth != NULL)
		*minwidth = round_nearest(maxxscale);
	if (minheight != NULL)
		*minheight = round_nearest(maxyscale);
}


/*-------------------------------------------------
    render_target_get_primitives - return a list
    of primitives for a given render target
-------------------------------------------------*/

const render_primitive_list *render_target_get_primitives(render_target *target)
{
	object_transform root_xform, ui_xform;
	int itemcount[ITEM_LAYER_MAX];
	render_primitive *prim;
	INT32 viswidth, visheight;
	int layer, listnum;

	/* remember the base values if this is the first frame */
	if (target->base_view == NULL)
		target->base_view = target->curview;
	if (target->base_orientation == -1)
		target->base_orientation = target->orientation;
	if (target->base_layerconfig == -1)
		target->base_layerconfig = target->layerconfig;

	/* find a non-busy list to work with */
	for (listnum = 0; listnum < NUM_PRIMLISTS; listnum++)
		if (osd_lock_try(target->primlist[listnum].lock))
			break;
	if (listnum == NUM_PRIMLISTS)
	{
		listnum = 0;
		osd_lock_acquire(target->primlist[listnum].lock);
	}

	/* free any previous primitives */
	release_render_list(&target->primlist[listnum]);

	/* start with a clip push primitive */
	prim = alloc_render_primitive(RENDER_PRIMITIVE_CLIP_PUSH);
	prim->bounds.x0 = 0.0f;
	prim->bounds.y0 = 0.0f;
	prim->bounds.x1 = (float)target->width;
	prim->bounds.y1 = (float)target->height;
	append_render_primitive(&target->primlist[listnum], prim);

	/* compute the visible width/height */
	render_target_compute_visible_area(target, target->width, target->height, target->pixel_aspect, target->orientation, &viswidth, &visheight);

	/* create a root transform for the target */
	root_xform.xoffs = (target->width - viswidth) / 2;
	root_xform.yoffs = (target->height - visheight) / 2;
	root_xform.xscale = viswidth;
	root_xform.yscale = visheight;
	root_xform.color.r = root_xform.color.g = root_xform.color.b = root_xform.color.a = 1.0f;
	root_xform.orientation = target->orientation;

	/* iterate over layers back-to-front */
	if (mame_get_phase() >= MAME_PHASE_RESET)
		for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
			if (target->curview->layenabled[layer])
			{
				int blendmode = BLENDMODE_ALPHA;
				view_item *item;

				/* pick a blendmode */
				if (layer == ITEM_LAYER_SCREEN)
					blendmode = BLENDMODE_ADD;
				else if (layer == ITEM_LAYER_OVERLAY)
					blendmode = BLENDMODE_RGB_MULTIPLY;

				/* iterate over items in the layer */
				itemcount[layer] = 0;
				for (item = target->curview->itemlist[layer]; item != NULL; item = item->next)
				{
					object_transform item_xform;
					render_bounds bounds;

					/* first apply orientation to the bounds */
					bounds = item->bounds;
					apply_orientation(&bounds, root_xform.orientation);
					normalize_bounds(&bounds);

					/* apply the transform to the item */
					item_xform.xoffs = root_xform.xoffs + bounds.x0 * root_xform.xscale;
					item_xform.yoffs = root_xform.yoffs + bounds.y0 * root_xform.yscale;
					item_xform.xscale = (bounds.x1 - bounds.x0) * root_xform.xscale;
					item_xform.yscale = (bounds.y1 - bounds.y0) * root_xform.yscale;
					item_xform.color.r = item->color.r * root_xform.color.r;
					item_xform.color.g = item->color.g * root_xform.color.g;
					item_xform.color.b = item->color.b * root_xform.color.b;
					item_xform.color.a = item->color.a * root_xform.color.a;
					item_xform.orientation = orientation_add(item->orientation, root_xform.orientation);

					/* if there is no associated element, it must be a screen element */
					if (item->element != NULL)
						add_element_primitives(target, &target->primlist[listnum], &item_xform, item->element, item->state->curstate, blendmode);
					else
						add_container_primitives(target, &target->primlist[listnum], &item_xform, screen_container[item->index], blendmode);

					/* keep track of how many items are in the layer */
					itemcount[layer]++;
				}
			}

	/* process the UI if we are the UI target */
	if (target == render_get_ui_target())
	{
		/* compute the transform for the UI */
		ui_xform.xoffs = 0;
		ui_xform.yoffs = 0;
		ui_xform.xscale = target->width;
		ui_xform.yscale = target->height;
		ui_xform.color.r = ui_xform.color.g = ui_xform.color.b = ui_xform.color.a = 1.0f;
		ui_xform.orientation = target->orientation;

		/* add UI elements */
		add_container_primitives(target, &target->primlist[listnum], &ui_xform, ui_container, BLENDMODE_ALPHA);
	}

	/* add a clip pop primitive for completeness */
	prim = alloc_render_primitive(RENDER_PRIMITIVE_CLIP_POP);
	append_render_primitive(&target->primlist[listnum], prim);

	/* optimize the list before handing it off */
	add_clear_and_optimize_primitive_list(target, &target->primlist[listnum]);
	osd_lock_release(target->primlist[listnum].lock);
	return &target->primlist[listnum];
}


/*-------------------------------------------------
    release_render_list - release the contents of
    a render list
-------------------------------------------------*/

static void release_render_list(render_primitive_list *list)
{
	/* take the lock */
	osd_lock_acquire(list->lock);

	/* free everything on the list */
	while (list->head != NULL)
	{
		render_primitive *temp = list->head;
		list->head = temp->next;
		free_render_primitive(temp);
	}
	list->nextptr = &list->head;

	/* release all our references */
	while (list->reflist != NULL)
	{
		render_ref *temp = list->reflist;
		list->reflist = temp->next;
		free_render_ref(temp);
	}

	/* let other people at it again */
	osd_lock_release(list->lock);
}


/*-------------------------------------------------
    add_container_primitives - add primitives
    based on the container
-------------------------------------------------*/

static void add_container_primitives(render_target *target, render_primitive_list *list, const object_transform *xform, const render_container *container, int blendmode)
{
	int orientation = orientation_add(container->orientation, xform->orientation);
	render_primitive *prim;
	container_item *item;

	/* add a clip push primitive */
	prim = alloc_render_primitive(RENDER_PRIMITIVE_CLIP_PUSH);
	prim->bounds.x0 = round_nearest(xform->xoffs);
	prim->bounds.y0 = round_nearest(xform->yoffs);
	prim->bounds.x1 = prim->bounds.x0 + round_nearest(xform->xscale);
	prim->bounds.y1 = prim->bounds.y0 + round_nearest(xform->yscale);
	append_render_primitive(list, prim);

	/* iterate over elements */
	for (item = container->itemlist; item != NULL; item = item->next)
	{
		render_bounds bounds;
		int width, height;

		/* compute the oriented bounds */
		bounds = item->bounds;
		apply_orientation(&bounds, orientation);

		/* allocate the primitive */
		prim = alloc_render_primitive(0);
		prim->bounds.x0 = round_nearest(xform->xoffs + bounds.x0 * xform->xscale);
		prim->bounds.y0 = round_nearest(xform->yoffs + bounds.y0 * xform->yscale);
		if (item->internal & INTERNAL_FLAG_CHAR)
		{
			prim->bounds.x1 = prim->bounds.x0 + round_nearest((bounds.x1 - bounds.x0) * xform->xscale);
			prim->bounds.y1 = prim->bounds.y0 + round_nearest((bounds.y1 - bounds.y0) * xform->yscale);
		}
		else
		{
			prim->bounds.x1 = round_nearest(xform->xoffs + bounds.x1 * xform->xscale);
			prim->bounds.y1 = round_nearest(xform->yoffs + bounds.y1 * xform->yscale);
		}
		prim->color.r = xform->color.r * item->color.r;
		prim->color.g = xform->color.g * item->color.g;
		prim->color.b = xform->color.b * item->color.b;
		prim->color.a = xform->color.a * item->color.a;

		/* now switch off the type */
		switch (item->type)
		{
			case CONTAINER_ITEM_LINE:
				/* set the line type */
				prim->type = RENDER_PRIMITIVE_LINE;

				/* scale the width by the minimum of X/Y scale factors */
				prim->width = item->width * MIN(xform->xscale, xform->yscale);
				prim->flags = item->flags;
				break;

			case CONTAINER_ITEM_QUAD:
				/* set the quad type */
				prim->type = RENDER_PRIMITIVE_QUAD;

				/* normalize the bounds */
				normalize_bounds(&prim->bounds);

				/* get the scaled bitmap and set the resulting palette */
				if (item->texture != NULL)
				{
					/* determine the final orientation */
					int finalorient = orientation_add(PRIMFLAG_GET_TEXORIENT(item->flags), orientation);

					/* based on the swap values, get the scaled final texture */
					width = (finalorient & ORIENTATION_SWAP_XY) ? (prim->bounds.y1 - prim->bounds.y0) : (prim->bounds.x1 - prim->bounds.x0);
					height = (finalorient & ORIENTATION_SWAP_XY) ? (prim->bounds.x1 - prim->bounds.x0) : (prim->bounds.y1 - prim->bounds.y0);
					width = MIN(width, target->maxtexwidth);
					height = MIN(height, target->maxtexheight);
					render_texture_get_scaled(item->texture, width, height, &prim->texture, &list->reflist);

					/* apply the final orientation from the quad flags and then build up the final flags */
					prim->flags = (item->flags & ~(PRIMFLAG_TEXORIENT_MASK | PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) |
									PRIMFLAG_TEXORIENT(finalorient) |
									PRIMFLAG_BLENDMODE(blendmode) |
									PRIMFLAG_TEXFORMAT(item->texture->format);
				}
				else
				{
					/* no texture -- set the basic flags */
					prim->texture.base = NULL;
					prim->flags = PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA);
				}
				break;
		}

		/* add to the list */
		append_render_primitive(list, prim);
	}

	/* add a clip pop primitive */
	prim = alloc_render_primitive(RENDER_PRIMITIVE_CLIP_POP);
	append_render_primitive(list, prim);
}


/*-------------------------------------------------
    add_element_primitives - add the primitive
    for an element in the current state
-------------------------------------------------*/

static void add_element_primitives(render_target *target, render_primitive_list *list, const object_transform *xform, const layout_element *element, int state, int blendmode)
{
	INT32 width = round_nearest(xform->xscale);
	INT32 height = round_nearest(xform->yscale);
	render_texture *texture;

	/* if we're out of range, bail */
	if (state > element->maxstate)
		return;
	if (state < 0)
		state = 0;

	/* get a pointer to the relevant texture */
	texture = element->elemtex[state].texture;
	if (texture != NULL)
	{
		render_primitive *prim = alloc_render_primitive(RENDER_PRIMITIVE_QUAD);

		/* configure the basics */
		prim->color = xform->color;
		prim->flags = PRIMFLAG_TEXORIENT(xform->orientation) | PRIMFLAG_BLENDMODE(blendmode) | PRIMFLAG_TEXFORMAT(texture->format);

		/* compute the bounds */
		set_bounds_wh(&prim->bounds, round_nearest(xform->xoffs), round_nearest(xform->yoffs), width, height);
		if (xform->orientation & ORIENTATION_SWAP_XY)
			ISWAP(width, height);
		width = MIN(width, target->maxtexwidth);
		height = MIN(height, target->maxtexheight);

		/* get the scaled texture and append it */
		render_texture_get_scaled(texture, width, height, &prim->texture, &list->reflist);
		append_render_primitive(list, prim);
	}
}


/*-------------------------------------------------
    init_clear_extents - reset the extents list
-------------------------------------------------*/

static void init_clear_extents(INT32 width, INT32 height)
{
	clear_extents[0] = -height;
	clear_extents[1] = 1;
	clear_extents[2] = width;
	clear_extent_count = 3;
}


/*-------------------------------------------------
    remove_clear_extent - remove a quad from the
    list of stuff to clear, unless it overlaps
    a previous quad
-------------------------------------------------*/

static int remove_clear_extent(const render_bounds *bounds)
{
	INT32 *max = &clear_extents[MAX_CLEAR_EXTENTS];
	INT32 *last = &clear_extents[clear_extent_count];
	INT32 *ext = &clear_extents[0];
	INT32 boundsx0 = round_nearest(bounds->x0);
	INT32 boundsx1 = round_nearest(bounds->x1);
	INT32 boundsy0 = round_nearest(bounds->y0);
	INT32 boundsy1 = round_nearest(bounds->y1);
	INT32 y0, y1 = 0;

	/* loop over Y extents */
	while (ext < last)
	{
		INT32 *linelast;

		/* first entry of each line should always be negative */
		assert(ext[0] < 0.0f);
		y0 = y1;
		y1 = y0 - ext[0];

		/* do we intersect this extent? */
		if (boundsy0 < y1 && boundsy1 > y0)
		{
			INT32 *xext;
			INT32 x0, x1 = 0;

			/* split the top */
			if (y0 < boundsy0)
			{
				int diff = boundsy0 - y0;

				/* make a copy of this extent */
				memmove(&ext[ext[1] + 2], &ext[0], (last - ext) * sizeof(*ext));
				last += ext[1] + 2;
				assert_always(last < max, "Ran out of clear extents!\n");

				/* split the extent between pieces */
				ext[ext[1] + 2] = -(-ext[0] - diff);
				ext[0] = -diff;

				/* advance to the new extent */
				y0 -= ext[0];
				ext += ext[1] + 2;
				y1 = y0 - ext[0];
			}

			/* split the bottom */
			if (y1 > boundsy1)
			{
				int diff = y1 - boundsy1;

				/* make a copy of this extent */
				memmove(&ext[ext[1] + 2], &ext[0], (last - ext) * sizeof(*ext));
				last += ext[1] + 2;
				assert_always(last < max, "Ran out of clear extents!\n");

				/* split the extent between pieces */
				ext[ext[1] + 2] = -diff;
				ext[0] = -(-ext[0] - diff);

				/* recompute y1 */
				y1 = y0 - ext[0];
			}

			/* now remove the X extent */
			linelast = &ext[ext[1] + 2];
			xext = &ext[2];
			while (xext < linelast)
			{
				x0 = x1;
				x1 = x0 + xext[0];

				/* do we fully intersect this extent? */
				if (boundsx0 >= x0 && boundsx1 <= x1)
				{
					/* yes; split it */
					memmove(&xext[2], &xext[0], (last - xext) * sizeof(*xext));
					last += 2;
					linelast += 2;
					assert_always(last < max, "Ran out of clear extents!\n");

					/* split this extent into three parts */
					xext[0] = boundsx0 - x0;
					xext[1] = boundsx1 - boundsx0;
					xext[2] = x1 - boundsx1;

					/* recompute x1 */
					x1 = boundsx1;
					xext += 2;
				}

				/* do we partially intersect this extent? */
				else if (boundsx0 < x1 && boundsx1 > x0)
					goto abort;

				/* advance */
				xext++;

				/* do we partially intersect the next extent (which is a non-clear extent)? */
				if (xext < linelast)
				{
					x0 = x1;
					x1 = x0 + xext[0];
					if (boundsx0 < x1 && boundsx1 > x0)
						goto abort;
					xext++;
				}
			}

			/* update the count */
			ext[1] = linelast - &ext[2];
		}

		/* advance to the next row */
		ext += 2 + ext[1];
	}

	/* update the total count */
	clear_extent_count = last - &clear_extents[0];
	return TRUE;

abort:
	/* update the total count even on a failure as we may have split extents */
	clear_extent_count = last - &clear_extents[0];
	return FALSE;
}


/*-------------------------------------------------
    add_clear_extents - add the accumulated
    extents as a series of quads to clear
-------------------------------------------------*/

static void add_clear_extents(render_primitive_list *list)
{
	render_primitive *clearlist = NULL;
	render_primitive **clearnext = &clearlist;
	INT32 *last = &clear_extents[clear_extent_count];
	INT32 *ext = &clear_extents[0];
	INT32 y0, y1 = 0;

	/* loop over all extents */
	while (ext < last)
	{
		INT32 *linelast = &ext[ext[1] + 2];
		INT32 *xext = &ext[2];
		INT32 x0, x1 = 0;

		/* first entry should always be negative */
		assert(ext[0] < 0);
		y0 = y1;
		y1 = y0 - ext[0];

		/* now remove the X extent */
		while (xext < linelast)
		{
			x0 = x1;
			x1 = x0 + *xext++;

			/* only add entries for non-zero widths */
			if (x1 - x0 > 0)
			{
				render_primitive *prim = alloc_render_primitive(RENDER_PRIMITIVE_QUAD);
				set_bounds_xy(&prim->bounds, (float)x0, (float)y0, (float)x1, (float)y1);
				set_color(&prim->color, 1.0f, 0.0f, 0.0f, 0.0f);
				prim->texture.base = NULL;
				prim->flags = PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA);
				*clearnext = prim;
				clearnext = &prim->next;
			}

			/* skip the non-clearing extent */
			x0 = x1;
			x1 = x0 + *xext++;
		}

		/* advance to the next part */
		ext += 2 + ext[1];
	}

	/* we know that the first primitive in the list will be the global clip */
	/* so we insert the clears immediately after */
	assert(list->head->type == RENDER_PRIMITIVE_CLIP_PUSH);
	*clearnext = list->head->next;
	list->head->next = clearlist;
}


/*-------------------------------------------------
    add_clear_and_optimize_primitive_list -
    optimize the primitive list
-------------------------------------------------*/

static void add_clear_and_optimize_primitive_list(render_target *target, render_primitive_list *list)
{
	render_bounds *clipstack[8];
	render_bounds **clip = &clipstack[-1];
	render_primitive *prim;

	/* start with the assumption that we need to clear the whole screen */
	init_clear_extents(target->width, target->height);

	/* scan the list until we hit an intersection quad or a line */
	for (prim = list->head; prim != NULL; prim = prim->next)
	{
		/* switch off the type */
		switch (prim->type)
		{
			case RENDER_PRIMITIVE_CLIP_PUSH:
				*++clip = &prim->bounds;
				break;

			case RENDER_PRIMITIVE_CLIP_POP:
				clip--;
				break;

			case RENDER_PRIMITIVE_LINE:
				goto done;

			case RENDER_PRIMITIVE_QUAD:
			{
				render_bounds inner = prim->bounds;

				/* stop when we hit an alpha texture */
				if (PRIMFLAG_GET_TEXFORMAT(prim->flags) == TEXFORMAT_ARGB32)
					goto done;

				/* if this quad can't be cleanly removed from the extents list, we're done */
				sect_bounds(&inner, *clip);
				if (!remove_clear_extent(&inner))
					goto done;

				/* change the blendmode on the first primitive to be NONE */
				if (PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_RGB_MULTIPLY)
				{
					/* RGB multiply will multiply against 0, leaving nothing */
					set_color(&prim->color, 1.0f, 0.0f, 0.0f, 0.0f);
					prim->texture.base = NULL;
					prim->flags = (prim->flags & ~PRIMFLAG_BLENDMODE_MASK) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE);
				}
				else
				{
					/* for alpha or add modes, we will blend against 0 or add to 0; treat it like none */
					prim->flags = (prim->flags & ~PRIMFLAG_BLENDMODE_MASK) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE);
				}

				/* since alpha is disabled, premultiply the RGB values and reset the alpha to 1.0 */
				prim->color.r *= prim->color.a;
				prim->color.g *= prim->color.a;
				prim->color.b *= prim->color.a;
				prim->color.a = 1.0f;
				break;
			}
		}
	}

done:
	/* now add the extents to the clear list */
	add_clear_extents(list);
}



/***************************************************************************

    View item state tracking

***************************************************************************/

/*-------------------------------------------------
    render_view_item_get_state_ptr - get a pointer
    to the state block associated with the given
    name
-------------------------------------------------*/

static view_item_state *render_view_item_get_state_ptr(const char *itemname)
{
	view_item_state *state;

	/* search the statelist for a matching string */
	for (state = item_statelist; state != NULL; state = state->next)
		if (strcmp(itemname, state->name) == 0)
			return state;

	/* allocate a new one */
	state = malloc_or_die(sizeof(*state));
	state->name = copy_string(itemname);
	state->curstate = 0;

	/* add us to the list */
	state->next = item_statelist;
	item_statelist = state;
	return state;
}


/*-------------------------------------------------
    render_view_item_get_state - return the value
    associated with a given state name
-------------------------------------------------*/

int render_view_item_get_state(const char *itemname)
{
	view_item_state *state;

	/* search the statelist for a matching string */
	for (state = item_statelist; state != NULL; state = state->next)
		if (strcmp(itemname, state->name) == 0)
			return state->curstate;

	return 0;
}


/*-------------------------------------------------
    render_view_item_set_state - set the value
    associated with a given state name
-------------------------------------------------*/

void render_view_item_set_state(const char *itemname, int newstate)
{
	view_item_state *state;

	/* search the statelist for a matching string */
	for (state = item_statelist; state != NULL; state = state->next)
		if (strcmp(itemname, state->name) == 0)
		{
			state->curstate = newstate;
			break;
		}
}



/***************************************************************************

    Render references

***************************************************************************/

/*-------------------------------------------------
    invalidate_all_render_ref - remove all refs
    to a particular reference pointer
-------------------------------------------------*/

static void invalidate_all_render_ref(void *refptr)
{
	render_target *target;
	int listnum;

	/* loop over targets */
	for (target = targetlist; target != NULL; target = target->next)
		for (listnum = 0; listnum < NUM_PRIMLISTS; listnum++)
		{
			render_primitive_list *list = &target->primlist[listnum];
			osd_lock_acquire(list->lock);
			if (has_render_ref(list->reflist, refptr))
				release_render_list(list);
			osd_lock_release(list->lock);
		}
}



/***************************************************************************

    Render textures

***************************************************************************/

/*-------------------------------------------------
    render_texture_alloc - allocate a new texture
-------------------------------------------------*/

render_texture *render_texture_alloc(mame_bitmap *bitmap, const rectangle *sbounds, rgb_t *palette, int format, texture_scaler scaler, void *param)
{
	render_texture *texture;

	/* allocate memory for a texture */
	texture = malloc_or_die(sizeof(*texture));
	memset(texture, 0, sizeof(*texture));

	/* fill in the data */
	texture->bitmap = bitmap;
	texture->sbounds.min_x = (sbounds != NULL) ? sbounds->min_x : 0;
	texture->sbounds.min_y = (sbounds != NULL) ? sbounds->min_y : 0;
	texture->sbounds.max_x = (sbounds != NULL) ? sbounds->max_x : (bitmap != NULL) ? bitmap->width : 1000;
	texture->sbounds.max_y = (sbounds != NULL) ? sbounds->max_y : (bitmap != NULL) ? bitmap->height : 1000;
	texture->palette = palette;
	texture->format = format;
	texture->scaler = scaler;
	texture->param = param;
	return texture;
}


/*-------------------------------------------------
    render_texture_free - free an allocated
    texture
-------------------------------------------------*/

void render_texture_free(render_texture *texture)
{
	int scalenum;

	/* free all scaled versions */
	for (scalenum = 0; scalenum < ARRAY_LENGTH(texture->scaled); scalenum++)
		if (texture->scaled[scalenum].bitmap != NULL)
		{
			invalidate_all_render_ref(texture->scaled[scalenum].bitmap);
			bitmap_free(texture->scaled[scalenum].bitmap);
		}

	/* invalidate references to the original bitmap as well */
	invalidate_all_render_ref(texture->bitmap);

	/* and the texture itself */
	free(texture);
}


/*-------------------------------------------------
    render_texture_set_bitmap - set a new source
    bitmap
-------------------------------------------------*/

void render_texture_set_bitmap(render_texture *texture, mame_bitmap *bitmap, const rectangle *sbounds, rgb_t *palette, int format)
{
	int scalenum;

	/* invalidate references to the old bitmap */
	if (bitmap != texture->bitmap)
		invalidate_all_render_ref(texture->bitmap);

	/* set the new bitmap/palette */
	texture->bitmap = bitmap;
	texture->sbounds.min_x = (sbounds != NULL) ? sbounds->min_x : 0;
	texture->sbounds.min_y = (sbounds != NULL) ? sbounds->min_y : 0;
	texture->sbounds.max_x = (sbounds != NULL) ? sbounds->max_x : (bitmap != NULL) ? bitmap->width : 1000;
	texture->sbounds.max_y = (sbounds != NULL) ? sbounds->max_y : (bitmap != NULL) ? bitmap->height : 1000;
	texture->palette = palette;
	texture->format = format;

	/* invalidate all scaled versions */
	for (scalenum = 0; scalenum < ARRAY_LENGTH(texture->scaled); scalenum++)
	{
		if (texture->scaled[scalenum].bitmap != NULL)
		{
			invalidate_all_render_ref(texture->scaled[scalenum].bitmap);
			bitmap_free(texture->scaled[scalenum].bitmap);
		}
		texture->scaled[scalenum].bitmap = NULL;
		texture->scaled[scalenum].seqid = 0;
	}
}


/*-------------------------------------------------
    render_texture_get_scaled - get a scaled
    bitmap (if we can)
-------------------------------------------------*/

static void render_texture_get_scaled(render_texture *texture, UINT32 dwidth, UINT32 dheight, render_texinfo *texinfo, render_ref **reflist)
{
	UINT8 bpp = (texture->format == TEXFORMAT_PALETTE16 || texture->format == TEXFORMAT_RGB15) ? 16 : 32;
	scaled_texture *scaled = NULL;
	int swidth, sheight;
	int scalenum;

	/* source width/height come from the source bounds */
	swidth = texture->sbounds.max_x - texture->sbounds.min_x;
	sheight = texture->sbounds.max_y - texture->sbounds.min_y;

	/* ensure height/width are non-zero */
	if (dwidth < 1) dwidth = 1;
	if (dheight < 1) dheight = 1;

	/* are we scaler-free? if so, just return the source bitmap */
	if (texture->scaler == NULL || (texture->bitmap != NULL && swidth == dwidth && sheight == dheight))
	{
		add_render_ref(reflist, texture->bitmap);
		texinfo->base = (UINT8 *)texture->bitmap->base + (texture->sbounds.min_y * texture->bitmap->rowpixels + texture->sbounds.min_x) * (bpp / 8);
		texinfo->rowpixels = texture->bitmap->rowpixels;
		texinfo->width = swidth;
		texinfo->height = sheight;
		texinfo->palette = texture->palette;
		texinfo->seqid = ++texture->curseq;
		return;
	}

	/* is it a size we already have? */
	for (scalenum = 0; scalenum < ARRAY_LENGTH(texture->scaled); scalenum++)
	{
		scaled = &texture->scaled[scalenum];

		/* we need a non-NULL bitmap with matching dest size */
		if (scaled->bitmap != NULL && dwidth == scaled->bitmap->width && dheight == scaled->bitmap->height)
			break;
	}

	/* did we get one? */
	if (scalenum == ARRAY_LENGTH(texture->scaled))
	{
		/* didn't find one -- take the entry with the lowest seqnum */
		int lowest = -1;
		for (scalenum = 0; scalenum < ARRAY_LENGTH(texture->scaled); scalenum++)
			if ((lowest == -1 || texture->scaled[scalenum].seqid < texture->scaled[lowest].seqid) && !has_render_ref(*reflist, texture->scaled[scalenum].bitmap))
				lowest = scalenum;
		assert_always(lowest != -1, "Too many live texture instances!");

		/* throw out any existing entries */
		scaled = &texture->scaled[lowest];
		if (scaled->bitmap != NULL)
		{
			invalidate_all_render_ref(scaled->bitmap);
			bitmap_free(scaled->bitmap);
		}

		/* allocate a new bitmap */
		scaled->bitmap = bitmap_alloc_depth(dwidth, dheight, 32);
		scaled->seqid = ++texture->curseq;

		/* let the scaler do the work */
		(*texture->scaler)(scaled->bitmap, texture->bitmap, &texture->sbounds, texture->param);
	}

	/* finally fill out the new info */
	add_render_ref(reflist, scaled->bitmap);
	texinfo->base = scaled->bitmap->base;
	texinfo->rowpixels = scaled->bitmap->rowpixels;
	texinfo->width = dwidth;
	texinfo->height = dheight;
	texinfo->palette = texture->palette;
	texinfo->seqid = scaled->seqid;
}


/*-------------------------------------------------
    render_texture_hq_scale - generic high quality
    resampling scaler
-------------------------------------------------*/

void render_texture_hq_scale(mame_bitmap *dest, const mame_bitmap *source, const rectangle *sbounds, void *param)
{
	render_color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	render_resample_argb_bitmap_hq(dest->base, dest->rowpixels, dest->width, dest->height, source, sbounds, &color);
}



/***************************************************************************

    Render containers

***************************************************************************/

/*-------------------------------------------------
    render_container_alloc - allocate a render
    container
-------------------------------------------------*/

static render_container *render_container_alloc(void)
{
	render_container *container;

	/* allocate and clear memory */
	container = malloc_or_die(sizeof(*container));
	memset(container, 0, sizeof(*container));

	/* make sure it is empty */
	render_container_empty(container);
	return container;
}


/*-------------------------------------------------
    render_container_free - free a render
    container
-------------------------------------------------*/

static void render_container_free(render_container *container)
{
	/* free all the container items */
	render_container_empty(container);

	/* free the container itself */
	free(container);
}


/*-------------------------------------------------
    render_container_empty - empty a container
    in preparation for new stuff
-------------------------------------------------*/

void render_container_empty(render_container *container)
{
	/* free all the container items */
	while (container->itemlist != NULL)
	{
		container_item *temp = container->itemlist;
		container->itemlist = temp->next;
		free_container_item(temp);
	}

	/* reset our newly-added pointer */
	container->nextitem = &container->itemlist;
}


/*-------------------------------------------------
    render_container_is_empty - return true if
    a container has nothing in it
-------------------------------------------------*/

int render_container_is_empty(render_container *container)
{
	return (container->itemlist == NULL);
}


/*-------------------------------------------------
    render_container_get_orientation - return the
    orientation of a container
-------------------------------------------------*/

int render_container_get_orientation(render_container *container)
{
	return container->orientation;
}


/*-------------------------------------------------
    render_container_set_orientation - set the
    orientation of a container
-------------------------------------------------*/

void render_container_set_orientation(render_container *container, int orientation)
{
	container->orientation = orientation;
}


/*-------------------------------------------------
    render_container_get_ui - return a pointer
    to the UI container
-------------------------------------------------*/

render_container *render_container_get_ui(void)
{
	return ui_container;
}


/*-------------------------------------------------
    render_container_get_screen - return a pointer
    to the indexed screen container
-------------------------------------------------*/

render_container *render_container_get_screen(int screen)
{
	return screen_container[screen];
}


/*-------------------------------------------------
    render_container_item_add_generic - add a
    generic item to a container
-------------------------------------------------*/

static container_item *render_container_item_add_generic(render_container *container, UINT8 type, float x0, float y0, float x1, float y1, rgb_t argb)
{
	container_item *item = alloc_container_item();

	assert(container != NULL);

	/* copy the data into the new item */
	item->type = type;
	item->bounds.x0 = x0;
	item->bounds.y0 = y0;
	item->bounds.x1 = x1;
	item->bounds.y1 = y1;
	item->color.r = (float)RGB_RED(argb) * (1.0f / 255.0f);
	item->color.g = (float)RGB_GREEN(argb) * (1.0f / 255.0f);
	item->color.b = (float)RGB_BLUE(argb) * (1.0f / 255.0f);
	item->color.a = (float)RGB_ALPHA(argb) * (1.0f / 255.0f);

	/* add the item to the container */
	*container->nextitem = item;
	container->nextitem = &item->next;

	return item;
}


/*-------------------------------------------------
    render_container_add_line - add a line item
    to the specified container
-------------------------------------------------*/

void render_container_add_line(render_container *container, float x0, float y0, float x1, float y1, float width, rgb_t argb, UINT32 flags)
{
	container_item *item = render_container_item_add_generic(container, CONTAINER_ITEM_LINE, x0, y0, x1, y1, argb);
	item->width = width;
	item->flags = flags;
}


/*-------------------------------------------------
    render_container_add_quad - add a quad item
    to the specified container
-------------------------------------------------*/

void render_container_add_quad(render_container *container, float x0, float y0, float x1, float y1, rgb_t argb, render_texture *texture, UINT32 flags)
{
	container_item *item = render_container_item_add_generic(container, CONTAINER_ITEM_QUAD, x0, y0, x1, y1, argb);
	item->texture = texture;
	item->flags = flags;
}


/*-------------------------------------------------
    render_container_add_char - add a char item
    to the specified container
-------------------------------------------------*/

void render_container_add_char(render_container *container, float x0, float y0, float height, float aspect, rgb_t argb, render_font *font, UINT16 ch)
{
	render_texture *texture;
	render_bounds bounds;
	container_item *item;

	/* compute the bounds of the character cell and get the texture */
	bounds.x0 = x0;
	bounds.y0 = y0;
	texture = render_font_get_char_texture_and_bounds(font, height, aspect, ch, &bounds);

	/* add it like a quad */
 	item = render_container_item_add_generic(container, CONTAINER_ITEM_QUAD, bounds.x0, bounds.y0, bounds.x1, bounds.y1, argb);
 	item->texture = texture;
	item->flags = PRIMFLAG_TEXORIENT(ROT0) | PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA);
	item->internal = INTERNAL_FLAG_CHAR;
}



/***************************************************************************

    Generic high-quality texture resampler

***************************************************************************/

/*-------------------------------------------------
    render_resample_argb_bitmap_hq - perform a high
    quality resampling of a texture
-------------------------------------------------*/

void render_resample_argb_bitmap_hq(void *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const mame_bitmap *source, const rectangle *orig_sbounds, const render_color *color)
{
	const UINT32 *sbase;
	rectangle sbounds;
	UINT32 dx, dy;

	/* compute the real source bounds */
	if (orig_sbounds != NULL)
		sbounds = *orig_sbounds;
	else
	{
		sbounds.min_x = sbounds.min_y = 0;
		sbounds.max_x = source->width;
		sbounds.max_y = source->height;
	}

	/* adjust the source base */
	sbase = (const UINT32 *)source->base + sbounds.min_y * source->rowpixels + sbounds.min_x;

	/* determine the steppings */
	dx = ((sbounds.max_x - sbounds.min_x) << 12) / dwidth;
	dy = ((sbounds.max_y - sbounds.min_y) << 12) / dheight;

	/* if the source is higher res than the target, use full averaging */
	if (dx > 0x1000 || dy > 0x1000)
		resample_argb_bitmap_average(dest, drowpixels, dwidth, dheight, sbase, source->rowpixels, color, dx, dy);
	else
		resample_argb_bitmap_bilinear(dest, drowpixels, dwidth, dheight, sbase, source->rowpixels, color, dx, dy);
}


/*-------------------------------------------------
    resample_argb_bitmap_average - resample a texture
    by performing a true weighted average over
    all contributing pixels
-------------------------------------------------*/

static void resample_argb_bitmap_average(UINT32 *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const UINT32 *source, UINT32 srowpixels, const render_color *color, UINT32 dx, UINT32 dy)
{
	UINT64 sumscale = (UINT64)dx * (UINT64)dy;
	UINT32 r, g, b, a;
	UINT32 x, y;

	/* precompute premultiplied R/G/B/A factors */
	r = color->r * color->a * 256.0;
	g = color->g * color->a * 256.0;
	b = color->b * color->a * 256.0;
	a = color->a * 256.0;

	/* loop over the target vertically */
	for (y = 0; y < dheight; y++)
	{
		UINT32 starty = y * dy;

		/* loop over the target horizontally */
		for (x = 0; x < dwidth; x++)
		{
			UINT64 sumr = 0, sumg = 0, sumb = 0, suma = 0;
			UINT32 startx = x * dx;
			UINT32 xchunk, ychunk;
			UINT32 curx, cury;

			UINT32 yremaining = dy;

			/* accumulate all source pixels that contribute to this pixel */
			for (cury = starty; yremaining; cury += ychunk)
			{
				UINT32 xremaining = dx;

				/* determine the Y contribution, clamping to the amount remaining */
				ychunk = 0x1000 - (cury & 0xfff);
				if (ychunk > yremaining)
					ychunk = yremaining;
				yremaining -= ychunk;

				/* loop over all source pixels in the X direction */
				for (curx = startx; xremaining; curx += xchunk)
				{
					UINT32 factor;
					UINT32 pix;

					/* determine the X contribution, clamping to the amount remaining */
					xchunk = 0x1000 - (curx & 0xfff);
					if (xchunk > xremaining)
						xchunk = xremaining;
					xremaining -= xchunk;

					/* total contribution = x * y */
					factor = xchunk * ychunk;

					/* fetch the source pixel */
					pix = source[(cury >> 12) * srowpixels + (curx >> 12)];

					/* accumulate the RGBA values */
					sumr += factor * RGB_RED(pix);
					sumg += factor * RGB_GREEN(pix);
					sumb += factor * RGB_BLUE(pix);
					suma += factor * RGB_ALPHA(pix);
				}
			}

			/* apply scaling */
			suma = (suma / sumscale) * a / 256;
			sumr = (sumr / sumscale) * r / 256;
			sumg = (sumg / sumscale) * g / 256;
			sumb = (sumb / sumscale) * b / 256;

			/* if we're translucent, add in the destination pixel contribution */
			if (a < 256)
			{
				UINT32 dpix = dest[y * drowpixels + x];
				suma += RGB_ALPHA(dpix) * (256 - a);
				sumr += RGB_RED(dpix) * (256 - a);
				sumg += RGB_GREEN(dpix) * (256 - a);
				sumb += RGB_BLUE(dpix) * (256 - a);
			}

			/* store the target pixel, dividing the RGBA values by the overall scale factor */
			dest[y * drowpixels + x] = MAKE_ARGB(suma, sumr, sumg, sumb);
		}
	}
}


/*-------------------------------------------------
    resample_argb_bitmap_bilinear - perform texture
    sampling via a bilinear filter
-------------------------------------------------*/

static void resample_argb_bitmap_bilinear(UINT32 *dest, UINT32 drowpixels, UINT32 dwidth, UINT32 dheight, const UINT32 *source, UINT32 srowpixels, const render_color *color, UINT32 dx, UINT32 dy)
{
	UINT32 r, g, b, a;
	UINT32 x, y;

	/* precompute premultiplied R/G/B/A factors */
	r = color->r * color->a * 256.0;
	g = color->g * color->a * 256.0;
	b = color->b * color->a * 256.0;
	a = color->a * 256.0;

	/* loop over the target vertically */
	for (y = 0; y < dheight; y++)
	{
		UINT32 starty = y * dy;

		/* loop over the target horizontally */
		for (x = 0; x < dwidth; x++)
		{
			UINT32 startx = x * dx;
			UINT32 pix0, pix1, pix2, pix3;
			UINT32 sumr, sumg, sumb, suma;
			UINT32 nextx, nexty;
			UINT32 curx, cury;
			UINT32 factor;

			/* adjust start to the center; note that this math will tend to produce */
			/* negative results on the first pixel, which is why we clamp below */
			curx = startx + dx / 2 - 0x800;
			cury = starty + dy / 2 - 0x800;

			/* compute the neighboring pixel */
			nextx = curx + 0x1000;
			nexty = cury + 0x1000;

			/* clamp start */
			if ((INT32)curx < 0) curx += 0x1000;
			if ((INT32)cury < 0) cury += 0x1000;

			/* fetch the four relevant pixels */
			pix0 = source[(cury >> 12) * srowpixels + (curx >> 12)];
			pix1 = source[(cury >> 12) * srowpixels + (nextx >> 12)];
			pix2 = source[(nexty >> 12) * srowpixels + (curx >> 12)];
			pix3 = source[(nexty >> 12) * srowpixels + (nextx >> 12)];

			/* compute the x/y scaling factors */
			curx &= 0xfff;
			cury &= 0xfff;

			/* contributions from pixel 0 (top,left) */
			factor = (0x1000 - curx) * (0x1000 - cury);
			sumr = factor * RGB_RED(pix0);
			sumg = factor * RGB_GREEN(pix0);
			sumb = factor * RGB_BLUE(pix0);
			suma = factor * RGB_ALPHA(pix0);

			/* contributions from pixel 1 (top,right) */
			factor = curx * (0x1000 - cury);
			sumr += factor * RGB_RED(pix1);
			sumg += factor * RGB_GREEN(pix1);
			sumb += factor * RGB_BLUE(pix1);
			suma += factor * RGB_ALPHA(pix1);

			/* contributions from pixel 2 (bottom,left) */
			factor = (0x1000 - curx) * cury;
			sumr += factor * RGB_RED(pix2);
			sumg += factor * RGB_GREEN(pix2);
			sumb += factor * RGB_BLUE(pix2);
			suma += factor * RGB_ALPHA(pix2);

			/* contributions from pixel 3 (bottom,right) */
			factor = curx * cury;
			sumr += factor * RGB_RED(pix3);
			sumg += factor * RGB_GREEN(pix3);
			sumb += factor * RGB_BLUE(pix3);
			suma += factor * RGB_ALPHA(pix3);

			/* apply scaling */
			suma = (suma >> 24) * a / 256;
			sumr = (sumr >> 24) * r / 256;
			sumg = (sumg >> 24) * g / 256;
			sumb = (sumb >> 24) * b / 256;

			/* if we're translucent, add in the destination pixel contribution */
			if (a < 256)
			{
				UINT32 dpix = dest[y * drowpixels + x];
				suma += RGB_ALPHA(dpix) * (256 - a);
				sumr += RGB_RED(dpix) * (256 - a);
				sumg += RGB_GREEN(dpix) * (256 - a);
				sumb += RGB_BLUE(dpix) * (256 - a);
			}

			/* store the target pixel, dividing the RGBA values by the overall scale factor */
			dest[y * drowpixels + x] = MAKE_ARGB(suma, sumr, sumg, sumb);
		}
	}
}



/***************************************************************************

    Generic line clipper

***************************************************************************/

int render_clip_line(render_bounds *bounds, const render_bounds *clip)
{
	/* loop until we get a final result */
	while (1)
	{
		UINT8 code0 = 0, code1 = 0;
		UINT8 thiscode;
		float x, y;

		/* compute Cohen Sutherland bits for first coordinate */
		if (bounds->y0 > clip->y1)
			code0 |= 1;
		if (bounds->y0 < clip->y0)
			code0 |= 2;
		if (bounds->x0 > clip->x1)
			code0 |= 4;
		if (bounds->x0 < clip->x0)
			code0 |= 8;

		/* compute Cohen Sutherland bits for second coordinate */
		if (bounds->y1 > clip->y1)
			code1 |= 1;
		if (bounds->y1 < clip->y0)
			code1 |= 2;
		if (bounds->x1 > clip->x1)
			code1 |= 4;
		if (bounds->x1 < clip->x0)
			code1 |= 8;

		/* trivial accept: just return FALSE */
		if ((code0 | code1) == 0)
			return FALSE;

		/* trivial reject: just return TRUE */
		if ((code0 & code1) != 0)
			return TRUE;

		/* fix one of the OOB cases */
		thiscode = code0 ? code0 : code1;

		/* off the bottom */
		if (thiscode & 1)
		{
			x = bounds->x0 + (bounds->x1 - bounds->x0) * (clip->y1 - bounds->y0) / (bounds->y1 - bounds->y0);
			y = clip->y1;
		}

		/* off the top */
		else if (thiscode & 2)
		{
			x = bounds->x0 + (bounds->x1 - bounds->x0) * (clip->y0 - bounds->y0) / (bounds->y1 - bounds->y0);
			y = clip->y0;
		}

		/* off the right */
		else if (thiscode & 4)
		{
			y = bounds->y0 + (bounds->y1 - bounds->y0) * (clip->x1 - bounds->x0) / (bounds->x1 - bounds->x0);
			x = clip->x1;
		}

		/* off the left */
		else
		{
			y = bounds->y0 + (bounds->y1 - bounds->y0) * (clip->x0 - bounds->x0) / (bounds->x1 - bounds->x0);
			x = clip->x0;
		}

		/* fix the appropriate coordinate */
		if (thiscode == code0)
		{
			bounds->x0 = x;
			bounds->y0 = y;
		}
		else
		{
			bounds->x1 = x;
			bounds->y1 = y;
		}
	}
}



/***************************************************************************

    Generic quad clipper

***************************************************************************/

int render_clip_quad(render_bounds *bounds, const render_bounds *clip, float *u, float *v)
{
	/*
        note: this code assumes that u,v coordinates are in clockwise order
        starting from the top-left:

          u0,v0    u1,v1
            +--------+
            |        |
            |        |
            +--------+
          u2,v2    u3,v3
    */

	/* ensure our assumptions about the bounds are correct */
	assert(bounds->x0 <= bounds->x1);
	assert(bounds->y0 <= bounds->y1);

	/* trivial reject */
	if (bounds->y1 < clip->y0)
		return TRUE;
	if (bounds->y0 > clip->y1)
		return TRUE;
	if (bounds->x1 < clip->x0)
		return TRUE;
	if (bounds->x0 > clip->x1)
		return TRUE;

	/* clip top (x0,y0)-(x1,y1) */
	if (bounds->y0 < clip->y0)
	{
		float frac = (clip->y0 - bounds->y0) / (bounds->y1 - bounds->y0);
		bounds->y0 = clip->y0;
		if (u != NULL && v != NULL)
		{
			u[0] += (u[2] - u[0]) * frac;
			v[0] += (v[2] - v[0]) * frac;
			u[1] += (u[3] - u[1]) * frac;
			v[1] += (v[3] - v[1]) * frac;
		}
	}

	/* clip bottom (x3,y3)-(x2,y2) */
	if (bounds->y1 > clip->y1)
	{
		float frac = (bounds->y1 - clip->y1) / (bounds->y1 - bounds->y0);
		bounds->y1 = clip->y1;
		if (u != NULL && v != NULL)
		{
			u[2] -= (u[2] - u[0]) * frac;
			v[2] -= (v[2] - v[0]) * frac;
			u[3] -= (u[3] - u[1]) * frac;
			v[3] -= (v[3] - v[1]) * frac;
		}
	}

	/* clip left (x0,y0)-(x3,y3) */
	if (bounds->x0 < clip->x0)
	{
		float frac = (clip->x0 - bounds->x0) / (bounds->x1 - bounds->x0);
		bounds->x0 = clip->x0;
		if (u != NULL && v != NULL)
		{
			u[0] += (u[1] - u[0]) * frac;
			v[0] += (v[1] - v[0]) * frac;
			u[2] += (u[3] - u[2]) * frac;
			v[2] += (v[3] - v[2]) * frac;
		}
	}

	/* clip right (x1,y1)-(x2,y2) */
	if (bounds->x1 > clip->x1)
	{
		float frac = (bounds->x1 - clip->x1) / (bounds->x1 - bounds->x0);
		bounds->x1 = clip->x1;
		if (u != NULL && v != NULL)
		{
			u[1] -= (u[1] - u[0]) * frac;
			v[1] -= (v[1] - v[0]) * frac;
			u[3] -= (u[3] - u[2]) * frac;
			v[3] -= (v[3] - v[2]) * frac;
		}
	}
	return FALSE;
}



/***************************************************************************

    Layout views

***************************************************************************/

/*-------------------------------------------------
    layout_view_recompute - recompute the bounds
    and aspect ratio of a view and all of its
    contained items
-------------------------------------------------*/

static void layout_view_recompute(layout_view *view, int layerconfig)
{
	render_bounds target_bounds;
	float xscale, yscale;
	float xoffs, yoffs;
	int scrfirst = TRUE;
	int first = TRUE;
	int layer;

	/* reset the bounds */
	view->bounds.x0 = view->bounds.y0 = view->bounds.x1 = view->bounds.y1 = 0.0f;
	view->scrbounds.x0 = view->scrbounds.y0 = view->scrbounds.x1 = view->scrbounds.y1 = 0.0f;
	view->screens = 0;

	/* loop over all layers */
	for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
	{
		static const int layer_mask[ITEM_LAYER_MAX] = { LAYER_CONFIG_ENABLE_BACKDROP, 0, LAYER_CONFIG_ENABLE_OVERLAY, LAYER_CONFIG_ENABLE_BEZEL };

		/* determine if this layer should be visible */
		view->layenabled[layer] = (layer_mask[layer] == 0 || (layerconfig & layer_mask[layer]));

		/* only do it if requested */
		if (view->layenabled[layer])
		{
			view_item *item;

			for (item = view->itemlist[layer]; item != NULL; item = item->next)
			{
				/* accumulate bounds */
				if (first)
					view->bounds = item->rawbounds;
				else
					union_bounds(&view->bounds, &item->rawbounds);
				first = FALSE;

				/* accumulate screen bounds */
				if (item->element == NULL)
				{
					if (scrfirst)
						view->scrbounds = item->rawbounds;
					else
						union_bounds(&view->scrbounds, &item->rawbounds);
					scrfirst = FALSE;
				}
			}
		}
	}

	/* if we have an explicit bounds, override it */
	if (view->expbounds.x1 > view->expbounds.x0)
		view->bounds = view->expbounds;

	/* compute the aspect ratio of the view */
	view->aspect = (view->bounds.x1 - view->bounds.x0) / (view->bounds.y1 - view->bounds.y0);
	view->scraspect = (view->scrbounds.x1 - view->scrbounds.x0) / (view->scrbounds.y1 - view->scrbounds.y0);

	/* if we're handling things normally, the target bounds are (0,0)-(1,1) */
	if (!(layerconfig & LAYER_CONFIG_ZOOM_TO_SCREEN))
	{
		target_bounds.x0 = target_bounds.y0 = 0.0f;
		target_bounds.x1 = target_bounds.y1 = 1.0f;
	}

	/* if we're cropping, we want the screen area to fill (0,0)-(1,1) */
	else
	{
		float targwidth = (view->bounds.x1 - view->bounds.x0) / (view->scrbounds.x1 - view->scrbounds.x0);
		float targheight = (view->bounds.y1 - view->bounds.y0) / (view->scrbounds.y1 - view->scrbounds.y0);
		target_bounds.x0 = (view->bounds.x0 - view->scrbounds.x0) / (view->bounds.x1 - view->bounds.x0) * targwidth;
		target_bounds.y0 = (view->bounds.y0 - view->scrbounds.y0) / (view->bounds.y1 - view->bounds.y0) * targheight;
		target_bounds.x1 = target_bounds.x0 + targwidth;
		target_bounds.y1 = target_bounds.y0 + targheight;
	}

	/* determine the scale/offset for normalization */
	xoffs = view->bounds.x0;
	yoffs = view->bounds.y0;
	xscale = (target_bounds.x1 - target_bounds.x0) / (view->bounds.x1 - view->bounds.x0);
	yscale = (target_bounds.y1 - target_bounds.y0) / (view->bounds.y1 - view->bounds.y0);

	/* normalize all the item bounds */
	for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
	{
		view_item *item;

		/* adjust the bounds for each item */
		for (item = view->itemlist[layer]; item; item = item->next)
		{
			item->bounds.x0 = target_bounds.x0 + (item->rawbounds.x0 - xoffs) * xscale;
			item->bounds.x1 = target_bounds.x0 + (item->rawbounds.x1 - xoffs) * xscale;
			item->bounds.y0 = target_bounds.y0 + (item->rawbounds.y0 - yoffs) * yscale;
			item->bounds.y1 = target_bounds.y0 + (item->rawbounds.y1 - yoffs) * yscale;

			/* accumulate the screens in use while we're scanning */
			if (item->element == NULL)
				view->screens |= 1 << item->index;
		}
	}
}



/***************************************************************************

    Layout elements

***************************************************************************/

/*-------------------------------------------------
    layout_element_scale - scale an element by
    rendering all the components at the
    appropriate resolution
-------------------------------------------------*/

static void layout_element_scale(mame_bitmap *dest, const mame_bitmap *source, const rectangle *sbounds, void *param)
{
	element_texture *elemtex = param;
	element_component *component;

	/* iterate over components that are part of the current state */
	for (component = elemtex->element->complist; component != NULL; component = component->next)
		if (component->state == -1 || component->state == elemtex->state)
		{
			rectangle bounds;

			/* get the local scaled bounds */
			bounds.min_x = round_nearest(component->bounds.x0 * dest->width);
			bounds.min_y = round_nearest(component->bounds.y0 * dest->height);
			bounds.max_x = round_nearest(component->bounds.x1 * dest->width);
			bounds.max_y = round_nearest(component->bounds.y1 * dest->height);

			/* based on the component type, add to the texture */
			switch (component->type)
			{
				case COMPONENT_TYPE_IMAGE:
					render_resample_argb_bitmap_hq(
							(UINT32 *)dest->base + bounds.min_y * dest->rowpixels + bounds.min_x,
							dest->rowpixels,
							bounds.max_x - bounds.min_x,
							bounds.max_y - bounds.min_y,
							component->bitmap, NULL, &component->color);
					break;

				case COMPONENT_TYPE_RECT:
					layout_element_draw_rect(dest, &bounds, &component->color);
					break;

				case COMPONENT_TYPE_DISK:
					layout_element_draw_disk(dest, &bounds, &component->color);
					break;
			}
		}
}


/*-------------------------------------------------
    layout_element_draw_rect - draw a rectangle
    in the specified color
-------------------------------------------------*/

static void layout_element_draw_rect(mame_bitmap *dest, const rectangle *bounds, const render_color *color)
{
	UINT32 r, g, b, inva;
	UINT32 x, y;

	/* compute premultiplied colors */
	r = color->r * color->a * 255.0;
	g = color->g * color->a * 255.0;
	b = color->b * color->a * 255.0;
	inva = (1.0f - color->a) * 255.0;

	/* iterate over X and Y */
	for (y = bounds->min_y; y < bounds->max_y; y++)
		for (x = bounds->min_x; x < bounds->max_x; x++)
		{
			UINT32 finalr = r;
			UINT32 finalg = g;
			UINT32 finalb = b;

			/* if we're translucent, add in the destination pixel contribution */
			if (inva > 0)
			{
				UINT32 dpix = *((UINT32 *)dest->base + y * dest->rowpixels + x);
				finalr += (RGB_RED(dpix) * inva) >> 8;
				finalg += (RGB_GREEN(dpix) * inva) >> 8;
				finalb += (RGB_BLUE(dpix) * inva) >> 8;
			}

			/* store the target pixel, dividing the RGBA values by the overall scale factor */
			*((UINT32 *)dest->base + y * dest->rowpixels + x) = MAKE_ARGB(0xff, finalr, finalg, finalb);
		}
}


/*-------------------------------------------------
    layout_element_draw_disk - draw an ellipse
    in the specified color
-------------------------------------------------*/

static void layout_element_draw_disk(mame_bitmap *dest, const rectangle *bounds, const render_color *color)
{
	float xcenter, ycenter;
	float xradius, yradius, ooyradius2;
	UINT32 r, g, b, inva;
	UINT32 x, y;

	/* compute premultiplied colors */
	r = color->r * color->a * 255.0;
	g = color->g * color->a * 255.0;
	b = color->b * color->a * 255.0;
	inva = (1.0f - color->a) * 255.0;

	/* find the center */
	xcenter = (float)(bounds->min_x + bounds->max_x) * 0.5f;
	ycenter = (float)(bounds->min_y + bounds->max_y) * 0.5f;
	xradius = (float)(bounds->max_x - bounds->min_x) * 0.5f;
	yradius = (float)(bounds->max_y - bounds->min_y) * 0.5f;
	ooyradius2 = 1.0f / (yradius * yradius);

	/* iterate over y */
	for (y = bounds->min_y; y < bounds->max_y; y++)
	{
		float ycoord = ycenter - ((float)y + 0.5f);
		float xval = xradius * sqrt(1.0f - (ycoord * ycoord) * ooyradius2);
		INT32 left, right;

		/* compute left/right coordinates */
		left = (INT32)(xcenter - xval + 0.5f);
		right = (INT32)(xcenter + xval + 0.5f);

		/* draw this scanline */
		for (x = left; x < right; x++)
		{
			UINT32 finalr = r;
			UINT32 finalg = g;
			UINT32 finalb = b;

			/* if we're translucent, add in the destination pixel contribution */
			if (inva > 0)
			{
				UINT32 dpix = *((UINT32 *)dest->base + y * dest->rowpixels + x);
				finalr += (RGB_RED(dpix) * inva) >> 8;
				finalg += (RGB_GREEN(dpix) * inva) >> 8;
				finalb += (RGB_BLUE(dpix) * inva) >> 8;
			}

			/* store the target pixel, dividing the RGBA values by the overall scale factor */
			*((UINT32 *)dest->base + y * dest->rowpixels + x) = MAKE_ARGB(0xff, finalr, finalg, finalb);
		}
	}
}



/***************************************************************************

    Layout file parsing

***************************************************************************/

/*-------------------------------------------------
    get_variable_value - compute the value of
    a variable in an XML attribute
-------------------------------------------------*/

static int get_variable_value(const char *string, char **outputptr)
{
	int num, den, scrnum;
	char temp[100];

	/* screen 0 parameters */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
	{
		/* native X aspect factor */
		sprintf(temp, "~scr%dnativexaspect~", scrnum);
		if (!strncmp(string, temp, strlen(temp)))
		{
			num = Machine->drv->screen[scrnum].default_visible_area.max_x + 1 - Machine->drv->screen[scrnum].default_visible_area.min_x;
			den = Machine->drv->screen[scrnum].default_visible_area.max_y + 1 - Machine->drv->screen[scrnum].default_visible_area.min_y;
			reduce_fraction(&num, &den);
			*outputptr += sprintf(*outputptr, "%d", num);
			return strlen(temp);
		}

		/* native Y aspect factor */
		sprintf(temp, "~scr%dnativeyaspect~", scrnum);
		if (!strncmp(string, temp, strlen(temp)))
		{
			num = Machine->drv->screen[scrnum].default_visible_area.max_x + 1 - Machine->drv->screen[scrnum].default_visible_area.min_x;
			den = Machine->drv->screen[scrnum].default_visible_area.max_y + 1 - Machine->drv->screen[scrnum].default_visible_area.min_y;
			reduce_fraction(&num, &den);
			*outputptr += sprintf(*outputptr, "%d", den);
			return strlen(temp);
		}

		/* native width */
		sprintf(temp, "~scr%dwidth~", scrnum);
		if (!strncmp(string, temp, strlen(temp)))
		{
			*outputptr += sprintf(*outputptr, "%d", Machine->drv->screen[scrnum].default_visible_area.max_x + 1 - Machine->drv->screen[0].default_visible_area.min_x);
			return strlen(temp);
		}

		/* native height */
		sprintf(temp, "~scr%dheight~", scrnum);
		if (!strncmp(string, temp, strlen(temp)))
		{
			*outputptr += sprintf(*outputptr, "%d", Machine->drv->screen[scrnum].default_visible_area.max_y + 1 - Machine->drv->screen[0].default_visible_area.min_y);
			return strlen(temp);
		}
	}

	/* default: copy the first character and continue */
	**outputptr = *string;
	*outputptr += 1;
	return 1;
}


/*-------------------------------------------------
    xml_get_attribute_string_with_subst - analog
    to xml_get_attribute_string but with variable
    substitution
-------------------------------------------------*/

static const char *xml_get_attribute_string_with_subst(xml_data_node *node, const char *attribute, const char *defvalue)
{
	const char *str = xml_get_attribute_string(node, attribute, NULL);
	static char buffer[1000];
	const char *s;
	char *d;

	/* if nothing, just return the default */
	if (str == NULL)
		return defvalue;

	/* if no tildes, don't worry */
	if (strchr(str, '~') == NULL)
		return str;

	/* make a copy of the string, doing substitutions along the way */
	for (s = str, d = buffer; *s != 0; )
	{
		/* if not a variable, just copy */
		if (*s != '~')
			*d++ = *s++;

		/* extract the variable */
		else
			s += get_variable_value(s, &d);
	}
	*d = 0;
	return buffer;
}


/*-------------------------------------------------
    xml_get_attribute_int_with_subst - analog
    to xml_get_attribute_int but with variable
    substitution
-------------------------------------------------*/

static int xml_get_attribute_int_with_subst(xml_data_node *node, const char *attribute, int defvalue)
{
	const char *string = xml_get_attribute_string_with_subst(node, attribute, NULL);
	int value;

	if (!string || sscanf(string, "%d", &value) != 1)
		return defvalue;
	return value;
}


/*-------------------------------------------------
    xml_get_attribute_float_with_subst - analog
    to xml_get_attribute_float but with variable
    substitution
-------------------------------------------------*/

static float xml_get_attribute_float_with_subst(xml_data_node *node, const char *attribute, float defvalue)
{
	const char *string = xml_get_attribute_string_with_subst(node, attribute, NULL);
	float value;

	if (!string || sscanf(string, "%f", &value) != 1)
		return defvalue;
	return value;
}


/*-------------------------------------------------
    load_layout_file - parse a layout XML file
    into a layout_file
-------------------------------------------------*/

static layout_file *load_layout_file(const char *dirname, const char *filename)
{
	xml_data_node *rootnode, *mamelayoutnode, *elemnode, *viewnode;
	layout_element **elemnext;
	layout_view **viewnext;
	layout_file *file;
	int version;

	/* if the first character of the "file" is an open brace, assume it is an XML string */
	if (filename[0] == '<')
		rootnode = xml_string_read(filename, NULL);

	/* otherwise, assume it is a file */
	else
	{
		mame_file *layoutfile = mame_fopen(dirname, filename, FILETYPE_ARTWORK, 0);
		if (layoutfile == NULL)
			return NULL;
		rootnode = xml_file_read(layoutfile, NULL);
		mame_fclose(layoutfile);
	}

	/* if unable to parse the file, just bail */
	if (rootnode == NULL)
		return NULL;

	/* allocate the layout group object first */
	file = malloc_or_die(sizeof(*file));
	memset(file, 0, sizeof(*file));

	/* find the layout node */
	mamelayoutnode = xml_get_sibling(rootnode->child, "mamelayout");
	if (mamelayoutnode == NULL)
		goto error;

	/* validate the config data version */
	version = xml_get_attribute_int(mamelayoutnode, "version", 0);
	if (version != LAYOUT_VERSION)
		goto error;

	/* parse all the elements */
	file->elemlist = NULL;
	elemnext = &file->elemlist;
	for (elemnode = xml_get_sibling(mamelayoutnode->child, "element"); elemnode; elemnode = xml_get_sibling(elemnode->next, "element"))
	{
		layout_element *element = load_layout_element(elemnode, dirname);
		if (element == NULL)
			goto error;

		/* add to the end of the list */
		*elemnext = element;
		elemnext = &element->next;
	}

	/* parse all the views */
	file->viewlist = NULL;
	viewnext = &file->viewlist;
	for (viewnode = xml_get_sibling(mamelayoutnode->child, "view"); viewnode; viewnode = xml_get_sibling(viewnode->next, "view"))
	{
		layout_view *view = load_layout_view(viewnode, file->elemlist);
		if (view == NULL)
			goto error;

		/* add to the end of the list */
		*viewnext = view;
		viewnext = &view->next;
	}
	xml_file_free(rootnode);
	return file;

error:
	layout_file_free(file);
	xml_file_free(rootnode);
	return NULL;
}


/*-------------------------------------------------
    load_layout_element - parse an element XML
    node from the layout file
-------------------------------------------------*/

static layout_element *load_layout_element(xml_data_node *elemnode, const char *dirname)
{
	render_bounds bounds = { 0 };
	element_component **nextcomp;
	element_component *component;
	xml_data_node *compnode;
	layout_element *element;
	float xscale, yscale;
	float xoffs, yoffs;
	const char *name;
	int state;
	int first;

	/* allocate a new element */
	element = malloc_or_die(sizeof(*element));
	memset(element, 0, sizeof(*element));

	/* extract the name */
	name = xml_get_attribute_string_with_subst(elemnode, "name", NULL);
	if (name == NULL)
	{
		logerror("All layout elements must have a name!\n");
		goto error;
	}
	element->name = copy_string(name);
	element->defstate = xml_get_attribute_int_with_subst(elemnode, "defstate", -1);

	/* parse components in order */
	first = TRUE;
	nextcomp = &element->complist;
	for (compnode = elemnode->child; compnode; compnode = compnode->next)
	{
		/* allocate a new component */
		element_component *component = load_element_component(compnode, dirname);
		if (component == NULL)
			goto error;

		/* link it into the list */
		*nextcomp = component;
		nextcomp = &component->next;

		/* accumulate bounds */
		if (first)
			bounds = component->bounds;
		else
			union_bounds(&bounds, &component->bounds);
		first = FALSE;

		/* determine the maximum state */
		if (component->state > element->maxstate)
			element->maxstate = component->state;
	}

	/* determine the scale/offset for normalization */
	xoffs = bounds.x0;
	yoffs = bounds.y0;
	xscale = 1.0f / (bounds.x1 - bounds.x0);
	yscale = 1.0f / (bounds.y1 - bounds.y0);

	/* normalize all the component bounds */
	for (component = element->complist; component != NULL; component = component->next)
	{
		component->bounds.x0 = (component->bounds.x0 - xoffs) * xscale;
		component->bounds.x1 = (component->bounds.x1 - xoffs) * xscale;
		component->bounds.y0 = (component->bounds.y0 - yoffs) * yscale;
		component->bounds.y1 = (component->bounds.y1 - yoffs) * yscale;
	}

	/* allocate an array of textures for the states */
	element->elemtex = malloc_or_die((element->maxstate + 1) * sizeof(element->elemtex[0]));
	for (state = 0; state <= element->maxstate; state++)
	{
		element_component *component;

		element->elemtex[state].element = element;
		element->elemtex[state].state = state;

		/* look for at least one visible component in this state */
		for (component = element->complist; component != NULL; component = component->next)
			if (component->state == -1 || component->state == state)
				break;

		/* allocate a texture only if we have some visible components in this state */
		if (component != NULL)
			element->elemtex[state].texture = render_texture_alloc(NULL, NULL, NULL, TEXFORMAT_ARGB32, layout_element_scale, &element->elemtex[state]);
		else
			element->elemtex[state].texture = NULL;
	}
	return element;

error:
	layout_element_free(element);
	return NULL;
}


/*-------------------------------------------------
    load_element_component - parse a component
    XML node (image/rect/disk)
-------------------------------------------------*/

static element_component *load_element_component(xml_data_node *compnode, const char *dirname)
{
	element_component *component;

	/* allocate memory for the component */
	component = malloc_or_die(sizeof(*component));
	memset(component, 0, sizeof(*component));

	/* fetch common data */
	component->state = xml_get_attribute_int_with_subst(compnode, "state", 0);
	if (load_bounds(xml_get_sibling(compnode->child, "bounds"), &component->bounds))
		goto error;
	if (load_color(xml_get_sibling(compnode->child, "color"), &component->color))
		goto error;

	/* image nodes */
	if (strcmp(compnode->name, "image") == 0)
	{
		const char *file = xml_get_attribute_string_with_subst(compnode, "file", NULL);
		const char *afile = xml_get_attribute_string_with_subst(compnode, "alphafile", NULL);

		/* load and allocate the bitmap */
		component->type = COMPONENT_TYPE_IMAGE;
		component->bitmap = load_component_bitmap(dirname, file, afile, &component->hasalpha);
		if (component->bitmap == NULL)
			goto error;
	}

	/* rect nodes */
	else if (strcmp(compnode->name, "rect") == 0)
		component->type = COMPONENT_TYPE_RECT;

	/* disk nodes */
	else if (strcmp(compnode->name, "disk") == 0)
		component->type = COMPONENT_TYPE_DISK;

	/* error otherwise */
	else
	{
		logerror("Unknown element component: %s\n", compnode->name);
		goto error;
	}

	return component;

error:
	free(component);
	return NULL;
}


/*-------------------------------------------------
    load_layout_view - parse a view XML node
-------------------------------------------------*/

static layout_view *load_layout_view(xml_data_node *viewnode, layout_element *elemlist)
{
	xml_data_node *boundsnode;
	view_item **itemnext;
	layout_view *view;
	int layer;

	/* first allocate memory */
	view = malloc_or_die(sizeof(*view));
	memset(view, 0, sizeof(*view));

	/* allocate a copy of the name */
	view->name = copy_string(xml_get_attribute_string_with_subst(viewnode, "name", ""));

	/* if we have a bounds item, load it */
	boundsnode = xml_get_sibling(viewnode->child, "bounds");
	if (boundsnode != NULL && load_bounds(xml_get_sibling(boundsnode, "bounds"), &view->expbounds))
		goto error;

	/* loop over all the layer types we support */
	for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
	{
		static const char *layer_node_name[ITEM_LAYER_MAX] = { "backdrop", "screen", "overlay", "bezel" };
		xml_data_node *itemnode;

		/* initialize the list */
		view->itemlist[layer] = NULL;
		itemnext = &view->itemlist[layer];

		/* parse all of the elements of that type */
		for (itemnode = xml_get_sibling(viewnode->child, layer_node_name[layer]); itemnode; itemnode = xml_get_sibling(itemnode->next, layer_node_name[layer]))
		{
			view_item *item = load_view_item(itemnode, elemlist);
			if (!item)
				goto error;

			/* add to the end of the list */
			*itemnext = item;
			itemnext = &item->next;
		}
	}

	/* recompute the data for the view */
	layout_view_recompute(view, ~0);
	return view;

error:
	layout_view_free(view);
	return NULL;
}


/*-------------------------------------------------
    load_view_item - parse an item XML node
-------------------------------------------------*/

static view_item *load_view_item(xml_data_node *itemnode, layout_element *elemlist)
{
	view_item *item;
	const char *name;

	/* allocate a new item */
	item = malloc_or_die(sizeof(*item));
	memset(item, 0, sizeof(*item));

	/* allocate a copy of the name */
	item->name = copy_string(xml_get_attribute_string_with_subst(itemnode, "name", ""));

	/* find the associated element */
	name = xml_get_attribute_string_with_subst(itemnode, "element", NULL);
	if (name != NULL)
	{
		layout_element *element;

		/* search the list of elements for a match */
		for (element = elemlist; element; element = element->next)
			if (strcmp(name, element->name) == 0)
				break;

		/* error if not found */
		if (element == NULL)
		{
			logerror("Unable to find element %s\n", name);
			goto error;
		}
		item->element = element;
	}

	/* fetch common data */
	item->index = xml_get_attribute_int_with_subst(itemnode, "index", -1);
	item->state = render_view_item_get_state_ptr(item->name);
	item->state->curstate = (item->name[0] != 0 && item->element != NULL) ? item->element->defstate : 0;
	if (load_bounds(xml_get_sibling(itemnode->child, "bounds"), &item->rawbounds))
		goto error;
	if (load_color(xml_get_sibling(itemnode->child, "color"), &item->color))
		goto error;
	if (load_orientation(xml_get_sibling(itemnode->child, "orientation"), &item->orientation))
		goto error;

	/* sanity checks */
	if (strcmp(itemnode->name, "screen") == 0)
	{
		if (item->index >= MAX_SCREENS)
		{
			logerror("Layout references invalid screen index %d\n", item->index);
			goto error;
		}
	}
	else
	{
		if (item->element == NULL)
		{
			logerror("Layout item of type %s require an element tag\n", itemnode->name);
			goto error;
		}
	}

	return item;

error:
	if (item->name != NULL)
		free((void *)item->name);
	free(item);
	return NULL;
}


/*-------------------------------------------------
    load_component_bitmap - load a PNG file
    with artwork for a component
-------------------------------------------------*/

static mame_bitmap *load_component_bitmap(const char *dirname, const char *file, const char *alphafile, int *hasalpha)
{
	mame_bitmap *bitmap;
	png_info png;
	UINT8 *src;
	int x, y;

	/* assume no alpha by default */
	*hasalpha = FALSE;

	/* open and read the main png file */
	if (open_and_read_png(dirname, file, &png))
	{
		logerror("Can't load PNG file: %s\n", file);
		return NULL;
	}

	/* allocate the rawbitmap and erase it */
	bitmap = bitmap_alloc_depth(png.width, png.height, 32);
	if (bitmap == NULL)
		return NULL;
	fillbitmap(bitmap, 0, NULL);

	/* handle 8bpp palettized case */
	if (png.color_type == 3)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src++)
			{
				/* determine alpha and expand to 32bpp */
				UINT8 alpha = (*src < png.num_trans) ? png.trans[*src] : 0xff;
				if (alpha < 0xff)
					*hasalpha = TRUE;
				plot_pixel(bitmap, x, y, MAKE_ARGB(alpha, png.palette[*src * 3], png.palette[*src * 3 + 1], png.palette[*src * 3 + 2]));
			}

		/* free memory for the palette */
		free(png.palette);
	}

	/* handle 8bpp grayscale case */
	else if (png.color_type == 0)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src++)
				plot_pixel(bitmap, x, y, MAKE_ARGB(0xff, *src, *src, *src));
	}

	/* handle 32bpp non-alpha case */
	else if (png.color_type == 2)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src += 3)
				plot_pixel(bitmap, x, y, MAKE_ARGB(0xff, src[0], src[1], src[2]));
	}

	/* handle 32bpp alpha case */
	else
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src += 4)
			{
				if (src[3] < 0xff)
					*hasalpha = TRUE;
				plot_pixel(bitmap, x, y, MAKE_ARGB(src[3], src[0], src[1], src[2]));
			}
	}

	/* free the raw image data and return after loading any alpha map */
	free(png.image);

	/* now load the alpha bitmap if present */
	if (alphafile != NULL)
	{
		if (load_alpha_bitmap(dirname, alphafile, bitmap, &png))
		{
			bitmap_free(bitmap);
			bitmap = NULL;
		}
		*hasalpha = TRUE;
	}
	return bitmap;
}


/*-------------------------------------------------
    load_alpha_bitmap - load the external alpha
    mask
-------------------------------------------------*/

static int load_alpha_bitmap(const char *dirname, const char *alphafile, mame_bitmap *bitmap, const png_info *original)
{
	png_info png;
	UINT8 *src;
	int x, y;

	/* open and read the alpha png file */
	if (open_and_read_png(dirname, alphafile, &png))
	{
		logerror("Can't load PNG file: %s\n", alphafile);
		return 1;
	}

	/* must be the same size */
	if (png.height != original->height || png.width != original->width)
	{
		logerror("Alpha PNG must match original's dimensions: %s\n", alphafile);
		return 1;
	}

	/* handle 8bpp palettized case */
	if (png.color_type == 3)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src++)
			{
				rgb_t pixel = read_pixel(bitmap, x, y);
				UINT8 alpha = compute_brightness(MAKE_RGB(png.palette[*src * 3], png.palette[*src * 3 + 1], png.palette[*src * 3 + 2]));
				plot_pixel(bitmap, x, y, MAKE_ARGB(alpha, RGB_RED(pixel), RGB_GREEN(pixel), RGB_BLUE(pixel)));
			}

		/* free memory for the palette */
		free(png.palette);
	}

	/* handle 8bpp grayscale case */
	else if (png.color_type == 0)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src++)
			{
				rgb_t pixel = read_pixel(bitmap, x, y);
				plot_pixel(bitmap, x, y, MAKE_ARGB(*src, RGB_RED(pixel), RGB_GREEN(pixel), RGB_BLUE(pixel)));
			}
	}

	/* handle 32bpp non-alpha case */
	else if (png.color_type == 2)
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src += 3)
			{
				rgb_t pixel = read_pixel(bitmap, x, y);
				UINT8 alpha = compute_brightness(MAKE_RGB(src[0], src[1], src[2]));
				plot_pixel(bitmap, x, y, MAKE_ARGB(alpha, RGB_RED(pixel), RGB_GREEN(pixel), RGB_BLUE(pixel)));
			}
	}

	/* handle 32bpp alpha case */
	else
	{
		/* loop over width/height */
		src = png.image;
		for (y = 0; y < png.height; y++)
			for (x = 0; x < png.width; x++, src += 4)
			{
				rgb_t pixel = read_pixel(bitmap, x, y);
				UINT8 alpha = compute_brightness(MAKE_RGB(src[0], src[1], src[2]));
				plot_pixel(bitmap, x, y, MAKE_ARGB(alpha, RGB_RED(pixel), RGB_GREEN(pixel), RGB_BLUE(pixel)));
			}
	}

	/* free the raw image data */
	free(png.image);
	return 0;
}


/*-------------------------------------------------
    load_bounds - parse a bounds XML node
-------------------------------------------------*/

static int load_bounds(xml_data_node *boundsnode, render_bounds *bounds)
{
	/* skip if nothing */
	if (boundsnode == NULL)
	{
		bounds->x0 = bounds->y0 = 0.0f;
		bounds->x1 = bounds->y1 = 1.0f;
		return 0;
	}

	/* parse out the data */
	if (xml_get_attribute(boundsnode, "left") != NULL)
	{
		/* left/right/top/bottom format */
		bounds->x0 = xml_get_attribute_float_with_subst(boundsnode, "left", 0.0);
		bounds->x1 = xml_get_attribute_float_with_subst(boundsnode, "right", 1.0);
		bounds->y0 = xml_get_attribute_float_with_subst(boundsnode, "top", 0.0);
		bounds->y1 = xml_get_attribute_float_with_subst(boundsnode, "bottom", 1.0);
	}
	else if (xml_get_attribute(boundsnode, "x") != NULL)
	{
		/* x/y/width/height format */
		bounds->x0 = xml_get_attribute_float_with_subst(boundsnode, "x", 0.0);
		bounds->x1 = bounds->x0 + xml_get_attribute_float_with_subst(boundsnode, "width", 1.0);
		bounds->y0 = xml_get_attribute_float_with_subst(boundsnode, "y", 0.0);
		bounds->y1 = bounds->y0 + xml_get_attribute_float_with_subst(boundsnode, "height", 1.0);
	}
	else
		return 1;

	/* check for errors */
	if (bounds->x0 > bounds->x1 || bounds->y0 > bounds->y1)
	{
		logerror("Illegal bounds value: (%f-%f)-(%f-%f)\n", bounds->x0, bounds->x1, bounds->y0, bounds->y1);
		return 1;
	}
	return 0;
}


/*-------------------------------------------------
    load_color - parse a color XML node
-------------------------------------------------*/

static int load_color(xml_data_node *colornode, render_color *color)
{
	/* skip if nothing */
	if (colornode == NULL)
	{
		color->r = color->g = color->b = color->a = 1.0f;
		return 0;
	}

	/* parse out the data */
	color->r = xml_get_attribute_float_with_subst(colornode, "red", 1.0);
	color->g = xml_get_attribute_float_with_subst(colornode, "green", 1.0);
	color->b = xml_get_attribute_float_with_subst(colornode, "blue", 1.0);
	color->a = xml_get_attribute_float_with_subst(colornode, "alpha", 1.0);

	/* check for errors */
	if (color->r < 0.0 || color->r > 1.0 || color->g < 0.0 || color->g > 1.0 ||
		color->b < 0.0 || color->b > 1.0 || color->a < 0.0 || color->a > 1.0)
	{
		logerror("Illegal ARGB color value: %f,%f,%f,%f\n", color->r, color->g, color->b, color->a);
		return 1;
	}
	return 0;
}


/*-------------------------------------------------
    load_orientation - parse an orientation XML
    node
-------------------------------------------------*/

static int load_orientation(xml_data_node *orientnode, int *orientation)
{
	int rotate;

	/* skip if nothing */
	if (orientnode == NULL)
	{
		*orientation = ROT0;
		return 0;
	}

	/* parse out the data */
	rotate = xml_get_attribute_int_with_subst(orientnode, "rotate", 0);
	switch (rotate)
	{
		case 0:		*orientation = ROT0;	break;
		case 90:	*orientation = ROT90;	break;
		case 180:	*orientation = ROT180;	break;
		case 270:	*orientation = ROT270;	break;
		default:
			logerror("Invalid rotation in orientation node: %d\n", rotate);
			return 1;
	}
	if (strcmp("yes", xml_get_attribute_string_with_subst(orientnode, "swapxy", "no")) == 0)
		*orientation ^= ORIENTATION_SWAP_XY;
	if (strcmp("yes", xml_get_attribute_string_with_subst(orientnode, "flipx", "no")) == 0)
		*orientation ^= ORIENTATION_FLIP_X;
	if (strcmp("yes", xml_get_attribute_string_with_subst(orientnode, "flipy", "no")) == 0)
		*orientation ^= ORIENTATION_FLIP_Y;
	return 0;
}


/*-------------------------------------------------
    open_and_read_png - open a PNG file, read it
    in, and verify that we can do something with
    it
-------------------------------------------------*/

static int open_and_read_png(const char *dirname, const char *filename, png_info *png)
{
	mame_file *file;
	int result;

	/* open the file */
	file = mame_fopen(dirname, filename, FILETYPE_ARTWORK, 0);
	if (file == NULL)
		return 1;

	/* read the PNG data */
	result = png_read_file(file, png);
	mame_fclose(file);
	if (result == 0)
		return 1;

	/* verify we can handle this PNG */
	if (png->bit_depth > 8)
	{
		logerror("%s: Unsupported bit depth %d (8 bit max)\n", filename, png->bit_depth);
		return 1;
	}
	if (png->interlace_method != 0)
	{
		logerror("%s: Interlace unsupported\n", filename);
		return 1;
	}
	if (png->color_type != 0 && png->color_type != 3 && png->color_type != 2 && png->color_type != 6)
	{
		logerror("%s: Unsupported color type %d\n", filename, png->color_type);
		return 1;
	}

	/* if less than 8 bits, upsample */
	png_expand_buffer_8bit(png);
	return 0;
}


/*-------------------------------------------------
    layout_file_free - free memory for a
    layout_file and all of its subelements
-------------------------------------------------*/

static void layout_file_free(layout_file *file)
{
	/* free each element in the list */
	while (file->elemlist != NULL)
	{
		layout_element *temp = file->elemlist;
		file->elemlist = temp->next;
		layout_element_free(temp);
	}

	/* free each layout */
	while (file->viewlist != NULL)
	{
		layout_view *temp = file->viewlist;
		file->viewlist = temp->next;
		layout_view_free(temp);
	}

	/* free the file itself */
	free(file);
}


/*-------------------------------------------------
    layout_view_free - free memory for a
    layout_view and all of its subelements
-------------------------------------------------*/

static void layout_view_free(layout_view *view)
{
	int layer;

	/* for each layer, free each item in that layer */
	for (layer = 0; layer < ITEM_LAYER_MAX; layer++)
		while (view->itemlist[layer] != NULL)
		{
			view_item *temp = view->itemlist[layer];
			view->itemlist[layer] = temp->next;
			if (temp->name != NULL)
				free((void *)temp->name);
			free(temp);
		}

	/* free the view itself */
	if (view->name != NULL)
		free((void *)view->name);
	free(view);
}


/*-------------------------------------------------
    layout_element_free - free memory for a
    layout_element and its components
-------------------------------------------------*/

static void layout_element_free(layout_element *element)
{
	/* free all allocated components */
	while (element->complist != NULL)
	{
		element_component *temp = element->complist;
		element->complist = temp->next;
		if (temp->bitmap != NULL)
			bitmap_free(temp->bitmap);
		free(temp);
	}

	/* free all textures */
	if (element->elemtex != NULL)
	{
		int state;

		/* loop over all states and free their textures */
		for (state = 0; state <= element->maxstate; state++)
			if (element->elemtex[state].texture != NULL)
				render_texture_free(element->elemtex[state].texture);
		free(element->elemtex);
	}

	/* free the element itself */
	if (element->name != NULL)
		free((void *)element->name);
	free(element);
}
