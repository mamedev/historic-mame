#include "driver.h"
#include <math.h>
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include "TwkUser.c"
#include <allegro.h>
#include "vgafreq.h"
#include "vidhrdw/vector.h"

DECLARE_GFX_DRIVER_LIST(
	GFX_DRIVER_VGA
	GFX_DRIVER_VESA2L
	GFX_DRIVER_VESA2B
	GFX_DRIVER_VESA1)

DECLARE_COLOR_DEPTH_LIST(
	COLOR_DEPTH_8
	COLOR_DEPTH_15
	COLOR_DEPTH_16)

#define MAX_GFX_WIDTH 1600


void scale_vectorgames(int gfx_width,int gfx_height,int *width,int *height);
void joy_calibration(void);


static struct osd_bitmap *scrbitmap;
static unsigned char current_palette[256][3];
static unsigned char current_background_color;
static PALETTE adjusted_palette;
static unsigned char dirtycolor[256];
static int dirtypalette;

int vesa;
int ntsc;
int vgafreq;
int video_sync;
int color_depth;
int skiplines;
int skipcolumns;
int scanlines;
int use_double;
int use_synced;
float gamma_correction;
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
int throttle = 1;       /* toggled by F10 */

static int gone_to_gfx_mode;
static int frames_displayed;
static uclock_t start_time,end_time;	/* to calculate fps average on exit */
#define FRAMES_TO_SKIP 20	/* skip the first few frames from the FPS calculation */
							/* to avoid counting the copyright and info screens */


