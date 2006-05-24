//============================================================
//
//  video.c - Win32 video handling
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// needed for multimonitor
#define _WIN32_WINNT 0x501

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// standard C headers
#include <math.h>

// Windows 95/NT4 multimonitor stubs
#ifdef WIN95_MULTIMON
#include "multidef.h"
#endif

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "vidhrdw/vector.h"

#ifdef NEW_RENDER
#include "render.h"
#endif

// MAMEOS headers
#include "blit.h"
#include "video.h"
#include "window.h"
#include "rc.h"
#include "input.h"

#ifdef NEW_DEBUGGER
#include "debugwin.h"
#endif

#ifdef MESS
#include "menu.h"
#endif


//============================================================
//  IMPORTS
//============================================================

// from input.c
extern int verbose;

// from sound.c
extern void sound_update_refresh_rate(float newrate);

// from wind3dfx.c
extern struct rc_option win_d3d_opts[];



//============================================================
//  CONSTANTS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS			12

// refresh rate while paused
#define PAUSED_REFRESH_RATE			60


//============================================================
//  TYPE DEFINITIONS
//============================================================


//============================================================
//  GLOBAL VARIABLES
//============================================================

// screen info
char *screen_name;
char *layout_name;

int video_orientation;

// speed throttling
int throttle = 1;

// current frameskip/autoframeskip settings
static int frameskip;
static int autoframeskip;



//============================================================
//  LOCAL VARIABLES
//============================================================

// monitor info
win_monitor_info *win_monitor_list;
static win_monitor_info *primary_monitor;

// options decoding
static char *cleanstretch;
static char *resolution;
static char *aspect;
static char *mngwrite;

static int win_gfx_width, win_gfx_height;
int win_start_maximized;

// timing measurements for throttling
static int allow_sleep;

// average FPS calculation
static cycles_t fps_start_time;
static cycles_t fps_end_time;
static int fps_frames_displayed;
static int frames_to_run;

// throttling calculations
static cycles_t throttle_last_cycles;
static mame_time throttle_realtime;
static mame_time throttle_emutime;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// frameskipping tables
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



//============================================================
//  PROTOTYPES
//============================================================

static void video_exit(void);
static void init_monitors(void);
static BOOL CALLBACK monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data);

static int decode_cleanstretch(struct rc_option *option, const char *arg, int priority);
static int decode_resolution(struct rc_option *option, const char *arg, int priority);
static int decode_ftr(struct rc_option *option, const char *arg, int priority);
static int decode_aspect(struct rc_option *option, const char *arg, int priority);
static void update_throttle(mame_time emutime);
static void update_fps(mame_time emutime);
static void update_autoframeskip(void);
static void check_osd_inputs(void);



//============================================================
//  OPTIONS
//============================================================

