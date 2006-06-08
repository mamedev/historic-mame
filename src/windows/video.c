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
#include "options.h"
#endif

// MAMEOS headers
#include "winmain.h"
#include "video.h"
#include "window.h"
#include "input.h"
#include "options.h"

#ifdef NEW_DEBUGGER
#include "debugwin.h"
#endif

#ifdef MESS
#include "menu.h"
#endif


//============================================================
//  IMPORTS
//============================================================

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

int video_orientation;
win_video_config video_config;



//============================================================
//  LOCAL VARIABLES
//============================================================

// monitor info
win_monitor_info *win_monitor_list;
static win_monitor_info *primary_monitor;

// average FPS calculation
static cycles_t fps_start_time;
static cycles_t fps_end_time;
static int fps_frames_displayed;

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
static win_monitor_info *pick_monitor(int index);

static void update_throttle(mame_time emutime);
static void update_fps(mame_time emutime);
static void update_autoframeskip(void);
static void check_osd_inputs(void);

static void extract_video_config(void);
static float get_aspect(const char *name, int report_error);
static void get_resolution(const char *name, win_window_config *config, int report_error);



//============================================================
//  OPTIONS
//============================================================

const options_entry video_opts[] =
{
	// performance options
	{ NULL,                       NULL,   OPTION_HEADER,  "PERFORMANCE OPTIONS" },
	{ "autoframeskip;afs",        "1",    OPTION_BOOLEAN, "enable automatic frameskip selection" },
	{ "frameskip;fs",             "0",    0,              "set frameskip to fixed value, 0-12 (autoframeskip must be disabled)" },
	{ "throttle",                 "1",    OPTION_BOOLEAN, "enable throttling to keep game running in sync with real time" },
	{ "sleep",                    "1",    OPTION_BOOLEAN, "enable sleeping, which gives time back to other applications when idle" },
	{ "rdtsc",                    "0",    OPTION_BOOLEAN, "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",                 "0",    0,              "thread priority for the main game thread; range from -15 to 1" },

	// misc options
	{ NULL,                       NULL,   OPTION_HEADER,  "MISC VIDEO OPTIONS" },
	{ "frames_to_run;ftr",        "0",    0,              "number of frames to run before automatically exiting" },
	{ "mngwrite",                 NULL,   0,              "optional filename to write a MNG movie of the current session" },

	// global options
	{ NULL,                       NULL,   OPTION_HEADER,  "GLOBAL VIDEO OPTIONS" },
	{ "window;w",                 "0",    OPTION_BOOLEAN, "enable window mode; otherwise, full screen mode is assumed" },
	{ "maximize;max",             "1",    OPTION_BOOLEAN, "default to maximized windows; otherwise, windows will be minimized" },
	{ "numscreens",               "1",    0,              "number of screens to create; usually, you want just one" },
	{ "extra_layout;layout",      NULL,   0,              "name of an extra layout file to parse" },

	// per-window options
	{ NULL,                       NULL,   OPTION_HEADER,  "PER-WINDOW VIDEO OPTIONS" },
	{ "screen0;screen",           "auto", 0,              "explicit name of the first screen; 'auto' here will try to make a best guess" },
	{ "aspect0;screen_aspect",    "auto", 0,              "aspect ratio of the first screen; 'auto' here will try to make a best guess" },
	{ "resolution0;resolution;r", "auto", 0,              "preferred resolution of the first screen; format is <width>x<height>[x<depth>[@<refreshrate>]] or 'auto'" },
	{ "view0;view",               "auto", 0,              "preferred view for the first screen" },

	{ "screen1",                  "auto", 0,              "explicit name of the second screen; 'auto' here will try to make a best guess" },
	{ "aspect1",                  "auto", 0,              "aspect ratio of the second screen; 'auto' here will try to make a best guess" },
	{ "resolution1",              "auto", 0,              "preferred resolution of the second screen; format is <width>x<height>[x<depth>[@<refreshrate>]] or 'auto'" },
	{ "view1",                    "auto", 0,              "preferred view for the second screen" },

	{ "screen2",                  "auto", 0,              "explicit name of the third screen; 'auto' here will try to make a best guess" },
	{ "aspect2",                  "auto", 0,              "aspect ratio of the third screen; 'auto' here will try to make a best guess" },
	{ "resolution2",              "auto", 0,              "preferred resolution of the third screen; format is <width>x<height>[x<depth>[@<refreshrate>]] or 'auto'" },
	{ "view2",                    "auto", 0,              "preferred view for the third screen" },

	{ "screen3",                  "auto", 0,              "explicit name of the fourth screen; 'auto' here will try to make a best guess" },
	{ "aspect3",                  "auto", 0,              "aspect ratio of the fourth screen; 'auto' here will try to make a best guess" },
	{ "resolution3",              "auto", 0,              "preferred resolution of the fourth screen; format is <width>x<height>[x<depth>[@<refreshrate>]] or 'auto'" },
	{ "view3",                    "auto", 0,              "preferred view for the fourth screen" },

	// directx options
	{ NULL,                       NULL,   OPTION_HEADER,  "DIRECTX VIDEO OPTIONS" },
	{ "direct3d;d3d",             "1",    OPTION_BOOLEAN, "enable using Direct3D 9 for video rendering if available (preferred)" },
	{ "d3dversion",               "9",    0,              "specify the preferred Direct3D version (8 or 9)" },
	{ "waitvsync",                "0",    OPTION_BOOLEAN, "enable waiting for the start of VBLANK before flipping screens; reduces tearing effects" },
	{ "syncrefresh",              "0",    OPTION_BOOLEAN, "enable using the start of VBLANK for throttling instead of the game time" },
	{ "triplebuffer;tb",          "0",    OPTION_BOOLEAN, "enable triple buffering" },
	{ "switchres",                "0",    OPTION_BOOLEAN, "enable resolution switching" },
	{ "filter;d3dfilter;flt",     "1",    OPTION_BOOLEAN, "enable bilinear filtering on screen output" },
	{ "prescale;d3dprescale",     "0",    0,              "screen texture prescaling amount" },
	{ "full_screen_gamma;fsg",    "1.0",  0,              "gamma value in full screen mode" },

	// deprecated junk
	{ "ddraw",                    "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "hwstretch",                "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "switchbpp",                "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "effect",                   "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "zoom",                     "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "d3dtexmanage",             "0",    OPTION_DEPRECATED, "(deprecated)" },
#ifndef NEW_RENDER
	{ "d3dfilter",                "0",    OPTION_DEPRECATED, "(deprecated)" },
#endif
	{ NULL }
};



//============================================================
//  winvideo_init
//============================================================

int winvideo_init(void)
{
	const char *stemp;
	int index;

	// ensure we get called on the way out
	add_exit_callback(video_exit);

	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// initialize the window system so we can make windows
	if (winwindow_init())
		goto error;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
		if (winwindow_video_window_create(index, pick_monitor(index), &video_config.window[index]))
			goto error;
	SetForegroundWindow(win_window_list->hwnd);

	// possibly create the debug window, but don't show it yet
#ifdef MAME_DEBUG
	if (options.mame_debug)
		if (debugwin_init_windows())
			return 1;
#endif

	// start recording movie
	stemp = options_get_string("mngwrite", TRUE);
	if (stemp != NULL)
		record_movie_start(stemp);

	// if we're running < 5 minutes, allow us to skip warnings to facilitate benchmarking/validation testing
	if (video_config.framestorun > 0 && video_config.framestorun < 60*60*5)
		options.skip_warnings = options.skip_gameinfo = options.skip_disclaimer = TRUE;

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

	// print a final result to the stdout
	if (fps_frames_displayed != 0)
	{
		cycles_t cps = osd_cycles_per_second();
		printf("Average FPS: %f (%d frames)\n", (double)cps / (fps_end_time - fps_start_time) * fps_frames_displayed, fps_frames_displayed);
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
		video_config.frameskip = value;
		video_config.autoframeskip = 0;
	}
	else
	{
		video_config.frameskip = 0;
		video_config.autoframeskip = 1;
	}
}



//============================================================
//  winvideo_get_frameskip
//============================================================

int winvideo_get_frameskip(void)
{
	return video_config.autoframeskip ? -1 : video_config.frameskip;
}



//============================================================
//  osd_update
//============================================================

int osd_update(mame_time emutime)
{
	win_window_info *window;

	// if we're throttling, paused, or if the UI is up, synchronize
	if (video_config.throttle || mame_is_paused() || ui_is_setup_active() || ui_is_onscrd_active())
		update_throttle(emutime);

	// update the FPS computations
	update_fps(emutime);

	// update all the windows, but only if we're not skipping this frame
	if (!skiptable[video_config.frameskip][frameskip_counter])
	{
		profiler_mark(PROFILER_BLIT);
		for (window = win_window_list; window != NULL; window = window->next)
			winwindow_video_window_update(window);
		profiler_mark(PROFILER_END);
	}

	// if we're throttling and autoframeskip is on, adjust
	if (video_config.throttle && video_config.autoframeskip && frameskip_counter == 0)
		update_autoframeskip();

	// poll the joystick values here
	winwindow_process_events(TRUE);
	wininput_poll();
	check_osd_inputs();

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// return whether or not to skip the next frame
	return skiptable[video_config.frameskip][frameskip_counter];
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
				video_config.autoframeskip ? "auto" : "fskp", video_config.frameskip,
				(int)(performance->frames_per_second + 0.5),
				PAUSED_REFRESH_RATE);
	else
	{
		dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
				video_config.autoframeskip ? "auto" : "fskp", video_config.frameskip,
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
	{
		win_monitor_info *monitor;
		for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
			verbose_printf("Video: Monitor %p = \"%s\" %s\n", monitor->handle, monitor->info.szDevice, (monitor == primary_monitor) ? "(primary)" : "");
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
//  pick_monitor
//============================================================

static win_monitor_info *pick_monitor(int index)
{
	win_monitor_info *monitor;
	const char *scrname;
	int moncount = 0;
	char option[20];
	float aspect;

	// get the screen option
	sprintf(option, "screen%d", index);
	scrname = options_get_string(option, TRUE);

	// get the aspect ratio
	sprintf(option, "aspect%d", index);
	aspect = get_aspect(option, TRUE);

	// look for a match in the name first
	if (scrname != NULL)
		for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		{
			moncount++;
			if (strcmp(scrname, monitor->info.szDevice) == 0)
				goto finishit;
		}

	// didn't find it; alternate monitors until we hit the jackpot
	index %= moncount;
	for (monitor = win_monitor_list; monitor != NULL; monitor = monitor->next)
		if (index-- == 0)
			goto finishit;

	// return the primary just in case all else fails
	monitor = primary_monitor;

finishit:
	if (aspect != 0)
		monitor->aspect = aspect;
	return monitor;
}



//============================================================
//  update_throttle
//============================================================

static void update_throttle(mame_time emutime)
{
	static double ticks_per_sleep_msec = 0;
	cycles_t target, curr, cps, diffcycles;
	int allowed_to_sleep;
	subseconds_t subsecs_per_cycle;
	int paused = mame_is_paused();

	// if we're only syncing to the refresh, bail now
	if (video_config.syncrefresh)
		return;

	// if we're paused, emutime will not advance; explicitly resync and set us backwards
	// 1/60th of a second
	if (paused)
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

	// determine whether or not we are allowed to sleep
	allowed_to_sleep = video_config.sleep && (!video_config.autoframeskip || video_config.frameskip == 0);

	// sync
	for (curr = osd_cycles(); curr - target < 0; curr = osd_cycles())
	{
		// if we have enough time to sleep, do it
		// ...but not if we're autoframeskipping and we're behind
		if (paused || (allowed_to_sleep && (target - curr) > (cycles_t)(ticks_per_sleep_msec * 1.1)))
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
		if (fps_frames_displayed == video_config.framestorun)
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
	// skip if paused
	if (mame_is_paused())
		return;

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
				if (video_config.frameskip > 0) video_config.frameskip--;
			}
		}

		// if we're too slow, attempt to increase the frameskip
		else
		{
			// if below 80% speed, be more aggressive
			if (performance->game_speed_percent < 80)
				frameskipadjust -= (90 - performance->game_speed_percent) / 5;

			// if we're close, only force it up to frameskip 8
			else if (video_config.frameskip < 8)
				frameskipadjust--;

			// perform the adjustment
			while (frameskipadjust <= -2)
			{
				frameskipadjust += 2;
				if (video_config.frameskip < FRAMESKIP_LEVELS - 1)
					video_config.frameskip++;
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
		if (video_config.autoframeskip)
		{
			video_config.autoframeskip = 0;
			video_config.frameskip = 0;
		}

		// wrap from maximum to auto
		else if (video_config.frameskip == FRAMESKIP_LEVELS - 1)
		{
			video_config.frameskip = 0;
			video_config.autoframeskip = 1;
		}

		// else just increment
		else
			video_config.frameskip++;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (video_config.autoframeskip)
		{
			video_config.autoframeskip = 0;
			video_config.frameskip = FRAMESKIP_LEVELS-1;
		}

		// wrap from 0 to auto
		else if (video_config.frameskip == 0)
			video_config.autoframeskip = 1;

		// else just decrement
		else
			video_config.frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		video_config.throttle = !video_config.throttle;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		fps_frames_displayed = 0;
	}

	// check for toggling fullscreen mode
	if (input_ui_pressed(IPT_OSD_1))
		winwindow_toggle_full_screen();

#ifdef MESS
	// check for toggling menu bar
	if (input_ui_pressed(IPT_OSD_2))
		win_toggle_menubar();
#endif
}



//============================================================
//  extract_video_config
//============================================================

static void extract_video_config(void)
{
	int itemp;

	// performance options: extract the data
	video_config.autoframeskip = options_get_bool ("autoframeskip", TRUE);
	video_config.frameskip     = options_get_int  ("frameskip", TRUE);
	video_config.throttle      = options_get_bool ("throttle", TRUE);
	video_config.sleep         = options_get_bool ("sleep", TRUE);

	// misc options: extract the data
	video_config.framestorun   = options_get_int  ("frames_to_run", TRUE);

	// global options: extract the data
	video_config.windowed      = options_get_bool ("window", TRUE);
	video_config.numscreens    = options_get_int  ("numscreens", TRUE);
#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options.mame_debug)
		video_config.windowed = TRUE;
#endif

	// per-window options: extract the data
	get_resolution("resolution0", &video_config.window[0], TRUE);
	get_resolution("resolution1", &video_config.window[1], TRUE);
	get_resolution("resolution2", &video_config.window[2], TRUE);
	get_resolution("resolution3", &video_config.window[3], TRUE);

	// d3d options: extract the data
	video_config.d3d           = options_get_bool ("direct3d", TRUE);
	video_config.waitvsync     = options_get_bool ("waitvsync", TRUE);
	video_config.syncrefresh   = options_get_bool ("syncrefresh", TRUE);
	video_config.triplebuf     = options_get_bool ("triplebuffer", TRUE);
	video_config.switchres     = options_get_bool ("switchres", TRUE);
	video_config.filter        = options_get_bool ("filter", TRUE);
	video_config.prescale      = options_get_int  ("prescale", TRUE);
	video_config.gamma         = options_get_float("full_screen_gamma", TRUE);

	// performance options: sanity check values
	if (video_config.frameskip < 0 || video_config.frameskip > FRAMESKIP_LEVELS)
	{
		fprintf(stderr, "Invalid frameskip value %d; reverting to 0\n", video_config.frameskip);
		video_config.frameskip = 0;
	}
	itemp = options_get_int("priority", TRUE);
	if (itemp < -15 || itemp > 1)
	{
		fprintf(stderr, "Invalid priority value %d; reverting to 0\n", itemp);
		options_set_int("priority", 0);
	}

	// misc options: sanity check values

	// global options: sanity check values
	if (video_config.numscreens < 1 || video_config.numscreens > MAX_WINDOWS)
	{
		fprintf(stderr, "Invalid numscreens value %d; reverting to 1\n", video_config.numscreens);
		video_config.numscreens = 1;
	}

	// per-window options: sanity check values

	// d3d options: sanity check values
	itemp = options_get_int("d3dversion", TRUE);
	if (itemp < 8 || itemp > 9)
	{
		fprintf(stderr, "Invalid d3dversion value %d; reverting to 9\n", itemp);
		options_set_int("d3dversion", 9);
	}
	if (video_config.gamma < 0.0 || video_config.gamma > 4.0)
	{
		fprintf(stderr, "Invalid full_screen_gamma value %f; reverting to 1.0\n", video_config.gamma);
		video_config.gamma = 1.0;
	}
}



//============================================================
//  get_aspect
//============================================================

static float get_aspect(const char *name, int report_error)
{
	const char *data = options_get_string(name, report_error);
	int num = 0, den = 1;

	if (strcmp(data, "auto") == 0)
		return 0;
	else if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		fprintf(stderr, "Illegal aspect ratio value for %s = %s\n", name, data);
	return (float)num / (float)den;
}



//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *name, win_window_config *config, int report_error)
{
	const char *data = options_get_string(name, report_error);

	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, "auto") == 0)
		return;
	else if (sscanf(data, "%dx%dx%d@%d", &config->width, &config->height, &config->depth, &config->refresh) < 2 && report_error)
		fprintf(stderr, "Illegal resolution value for %s = %s\n", name, data);
}
