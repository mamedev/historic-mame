#define __INLINE__ static __inline__	/* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include "TwkUser.c"
#include <math.h>
#include "vgafreq.h"
#include "vidhrdw/vector.h"

BEGIN_GFX_DRIVER_LIST
	GFX_DRIVER_VGA
	GFX_DRIVER_VESA3
	GFX_DRIVER_VESA2L
	GFX_DRIVER_VESA2B
	GFX_DRIVER_VESA1
END_GFX_DRIVER_LIST

BEGIN_COLOR_DEPTH_LIST
	COLOR_DEPTH_8
	COLOR_DEPTH_15
	COLOR_DEPTH_16
END_COLOR_DEPTH_LIST

#define BACKGROUND 0

#define MAX_GFX_WIDTH 1600
#define MAX_GFX_HEIGHT 1600
#define DIRTY_H 256 			/* 160 would be enough, but * 256 is faster */
#define DIRTY_V (MAX_GFX_HEIGHT/16)


typedef char dirtygrid[DIRTY_V * DIRTY_H];
#define ISDIRTY(x,y) (dirty_new[(y)/16 * DIRTY_H + (x)/16] || dirty_old[(y)/16 * DIRTY_H + (x)/16])
#define MARKDIRTY(x,y) dirty_new[(y)/16 * DIRTY_H + (x)/16] = 1

dirtygrid grid1;
dirtygrid grid2;
char *dirty_old=grid1;
char *dirty_new=grid2;

void scale_vectorgames(int gfx_width,int gfx_height,int *width,int *height);



/* in msdos/sound.c */
int msdos_update_audio(void);


extern int use_profiler;
void osd_profiler_display(void);


/* HJB 10/07/98 specialized update_screen functions */
/* dirty mode 1 (VIDEO_SUPPORTS_DIRTY) */
static void update_screen_dirty1_vga(void);
static void update_screen_dirty1_vesa_16bpp_double_scanlines(void);
static void update_screen_dirty1_vesa_16bpp_double_noscanlines(void);
static void update_screen_dirty1_vesa_16bpp_single(void);
static void update_screen_dirty1_vesa_16bpp_hsingle_vdouble_scanlines(void);
static void update_screen_dirty1_vesa_16bpp_hsingle_vdouble_noscanlines(void);
static void update_screen_dirty1_vesa_8bpp_double_scanlines(void);
static void update_screen_dirty1_vesa_8bpp_double_noscanlines(void);
static void update_screen_dirty1_vesa_8bpp_single(void);
static void update_screen_dirty1_vesa_8bpp_hsingle_vdouble_scanlines(void);
static void update_screen_dirty1_vesa_8bpp_hsingle_vdouble_noscanlines(void);
/* dirty mode 0 (no osd_mark_dirty calls) */
static void update_screen_dirty0_vga(void);
static void update_screen_dirty0_vesa_16bpp_double_scanlines(void);
static void update_screen_dirty0_vesa_16bpp_double_noscanlines(void);
static void update_screen_dirty0_vesa_16bpp_single(void);
static void update_screen_dirty0_vesa_16bpp_hsingle_vdouble_scanlines(void);
static void update_screen_dirty0_vesa_16bpp_hsingle_vdouble_noscanlines(void);
static void update_screen_dirty0_vesa_8bpp_double_scanlines(void);
static void update_screen_dirty0_vesa_8bpp_double_noscanlines(void);
static void update_screen_dirty0_vesa_8bpp_single(void);
static void update_screen_dirty0_vesa_8bpp_hsingle_vdouble_scanlines(void);
static void update_screen_dirty0_vesa_8bpp_hsingle_vdouble_noscanlines(void);

static void update_screen_dummy(void);
void (*update_screen)(void) = update_screen_dummy;

static struct osd_bitmap *scrbitmap;
static unsigned char current_palette[256][3];
static PALETTE adjusted_palette;
static unsigned char dirtycolor[256];
static int dirtypalette;


int frameskip,autoframeskip;
#define FRAMESKIP_LEVELS 12


int ntsc;
int vgafreq;
int always_synced;
int video_sync;
int color_depth;
int skiplines;
int skipcolumns;
int scanlines;
int use_double;
int use_vesa;
int gfx_mode;
float gamma_correction = 1.0;
int brightness = 100;
char *pcxdir;
char *resolution;
char *mode_desc;
int gfx_mode;
int gfx_width;
int gfx_height;

static int auto_resolution;
static int viswidth;
static int visheight;
static int skiplinesmax;
static int skipcolumnsmax;
static int skiplinesmin;
static int skipcolumnsmin;

static int vector_game;
static int use_dirty;

static Register *reg = 0;       /* for VGA modes */
static int reglen = 0;  /* for VGA modes */
static int videofreq;   /* for VGA modes */

static int gfx_xoffset;
static int gfx_yoffset;
static int gfx_display_lines;
static int gfx_display_columns;
static int doubling = 0;
static int vdoubling = 0;
int throttle = 1;       /* toggled by F10 */

static int gone_to_gfx_mode;
static int frameskip_counter;
static int frames_displayed;
static uclock_t start_time,end_time;	/* to calculate fps average on exit */
#define FRAMES_TO_SKIP 20	/* skip the first few frames from the FPS calculation */
							/* to avoid counting the copyright and info screens */


struct vga_tweak { int x, y; Register *reg; int reglen; int syncvgafreq; int scanlines; };
struct vga_tweak vga_orig_tweaked[] = {
	{ 288, 224, orig_scr288x224scanlines, sizeof(scr288x224scanlines)/sizeof(Register), 0, 1},
	{ 256, 256, orig_scr256x256scanlines, sizeof(scr256x256scanlines)/sizeof(Register), 0, 1},
	{ 224, 288, orig_scr224x288scanlines, sizeof(scr224x288scanlines)/sizeof(Register), 0, 1},
	{ 288, 224, orig_scr288x224, sizeof(scr288x224)/sizeof(Register),  0, 0 },
	{ 256, 256, orig_scr256x256, sizeof(scr256x256)/sizeof(Register),  0, 0 },
	{ 224, 288, orig_scr224x288, sizeof(scr224x288)/sizeof(Register),  0, 0 },
	{ 0, 0 }
};
struct vga_tweak vga_tweaked[] = {
	{ 288, 224, scr288x224scanlines, sizeof(scr288x224scanlines)/sizeof(Register), 0, 1},
	{ 256, 256, scr256x256scanlines, sizeof(scr256x256scanlines)/sizeof(Register), 1, 1},
	{ 224, 288, scr224x288scanlines, sizeof(scr224x288scanlines)/sizeof(Register), 2, 1},
	{ 320, 204, scr320x204, sizeof(scr320x204)/sizeof(Register), -1, 0 },
	{ 288, 224, scr288x224, sizeof(scr288x224)/sizeof(Register),  0, 0 },
	{ 256, 256, scr256x256, sizeof(scr256x256)/sizeof(Register),  1, 0 },
	{ 224, 288, scr224x288, sizeof(scr224x288)/sizeof(Register),  1, 0 },
	{ 200, 320, scr200x320, sizeof(scr200x320)/sizeof(Register), -1, 0 },
	{ 0, 0 }
};


/* Create a bitmap. Also calls osd_clearbitmap() to appropriately initialize */
/* it to the background color. */
/* VERY IMPORTANT: the function must allocate also a "safety area" 8 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */
struct osd_bitmap *osd_new_bitmap(int width,int height,int depth)       /* ASG 980209 */
{
	struct osd_bitmap *bitmap;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	if ((bitmap = malloc(sizeof(struct osd_bitmap))) != 0)
	{
		int i,rowlen,rdwidth;
		unsigned char *bm;
		int safety;