// options struct
struct rc_option video_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },

	// performance options
	{ "autoframeskip", "afs", rc_bool, &autoframeskip, "1", 0, 0, NULL, "skip frames to speed up emulation" },
	{ "frameskip", "fs", rc_int, &frameskip, "0", 0, FRAMESKIP_LEVELS, NULL, "set frameskip explicitly (autoframeskip needs to be off)" },
	{ "throttle", NULL, rc_bool, &throttle, "1", 0, 0, NULL, "throttle speed to the game's framerate" },
	{ "sleep", NULL, rc_bool, &allow_sleep, "1", 0, 0, NULL, "allow " APPNAME " to give back time to the system when it's not needed" },
	{ "rdtsc", NULL, rc_bool, &win_force_rdtsc, "0", 0, 0, NULL, "prefer RDTSC over QueryPerformanceCounter for timing" },
	{ "priority", NULL, rc_int, &win_priority, "0", -15, 1, NULL, "thread priority" },

	// per-window positioning/layout options
	{ "window", "w", rc_bool, &win_window_mode, "0", 0, 0, NULL, "run in a window/run on full screen" },
	{ "screen", NULL, rc_string, &screen_name, NULL, 0, 0, NULL, "specify which screen to use" },
	{ "layout", NULL, rc_string, &layout_name, NULL, 0, 0, NULL, "specify which [file:]layout to use for the screen" },
	{ "resolution", "r", rc_string, &resolution, "auto", 0, 0, decode_resolution, "set resolution" },
	{ "screen_aspect", NULL, rc_string, &aspect, "4:3", 0, 0, decode_aspect, "specify an alternate monitor aspect ratio" },

	// global sizing options
	{ "cleanstretch", "cs", rc_string, &cleanstretch, "auto", 0, 0, decode_cleanstretch, "stretch to integer ratios" },
	{ "keepaspect", "ka", rc_bool, &win_keep_aspect, "1", 0, 0, NULL, "enforce aspect ratio" },
	{ "maximize", "max", rc_bool, &win_start_maximized, "1", 0, 0, NULL, "start out maximized" },
	{ "scanlines", "sl", rc_bool, &win_old_scanlines, "0", 0, 0, NULL, "emulate win_old_scanlines" },

	// misc other options
	{ "frames_to_run", "ftr", rc_int, &frames_to_run, "0", 0, 0, decode_ftr, "sets the number of frames to run within the game" },
	{ "mngwrite", NULL, rc_string, &mngwrite, NULL, 0, 0, NULL, "save video in specified mng file" },

	// ddraw/d3d options
	{ "waitvsync", NULL, rc_bool, &win_wait_vsync, "0", 0, 0, NULL, "wait for vertical sync (reduces tearing)"},
	{ "matchrefresh", NULL, rc_bool, &win_match_refresh, "0", 0, 0, NULL, "attempt to match the game's refresh rate" },
	{ "syncrefresh", NULL, rc_bool, &win_sync_refresh, "0", 0, 0, NULL, "syncronize only to the monitor refresh" },
	{ "triplebuffer", "tb", rc_bool, &win_triple_buffer, "0", 0, 0, NULL, "triple buffering (only if fullscreen)" },
	{ "refresh", NULL, rc_int, &win_gfx_refresh, "0", 0, 0, NULL, "set specific monitor refresh rate" },
	{ "switchres", NULL, rc_bool, &win_switch_res, "0", 0, 0, NULL, "switch resolutions to best fit" },
	{ "full_screen_gamma", "fsg", rc_float, &win_gfx_gamma, "1.0", 0.0, 4.0, NULL, "sets the gamma in full screen mode" },

	// d3d options
	{ "direct3d", "d3d", rc_bool, &win_use_d3d, "1", 0, 0, NULL, "use Direct3D for rendering" },
#ifndef NEW_RENDER
	{ NULL, NULL, rc_link, win_d3d_opts, NULL, 0, 0, NULL, NULL },
#endif
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//  winvideo_init
//============================================================

int winvideo_init(void)
{
	int index;

	// ensure we get called on the way out
	add_exit_callback(video_exit);

	// set up monitors first
	init_monitors();

#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options.mame_debug)
		win_window_mode = TRUE;
#endif

	// initialize the window system so we can make windows
	if (win_window_init())
		goto error;

	// create the windows (todo: how many windows?)
	for (index = 0; index < 1; index++)
	{
		win_monitor_info *monitor = NULL;
		char *layout = NULL;
		char *view = NULL;

		// find the monitor for this window
		if (screen_name != NULL)
			for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
				if (strcmp(screen_name, monitor->info.szDevice) == 0)
					break;
		if (monitor == NULL)
			monitor = primary_monitor;

		// extract the layout name and file
		if (layout_name != NULL)
		{
			layout = layout_name;
			view = strchr(layout, ':');
			if (view != NULL)
				*view++ = 0;
		}

		// create the window
		if (win_window_create(monitor, layout, view, win_gfx_width, win_gfx_height, win_switch_res))
			goto error;
	}

	// possibly create the debug window, but don't show it yet
#ifdef MAME_DEBUG
	if (options.mame_debug)
		if (debugwin_init_windows())
			return 1;
#endif

	// start recording movie
	if (mngwrite != NULL)
		record_movie_start(mngwrite);

	return 0;

error:
	return 1;
}



//============================================================
//  video_exit
//============================================================