struct { int x, y; Register *reg; int reglen; int syncvgafreq; int scanlines; }
vga_tweaked[] = {
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

static char *vesa_desc[6]= {
	"AUTO_DETECT", "VGA", "MODEX (not supported)",
	"VESA 1.2", "VESA 2.0 (banked)", "VESA 2.0 (linear)"
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
			memset(bitmap->line[i],current_background_color,bitmap->width);
	}


	if (bitmap == scrbitmap)
	{
		osd_mark_dirty (0,0,bitmap->width-1,bitmap->height-1,1);

		/* signal the layer system that the screen needs a complete refresh */
		layer_mark_full_screen_dirty();
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




#define MAXDIRTY 1600
char line1[MAXDIRTY];
char line2[MAXDIRTY];
char *dirty_old=line1;
char *dirty_new=line2;


/* ASG 971011 */
void osd_mark_dirty(int x1, int _y1, int x2, int y2, int ui)
{
	if (use_dirty == 2)	/* GfxLayer dirty handling */
	{
		layer_mark_rectangle_dirty_norotate(Machine->dirtylayer,x1,x2,_y1,y2);
	}
	else if (use_dirty)
	{
		if (_y1 >= MAXDIRTY || y2 < 0) return;
		if (_y1 < 0) _y1 = 0;
		if (y2 >= MAXDIRTY) y2 = MAXDIRTY-1;

		memset(&dirty_new[_y1], 1, y2-_y1+1);
	}
}

static void init_dirty(char dirty)
{
	if (use_dirty == 2)
	{
		layer_mark_all_dirty(Machine->dirtylayer,dirty ? MARK_ALL_DIRTY : MARK_ALL_CLEAN);
	}
	else
		memset(&dirty_new[0], dirty, MAXDIRTY);
}

static void swap_dirty(void)
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

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	/* 16 bit color is supported only by VESA modes */
	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
		gfx_mode = GFX_VESA2L;

	/* Check for special NTSC mode */
	if (ntsc == 1)
	{
		gfx_mode = GFX_VGA; gfx_width = 288; gfx_height = 224;
	}

	/* If a specific resolution was given, reconsider VESA against */
	/* tweaked VGA modes */
	if (gfx_width && gfx_height)
	{
		if (gfx_width >= 320 && gfx_height >= 240)
			gfx_mode = GFX_VESA2L;
		else
			gfx_mode = GFX_VGA;
	}
	/* if no gfx mode specified, choose the best one */
	else if (!gfx_mode)
	{
		if (width >= 320 && height >= 240)
			gfx_mode = GFX_VESA2L;
		else
			gfx_mode = GFX_VGA;
	}


	/* If using tweaked modes, check if there exists one to fit
	   the screen in, otherwise use VESA */
	if (gfx_mode==GFX_VGA && !gfx_width && !gfx_height)
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
			gfx_mode = GFX_VESA2L;
	}


	/* If no VESA resolution has been given, we choose a sensible one. */
	/* 640x480, 800x600 and 1024x768 are common to all VESA drivers. */

	if ((gfx_mode!=GFX_VGA) && !gfx_width && !gfx_height)
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
			doubling = use_double;

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

	if (doubling == 0 || gfx_mode == GFX_VGA ||
			viswidth > gfx_width/2 || visheight > gfx_height/2)
		doubling = 0;
	else
		doubling = 1;

	if (doubling == 0)
	{
		gfx_yoffset = (gfx_height - visheight) / 2;
		gfx_xoffset = (gfx_width - viswidth) / 2;
		gfx_display_lines = visheight;
		gfx_display_columns = viswidth;
		if (gfx_display_lines > gfx_height)
			gfx_display_lines = gfx_height;
		if (gfx_display_columns > gfx_width)
			gfx_display_columns = gfx_width;
	}
	else
	{
		gfx_yoffset = (gfx_height - visheight * 2) / 2;
		gfx_xoffset = (gfx_width - viswidth * 2) / 2;
		gfx_display_lines = visheight;
		gfx_display_columns = viswidth;
		if (gfx_display_lines > gfx_height / 2)
			gfx_display_lines = gfx_height / 2;
		if (gfx_display_columns > gfx_width / 2)
			gfx_display_columns = gfx_width / 2;
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

	/* if driver uses the GfxLayer system, use different dirty support */
	if (Machine->drv->layer) use_dirty = 2;

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

	if (gfx_mode == GFX_VGA)
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

/* TODO: this is a temporary hack */
if (reg == scr256x256scanlines && use_synced == 0)
	reg = orig_scr256x256scanlines;

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

	if (gfx_mode!=GFX_VGA)
	{
		int mode, bits;


retry:
		/* Try the specified vesamode and all lower ones BW 131097 */
		for (mode=gfx_mode; mode>=GFX_VESA1; mode--)
		{
			/* try 5-6-5 first and 5-5-5 in case of 16 bit colors */
			bits = scrbitmap->depth;
			set_color_depth(bits);

			if (errorlog)
				fprintf (errorlog,"Trying %s, %dx%d, %d bit\n", vesa_desc[mode],gfx_width,gfx_height,bits);
			if (set_gfx_mode(mode,gfx_width,gfx_height,0,0) == 0)
				break;
			if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);

			if (bits ==8 )
				continue;

			bits = 15;
			set_color_depth(bits);

			if (errorlog)
				fprintf (errorlog,"Trying %s, %d bit\n", vesa_desc[mode],bits);
			if (set_gfx_mode(mode,gfx_width,gfx_height,0,0) == 0)
				break;
			if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);
		}

		if (mode < GFX_VESA1)
		{
			if (auto_resolution && gfx_width <= 512)
			{
				/* low res VESA mode not available, try an high res one */
				if (use_double == 0)
				{
					/* if pixel doubling disabledm use 640x480 */
					gfx_width = 640;
					gfx_height = 480;
					goto retry;
				}
				else
				{
					/* if pixel doubling enabled, turn it one and use the double resolution */
					doubling = 1;
					gfx_width *= 2;
					gfx_height *= 2;
					goto retry;
				}
			}

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

		gfx_mode = mode;
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
		set_gfx_mode(GFX_TEXT,80,25,0,0);

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
		int r,g,b;


		for (r = 0; r < 32; r++)
			for (g = 0; g < 32; g++)
				for (b = 0; b < 32; b++)
					*pens++ = makecol((r << 3) | (r >> 2),(g << 3) | (g >> 2),(b << 3) | (b >> 2));
	}
	else
	{
		int i;


		/* initialize the palette */
		for (i = 0;i < 256;i++)
			current_palette[i][0] = current_palette[i][1] = current_palette[i][2] = 0;

		/* fill the palette starting from the end, so we mess up badly written */
		/* drivers which don't go through Machine->pens[] */
		for (i = 0;i < totalcolors;i++)
			pens[i] = 255-i;

		for (i = 0;i < totalcolors;i++)
		{
			current_palette[pens[i]][0] = palette[3*i];
			current_palette[pens[i]][1] = palette[3*i+1];
			current_palette[pens[i]][2] = palette[3*i+2];
		}
	}

	/* do a first screen update to pick the */
	/* background pen before the copyright screen is displayed. */
	osd_update_display();
	/* the first call picked the color, the second call actually refreshes the screen */
	osd_update_display();
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

/* Writes messages in the middle of the screen. */
void my_textout (char *buf)
{
	int trueorientation,l,x,y,i;


	/* hack: force the display into standard orientation to avoid */
	/* rotating the text */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	l = strlen(buf);
	x = (gfx_display_columns - Machine->uifont->width * l) / 2;
	y = (gfx_display_lines   - Machine->uifont->height) / 2;
	for (i = 0;i < l;i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				x + i*Machine->uifont->width + skipcolumns,
				y + skiplines, 0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;

	if (use_dirty) init_dirty(1);
}


inline void double_pixels(unsigned long *lb, short seg,
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

inline void double_pixels16(unsigned long *lb, short seg,
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



void update_screen(void)
{
	if (use_dirty == 2)	/* different dirty handling for GfxLayer games */
	{
		if (gfx_mode==GFX_VGA)
		{
			int width,width4,y,columns4;
			unsigned long *lb, address;


			width = gfx_width;
			width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
			columns4 = gfx_display_columns/4;
			address = 0xa0000 + gfx_xoffset + gfx_yoffset * width;
			lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
			for (y = 0; y < gfx_display_lines; y++)
			{
				if (Machine->dirtylayer->dirtyline[(y+skiplines) >> 3] != 0)
					_dosmemputl(lb,columns4,address);

				lb+=width4;
				address+=width;
			}
		}
		else
		{
			short src_seg, dest_seg;
			int y, vesa_line, width4, columns4, skipcol2;
			unsigned long *lb, address;
			int xoffs, depth;

			src_seg = _my_ds();
			dest_seg = screen->seg;
			vesa_line = gfx_yoffset;
			width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
			depth = scrbitmap->depth;
			if (depth == 8)
			{
				xoffs = gfx_xoffset;
				columns4 = gfx_display_columns/4;
				skipcol2 = skipcolumns;
			}
			else /* depth == 16 */
			{
				xoffs = 2 * gfx_xoffset;
				columns4 = gfx_display_columns/2;
				skipcol2 = skipcolumns*2;
			}

			lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
			for (y = 0; y < gfx_display_lines; y++)
			{
				if (Machine->dirtylayer->dirtyline[(y+skiplines) >> 3] != 0)
				{
					address = bmp_write_line (screen, vesa_line) + xoffs;
					if (doubling != 0)
					{
						if (depth == 16)
							double_pixels16(lb,dest_seg,address,columns4);
						else
							double_pixels(lb,dest_seg,address,columns4);
						if (!scanlines)
						{
						   address = bmp_write_line (screen, vesa_line+1) + xoffs;
						   if (depth == 16)
							   double_pixels16(lb,dest_seg,address,columns4);
						   else
							   double_pixels(lb,dest_seg,address,columns4);
						}
					}
					else
						_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);
				}

				vesa_line++;
				if (doubling != 0) vesa_line++;
				lb+=width4;
			}
		}
	}
	else
	{
		if (gfx_mode==GFX_VGA)
		{
			int width,width4,y,columns4;
			unsigned long *lb, address;


			width = gfx_width;
			width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
			columns4 = gfx_display_columns/4;
			address = 0xa0000 + gfx_xoffset + gfx_yoffset * width;
			lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcolumns);
			for (y = 0; y < gfx_display_lines; y++)
			{
				if (!use_dirty || (dirty_new[y+skiplines]+dirty_old[y+skiplines]) != 0)
					_dosmemputl(lb,columns4,address);

				lb+=width4;
				address+=width;
			}
		}
		else
		{
			short src_seg, dest_seg;
			int y, vesa_line, width4, columns4, skipcol2;
			unsigned long *lb, address;
			int xoffs, depth;

			src_seg = _my_ds();
			dest_seg = screen->seg;
			vesa_line = gfx_yoffset;
			width4 = (scrbitmap->line[1] - scrbitmap->line[0]) / 4;
			depth = scrbitmap->depth;
			if (depth == 8)
			{
				xoffs = gfx_xoffset;
				columns4 = gfx_display_columns/4;
				skipcol2 = skipcolumns;
			}
			else /* depth == 16 */
			{
				xoffs = 2 * gfx_xoffset;
				columns4 = gfx_display_columns/2;
				skipcol2 = skipcolumns*2;
			}

			lb = (unsigned long *)(scrbitmap->line[skiplines] + skipcol2 );
			for (y = 0; y < gfx_display_lines; y++)
			{
				if (!use_dirty || (dirty_new[y+skiplines]+dirty_old[y+skiplines]) != 0)
				{
					address = bmp_write_line (screen, vesa_line) + xoffs;
					if (doubling != 0)
					{
						if (depth == 16)
							double_pixels16(lb,dest_seg,address,columns4);
						else
							double_pixels(lb,dest_seg,address,columns4);
						if (!scanlines)
						{
						   address = bmp_write_line (screen, vesa_line+1) + xoffs;
						   if (depth == 16)
							   double_pixels16(lb,dest_seg,address,columns4);
						   else
							   double_pixels(lb,dest_seg,address,columns4);
						}
					}
					else
						_movedatal(src_seg,(unsigned long)lb,dest_seg,address,columns4);
				}

				vesa_line++;
				if (doubling != 0) vesa_line++;
				lb+=width4;
			}
		}
	}
}

void clear_screen(void)
{
	unsigned char buf[MAX_GFX_WIDTH];


	memset(buf,current_background_color,gfx_width);

	if (gfx_mode==GFX_VGA)
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
			/* TODO: support clearing a 16-bit video mode */
			return;
		}
		for (y = 0; y < gfx_height; y++)
		{
			address = bmp_write_line (screen, y);
			_movedatal(src_seg,(unsigned long)buf,dest_seg,address,columns4);
		}
	}

	osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
}