		if (width > 32) safety = 8;
		else safety = 0;        /* don't create the safety area for GfxElement bitmaps */

		if (depth != 8 && depth != 16) depth = 8;

		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		rdwidth = (width + 7) & ~7;     /* round width to a quadword */
		if (depth == 16)
			rowlen = 2 * (rdwidth + 2 * safety) * sizeof(unsigned char);
		else
			rowlen =     (rdwidth + 2 * safety) * sizeof(unsigned char);

		if ((bm = malloc((height + 2 * safety) * rowlen)) == 0)
		{
			free(bitmap);
			return 0;
		}

		if ((bitmap->line = malloc(height * sizeof(unsigned char *))) == 0)
		{
			free(bm);
			free(bitmap);
			return 0;
		}

		for (i = 0;i < height;i++)
			bitmap->line[i] = &bm[(i + safety) * rowlen + safety];

		bitmap->_private = bm;

		osd_clearbitmap(bitmap);
	}

	return bitmap;
}



/* set the bitmap to black */
void osd_clearbitmap(struct osd_bitmap *bitmap)
{
	int i;


	for (i = 0;i < bitmap->height;i++)
	{
		if (bitmap->depth == 16)
			memset(bitmap->line[i],0,2*bitmap->width);
		else
			memset(bitmap->line[i],BACKGROUND,bitmap->width);
	}


	if (bitmap == scrbitmap)
	{
		extern int bitmap_dirty;	/* in mame.c */

		osd_mark_dirty (0,0,bitmap->width-1,bitmap->height-1,1);
		bitmap_dirty = 1;
	}
}



void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		free(bitmap->line);
		free(bitmap->_private);
		free(bitmap);
	}
}


void osd_mark_dirty(int _x1, int _y1, int _x2, int _y2, int ui)
{
	if (use_dirty)
	{
		int x, y;

//        if (errorlog) fprintf(errorlog, "mark_dirty %3d,%3d - %3d,%3d\n", _x1,_y1, _x2,_y2);

		_x1 -= skipcolumns;
		_x2 -= skipcolumns;
		_y1 -= skiplines;
		_y2 -= skiplines;

        if (_y1 >= gfx_display_lines || _y2 < 0 || _x1 > gfx_display_columns || _x2 < 0) return;
		if (_y1 < 0) _y1 = 0;
		if (_y2 >= gfx_display_lines) _y2 = gfx_display_lines - 1;
		if (_x1 < 0) _x1 = 0;
		if (_x2 >= gfx_display_columns) _x2 = gfx_display_columns - 1;

		for (y = _y1; y <= _y2 + 15; y += 16)
			for (x = _x1; x <= _x2 + 15; x += 16)
				MARKDIRTY(x,y);
	}
}

static void init_dirty(char dirty)
{
	memset(dirty_new, dirty, MAX_GFX_WIDTH/16 * MAX_GFX_HEIGHT/16);
}

INLINE void swap_dirty(void)
{
    char *tmp;

	tmp = dirty_old;
	dirty_old = dirty_new;
	dirty_new = tmp;
}

/*
 * This function tries to find the best display mode.
 */
static void select_display_mode(void)
{
	int width,height;


	auto_resolution = 0;

	if (vector_game)
	{
		width = Machine->drv->screen_width;
		height = Machine->drv->screen_height;
	}
	else
	{
		width = Machine->drv->visible_area.max_x - Machine->drv->visible_area.min_x + 1;
		height = Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1;
	}

	doubling = use_double;
	vdoubling = 0;
	if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
			== VIDEO_PIXEL_ASPECT_RATIO_1_2)
	{
		doubling = 0;
		vdoubling = 1;
		height *= 2;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	/* 16 bit color is supported only by VESA modes */
	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
	{
		if (errorlog)
			fprintf (errorlog, "Game needs 16-bit colors. Using VESA\n");
		use_vesa = 1;
	}

	/* Check for special NTSC mode */
	if (ntsc == 1)
	{
		if (errorlog)
			fprintf (errorlog, "Using special NTSC video mode.\n");
		use_vesa = 0; gfx_width = 288; gfx_height = 224;
	}

	/* If a specific resolution was given, reconsider VESA against */
	/* tweaked VGA modes */
	if (gfx_width && gfx_height)
	{
		if (gfx_width >= 320 && gfx_height >= 240)
			use_vesa = 1;
		else
			use_vesa = 0;
	}

	/* Hack for 320x480 and 400x600 "vmame" video modes */
	if ((gfx_width == 320 && gfx_height == 480) ||
		(gfx_width == 400 && gfx_height == 600))
	{
		use_vesa = 1;
		doubling = 0;
		vdoubling = 1;
		height *= 2;
	}

	/* -autovesa ? */
	if (use_vesa == -1)
	{
		if (width >= 320 && height >= 240)
			use_vesa = 1;
		else
			use_vesa = 0;
	}


	/* If using tweaked modes, check if there exists one to fit
	   the screen in, otherwise use VESA */
	if (use_vesa == 0 && !gfx_width && !gfx_height)
	{
		int i;
		for (i=0; vga_tweaked[i].x != 0; i++)
		{
			if (width  <= vga_tweaked[i].x &&
				height <= vga_tweaked[i].y)
			{
				gfx_width  = vga_tweaked[i].x;
				gfx_height = vga_tweaked[i].y;
				/* if a resolution was chosen that is only available as */
				/* noscanline, we need to reset the scanline global */
				if (vga_tweaked[i].scanlines == 0)
					scanlines = 0;

				/* leave the loop on match */
				break;
			}
		}
		/* If we didn't find a tweaked VGA mode, use VESA */
		if (gfx_width == 0)
		{
			if (errorlog)
				fprintf (errorlog, "Did not find a tweaked VGA mode. Using VESA.\n");
			use_vesa = 1;
		}
	}


	/* If no VESA resolution has been given, we choose a sensible one. */
	/* 640x480, 800x600 and 1024x768 are common to all VESA drivers. */

	if ((use_vesa == 1) && !gfx_width && !gfx_height)
	{
		auto_resolution = 1;

		/* vector games use 640x480 as default */
		if (vector_game)
		{
			gfx_width = 640;
			gfx_height = 480;
		}
		else
		{
			/* turn off pixel doubling if we don't want scanlines */
			if (scanlines == 0) doubling = 0;

			if (doubling != 0)
			{
				/* see if pixel doubling can be applied at 640x480 */
				if (height <= 240 && width <= 320)
				{
					gfx_width = 640;
					gfx_height = 480;
				}
				/* see if pixel doubling can be applied at 800x600 */
				else if (height <= 300 && width <= 400)
				{
					gfx_width = 800;
					gfx_height = 600;
				}
				/* we don't want to use pixel doubling at 1024x768 */

				/* no pixel doubled modes fit, revert to not doubled */
				else
					doubling = 0;
			}

			if (doubling == 0)
			{
				if (height <= 240 && width <= 320)
				{
					gfx_width = 320;
					gfx_height = 240;
				}
				else if (height <= 300 && width <= 400)
				{
					gfx_width = 400;
					gfx_height = 300;
				}
				else if (height <= 384 && width <= 512)
				{
					gfx_width = 512;
					gfx_height = 384;
				}
				else if (height <= 480 && width <= 640)
				{
					gfx_width = 640;
					gfx_height = 480;
				}
				else if (height <= 600 && width <= 800)
				{
					gfx_width = 800;
					gfx_height = 600;
				}
				else
				{
					gfx_width = 1024;
					gfx_height = 768;
				}
			}
		}
	}
}