static void video_exit(void)
{
	// possibly kill the debug window
#ifdef MAME_DEBUG
	if (options.mame_debug)
		debugwin_destroy_windows();
#endif

	// free all of our monitor information
	while (win_monitor_list != NULL)
	{
		win_monitor_info *temp = win_monitor_list;
		win_monitor_list = temp->next;
		free(temp);
	}
}



//============================================================
//  winvideo_monitor_refresh
//============================================================

void winvideo_monitor_refresh(win_monitor_info *monitor)
{
	HRESULT result;

	// fetch the latest info about the monitor
	monitor->info.cbSize = sizeof(monitor->info);
	result = GetMonitorInfo(monitor->handle, (LPMONITORINFO)&monitor->info);
	assert(result);
}



//============================================================
//  winvideo_monitor_get_aspect
//============================================================

float winvideo_monitor_get_aspect(win_monitor_info *monitor)
{
	// refresh the monitor information and compute the aspect
	int width, height;
	winvideo_monitor_refresh(monitor);
	width = rect_width(&monitor->info.rcMonitor);
	height = rect_height(&monitor->info.rcMonitor);
	return ((float)width / (float)height) / monitor->aspect;
}



//============================================================
//  winvideo_monitor_from_handle
//============================================================

win_monitor_info *winvideo_monitor_from_handle(HMONITOR hmonitor)
{
	win_monitor_info *monitor;

	// find the matching monitor
	for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		if (monitor->handle == hmonitor)
			return monitor;
	return NULL;
}



//============================================================
//  winvideo_set_frameskip
//============================================================

void winvideo_set_frameskip(int value)
{
	if (value >= 0)
	{
		frameskip = value;
		autoframeskip = 0;
	}
	else
	{
		frameskip = 0;
		autoframeskip = 1;
	}
}



//============================================================
//  winvideo_get_frameskip
//============================================================

int winvideo_get_frameskip(void)
{
	return autoframeskip ? -1 : frameskip;
}



//============================================================
//  osd_update
//============================================================

void osd_update(mame_time emutime)
{
	win_window_info *window;

	// if we're throttling, paused, or if the UI is up, synchronize
	if (throttle || mame_is_paused() || ui_is_setup_active() || ui_is_onscrd_active())
		update_throttle(emutime);

	// update the FPS computations
	update_fps(emutime);

	// update all the windows
	profiler_mark(PROFILER_BLIT);
	for (window = win_window_list; window != NULL; window = window->next)
		win_update_video_window(window);
	profiler_mark(PROFILER_END);

	// if we're throttling and autoframeskip is on, adjust
	if (throttle && autoframeskip && frameskip_counter == 0)
		update_autoframeskip();

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// poll the joystick values here
	winwindow_process_events(TRUE);
	wininput_poll();
	check_osd_inputs();
}


//============================================================
//  osd_skip_this_frame
//============================================================

int osd_skip_this_frame(void)
{
	// skip the current frame?
	return skiptable[frameskip][frameskip_counter];
}



//============================================================
//  osd_get_fps_text
//============================================================

const char *osd_get_fps_text(const performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;

	// if we're paused, display less info
	if (mame_is_paused())
		dest += sprintf(dest, "%s%2d%4d/%d fps",
				autoframeskip ? "auto" : "fskp", frameskip,
				(int)(performance->frames_per_second + 0.5),
				PAUSED_REFRESH_RATE);
	else
	{
		dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
				autoframeskip ? "auto" : "fskp", frameskip,
				(int)(performance->game_speed_percent + 0.5),
				(int)(performance->frames_per_second + 0.5),
				(int)(Machine->refresh_rate[0] + 0.5));

		/* for vector games, add the number of vector updates */
		if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
			dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
		else if (performance->partial_updates_this_frame > 1)
			dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
}



//============================================================
//  osd_override_snapshot
//============================================================