static inline void pan_display(void)
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

/* this is optional and not required for porting */
static void save_screen(void)
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
	if (gfx_mode == GFX_VGA)
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

/* Update the display. */
void osd_update_display(void)
{
	int i;
	static float gamma_update = 0.00;
	static int showgammatemp;
	static int showfps,showfpstemp,f8pressed,f10pressed,f11pressed;
	uclock_t curr;
	#define MEMORY 10
	static uclock_t prev[MEMORY];
	static int memory,speed;
	extern int frameskip;
	static int vups,vfcount;
	int need_to_clear_bitmap = 0;



	/* Check for PGUP, PGDN and pan screen */
	if (osd_key_pressed(OSD_KEY_PGDN) || osd_key_pressed(OSD_KEY_PGUP))
		pan_display();


	if (osd_key_pressed(OSD_KEY_F8))
	{
		if (f8pressed == 0)
		{
			frameskip = (frameskip + 1) % 4;
			showfpstemp = 50;

			/* reset the frame counter every time the frameskip key is pressed, so */
			/* we'll measure the average FPS on a consistent status. */
			frames_displayed = 0;
		}
		f8pressed = 1;
	}
	else f8pressed = 0;

	if (osd_key_pressed(OSD_KEY_F10))
	{
		if (f10pressed == 0)
		{
			throttle ^= 1;

			/* reset the frame counter every time the throttle key is pressed, so */
			/* we'll measure the average FPS on a consistent status. */
			frames_displayed = 0;
		}
		f10pressed = 1;
	}
	else f10pressed = 0;

	if (osd_key_pressed(OSD_KEY_F11))
	{
		if (f11pressed == 0)
		{
			showfps ^= 1;
			if (showfps == 0)
			{
				need_to_clear_bitmap = 1;
			}
		}
		f11pressed = 1;
	}
	else f11pressed = 0;

	if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
	{
		showfpstemp--;
		if ((showfps == 0) && (showfpstemp == 0))
		{
			need_to_clear_bitmap = 1;
		}
	}

	/* now wait until it's time to update the screen */
	if (throttle)
	{
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
			do
			{
				curr = uclock();
			} while ((curr - prev[memory]) < (frameskip+1) * UCLOCKS_PER_SEC/Machine->drv->frames_per_second);
		}
	}
	else curr = uclock();


	/* for the FPS average calculation */
	if (++frames_displayed == FRAMES_TO_SKIP)
		start_time = curr;
	else
		end_time = curr;


	memory = (memory+1) % MEMORY;

	if (curr - prev[memory])
	{
		int divdr;


		divdr = Machine->drv->frames_per_second * (curr - prev[memory]) / (100 * MEMORY);
		speed = (UCLOCKS_PER_SEC * (frameskip+1) + divdr/2) / divdr;
	}

	prev[memory] = curr;

	vfcount += frameskip+1;
	if (vfcount >= Machine->drv->frames_per_second)
	{
		extern int vector_updates; /* avgdvg_go()'s per Mame frame, should be 1 */


		vfcount = 0;
		vups = vector_updates;
		vector_updates = 0;
	}

	if (showfps || showfpstemp) /* MAURY: nuove opzioni */
	{
		int trueorientation;
		int fps,l;
		char buf[30];


		/* hack: force the display into standard orientation to avoid */
		/* rotating the text */
		trueorientation = Machine->orientation;
		Machine->orientation = ORIENTATION_DEFAULT;

		fps = (Machine->drv->frames_per_second / (frameskip+1) * speed + 50) / 100;
		sprintf(buf," %3d%%(%3d/%d fps)",speed,fps,Machine->drv->frames_per_second);
		l = strlen(buf);
		for (i = 0;i < l;i++)
			drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,gfx_display_columns+skipcolumns-(l-i)*Machine->uifont->width,skiplines,0,TRANSPARENCY_NONE,0);
		if (vector_game)
		{
			sprintf(buf," %d vector updates",vups);
			l = strlen(buf);
			for (i = 0;i < l;i++)
				drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,gfx_display_columns+skipcolumns-(l-i)*Machine->uifont->width,skiplines+8,0,TRANSPARENCY_NONE,0);
		}

		Machine->orientation = trueorientation;
	}


	if (osd_key_pressed(OSD_KEY_F7))
		joy_calibration();


	if (scrbitmap->depth == 8)
	{
		if (osd_key_pressed(OSD_KEY_LSHIFT) &&
				(osd_key_pressed(OSD_KEY_PLUS_PAD) || osd_key_pressed(OSD_KEY_MINUS_PAD)))
		{
			for (i = 0;i < 256;i++) dirtycolor[i] = 1;
			dirtypalette = 1;

			if (osd_key_pressed(OSD_KEY_MINUS_PAD)) gamma_update -= 0.02;
			if (osd_key_pressed(OSD_KEY_PLUS_PAD)) gamma_update += 0.02;

			if (gamma_update < -0.09)
			{
				gamma_update = 0.00;
				gamma_correction -= 0.10;
			}
			if (gamma_update > 0.09)
			{
				gamma_update = 0.00;
				gamma_correction += 0.10;
			}

			if (gamma_correction < 0.2) gamma_correction = 0.2;
			if (gamma_correction > 3.0) gamma_correction = 3.0;

			showgammatemp = Machine->drv->frames_per_second;
		}

		if (showgammatemp > 0)
		{
			if (--showgammatemp)
			{
				char buf[20];

				sprintf(buf,"Gamma = %1.1f",gamma_correction);
				my_textout(buf);
			}
			else
			{
				need_to_clear_bitmap = 1;
			}
		}

		if (dirtypalette)
		{
			dirtypalette = 0;

			for (i = 0;i < 256;i++)
			{
				if (dirtycolor[i])
				{
					int r,g,b;


					dirtycolor[i] = 0;

					r = 255 * pow(current_palette[i][0] / 255.0, 1 / gamma_correction);
					g = 255 * pow(current_palette[i][1] / 255.0, 1 / gamma_correction);
					b = 255 * pow(current_palette[i][2] / 255.0, 1 / gamma_correction);

					adjusted_palette[i].r = r >> 2;
					adjusted_palette[i].g = g >> 2;
					adjusted_palette[i].b = b >> 2;

					set_color(i,&adjusted_palette[i]);
				}
			}

			if (current_palette[current_background_color][0] != 0 ||
					current_palette[current_background_color][1] != 0 ||
					current_palette[current_background_color][2] != 0)
			{
				/* start from pen 255 (which is the game's color 0 */
				/* because we allocated pens backwards) */
				for (i = 255;i >= 0;i--)
				{
					if (current_palette[i][0] == 0 &&
							current_palette[i][1] == 0 &&
							current_palette[i][2] == 0)
					{
						current_background_color = i;
						break;
					}
				}

				if (i >= 0)
				{
					/* update the background areas of the screen to the new color */
					clear_screen();

					inportb(STATUS_ADDR);  		/* reset read/write flip-flop */
					outportb(ATTRCON_ADDR, 0x11 | 0x20);
					outportb(ATTRCON_ADDR, current_background_color);
				}
			}
		}
	}


	/* copy the bitmap to screen memory */
	update_screen();

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

	/* if the user pressed F12, save a snapshot of the screen. */
	if (osd_key_pressed(OSD_KEY_F12))
	{
		save_screen();
		/* wait for the user to release F12 */
		while (osd_key_pressed(OSD_KEY_F12));
	}
}