/* center image inside the display based on the visual area */
static void adjust_display (int xmin, int ymin, int xmax, int ymax)
{
	int temp;
	int w,h;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		temp = xmin; xmin = ymin; ymin = temp;
		temp = xmax; xmax = ymax; ymax = temp;
		w = Machine->drv->screen_height;
		h = Machine->drv->screen_width;
	}
	else
	{
		w = Machine->drv->screen_width;
		h = Machine->drv->screen_height;
	}

	if (!vector_game)
	{
		if (Machine->orientation & ORIENTATION_FLIP_X)
		{
			temp = w - xmin - 1;
			xmin = w - xmax - 1;
			xmax = temp;
		}
		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			temp = h - ymin - 1;
			ymin = h - ymax - 1;
			ymax = temp;
		}
	}

	viswidth  = xmax - xmin + 1;
	visheight = ymax - ymin + 1;

	if (doubling == 0 || use_vesa == 0 ||
			(doubling != 1 && (viswidth > gfx_width/2 || visheight > gfx_height/2)))
		doubling = 0;
	else
		doubling = 1;

	gfx_display_lines = visheight;
	gfx_display_columns = viswidth;
	if (doubling || vdoubling)
	{
		gfx_yoffset = (gfx_height - visheight * 2) / 2;
		if (gfx_display_lines > gfx_height / 2)
			gfx_display_lines = gfx_height / 2;
	}
	else
	{
		gfx_yoffset = (gfx_height - visheight) / 2;
		if (gfx_display_lines > gfx_height)
			gfx_display_lines = gfx_height;
	}
	if (doubling)
	{
		gfx_xoffset = (gfx_width - viswidth * 2) / 2;
		if (gfx_display_columns > gfx_width / 2)
			gfx_display_columns = gfx_width / 2;
	}
	else
	{
		gfx_xoffset = (gfx_width - viswidth) / 2;
		if (gfx_display_columns > gfx_width)
			gfx_display_columns = gfx_width;
	}

	skiplinesmin = ymin;
	skiplinesmax = visheight - gfx_display_lines + ymin;
	skipcolumnsmin = xmin;
	skipcolumnsmax = viswidth - gfx_display_columns + xmin;

	/* Align on a quadword !*/
	gfx_xoffset &= ~7;

	/* the skipcolumns from mame.cfg/cmdline is relative to the visible area */
	skipcolumns = xmin + skipcolumns;
	skiplines   = ymin + skiplines;

	/* Just in case the visual area doesn't fit */
	if (gfx_xoffset < 0)
	{
		skipcolumns -= gfx_xoffset;
		gfx_xoffset = 0;
	}
	if (gfx_yoffset < 0)
	{
		skiplines   -= gfx_yoffset;
		gfx_yoffset = 0;
	}

	/* Failsafe against silly parameters */
	if (skiplines < skiplinesmin)
		skiplines = skiplinesmin;
	if (skipcolumns < skipcolumnsmin)
		skipcolumns = skipcolumnsmin;
	if (skiplines > skiplinesmax)
		skiplines = skiplinesmax;
	if (skipcolumns > skipcolumnsmax)
		skipcolumns = skipcolumnsmax;

	if (errorlog)
		fprintf(errorlog,
				"gfx_width = %d gfx_height = %d\n"
				"xmin %d ymin %d xmax %d ymax %d\n"
				"skiplines %d skipcolumns %d\n"
				"gfx_display_lines %d gfx_display_columns %d\n",
				gfx_width,gfx_height,
				xmin, ymin, xmax, ymax, skiplines, skipcolumns,gfx_display_lines,gfx_display_columns);

	set_ui_visarea (skipcolumns, skiplines, skipcolumns+gfx_display_columns-1, skiplines+gfx_display_lines-1);
}



int game_width;
int game_height;
int game_attributes;

/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int attributes)
{
	if (errorlog)
		fprintf (errorlog, "width %d, height %d\n", width,height);


	if (frameskip < 0) frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS) frameskip = FRAMESKIP_LEVELS-1;


	gone_to_gfx_mode = 0;

	/* Look if this is a vector game */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		vector_game = 1;
	else
		vector_game = 0;


	/* Is the game using a dirty system? */
	if ((Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY) || vector_game)
		use_dirty = 1;
	else
		use_dirty = 0;

	select_display_mode();

	if (vector_game)
	{
		/* for vector games, use_double == 0 means miniaturized. */
		if (use_double == 0)
			scale_vectorgames(gfx_width/2,gfx_height/2,&width, &height);
		else
			scale_vectorgames(gfx_width,gfx_height,&width, &height);
	}

	game_width = width;
	game_height = height;
	game_attributes = attributes;

	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
		scrbitmap = osd_new_bitmap(width,height,16);
	else
		scrbitmap = osd_new_bitmap(width,height,8);

	if (!scrbitmap) return 0;

	if (!osd_set_display(width, height, attributes))
		return 0;

	if (vector_game)
	{
		/* vector games are always non-doubling */
		doubling = 0;
		/* center display */
		adjust_display(0, 0, width-1, height-1);
	}
	else /* center display based on visible area */
	{
		struct rectangle vis = Machine->drv->visible_area;
		adjust_display (vis.min_x, vis.min_y, vis.max_x, vis.max_y);
	}

	if (use_dirty) /* supports dirty ? */
	{
		if (use_vesa == 0)
		{
			update_screen = update_screen_dirty1_vga;
			if (errorlog) fprintf (errorlog, "update_screen_dirty1_vga\n");
        }
		else
		if (scrbitmap->depth == 16)
		{
			if (doubling) {
				if (scanlines) {
					update_screen = update_screen_dirty1_vesa_16bpp_double_scanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_16bpp_double_scanlines\n");
                } else {
					update_screen = update_screen_dirty1_vesa_16bpp_double_noscanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_16bpp_double_noscanlines\n");
                }
			} else {
				if (vdoubling)
				{
					if (scanlines) {
						update_screen = update_screen_dirty1_vesa_16bpp_hsingle_vdouble_scanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_16bpp_hsingle_vdouble_scanlines\n");
					} else {
						update_screen = update_screen_dirty1_vesa_16bpp_hsingle_vdouble_noscanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_16bpp_hsingle_vdouble_noscanlines\n");
					}
				}
				else
				{
					update_screen = update_screen_dirty1_vesa_16bpp_single;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_16bpp_single\n");
				}
            }
		} else {
			if (doubling) {
				if (scanlines) {
					update_screen = update_screen_dirty1_vesa_8bpp_double_scanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_8bpp_double_scanlines\n");
                } else {
					update_screen = update_screen_dirty1_vesa_8bpp_double_noscanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_8bpp_double_noscanlines\n");
                }
			} else {
				if (vdoubling)
				{
					if (scanlines) {
						update_screen = update_screen_dirty1_vesa_8bpp_hsingle_vdouble_scanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_8bpp_hsingle_vdouble_scanlines\n");
					} else {
						update_screen = update_screen_dirty1_vesa_8bpp_hsingle_vdouble_noscanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_8bpp_hsingle_vdouble_noscanlines\n");
					}
				}
				else
				{
					update_screen = update_screen_dirty1_vesa_8bpp_single;
					if (errorlog) fprintf (errorlog, "update_screen_dirty1_vesa_8bpp_single\n");
				}
            }
		}
	}
	else	/* does not support dirty */
	{
		if (use_vesa == 0)
		{
			update_screen = update_screen_dirty0_vga;
			if (errorlog) fprintf (errorlog, "update_screen_dirty0_vga\n");
        }
		else
		if (scrbitmap->depth == 16)
		{
			if (doubling) {
				if (scanlines) {
					update_screen = update_screen_dirty0_vesa_16bpp_double_scanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_16bpp_double_scanlines\n");
                } else {
					update_screen = update_screen_dirty0_vesa_16bpp_double_noscanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_16bpp_double_noscanlines\n");
                }
			} else {
				if (vdoubling)
				{
					if (scanlines) {
						update_screen = update_screen_dirty0_vesa_16bpp_hsingle_vdouble_scanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_16bpp_hsingle_vdouble_scanlines\n");
					} else {
						update_screen = update_screen_dirty0_vesa_16bpp_hsingle_vdouble_noscanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_16bpp_hsingle_vdouble_noscanlines\n");
					}
				}
				else
				{
					update_screen = update_screen_dirty0_vesa_16bpp_single;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_16bpp_single\n");
				}
            }
		}
		else
		{
			if (doubling) {
				if (scanlines) {
					update_screen = update_screen_dirty0_vesa_8bpp_double_scanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_8bpp_double_scanlines\n");
                } else {
					update_screen = update_screen_dirty0_vesa_8bpp_double_noscanlines;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_8bpp_double_noscanlines\n");
                }
			} else {
				if (vdoubling)
				{
					if (scanlines) {
						update_screen = update_screen_dirty0_vesa_8bpp_hsingle_vdouble_scanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_8bpp_hsingle_vdouble_scanlines\n");
					} else {
						update_screen = update_screen_dirty0_vesa_8bpp_hsingle_vdouble_noscanlines;
						if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_8bpp_hsingle_vdouble_noscanlines\n");
					}
				}
				else
				{
					update_screen = update_screen_dirty0_vesa_8bpp_single;
					if (errorlog) fprintf (errorlog, "update_screen_dirty0_vesa_8bpp_single\n");
				}
            }
		}
    }

    return scrbitmap;
}

