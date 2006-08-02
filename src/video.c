/***************************************************************************

    video.c

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "png.h"
#include "debugger.h"
#include "vidhrdw/vector.h"
#include "render.h"
#include "ui.h"

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
#include "mamedbg.h"
#endif


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG_PARTIAL_UPDATES(x)		/* logerror x */

#define FRAMES_PER_FPS_UPDATE		12

/* VERY IMPORTANT: bitmap_alloc must allocate also a "safety area" 16 pixels wide all
   around the bitmap. This is required because, for performance reasons, some graphic
   routines don't clip at boundaries of the bitmap. */
#define BITMAP_SAFETY				16



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _internal_screen_info internal_screen_info;
struct _internal_screen_info
{
	render_texture *	texture;
	int					format;
	int					changed;
	render_bounds		bounds;
	mame_bitmap *		bitmap[2];
	int					curbitmap;
	int					last_partial_scanline;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* handy globals for other parts of the system */
int vector_updates = 0;

/* main bitmap to render to */
static int skipping_this_frame;
static internal_screen_info scrinfo[MAX_SCREENS];

/* speed computation */
static cycles_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static performance_info performance;

/* movie file */
static mame_file *movie_file = NULL;
static int movie_frame = 0;

/* misc other statics */
static UINT32 leds_status;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void video_exit(void);
static int allocate_graphics(const gfx_decode *gfxdecodeinfo);
static void decode_graphics(const gfx_decode *gfxdecodeinfo);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);
static void recompute_fps(int skipped_it);
static void recompute_visible_areas(void);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    video_init - start up the video system
-------------------------------------------------*/

int video_init(void)
{
	int scrnum;

	memset(scrinfo, 0, sizeof(scrinfo));
	movie_file = NULL;
	movie_frame = 0;

	add_exit_callback(video_exit);

	/* configure all of the screens */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			screen_state *state = &Machine->screen[scrnum];
			configure_screen(scrnum, state->width, state->height, &state->visarea, state->refresh);
		}

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			return 1;

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo != NULL)
		if (allocate_graphics(Machine->drv->gfxdecodeinfo))
			return 1;

	/* configure the palette */
	palette_config();

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* actually decode the graphics */
	if (Machine->drv->gfxdecodeinfo != NULL)
		decode_graphics(Machine->drv->gfxdecodeinfo);

	/* reset performance data */
	last_fps_time = osd_cycles();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = Machine->screen[0].refresh;
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	/* initialize tilemaps */
	if (tilemap_init() != 0)
		fatalerror("tilemap_init failed");

//  recompute_visible_areas();
	return 0;
}


/*-------------------------------------------------
    video_exit - close down the video system
-------------------------------------------------*/

static void video_exit(void)
{
	int scrnum;
	int i;

	/* stop recording any movie */
	record_movie_stop();

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

	/* free all the textures and bitmaps */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
	{
		internal_screen_info *info = &scrinfo[scrnum];
		if (info->texture != NULL)
			render_texture_free(info->texture);
		if (info->bitmap[0] != NULL)
			bitmap_free(info->bitmap[0]);
		if (info->bitmap[1] != NULL)
			bitmap_free(info->bitmap[1]);
	}
}


/*-------------------------------------------------
    allocate_graphics - allocate memory for the
    graphics
-------------------------------------------------*/