mame_bitmap *osd_override_snapshot(mame_bitmap *bitmap, rectangle *bounds)
{
	return NULL;
/*

    rectangle newbounds;
    mame_bitmap *copy;
    int x, y, w, h, t;

    // if we can send it in raw, no need to override anything
    if (!blit_swapxy && !blit_flipx && !blit_flipy)
        return NULL;

    // allocate a copy
    w = blit_swapxy ? bitmap->height : bitmap->width;
    h = blit_swapxy ? bitmap->width : bitmap->height;
    copy = bitmap_alloc_depth(w, h, bitmap->depth);
    if (!copy)
        return NULL;

    // populate the copy
    for (y = bounds->min_y; y <= bounds->max_y; y++)
        for (x = bounds->min_x; x <= bounds->max_x; x++)
        {
            int tx = x, ty = y;

            // apply the rotation/flipping
            if (blit_swapxy)
            {
                t = tx; tx = ty; ty = t;
            }
            if (blit_flipx)
                tx = copy->width - tx - 1;
            if (blit_flipy)
                ty = copy->height - ty - 1;

            // read the old pixel and copy to the new location
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

    // compute the oriented bounds
    newbounds = *bounds;

    // apply X/Y swap first
    if (blit_swapxy)
    {
        t = newbounds.min_x; newbounds.min_x = newbounds.min_y; newbounds.min_y = t;
        t = newbounds.max_x; newbounds.max_x = newbounds.max_y; newbounds.max_y = t;
    }

    // apply X flip
    if (blit_flipx)
    {
        t = copy->width - newbounds.min_x - 1;
        newbounds.min_x = copy->width - newbounds.max_x - 1;
        newbounds.max_x = t;
    }

    // apply Y flip
    if (blit_flipy)
    {
        t = copy->height - newbounds.min_y - 1;
        newbounds.min_y = copy->height - newbounds.max_y - 1;
        newbounds.max_y = t;
    }

    *bounds = newbounds;
    return copy;*/
}



//============================================================
//  init_monitors
//============================================================

static void init_monitors(void)
{
	win_monitor_info **tailptr;

	// make a list of monitors
	win_monitor_list = NULL;
	tailptr = &win_monitor_list;
	EnumDisplayMonitors(NULL, NULL, monitor_enum_callback, (LPARAM)&tailptr);

	// if we're verbose, print the list of monitors
	if (verbose)
	{
		win_monitor_info *monitor;
		for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
			printf("Monitor %p = \"%s\" %s\n", monitor->handle, monitor->info.szDevice, (monitor == primary_monitor) ? "(primary)" : "");
	}
}



//============================================================
//  monitor_enum_callback
//============================================================

static BOOL CALLBACK monitor_enum_callback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data)
{
	win_monitor_info ***tailptr = (win_monitor_info ***)data;
	win_monitor_info *monitor;
	MONITORINFOEX info;
	BOOL result;

	// get the monitor info
	info.cbSize = sizeof(info);
	result = GetMonitorInfo(handle, (LPMONITORINFO)&info);
	assert(result);

	// allocate a new monitor info
	monitor = malloc_or_die(sizeof(*monitor));
	memset(monitor, 0, sizeof(*monitor));

	// copy in the data
	monitor->handle = handle;
	monitor->info = info;

	// guess the aspect ratio assuming square pixels
	monitor->aspect = (float)(info.rcMonitor.right - info.rcMonitor.left) / (float)(info.rcMonitor.bottom - info.rcMonitor.top);

	// save the primary monitor handle
	if (monitor->info.dwFlags & MONITORINFOF_PRIMARY)
		primary_monitor = monitor;

	// hook us into the list
	**tailptr = monitor;
	*tailptr = &monitor->next;

	// enumerate all the available monitors so to list their names in verbose mode
	return TRUE;
}



//============================================================
//  decode_cleanstretch
//============================================================