/* set the actual display screen but don't allocate the screen bitmap */
int osd_set_display(int width,int height, int attributes)
{
	int     i;


	if (!gfx_height || !gfx_width)
	{
		printf("Please specify height AND width (e.g. -640x480)\n");
		return 0;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}
	/* Mark the dirty buffers as dirty */

	if (use_dirty)
	{
		if (vector_game)
			/* vector games only use one dirty buffer */
			init_dirty (0);
		else
			init_dirty(1);
		swap_dirty();
		init_dirty(1);
	}
	for (i = 0;i < 256;i++)
	{
		dirtycolor[i] = 1;
	}
	dirtypalette = 1;

	if (use_vesa == 0)
	{
		int found;

		/* setup tweaked modes */
		videofreq = vgafreq;
		found = 0;

		/* handle special NTSC mode */
		if (ntsc == 1)
		{
			reg = scr288x224_NTSC;
			reglen = sizeof(scr288x224_NTSC)/sizeof(Register);
			found = 1;
		}

		/* handle special noscanlines 256x256 57Hz tweaked mode */
		if (gfx_width == 256 && gfx_height == 256 && scanlines == 0 &&
				video_sync && Machine->drv->frames_per_second == 57)
		{
			reg = scr256x256_57;
			reglen = sizeof(scr256x256_57)/sizeof(Register);
			videofreq = 0;
			found = 1;
		}

		/* find the matching tweaked mode */
		/* use noscanline modes if scanline modes not possible */
		if (video_sync == 0 && always_synced == 0)
		{
			/* if vsync not requested, first look for more compatible modes */
			for (i=0; ((vga_orig_tweaked[i].x != 0) && !found); i++)
			{
				int scan;
				scan = vga_orig_tweaked[i].scanlines;

				if (gfx_width  == vga_orig_tweaked[i].x &&
					gfx_height == vga_orig_tweaked[i].y &&
					(scanlines == scan || scan == 0))
				{
					reg = vga_orig_tweaked[i].reg;
					reglen = vga_orig_tweaked[i].reglen;
					videofreq = 0;	/* always use the most compatible vgafreq 0 */
					found = 1;
				}
			}
		}
		for (i=0; ((vga_tweaked[i].x != 0) && !found); i++)
		{
			int scan;
			scan = vga_tweaked[i].scanlines;

			if (gfx_width  == vga_tweaked[i].x &&
				gfx_height == vga_tweaked[i].y &&
				(scanlines == scan || scan == 0))
			{
				reg = vga_tweaked[i].reg;
				reglen = vga_tweaked[i].reglen;
				if (videofreq == -1)
					videofreq = vga_tweaked[i].syncvgafreq;
				found = 1;
			}
		}


		/* can't find a VGA mode, use VESA */
		if (found == 0)
		{
			printf ("\nNo %dx%d tweaked VGA mode available.\n",
					gfx_width,gfx_height);
			return 0;
		}

		if (videofreq < 0) videofreq = 0;
		else if (videofreq > 3) videofreq = 3;
	}

	if (use_vesa == 1)
	{
		int mode, bits, err, found;

		mode = gfx_mode;
		found = 0;
		bits = scrbitmap->depth;

		/* Try the specified vesamode, 565 and 555 for 16 bit color modes, */
		/* doubled resolution in case of noscanlines and if not succesful  */
		/* repeat for all "lower" VESA modes. NS/BW 19980102 */

		while (!found)
		{
			set_color_depth(bits);

			err = set_gfx_mode(mode,gfx_width,gfx_height,0,0);

			if (errorlog)
			{
				fprintf (errorlog,"Trying ");
				if      (mode == GFX_VESA1)
					fprintf (errorlog, "VESA1");
				else if (mode == GFX_VESA2B)
					fprintf (errorlog, "VESA2B");
				else if (mode == GFX_VESA2L)
				    fprintf (errorlog, "VESA2L");
				else if (mode == GFX_VESA3)
					fprintf (errorlog, "VESA3");
			    fprintf (errorlog, "  %dx%d, %d bit\n",
						gfx_width, gfx_height, bits);
			}

			if (err == 0)
			{
				found = 1;
				continue;
			}
			else if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);

			/* Now adjust parameters for the next loop */

			/* try 5-5-5 in case there is no 5-6-5 16 bit color mode */
			if (scrbitmap->depth == 16)
			{
				if (bits == 16)
				{
					bits = 15;
					continue;
				}
				else
					bits = 16; /* reset to 5-6-5 */
			}

			/* try VESA modes in VESA3-VESA2L-VESA2B-VESA1 order */

			if (mode == GFX_VESA3)
			{
				mode = GFX_VESA2L;
				continue;
			}
			else if (mode == GFX_VESA2L)
			{
				mode = GFX_VESA2B;
				continue;
			}
			else if (mode == GFX_VESA2B)
			{
				mode = GFX_VESA1;
				continue;
			}
			else if (mode == GFX_VESA1)
				mode = gfx_mode; /* restart with the mode given in mame.cfg */

			/* try higher resolutions */
			if (auto_resolution && gfx_width <= 512)
			{
				/* low res VESA mode not available, try an high res one */
				if (use_double == 0)
				{
					/* if pixel doubling disabled use 640x480 */
					gfx_width = 640;
					gfx_height = 480;
					continue;
				}
				else
				{
					/* if pixel doubling enabled, turn it one and use the double resolution */
					doubling = 1;
					gfx_width *= 2;
					gfx_height *= 2;
					continue;
				}
			}

			/* If there was no continue up to this point, we give up */
			break;
		}

		if (found == 0)
		{
			printf ("\nNo %d-bit %dx%d VESA mode available.\n",
					scrbitmap->depth,gfx_width,gfx_height);
			printf ("\nPossible causes:\n"
"1) Your video card does not support VESA modes at all. Almost all\n"
"   video cards support VESA modes natively these days, so you probably\n"
"   have an older card which needs some driver loaded first.\n"
"   In case you can't find such a driver in the software that came with\n"
"   your video card, Scitech Display Doctor or (for S3 cards) S3VBE\n"
"   are good alternatives.\n"
"2) Your VESA implementation does not support this resolution. For example,\n"
"   '-320x240', '-400x300' and '-512x384' are only supported by a few\n"
"   implementations.\n"
"3) Your video card doesn't support this resolution at this color depth.\n"
"   For example, 1024x768 in 16 bit colors requires 2MB video memory.\n"
"   You can either force an 8 bit video mode ('-depth 8') or use a lower\n"
"   resolution ('-640x480', '-800x600').\n");
			return 0;
		}
		else
		{
			if (errorlog)
				fprintf (errorlog, "Found matching %s mode\n", gfx_driver->desc);
			gfx_mode = mode;
		}
	}
	else
	{
		/* big hack: open a mode 13h screen using Allegro, then load the custom screen */
		/* definition over it. */
		if (set_gfx_mode(GFX_VGA,320,200,0,0) != 0)
			return 0;

		/* set the VGA clock */
		reg[0].value = (reg[0].value & 0xf3) | (videofreq << 2);

		outRegArray(reg,reglen);
	}


	gone_to_gfx_mode = 1;


	if (video_sync)
	{
		uclock_t a,b;
		float rate;


		/* wait some time to let everything stabilize */
		for (i = 0;i < 60;i++)
		{
			vsync();
			a = uclock();
		}

		/* small delay for really really fast machines */
		for (i = 0;i < 100000;i++) ;

		vsync();
		b = uclock();
		rate = ((float)UCLOCKS_PER_SEC)/(b-a);

if (errorlog) fprintf(errorlog,"target frame rate = %dfps, video frame rate = %3.2fHz\n",Machine->drv->frames_per_second,rate);

		/* don't allow more than 8% difference between target and actual frame rate */
		while (rate > Machine->drv->frames_per_second * 108 / 100)
			rate /= 2;

		if (rate < Machine->drv->frames_per_second * 92 / 100)
		{
			osd_close_display();
if (errorlog) fprintf(errorlog,"-vsync option cannot be used with this display mode:\n"
					"video refresh frequency = %dHz, target frame rate = %dfps\n",
					(int)(UCLOCKS_PER_SEC/(b-a)),Machine->drv->frames_per_second);
			return 0;
		}

		if (Machine->sample_rate)
		{
			Machine->sample_rate = Machine->sample_rate * Machine->drv->frames_per_second / rate;
if (errorlog) fprintf(errorlog,"sample rate adjusted to match video freq: %d\n",Machine->sample_rate);
		}
	}

	return 1;
}