static int allocate_graphics(const gfx_decode *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT32 extxoffs[MAX_ABS_GFX_SIZE], extyoffs[MAX_ABS_GFX_SIZE];
		gfx_layout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;
		if (glcopy.extxoffs)
		{
			memcpy(extxoffs, glcopy.extxoffs, glcopy.width * sizeof(extxoffs[0]));
			glcopy.extxoffs = extxoffs;
		}
		if (glcopy.extyoffs)
		{
			memcpy(extyoffs, glcopy.extyoffs, glcopy.height * sizeof(extyoffs[0]));
			glcopy.extyoffs = extyoffs;
		}

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* for non-raw graphics, decode the X and Y offsets */
		if (glcopy.planeoffset[0] != GFX_RAW)
		{
			UINT32 *xoffset = glcopy.extxoffs ? extxoffs : glcopy.xoffset;
			UINT32 *yoffset = glcopy.extyoffs ? extyoffs : glcopy.yoffset;

			/* loop over all the planes, converting fractions */
			for (j = 0; j < glcopy.planes; j++)
			{
				UINT32 value = glcopy.planeoffset[j];
				if (IS_FRAC(value))
					glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			/* loop over all the X/Y offsets, converting fractions */
			for (j = 0; j < glcopy.width; j++)
			{
				UINT32 value = xoffset[j];
				if (IS_FRAC(value))
					xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			for (j = 0; j < glcopy.height; j++)
			{
				UINT32 value = yoffset[j];
				if (IS_FRAC(value))
					yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}
		}

		/* otherwise, just use yoffset[0] as the line modulo */
		else
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * glcopy.yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* allocate the graphics */
		Machine->gfx[i] = allocgfx(&glcopy);

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}


/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static void decode_graphics(const gfx_decode *gfxdecodeinfo)
{
	int totalgfx = 0, curgfx = 0;
	char buffer[200];
	int i;

	/* count total graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
			totalgfx += Machine->gfx[i]->total_elements;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
		{
			/* if we have a valid region, decode it now */
			if (gfxdecodeinfo[i].memory_region > REGION_INVALID)
			{
				UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
				gfx_element *gfx = Machine->gfx[i];
				int j;

				/* now decode the actual graphics */
				for (j = 0; j < gfx->total_elements; j += 1024)
				{
					int num_to_decode = (j + 1024 < gfx->total_elements) ? 1024 : (gfx->total_elements - j);
					decodegfx(gfx, region_base + gfxdecodeinfo[i].start, j, num_to_decode);
					curgfx += num_to_decode;

					/* display some startup text */
					sprintf(buffer, "Decoding (%d%%)", curgfx * 100 / totalgfx);
					ui_set_startup_text(buffer, FALSE);
				}
			}

			/* otherwise, clear the target region */
			else
				memset(Machine->gfx[i]->gfxdata, 0, Machine->gfx[i]->char_modulo * Machine->gfx[i]->total_elements);
		}
}


/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		logerror("video_init():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);

	/* register for saving it */
	state_save_register_global_pointer(buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);

		/* register for saving it */
		state_save_register_global_pointer(buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (UINT16 *)buffered_spriteram;
	buffered_spriteram32 = (UINT32 *)buffered_spriteram;
	buffered_spriteram16_2 = (UINT16 *)buffered_spriteram_2;
	buffered_spriteram32_2 = (UINT32 *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************
    SCREEN RENDERING
***************************************************************************/

/*-------------------------------------------------
    configure_screen - configure the parameters
    of a screen
-------------------------------------------------*/

void configure_screen(int scrnum, int width, int height, const rectangle *visarea, float refresh)
{
	const screen_config *config = &Machine->drv->screen[scrnum];
	screen_state *state = &Machine->screen[scrnum];
	internal_screen_info *info = &scrinfo[scrnum];

	/* reallocate bitmap if necessary */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		int curwidth = 0, curheight = 0;

		/* reality checks */
		if (visarea->min_x < 0 || visarea->min_y < 0 || visarea->max_x >= width || visarea->max_y >= height)
			fatalerror("configure_screen(): visible area must be contained within the width/height!");

		/* extract the current width/height from the bitmap */
		if (info->bitmap[0] != NULL)
		{
			curwidth = info->bitmap[0]->width;
			curheight = info->bitmap[0]->height;
		}

		/* if we're too small to contain this width/height, reallocate our bitmaps and textures */
		if (width > curwidth || height > curheight)
		{
			/* free what we have currently */
			if (info->texture != NULL)
				render_texture_free(info->texture);
			if (info->bitmap[0] != NULL)
				bitmap_free(info->bitmap[0]);
			if (info->bitmap[1] != NULL)
				bitmap_free(info->bitmap[1]);

			/* compute new width/height */
			curwidth = MAX(width, curwidth);
			curheight = MAX(height, curheight);

			/* choose the texture format */
			if (Machine->color_depth == 16)
				info->format = TEXFORMAT_PALETTE16;
			else if (Machine->color_depth == 15)
				info->format = TEXFORMAT_RGB15;
			else
				info->format = TEXFORMAT_RGB32;

			/* allocate new stuff */
			info->bitmap[0] = bitmap_alloc_depth(curwidth, curheight, Machine->color_depth);
			info->bitmap[1] = bitmap_alloc_depth(curwidth, curheight, Machine->color_depth);
			info->texture = render_texture_alloc(info->bitmap[0], visarea, &adjusted_palette[config->palette_base], info->format, NULL, NULL);
		}
	}

	/* now fill in the new parameters */
	state->width = width;
	state->height = height;
	state->visarea = *visarea;
	state->refresh = refresh;

	/* TO DO: recompute the VBLANK timing */

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}


/*-------------------------------------------------
    set_visible_area - just set the visible area
    of a screen
-------------------------------------------------*/

void set_visible_area(int scrnum, int min_x, int max_x, int min_y, int max_y)
{
	screen_state *state = &Machine->screen[scrnum];
	rectangle visarea;

	visarea.min_x = min_x;
	visarea.max_x = max_x;
	visarea.min_y = min_y;
	visarea.max_y = max_y;

	configure_screen(scrnum, state->width, state->height, &visarea, state->refresh);
}


/*-------------------------------------------------
    force_partial_update - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scrnum, int scanline)
{
	internal_screen_info *screen = &scrinfo[scrnum];
	rectangle clip = Machine->screen[scrnum].visarea;

	LOG_PARTIAL_UPDATES(("Partial: force_partial_update(%d,%d): ", scrnum, scanline));

	/* if skipping this frame, bail */
	if (skip_this_frame())
	{
		LOG_PARTIAL_UPDATES(("skipped due to frameskipping\n"));
		return;
	}

	/* skip if less than the lowest so far */
	if (scanline < screen->last_partial_scanline)
	{
		LOG_PARTIAL_UPDATES(("skipped because less than previous\n"));
		return;
	}

	/* skip if this screen is not visible anywhere */
	if (!(render_get_live_screens_mask() & (1 << scrnum)))
	{
		LOG_PARTIAL_UPDATES(("skipped because screen not live\n"));
		return;
	}

	/* set the start/end scanlines */
	if (screen->last_partial_scanline > clip.min_y)
		clip.min_y = screen->last_partial_scanline;
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		UINT32 flags;

		profiler_mark(PROFILER_VIDEO);
		LOG_PARTIAL_UPDATES(("updating %d-%d\n", clip.min_y, clip.max_y));
		flags = (*Machine->drv->video_update)(scrnum, screen->bitmap[screen->curbitmap], &clip);
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);

		/* if we modified the bitmap, we have to commit */
		screen->changed |= (~flags & UPDATE_HAS_NOT_CHANGED);
	}

	/* remember where we left off */
	screen->last_partial_scanline = scanline + 1;
}