static int decode_cleanstretch(struct rc_option *option, const char *arg, int priority)
{
	// none: never contrain stretching
	if (!strcmp(arg, "none"))
	{
		win_force_int_stretch = FORCE_INT_STRECT_NONE;
	}
	// full: constrain both width and height to integer ratios
	else if (!strcmp(arg, "full"))
	{
		win_force_int_stretch = FORCE_INT_STRECT_FULL;
	}
	// auto: let the blitter module decide when/how to constrain stretching
	else if (!strcmp(arg, "auto"))
	{
		win_force_int_stretch = FORCE_INT_STRECT_AUTO;
	}
	// horizontal: constrain width to integer ratios (relative to game)
	else if (!strncmp(arg, "horizontal", strlen(arg)))
	{
		win_force_int_stretch = FORCE_INT_STRECT_HOR;
	}
	// vertical: constrain height to integer ratios (relative to game)
	else if (!strncmp(arg, "vertical", strlen(arg)))
	{
		win_force_int_stretch = FORCE_INT_STRECT_VER;
	}
	else
	{
		fprintf(stderr, "error: invalid value for cleanstretch: %s\n", arg);
		return -1;
	}

	option->priority = priority;
	return 0;
}



//============================================================
//  decode_resolution
//============================================================

static int decode_resolution(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
	}
	else if (sscanf(arg, "%dx%dx%d", &win_gfx_width, &win_gfx_height, &win_gfx_depth) < 2)
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	if ((win_gfx_depth != 0) &&
		(win_gfx_depth != 16) &&
		(win_gfx_depth != 24) &&
		(win_gfx_depth != 32))
	{
		win_gfx_width = win_gfx_height = win_gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	options.vector_width = win_gfx_width;
	options.vector_height = win_gfx_height;

	option->priority = priority;
	return 0;
}



//============================================================
//  decode_ftr
//============================================================

static int decode_ftr(struct rc_option *option, const char *arg, int priority)
{
	int ftr;

	if (sscanf(arg, "%d", &ftr) != 1)
	{
		fprintf(stderr, "error: invalid value for frames_to_run: %s\n", arg);
		return -1;
	}

	// if we're running < 5 minutes, allow us to skip warnings to facilitate benchmarking/validation testing
	frames_to_run = ftr;
	if (frames_to_run > 0 && frames_to_run < 60*60*5)
		options.skip_warnings = options.skip_gameinfo = options.skip_disclaimer = 1;

	option->priority = priority;
	return 0;
}



//============================================================
//  decode_aspect
//============================================================

static int decode_aspect(struct rc_option *option, const char *arg, int priority)
{
	int num, den;

	if (sscanf(arg, "%d:%d", &num, &den) != 2 || num == 0 || den == 0)
	{
		fprintf(stderr, "error: invalid value for aspect ratio: %s\n", arg);
		return -1;
	}
	win_screen_aspect = (double)num / (double)den;

	option->priority = priority;
	return 0;
}



//============================================================
//  update_throttle
//============================================================

static void update_throttle(mame_time emutime)
{
	static double ticks_per_sleep_msec = 0;
	cycles_t target, curr, cps, diffcycles;
	subseconds_t subsecs_per_cycle;

	// if we're only syncing to the refresh, bail now
	if (win_sync_refresh)
		return;

	// if we're paused, emutime will not advance; explicitly resync and set us backwards
	// 1/60th of a second
	if (mame_is_paused())
		throttle_realtime = throttle_emutime = sub_subseconds_from_mame_time(emutime, MAX_SUBSECONDS / PAUSED_REFRESH_RATE);

	// if time moved backwards (reset), or if it's been more than 1 second in emulated time, resync
	if (compare_mame_times(emutime, throttle_emutime) < 0 || sub_mame_times(emutime, throttle_emutime).seconds > 0)
		goto resync;

	// get the current realtime; if it's been more than 1 second realtime, just resync
	cps = osd_cycles_per_second();
	diffcycles = osd_cycles() - throttle_last_cycles;
	throttle_last_cycles += diffcycles;
	if (diffcycles >= cps)
		goto resync;

	// add the time that has passed to the real time
	subsecs_per_cycle = MAX_SUBSECONDS / cps;
	throttle_realtime = add_subseconds_to_mame_time(throttle_realtime, diffcycles * subsecs_per_cycle);

	// update the emulated time
	throttle_emutime = emutime;

	// if we're behind, just sync
	if (compare_mame_times(throttle_emutime, throttle_realtime) <= 0)
		goto resync;

	// determine our target cycles value
	target = throttle_last_cycles + sub_mame_times(throttle_emutime, throttle_realtime).subseconds / subsecs_per_cycle;

	// initialize the ticks per sleep
	if (ticks_per_sleep_msec == 0)
		ticks_per_sleep_msec = (double)(cps / 1000);

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// sync
	for (curr = osd_cycles(); curr - target < 0; curr = osd_cycles())
	{
		// if we have enough time to sleep, do it
		// ...but not if we're autoframeskipping and we're behind
		if (allow_sleep && (!autoframeskip || frameskip == 0) &&
			(target - curr) > (cycles_t)(ticks_per_sleep_msec * 1.1))
		{
			cycles_t next;

			// keep track of how long we actually slept
			Sleep(1);
			next = osd_cycles();
			ticks_per_sleep_msec = (ticks_per_sleep_msec * 0.90) + ((double)(next - curr) * 0.10);
		}
	}

	// idle time done
	profiler_mark(PROFILER_END);

	// update realtime
	diffcycles = osd_cycles() - throttle_last_cycles;
	throttle_last_cycles += diffcycles;
	throttle_realtime = add_subseconds_to_mame_time(throttle_realtime, diffcycles * subsecs_per_cycle);
	return;

resync:
	// reset realtime and emutime to the same value
	throttle_realtime = throttle_emutime = emutime;
}