/* shut up the display */
void osd_close_display(void)
{
	if (gone_to_gfx_mode != 0)
	{
		set_gfx_mode(GFX_TEXT,0,0,0,0);

		if (frames_displayed > FRAMES_TO_SKIP)
			printf("Average FPS: %f\n",(double)UCLOCKS_PER_SEC/(end_time-start_time)*(frames_displayed-FRAMES_TO_SKIP));
	}

	if (scrbitmap)
	{
		osd_free_bitmap(scrbitmap);
		scrbitmap = NULL;
	}
}




/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* If 'totalcolors' is 32768, 'palette' is ignored and the *pens array is filled */
/* with pen values corresponding to a 5-5-5 15-bit palette */
void osd_allocate_colors(unsigned int totalcolors,const unsigned char *palette,unsigned short *pens)
{
	if (totalcolors == 32768)
	{
		int r1,g1,b1;
		int r,g,b;


		for (r1 = 0; r1 < 32; r1++)
		{
			for (g1 = 0; g1 < 32; g1++)
			{
				for (b1 = 0; b1 < 32; b1++)
				{
					r = 255 * brightness * pow(r1 / 31.0, 1 / gamma_correction) / 100;
					g = 255 * brightness * pow(g1 / 31.0, 1 / gamma_correction) / 100;
					b = 255 * brightness * pow(b1 / 31.0, 1 / gamma_correction) / 100;
					*pens++ = makecol(r,g,b);
				}
			}
		}

		Machine->uifont->colortable[0] = makecol(0x00,0x00,0x00);
		Machine->uifont->colortable[1] = makecol(0xff,0xff,0xff);
		Machine->uifont->colortable[2] = makecol(0xff,0xff,0xff);
		Machine->uifont->colortable[3] = makecol(0x00,0x00,0x00);
	}
	else
	{
		int i;


		/* initialize the palette */
		for (i = 0;i < 256;i++)
			current_palette[i][0] = current_palette[i][1] = current_palette[i][2] = 0;

		if (totalcolors >= 255)
		{
			int bestblack,bestwhite;
			int bestblackscore,bestwhitescore;


			for (i = 0;i < totalcolors;i++)
				pens[i] = i;

			bestblack = bestwhite = 0;
			bestblackscore = 3*255*255;
			bestwhitescore = 0;
			for (i = 0;i < totalcolors;i++)
			{
				int r,g,b,score;

				r = palette[3*i];
				g = palette[3*i+1];
				b = palette[3*i+2];
				score = r*r + g*g + b*b;

				if (score < bestblackscore)
				{
					bestblack = i;
					bestblackscore = score;
				}
				if (score > bestwhitescore)
				{
					bestwhite = i;
					bestwhitescore = score;
				}
			}

			Machine->uifont->colortable[0] = pens[bestblack];
			Machine->uifont->colortable[1] = pens[bestwhite];
			Machine->uifont->colortable[2] = pens[bestwhite];
			Machine->uifont->colortable[3] = pens[bestblack];
		}
		else
		{
			/* reserve color 1 for the user interface text */
			current_palette[1][0] = current_palette[1][1] = current_palette[1][2] = 0xff;
			Machine->uifont->colortable[0] = 0;
			Machine->uifont->colortable[1] = 1;
			Machine->uifont->colortable[2] = 1;
			Machine->uifont->colortable[3] = 0;

			/* fill the palette starting from the end, so we mess up badly written */
			/* drivers which don't go through Machine->pens[] */
			for (i = 0;i < totalcolors;i++)
				pens[i] = 255-i;
		}

		for (i = 0;i < totalcolors;i++)
		{
			current_palette[pens[i]][0] = palette[3*i];
			current_palette[pens[i]][1] = palette[3*i+1];
			current_palette[pens[i]][2] = palette[3*i+2];
		}
	}
}


void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
	if (scrbitmap->depth != 8)
	{
		if (errorlog) fprintf(errorlog,"error: osd_modify_pen() doesn't work with %d bit video modes.\n",scrbitmap->depth);
		return;
	}


	if (current_palette[pen][0] != red ||
			current_palette[pen][1] != green ||
			current_palette[pen][2] != blue)
	{
		current_palette[pen][0] = red;
		current_palette[pen][1] = green;
		current_palette[pen][2] = blue;

		dirtycolor[pen] = 1;
		dirtypalette = 1;
	}
}