/*-------------------------------------------------
    reset_partial_updates - reset partial updates
    at the start of each frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	int scrnum;

	/* reset partial updates */
	LOG_PARTIAL_UPDATES(("Partial: reset to 0\n"));
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		scrinfo[scrnum].last_partial_scanline = 0;
	performance.partial_updates_this_frame = 0;
}


/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

static void recompute_fps(int skipped_it)
{
	/* increment the frame counters */
	frames_since_last_fps++;
	if (!skipped_it)
		rendered_frames_since_last_fps++;

	/* if we didn't skip this frame, we may be able to compute a new FPS */
	if (!skipped_it && frames_since_last_fps >= FRAMES_PER_FPS_UPDATE)
	{
		cycles_t cps = osd_cycles_per_second();
		cycles_t curr = osd_cycles();
		double seconds_elapsed = (double)(curr - last_fps_time) * (1.0 / (double)cps);
		double frames_per_sec = (double)frames_since_last_fps / seconds_elapsed;

		/* compute the performance data */
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->screen[0].refresh;
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->screen[0].refresh)
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->screen[0].refresh;
	}
}


/*-------------------------------------------------
    video_frame_update - handle frameskipping and
    UI, plus updating the screen during normal
    operations
-------------------------------------------------*/

void video_frame_update(void)
{
	int skipped_it = skip_this_frame();
	int paused = mame_is_paused();
	int phase = mame_get_phase();
	int livemask;
	int scrnum;

	/* only render sound and video if we're in the running phase */
	if (phase == MAME_PHASE_RUNNING)
	{
		/* update sound */
		sound_frame_update();

		/* finish updating the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (Machine->drv->screen[scrnum].tag != NULL)
				force_partial_update(scrnum, Machine->screen[scrnum].visarea.max_y);

		/* update our movie recording state */
		if (!paused)
			record_movie_frame(0);

		/* now add the quads for all the screens */
		livemask = render_get_live_screens_mask();
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (livemask & (1 << scrnum))
			{
				internal_screen_info *screen = &scrinfo[scrnum];

				/* only update if empty and not a vector game; otherwise assume the driver did it directly */
				if (render_container_is_empty(render_container_get_screen(scrnum)) && !(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
				{
					mame_bitmap *bitmap = screen->bitmap[screen->curbitmap];
					if (!skipping_this_frame && screen->changed)
					{
						rectangle fixedvis = Machine->screen[scrnum].visarea;
						fixedvis.max_x++;
						fixedvis.max_y++;
						render_texture_set_bitmap(screen->texture, bitmap, &fixedvis, &adjusted_palette[Machine->drv->screen[scrnum].palette_base], screen->format);
						screen->curbitmap = 1 - screen->curbitmap;
					}
					render_screen_add_quad(scrnum, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), screen->texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_SCREENTEX(1));
				}
			}

		/* reset the screen changed flags */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			scrinfo[scrnum].changed = 0;
	}

	/* draw the user interface */
	ui_update_and_render();

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	debug_trace_delay = 0;
#endif

	/* call the OSD to update */
	skipping_this_frame = osd_update(mame_timer_get_time());

	/* empty the containers */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
			render_container_empty(render_container_get_screen(scrnum));

	/* update FPS */
	recompute_fps(skipped_it);

	/* call the end-of-frame callback */
	if (phase == MAME_PHASE_RUNNING)
	{
		if (Machine->drv->video_eof && !paused)
		{
			profiler_mark(PROFILER_VIDEO);
			(*Machine->drv->video_eof)();
			profiler_mark(PROFILER_END);
		}

		/* reset partial updates if we're paused or if the debugger is active */
		if (paused || mame_debug_is_active())
			reset_partial_updates();

		/* recompute visible areas */
//      recompute_visible_areas();
	}
}


/*-------------------------------------------------
    recompute_visible_areas - determine the
    effective visible areas and screen bounds
-------------------------------------------------*/

#if 0
static void recompute_visible_areas(void)
{
	int scrnum;

	/* iterate over live screens */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			internal_screen_info *screen = &scrinfo[scrnum];
			render_container *scrcontainer = render_container_get_screen(scrnum);
			float xoffs = render_container_get_xoffset(scrcontainer);
			float yoffs = render_container_get_yoffset(scrcontainer);
			float xscale = render_container_get_xscale(scrcontainer);
			float yscale = render_container_get_yscale(scrcontainer);
			rectangle visarea = Machine->screen[scrnum].visarea;
			mame_bitmap *bitmap = screen->bitmap[screen->curbitmap];
			float viswidth, visheight;
			float x0, y0, x1, y1;
			float xrecip, yrecip;

			/* adjust the max values so they are exclusive rather than inclusive */
			visarea.max_x++;
			visarea.max_y++;

			/* based on the game-configured visible area, compute the bounds we will draw
                the screen at so that a clipping at (0,0)-(1,1) will exactly result in
                the requested visible area */
			viswidth = (float)(visarea.max_x - visarea.min_x);
			visheight = (float)(visarea.max_y - visarea.min_y);
			xrecip = 1.0f / viswidth;
			yrecip = 1.0f / visheight;
			screen->bounds.x0 = 0.0f - (float)(visarea.min_x -  * xrecip;
			screen->bounds.x1 = 1.0f + (float)(bitmap->width - visarea.max_x) * xrecip;
			screen->bounds.y0 = 0.0f - (float)visarea.min_y * yrecip;
			screen->bounds.y1 = 1.0f + (float)(bitmap->height - visarea.max_y) * yrecip;

			/* now apply the scaling/offset to the scrbounds */
			x0 = (0.5f - 0.5f * xscale + xoffs) + xscale * screen->bounds.x0;
			x1 = (0.5f - 0.5f * xscale + xoffs) + xscale * screen->bounds.x1;
			y0 = (0.5f - 0.5f * yscale + yoffs) + yscale * screen->bounds.y0;
			y1 = (0.5f - 0.5f * yscale + yoffs) + yscale * screen->bounds.y1;

			/* scale these values by the texture size */
			screen->eff_visible_area.min_x = floor((0.0f - x0) * viswidth);
			screen->eff_visible_area.max_x = bitmap->width - floor((x1 - 1.0f) * viswidth);
			screen->eff_visible_area.min_y = floor((0.0f - y0) * visheight);
			screen->eff_visible_area.max_y = bitmap->height - floor((y1 - 1.0f) * visheight);

			/* clamp against the width/height of the bitmaps */
			if (screen->eff_visible_area.min_x < 0) screen->eff_visible_area.min_x = 0;
			if (screen->eff_visible_area.max_x >= bitmap->width) screen->eff_visible_area.max_x = bitmap->width - 1;
			if (screen->eff_visible_area.min_y < 0) screen->eff_visible_area.min_y = 0;
			if (screen->eff_visible_area.max_y >= bitmap->height) screen->eff_visible_area.max_y = bitmap->height - 1;

			/* union this with the actual visible_area in case any game drivers rely
                on it */
			union_rect(&screen->eff_visible_area, &Machine->screen[scrnum].visarea);
		}
}
#endif


/*-------------------------------------------------
    skip_this_frame - accessor to determine if this
    frame is being skipped
-------------------------------------------------*/

int skip_this_frame(void)
{
	return skipping_this_frame;
}


/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const performance_info *mame_get_performance_info(void)
{
	return &performance;
}



/***************************************************************************
    SCREEN SNAPSHOTS/MOVIES
***************************************************************************/

/*-------------------------------------------------
    rotate_snapshot - rotate the snapshot in
    accordance with the orientation
-------------------------------------------------*/

static mame_bitmap *rotate_snapshot(mame_bitmap *bitmap, int orientation, rectangle *bounds)
{
	rectangle newbounds;
	mame_bitmap *copy;
	int x, y, w, h, t;

	/* if we can send it in raw, no need to override anything */
	if (orientation == 0)
		return bitmap;

	/* allocate a copy */
	w = (orientation & ORIENTATION_SWAP_XY) ? bitmap->height : bitmap->width;
	h = (orientation & ORIENTATION_SWAP_XY) ? bitmap->width : bitmap->height;
	copy = auto_bitmap_alloc_depth(w, h, bitmap->depth);

	/* populate the copy */
	for (y = bounds->min_y; y <= bounds->max_y; y++)
		for (x = bounds->min_x; x <= bounds->max_x; x++)
		{
			int tx = x, ty = y;

			/* apply the rotation/flipping */
			if ((orientation & ORIENTATION_SWAP_XY))
			{
				t = tx; tx = ty; ty = t;
			}
			if (orientation & ORIENTATION_FLIP_X)
				tx = copy->width - tx - 1;
			if (orientation & ORIENTATION_FLIP_Y)
				ty = copy->height - ty - 1;

			/* read the old pixel and copy to the new location */
			switch (copy->depth)
			{
				case 15:
				case 16:
					*((UINT16 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT16 *)bitmap->base + y * bitmap->rowpixels + x);
					break;

				case 32:
					*((UINT32 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT32 *)bitmap->base + y * bitmap->rowpixels + x);
					break;
			}
		}

	/* compute the oriented bounds */
	newbounds = *bounds;

	/* apply X/Y swap first */
	if (orientation & ORIENTATION_SWAP_XY)
	{
		t = newbounds.min_x; newbounds.min_x = newbounds.min_y; newbounds.min_y = t;
		t = newbounds.max_x; newbounds.max_x = newbounds.max_y; newbounds.max_y = t;
	}

	/* apply X flip */
	if (orientation & ORIENTATION_FLIP_X)
	{
		t = copy->width - newbounds.min_x - 1;
		newbounds.min_x = copy->width - newbounds.max_x - 1;
		newbounds.max_x = t;
	}

	/* apply Y flip */
	if (orientation & ORIENTATION_FLIP_Y)
	{
		t = copy->height - newbounds.min_y - 1;
		newbounds.min_y = copy->height - newbounds.max_y - 1;
		newbounds.max_y = t;
	}

	*bounds = newbounds;
	return copy;
}


/*-------------------------------------------------
    save_frame_with - save a frame with a
    given handler for screenshots and movies
-------------------------------------------------*/

static void save_frame_with(mame_file *fp, int scrnum, int (*write_handler)(mame_file *, mame_bitmap *))
{
	mame_bitmap *bitmap;
	int orientation;
	rectangle bounds;

	assert((scrnum >= 0) && (scrnum < MAX_SCREENS));

	bitmap = scrinfo[scrnum].bitmap[scrinfo[scrnum].curbitmap];
	assert(bitmap != NULL);

	orientation = render_container_get_orientation(render_container_get_screen(scrnum));

	begin_resource_tracking();

	/* allow the artwork system to override certain parameters */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		bounds.min_x = 0;
		bounds.max_x = bitmap->width - 1;
		bounds.min_y = 0;
		bounds.max_y = bitmap->height - 1;
	}
	else
	{
		bounds = Machine->screen[0].visarea;
	}

	/* rotate the snapshot, if necessary */
	bitmap = rotate_snapshot(bitmap, orientation, &bounds);

	/* now do the actual work */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		write_handler(fp, bitmap);
	}
	else
	{
		mame_bitmap *copy;
		int sizex, sizey;

		sizex = bounds.max_x - bounds.min_x + 1;
		sizey = bounds.max_y - bounds.min_y + 1;

		copy = bitmap_alloc_depth(sizex,sizey,bitmap->depth);
		if (copy)
		{
			int x,y,sx,sy;

			sx = bounds.min_x;
			sy = bounds.min_y;

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			write_handler(fp, copy);
			bitmap_free(copy);
		}
	}

	end_resource_tracking();
}