//============================================================
//  update_fps
//============================================================

static void update_fps(mame_time emutime)
{
	cycles_t curr = osd_cycles();

	// update stats for the FPS average calculation
	if (fps_start_time == 0)
	{
		// start the timer going 1 second into the game
		if (emutime.seconds > 1)
			fps_start_time = osd_cycles();
	}
	else
	{
		fps_frames_displayed++;
		if (fps_frames_displayed == frames_to_run)
		{
#ifndef NEW_RENDER
			char name[20];
			mame_file *fp;

			// make a filename with an underscore prefix
			sprintf(name, "_%.8s", Machine->gamedrv->name);

			// write out the screenshot
			if ((fp = mame_fopen(Machine->gamedrv->name, name, FILETYPE_SCREENSHOT, 1)) != NULL)
			{
				save_screen_snapshot_as(fp, artwork_get_ui_bitmap());
				mame_fclose(fp);
			}
#endif
			mame_schedule_exit();
		}
		fps_end_time = curr;
	}
}



//============================================================
//  update_autoframeskip
//============================================================

static void update_autoframeskip(void)
{
	// don't adjust frameskip if we're paused or if the debugger was
	// visible this cycle or if we haven't run yet
	if (cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS)
	{
		const performance_info *performance = mame_get_performance_info();

		// if we're too fast, attempt to increase the frameskip
		if (performance->game_speed_percent >= 99.5)
		{
			frameskipadjust++;

			// but only after 3 consecutive frames where we are too fast
			if (frameskipadjust >= 3)
			{
				frameskipadjust = 0;
				if (frameskip > 0) frameskip--;
			}
		}

		// if we're too slow, attempt to increase the frameskip
		else
		{
			// if below 80% speed, be more aggressive
			if (performance->game_speed_percent < 80)
				frameskipadjust -= (90 - performance->game_speed_percent) / 5;

			// if we're close, only force it up to frameskip 8
			else if (frameskip < 8)
				frameskipadjust--;

			// perform the adjustment
			while (frameskipadjust <= -2)
			{
				frameskipadjust += 2;
				if (frameskip < FRAMESKIP_LEVELS - 1)
					frameskip++;
			}
		}
	}
}



//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(void)
{
	// increment frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		// if autoframeskip, disable auto and go to 0
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = 0;
		}

		// wrap from maximum to auto
		else if (frameskip == FRAMESKIP_LEVELS - 1)
		{
			frameskip = 0;
			autoframeskip = 1;
		}

		// else just increment
		else
			frameskip++;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS-1;
		}

		// wrap from 0 to auto
		else if (frameskip == 0)
			autoframeskip = 1;

		// else just decrement
		else
			frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		throttle ^= 1;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		win_toggle_full_screen();

#ifdef MESS
	// check for toggling menu bar
	if (input_ui_pressed(IPT_OSD_2))
		win_toggle_menubar();
#endif
}