void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
	if (scrbitmap->depth == 16)
	{
		*red = getr(pen);
		*green = getg(pen);
		*blue = getb(pen);
	}
	else
	{
		*red = current_palette[pen][0];
		*green = current_palette[pen][1];
		*blue = current_palette[pen][2];
	}
}

/* Writes messages on the screen. */
void my_textout(char *buf,int x,int y)
{
	int trueorientation,l,i;


	/* hack: force the display into standard orientation to avoid */
	/* rotating the text */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	l = strlen(buf);
	for (i = 0;i < l;i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				x + i*Machine->uifont->width + skipcolumns,
				y + skiplines, 0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}


INLINE void double_pixels(unsigned long *lb, short seg,
			  unsigned long address, int width4)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"bswap %%eax             \n"
	"xchgw %%ax,%%bx         \n"
	"roll $8, %%eax          \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"rorl $8, %%eax          \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	::
	"d" (seg),
	"c" (width4),
	"S" (lb),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}

INLINE void double_pixels16(unsigned long *lb, short seg,
			  unsigned long address, int width4)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"roll $16, %%eax         \n"
	"xchgw %%ax,%%bx         \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	::
	"d" (seg),
	"c" (width4),
	"S" (lb),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}


void update_screen_dirty1_vga(void)
{
	int width4, x, y, columns4;
	unsigned long *lb, address;

	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	columns4 = gfx_display_columns/4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				unsigned long *lb0 = lb + x / 4;
				unsigned long address0 = address + x;
				int h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
				{
					_dosmemputl(lb0, w/4, address0);
					lb0 += width4;
					address0 += gfx_width;
				}
			}
			x += w;
        }
		lb += 16 * width4;
		address += 16 * gfx_width;
	}
}

void update_screen_dirty1_vesa_16bpp_double_scanlines(void)
{
	short dest_seg;
	int x, y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long *lb0 = lb + x/2;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 4*x;
					double_pixels16(lb0, dest_seg, address, w/2);
					vesa_line0 += 2;
					lb0 += width4;
				}
            }
			x += w;
        }
		vesa_line += 16 * 2;
		lb += 16 * width4;
	}
}

void update_screen_dirty1_vesa_16bpp_double_noscanlines(void)
{
	short dest_seg;
	int x, y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
    for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long *lb0 = lb + x/2;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 4*x;
					double_pixels16(lb0, dest_seg, address, w/2);
					address = bmp_write_line (screen, vesa_line0 + 1) + xoffs + 4*x;
                    double_pixels16(lb0, dest_seg, address, w/2);
                    vesa_line0 += 2;
					lb0 += width4;
				}
            }
			x += w;
        }
		vesa_line += 16 * 2;
		lb += 16 * width4;
    }
}

void update_screen_dirty1_vesa_16bpp_single(void)
{
	short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x*2;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 2*x;
					_movedatal(src_seg, src, dest_seg, address, w/2);
					vesa_line0++;
					src += 4 * width4;
				}
            }
			x += w;
        }
		vesa_line += 16;
		lb += 16 * width4;
    }
}

void update_screen_dirty1_vesa_16bpp_hsingle_vdouble_scanlines(void)
{
	short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x*2;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 2*x;
					_movedatal(src_seg, src, dest_seg, address, w/2);
					vesa_line0 += 2;
					src += 4 * width4;
				}
            }
			x += w;
        }
		vesa_line += 16 * 2;
		lb += 16 * width4;
    }
}

void update_screen_dirty1_vesa_16bpp_hsingle_vdouble_noscanlines(void)
{
	short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x*2;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 2*x;
					_movedatal(src_seg, src, dest_seg, address, w/2);
					address = bmp_write_line (screen, vesa_line0+1) + xoffs + 2*x;
					_movedatal(src_seg, src, dest_seg, address, w/2);
					vesa_line0 += 2;
					src += 4 * width4;
				}
            }
			x += w;
        }
		vesa_line += 16 * 2;
		lb += 16 * width4;
    }
}

void update_screen_dirty1_vesa_8bpp_double_scanlines(void)
{
	short dest_seg;
	int x, y, vesa_line, width4, columns4;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long *lb0 = lb + x/4;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 2*x;
					double_pixels(lb0, dest_seg, address, w/4);
					vesa_line0 += 2;
					lb0 += width4;
				}
			}
			x += w;
		}
		vesa_line += 16 * 2;
		lb += 16 * width4;
	}
}

void update_screen_dirty1_vesa_8bpp_double_noscanlines(void)
{
	short dest_seg;
	int x, y, vesa_line, width4, columns4;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long *lb0 = lb + x/4;
				int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + 2*x;
					double_pixels(lb0, dest_seg, address, w/4);
					address = bmp_write_line (screen, vesa_line0+1) + xoffs + 2*x;
                    double_pixels(lb0, dest_seg, address, w/4);
					vesa_line0 += 2;
					lb0 += width4;
				}
			}
			x += w;
		}
		vesa_line += 16 * 2;
		lb += 16 * width4;
    }
}

void update_screen_dirty1_vesa_8bpp_single(void)
{
    short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x;
                int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
					w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + x;
					_movedatal(src_seg, src, dest_seg, address, w/4);
					vesa_line0++;
					src += 4 * width4;
				}
			}
			x += w;
		}
		vesa_line += 16;
		lb += 16 * width4;
	}
}

void update_screen_dirty1_vesa_8bpp_hsingle_vdouble_scanlines(void)
{
    short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x;
                int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
					w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + x;
					_movedatal(src_seg, src, dest_seg, address, w/4);
					vesa_line0 += 2;
					src += 4 * width4;
				}
			}
			x += w;
		}
		vesa_line += 16 * 2;
		lb += 16 * width4;
	}
}

void update_screen_dirty1_vesa_8bpp_hsingle_vdouble_noscanlines(void)
{
    short src_seg, dest_seg;
	int x, y, vesa_line, width4, columns4;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; /* */ )
		{
			int w = 16;
			if (ISDIRTY(x,y))
            {
				unsigned long src = (unsigned long)lb + x;
                int vesa_line0 = vesa_line, h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
					w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
                {
					address = bmp_write_line (screen, vesa_line0) + xoffs + x;
					_movedatal(src_seg, src, dest_seg, address, w/4);
					address = bmp_write_line (screen, vesa_line0+1) + xoffs + x;
					_movedatal(src_seg, src, dest_seg, address, w/4);
					vesa_line0 += 2;
					src += 4 * width4;
				}
			}
			x += w;
		}
		vesa_line += 16 * 2;
		lb += 16 * width4;
	}
}

void update_screen_dirty0_vga(void)
{
	int width4,y,columns4;
	unsigned long *lb, address;

	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	columns4 = gfx_display_columns/4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y++)
	{
		_dosmemputl(lb,columns4,address);
		lb+=width4;
		address+=gfx_width;
	}
}