/*-------------------------------------------------
    snapshot_save_screen_indexed - save a snapshot to
    the given file handle
-------------------------------------------------*/

void snapshot_save_screen_indexed(mame_file *fp, int scrnum)
{
	save_frame_with(fp, scrnum, png_write_bitmap);
}


/*-------------------------------------------------
    open the next non-existing file of type
    filetype according to our numbering scheme
-------------------------------------------------*/

static mame_file *mame_fopen_next(int filetype)
{
	char name[FILENAME_MAX];
	int seq;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.xxx" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (mame_faccess(name, filetype))
	{
		seq = 0;
		do
		{
			/* otherwise use "nameNNNN.xxx" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name, seq++);
		} while (mame_faccess(name, filetype));
	}

    return (mame_fopen(Machine->gamedrv->name, name, filetype, 1));
}


/*-------------------------------------------------
    snapshot_save_all_screens - save a snapshot.
-------------------------------------------------*/

void snapshot_save_all_screens(void)
{
	UINT32 screenmask = render_get_live_screens_mask();
	mame_file *fp;
	int scrnum;

	/* write one snapshot per visible screen */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (screenmask & (1 << scrnum))
			if ((fp = mame_fopen_next(FILETYPE_SCREENSHOT)) != NULL)
			{
				snapshot_save_screen_indexed(fp, scrnum);
				mame_fclose(fp);
			}
}


/*-------------------------------------------------
    record_movie - start, stop and update the
    recording of a MNG movie
-------------------------------------------------*/

void record_movie_start(const char *name)
{
	if (movie_file != NULL)
		mame_fclose(movie_file);

	if (name)
		movie_file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MOVIE, 1);
	else
		movie_file = mame_fopen_next(FILETYPE_MOVIE);

	movie_frame = 0;
}


void record_movie_stop(void)
{
	if (movie_file)
	{
		mng_capture_stop(movie_file);
		mame_fclose(movie_file);
		movie_file = NULL;
	}
}


void record_movie_toggle(void)
{
	if (movie_file == NULL)
	{
		record_movie_start(NULL);
		if (movie_file)
			ui_popup("REC START");
	}
	else
	{
		record_movie_stop();
		ui_popup("REC STOP (%d frames)", movie_frame);
	}
}


void record_movie_frame(int scrnum)
{
	if (movie_file != NULL)
	{
		profiler_mark(PROFILER_MOVIE_REC);

		if (movie_frame++ == 0)
			save_frame_with(movie_file, scrnum, mng_capture_start);
		save_frame_with(movie_file, scrnum, mng_capture_frame);

		profiler_mark(PROFILER_END);
	}
}



/***************************************************************************
    BITMAP MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    pp_* -- pixel plotting callbacks
-------------------------------------------------*/

static void pp_8 (mame_bitmap *b, int x, int y, pen_t p)  { ((UINT8 *)b->line[y])[x] = p; }
static void pp_16(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT16 *)b->line[y])[x] = p; }
static void pp_32(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT32 *)b->line[y])[x] = p; }


/*-------------------------------------------------
    rp_* -- pixel reading callbacks
-------------------------------------------------*/

static pen_t rp_8 (mame_bitmap *b, int x, int y)  { return ((UINT8 *)b->line[y])[x]; }
static pen_t rp_16(mame_bitmap *b, int x, int y)  { return ((UINT16 *)b->line[y])[x]; }
static pen_t rp_32(mame_bitmap *b, int x, int y)  { return ((UINT32 *)b->line[y])[x]; }


/*-------------------------------------------------
    pb_* -- box plotting callbacks
-------------------------------------------------*/

static void pb_8 (mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT8 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_16(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT16 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_32(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT32 *)b->line[y])[x] = p; x++; } y++; } }


/*-------------------------------------------------
    bitmap_alloc_core
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(mame_bitmap)) : malloc(sizeof(mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		UINT8 *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(UINT8 *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (UINT8 *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		if (pixelsize == 1)
		{
			bitmap->read = rp_8;
			bitmap->plot = pp_8;
			bitmap->plot_box = pb_8;
		}
		else if (pixelsize == 2)
		{
			bitmap->read = rp_16;
			bitmap->plot = pp_16;
			bitmap->plot_box = pb_16;
		}
		else
		{
			bitmap->read = rp_32;
			bitmap->plot = pp_32;
			bitmap->plot_box = pb_32;
		}
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, FALSE);
}


/*-------------------------------------------------
    auto_bitmap_alloc_depth - allocate a bitmap
    for a specific depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, TRUE);
}


/*-------------------------------------------------
    bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}