void update_screen_dirty0_vesa_16bpp_double_scanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		double_pixels16(lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_16bpp_double_noscanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		double_pixels16(lb,dest_seg,address,columns4);
		address = bmp_write_line (screen, vesa_line+1) + xoffs;
		double_pixels16(lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_16bpp_single(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line++;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_16bpp_hsingle_vdouble_scanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_16bpp_hsingle_vdouble_noscanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = 2 * gfx_xoffset;
	columns4 = gfx_display_columns/2;
	skipcol2 = skipcolumns*2;
	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);
		address = bmp_write_line (screen, vesa_line+1) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_8bpp_double_scanlines(void)
{
	short dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	skipcol2 = skipcolumns;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		double_pixels(lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_8bpp_double_noscanlines(void)
{
	short dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	skipcol2 = skipcolumns;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		double_pixels(lb,dest_seg,address,columns4);
		address = bmp_write_line (screen, vesa_line+1) + xoffs;
		double_pixels(lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_8bpp_single(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	skipcol2 = skipcolumns;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line++;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_8bpp_hsingle_vdouble_scanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	skipcol2 = skipcolumns;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dirty0_vesa_8bpp_hsingle_vdouble_noscanlines(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4, columns4, skipcol2;
	unsigned long *lb, address;
	int xoffs;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
	xoffs = gfx_xoffset;
	columns4 = gfx_display_columns/4;
	skipcol2 = skipcolumns;

	lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
	for (y = 0; y < gfx_display_lines; y++)
	{
		address = bmp_write_line (screen, vesa_line) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);
		address = bmp_write_line (screen, vesa_line+1) + xoffs;
		_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);

		vesa_line += 2;
		lb+=width4;
	}
}

void update_screen_dummy(void)
{
	if (errorlog)
		fprintf(errorlog, "video.c: update_screen called before initialization!\n");
}

void clear_screen(void)
{
	char buf[MAX_GFX_WIDTH * 2];


	if (use_vesa == 0)
	{
		int columns4,y;
		unsigned long address;


		address = 0xa0000;
		columns4 = gfx_width/4;
		for (y = 0; y < gfx_height; y++)
		{
			_dosmemputl(buf,columns4,address);

			address+=gfx_width;
		}
	}
	else
	{
		short src_seg, dest_seg;
		int y, columns4;
		unsigned long address;

		src_seg = _my_ds();
		dest_seg = screen->seg;
		columns4 = gfx_width/4;
		if (scrbitmap->depth == 16)
		{
			memset(buf, 0, gfx_width);
            for (y = 0; y < gfx_height; y++)
			{
				address = bmp_write_line (screen, y);
				_movedatal(src_seg,(unsigned long)buf,dest_seg,address,columns4);
			}
		}
		else
		{
			memset(buf,BACKGROUND,gfx_width);
            for (y = 0; y < gfx_height; y++)
			{
				address = bmp_write_line (screen, y);
				_movedatal(src_seg,(unsigned long)buf,dest_seg,address,columns4);
			}
		}
	}

	osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
}

INLINE void pan_display(void)
{
	/* horizontal panning */
	if (osd_key_pressed(OSD_KEY_LSHIFT))
	{
		if (osd_key_pressed(OSD_KEY_PGUP))
		{
			if (skipcolumns < skipcolumnsmax)
			{
				skipcolumns++;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
		if (osd_key_pressed(OSD_KEY_PGDN))
		{
			if (skipcolumns > skipcolumnsmin)
			{
				skipcolumns--;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
	}
	else /*  vertical panning */
	{
		if (osd_key_pressed(OSD_KEY_PGDN))
		{
			if (skiplines < skiplinesmax)
			{
				skiplines++;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
		if (osd_key_pressed(OSD_KEY_PGUP))
		{
			if (skiplines > skiplinesmin)
			{
				skiplines--;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
	}

	if (use_dirty) init_dirty(1);

	set_ui_visarea (skipcolumns, skiplines, skipcolumns+gfx_display_columns-1, skiplines+gfx_display_lines-1);
}



void osd_save_snapshot(void)
{
	BITMAP *bmp;
	PALETTE pal;
	char name[30];
	FILE *f;
	static int snapno;
	int y;

	do
	{
		if (snapno == 0)        /* first of all try with the "gamename.pcx" */
			sprintf(name,"%s/%.8s.pcx", pcxdir, Machine->gamedrv->name);

		else    /* otherwise use "nameNNNN.pcx" */
			sprintf(name,"%s/%.4s%04d.pcx", pcxdir, Machine->gamedrv->name,snapno);
		/* avoid overwriting of existing files */

		if ((f = fopen(name,"rb")) != 0)
		{
			fclose(f);
			snapno++;
		}
	} while (f != 0);

	get_palette(pal);
	if (use_vesa == 0)
	{
		bmp = create_bitmap(scrbitmap->width,scrbitmap->height);
		for (y = 0;y < scrbitmap->height;y++)
			memcpy(bmp->line[y],scrbitmap->line[y],scrbitmap->width);
	}
	else /* VESA modes */
	{
		int width, height;

		width = scrbitmap->width-skipcolumns;
		height = scrbitmap->height-skiplines;

		if (doubling)
		{
			width *=2;
			height *=2;
		}

		bmp = create_sub_bitmap (screen, gfx_xoffset, gfx_yoffset,
				width, height);
	}
	save_pcx(name,bmp,pal);
	destroy_bitmap(bmp);
	snapno++;

}


int osd_skip_this_frame(void)
{
	static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0 },
		{ 0,0,0,0,0,0,0,0,0,0,0,1 },
		{ 0,0,0,0,0,1,0,0,0,0,0,1 },
		{ 0,0,0,1,0,0,0,1,0,0,0,1 },
		{ 0,0,1,0,0,1,0,0,1,0,0,1 },
		{ 0,1,0,0,1,0,1,0,0,1,0,1 },
		{ 0,1,0,1,0,1,0,1,0,1,0,1 },
		{ 0,1,0,1,1,0,1,0,1,1,0,1 },
		{ 0,1,1,0,1,1,0,1,1,0,1,1 },
		{ 0,1,1,1,0,1,1,1,0,1,1,1 },
		{ 0,1,1,1,1,1,0,1,1,1,1,1 },
		{ 0,1,1,1,1,1,1,1,1,1,1,1 }
	};

	return skiptable[frameskip][frameskip_counter];
}

/* Update the display. */
void osd_update_video_and_audio(void)
{
	static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 1,1,1,1,1,1,1,1,1,1,1,1 },
		{ 2,1,1,1,1,1,1,1,1,1,1,0 },
		{ 2,1,1,1,1,0,2,1,1,1,1,0 },
		{ 2,1,1,0,2,1,1,0,2,1,1,0 },
		{ 2,1,0,2,1,0,2,1,0,2,1,0 },
		{ 2,0,2,1,0,2,0,2,1,0,2,0 },
		{ 2,0,2,0,2,0,2,0,2,0,2,0 },
		{ 2,0,2,0,0,3,0,2,0,0,3,0 },
		{ 3,0,0,3,0,0,3,0,0,3,0,0 },
		{ 4,0,0,0,4,0,0,0,4,0,0,0 },
		{ 6,0,0,0,0,0,6,0,0,0,0,0 },
		{12,0,0,0,0,0,0,0,0,0,0,0 }
	};
	int i;
	static int showfps,showfpstemp,showprofile;
	uclock_t curr;
	static uclock_t prev_frames[FRAMESKIP_LEVELS],prev;
	int speed=0;
	static int vups,vfcount;
	int need_to_clear_bitmap = 0;
	static int frameskipadjust;
	int already_synced;


	already_synced = msdos_update_audio();

	if (osd_skip_this_frame() == 0)
	{
		/* Check for PGUP, PGDN and pan screen */
		if (osd_key_pressed(OSD_KEY_PGDN) || osd_key_pressed(OSD_KEY_PGUP))
			pan_display();

		if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
		{
			showfpstemp--;
			if ((showfps == 0) && (showfpstemp == 0))
			{
				need_to_clear_bitmap = 1;
			}
		}

		if (osd_key_pressed_memory(OSD_KEY_SHOW_FPS))
		{
			if (showfpstemp)
			{
				showfpstemp = 0;
				need_to_clear_bitmap = 1;
			}
			else
			{
				showfps ^= 1;
				if (showfps == 0)
				{
					need_to_clear_bitmap = 1;
				}
			}
		}

		if (use_profiler && osd_key_pressed_memory(OSD_KEY_SHOW_PROFILE))
		{
			showprofile ^= 1;
			if (showprofile == 0)
				need_to_clear_bitmap = 1;
		}

		/* now wait until it's time to update the screen */
		if (throttle)
		{
			osd_profiler(OSD_PROFILE_IDLE);
			if (video_sync)
			{
				static uclock_t last;


				do
				{
					vsync();
					curr = uclock();
				} while (UCLOCKS_PER_SEC / (curr - last) > Machine->drv->frames_per_second * 11 /10);

				last = curr;
			}
			else
			{
				uclock_t target,target2;


				curr = uclock();

				/* wait until enough time has passed since last frame... */
				target = prev +
						waittable[frameskip][frameskip_counter] * UCLOCKS_PER_SEC/Machine->drv->frames_per_second;

				/* ... OR since FRAMESKIP_LEVELS frames ago. This way, if a frame takes */
				/* longer than the allotted time, we can compensate in the following frames. */
				target2 = prev_frames[frameskip_counter] +
						FRAMESKIP_LEVELS * UCLOCKS_PER_SEC/Machine->drv->frames_per_second;

				if (target > target2) target = target2;

				if (curr < target)
				{
					/* wait only if the audio update hasn't synced us already */
					if (already_synced == 0)
					{
						do
						{
							curr = uclock();
						} while (curr < target);
					}
				}

				if (curr - target > UCLOCKS_PER_SEC/4/Machine->drv->frames_per_second)
				{
					/* we are behind, we need to increase frameskip */
					frameskipadjust++;
				}
				else if (curr - target < UCLOCKS_PER_SEC/8/Machine->drv->frames_per_second)
				{
					/* we are on time, we might try decreasing frameskip */
					frameskipadjust--;
				}
			}
			osd_profiler(OSD_PROFILE_END);
		}
		else curr = uclock();


		/* for the FPS average calculation */
		if (++frames_displayed == FRAMES_TO_SKIP)
			start_time = curr;
		else
			end_time = curr;


		if (curr - prev_frames[frameskip_counter])
		{
			int divdr;


			divdr = Machine->drv->frames_per_second * (curr - prev_frames[frameskip_counter]) / (100 * FRAMESKIP_LEVELS);
			speed = (UCLOCKS_PER_SEC + divdr/2) / divdr;
		}

		prev = curr;
		for (i = 0;i < waittable[frameskip][frameskip_counter];i++)
			prev_frames[(frameskip_counter + FRAMESKIP_LEVELS - i) % FRAMESKIP_LEVELS] = curr;

		vfcount += waittable[frameskip][frameskip_counter];
		if (vfcount >= Machine->drv->frames_per_second)
		{
			extern int vector_updates; /* avgdvg_go()'s per Mame frame, should be 1 */


			vfcount = 0;
			vups = vector_updates;
			vector_updates = 0;
		}

		if (showfps || showfpstemp)
		{
			int trueorientation;
			int fps,l;
			char buf[30];
			int divdr;


			/* hack: force the display into standard orientation to avoid */
			/* rotating the text */
			trueorientation = Machine->orientation;
			Machine->orientation = ORIENTATION_DEFAULT;

			divdr = 100 * FRAMESKIP_LEVELS;
			fps = (Machine->drv->frames_per_second * (FRAMESKIP_LEVELS - frameskip) * speed + (divdr / 2)) / divdr;
			sprintf(buf,"fskp%2d %3d%%(%3d/%d fps)",frameskip,speed,fps,Machine->drv->frames_per_second);
			l = strlen(buf);
			for (i = 0;i < l;i++)
				drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,gfx_display_columns+skipcolumns-(l-i)*Machine->uifont->width,skiplines,0,TRANSPARENCY_NONE,0);
			if (vector_game)
			{
				sprintf(buf," %d vector updates",vups);
				l = strlen(buf);
				for (i = 0;i < l;i++)
					drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,gfx_display_columns+skipcolumns-(l-i)*Machine->uifont->width,skiplines+Machine->uifont->height,0,TRANSPARENCY_NONE,0);
			}

			Machine->orientation = trueorientation;
		}


		if (showprofile) osd_profiler_display();


		if (scrbitmap->depth == 8)
		{
			if (dirtypalette)
			{
				dirtypalette = 0;

				for (i = 0;i < 256;i++)
				{
					if (dirtycolor[i])
					{
						int r,g,b;


						dirtycolor[i] = 0;

						r = 255 * brightness * pow(current_palette[i][0] / 255.0, 1 / gamma_correction) / 100;
						g = 255 * brightness * pow(current_palette[i][1] / 255.0, 1 / gamma_correction) / 100;
						b = 255 * brightness * pow(current_palette[i][2] / 255.0, 1 / gamma_correction) / 100;

						adjusted_palette[i].r = r >> 2;
						adjusted_palette[i].g = g >> 2;
						adjusted_palette[i].b = b >> 2;

						set_color(i,&adjusted_palette[i]);
					}
				}
			}
		}


		/* copy the bitmap to screen memory */
		osd_profiler(OSD_PROFILE_BLIT);
		update_screen();
		osd_profiler(OSD_PROFILE_END);

		if (need_to_clear_bitmap)
			osd_clearbitmap(scrbitmap);

		if (use_dirty)
		{
			if (!vector_game)
				swap_dirty();
			init_dirty(0);
		}

		if (need_to_clear_bitmap)
			osd_clearbitmap(scrbitmap);


		if (autoframeskip)
		{
			/* decrease frameskip slowly, increase it quickly */
			if (frameskipadjust <= -8)
			{
				frameskipadjust = 0;
				if (frameskip > 0) frameskip--;
			}
			else if (frameskipadjust >= 2)
			{
				frameskipadjust = 0;
//				if (frameskip < FRAMESKIP_LEVELS-1) frameskip++;
				/* don't go above frameskip 8 */
				if (frameskip < 8) frameskip++;
			}
		}
	}


	if (osd_key_pressed_memory(OSD_KEY_FRAMESKIP))
	{
		frameskip = (frameskip + 1) % 12;
		if (showfps == 0)
			showfpstemp = 2*Machine->drv->frames_per_second;

		/* reset the frame counter every time the frameskip key is pressed, so */
		/* we'll measure the average FPS on a consistent status. */
		frames_displayed = 0;
	}

	if (osd_key_pressed_memory(OSD_KEY_THROTTLE))
	{
		throttle ^= 1;

		/* reset the frame counter every time the throttle key is pressed, so */
		/* we'll measure the average FPS on a consistent status. */
		frames_displayed = 0;
	}


	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;
}



void osd_set_gamma(float _gamma)
{
	if (scrbitmap->depth == 8)
	{
		int i;


		gamma_correction = _gamma;

		for (i = 0;i < 256;i++)
		{
			if (i != Machine->uifont->colortable[1])	/* don't touch the user interface text */
				dirtycolor[i] = 1;
		}
		dirtypalette = 1;
	}
}

float osd_get_gamma(void)
{
	return gamma_correction;
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
	if (scrbitmap->depth == 8)
	{
		int i;


		brightness = _brightness;

		for (i = 0;i < 256;i++)
		{
			if (i != Machine->uifont->colortable[1])	/* don't touch the user interface text */
				dirtycolor[i] = 1;
		}
		dirtypalette = 1;
	}
}

int osd_get_brightness(void)
{
	return brightness;
}
