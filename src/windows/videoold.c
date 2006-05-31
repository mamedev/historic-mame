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
#include "videoold.h"
#include "windold.h"
#include "options.h"
#include "input.h"
#include "blit.h"

#ifndef NEW_DEBUGGER
#include "debugwin.h"
#endif

#ifdef MESS
#include "menu.h"
#endif


//============================================================
//  IMPORTS
//============================================================

// from input.c
extern void wininput_poll(void);
extern void win_pause_input(int pause);
extern int verbose;

// from sound.c
extern void sound_update_refresh_rate(float newrate);

// from wind3dfx.c
extern struct rc_option win_d3d_opts[];



//============================================================
//  PARAMETERS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS			12



//============================================================
//  GLOBAL VARIABLES
//============================================================

// screen info
HMONITOR monitor;
char *screen_name;

// current frameskip/autoframeskip settings
static int frameskip;
static int autoframeskip;

// speed throttling
int throttle = 1;

// palette lookups
UINT8 palette_lookups_invalid;
UINT32 palette_16bit_lookup[65536];
UINT32 palette_32bit_lookup[65536];

// rotation
UINT8 blit_flipx;
UINT8 blit_flipy;
UINT8 blit_swapxy;



//============================================================
//  LOCAL VARIABLES
//============================================================

// options decoding
//static char *cleanstretch;
//static char *resolution;
//static char *effect;
//static char *aspect;
static char *mngwrite;

// primary monitor handle
static HMONITOR primary_monitor;

// core video input parameters
static int video_width;
static int video_height;
static int video_depth;
static double video_fps;
static int video_attributes;

// internal readiness states
static int warming_up;

// timing measurements for throttling
static cycles_t last_skipcount0_time;
static cycles_t this_frame_base;
static int allow_sleep;

// average FPS calculation
static cycles_t start_time;
static cycles_t end_time;
static int frames_displayed;
static int frames_to_display;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// game states that invalidate autoframeskip
static int game_was_paused;
static int game_is_paused;
static int debugger_was_visible;

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

static int decode_cleanstretch(struct rc_option *option, const char *arg, int priority);
static int video_set_resolution(struct rc_option *option, const char *arg, int priority);
static int decode_ftr(struct rc_option *option, const char *arg, int priority);
static int decode_effect(struct rc_option *option, const char *arg, int priority);
static int decode_aspect(struct rc_option *option, const char *arg, int priority);
static void update_visible_area(mame_display *display);



//============================================================
//  OPTIONS
//============================================================

const options_entry video_opts[] =
{
	// performance options
	{ NULL,                       NULL,   OPTION_HEADER,     "PERFORMANCE OPTIONS" },
	{ "autoframeskip;afs",        "1",    OPTION_BOOLEAN,    "enable automatic frameskip selection" },
	{ "frameskip;fs",             "0",    0,                 "set frameskip to fixed value, 0-12 (autoframeskip must be disabled)" },
	{ "throttle",                 "1",    OPTION_BOOLEAN,    "enable throttling to keep game running in sync with real time" },
	{ "sleep",                    "1",    OPTION_BOOLEAN,    "enable sleeping, which gives time back to other applications when idle" },
	{ "rdtsc",                    "0",    OPTION_BOOLEAN,    "use the RDTSC instruction for timing; faster but may result in uneven performance" },
	{ "priority",                 "0",    0,                 "thread priority for the main game thread; range from -15 to 1" },

	// misc options
	{ NULL,                       NULL,   OPTION_HEADER,     "MISC VIDEO OPTIONS" },
	{ "frames_to_run;ftr",        "0",    0,                 "number of frames to run before automatically exiting" },
	{ "mngwrite",                 NULL,   0,                 "optional filename to write a MNG movie of the current session" },

	// global options
	{ NULL,                       NULL,   OPTION_HEADER,     "GLOBAL VIDEO OPTIONS" },
	{ "window;w",                 "0",    OPTION_BOOLEAN,    "enable window mode; otherwise, full screen mode is assumed" },
	{ "maximize;max",             "1",    OPTION_BOOLEAN,    "default to maximized windows; otherwise, windows will be minimized" },
	{ "numscreens",               "1",    0,                 "number of screens to create; usually, you want just one" },
	{ "extra_layout;layout",      NULL,   0,                 "name of an extra layout file to parse" },

	// per-window options
	{ NULL,                       NULL,   OPTION_HEADER,     "PER-WINDOW VIDEO OPTIONS" },
	{ "screen",                   NULL,   0,                 "specify which screen to use" },
	{ "screen_aspect",            "4:3",  0,                 "aspect ratio of the screen" },
	{ "resolution;r",             "auto", 0,                 "preferred resolution of the screen; format is <width>x<height>[x<depth>] or 'auto'" },

	// directx options
	{ NULL,                       NULL,   OPTION_HEADER,     "DIRECTX VIDEO OPTIONS" },
	{ "ddraw;dd",                 "1",    OPTION_BOOLEAN,    "enable using DirectDraw for video rendering (preferred)" },
	{ "direct3d;d3d",             "0",    OPTION_BOOLEAN,    "enable using Direct3D for video rendering" },
	{ "waitvsync",                "0",    OPTION_BOOLEAN,    "enable waiting for the start of VBLANK before flipping screens; reduces tearing effects" },
	{ "syncrefresh",              "0",    OPTION_BOOLEAN,    "enable using the start of VBLANK for throttling instead of the game time" },
	{ "triplebuffer;tb",          "0",    OPTION_BOOLEAN,    "enable triple buffering" },
	{ "switchres",                "1",    OPTION_BOOLEAN,    "enable resolution switching" },
	{ "full_screen_gamma;fsg",    "1.0",  0,                 "gamma value in full screen mode" },

	{ "hwstretch;hws",            "1",    OPTION_BOOLEAN,    "(dd) stretch video using the hardware" },
	{ "cleanstretch;cs",          "auto", 0,                 "stretch to integer ratios" },
	{ "refresh",                  "0",    0,                 "set specific monitor refresh rate" },
	{ "scanlines;sl",             "0",    OPTION_BOOLEAN,    "emulate win_old_scanlines" },
	{ "switchbpp",                "1",    OPTION_BOOLEAN,    "switch color depths to best fit" },
	{ "keepaspect;ka",            "1",    OPTION_BOOLEAN,    "enforce aspect ratio" },
	{ "matchrefresh",             "0",    OPTION_BOOLEAN,    "attempt to match the game's refresh rate" },
	{ "effect",                   "none", 0,                 "specify the blitting effect" },

	{ "zoom;z",                   "2",    0,                 "force specific zoom level" },
	{ "d3dtexmanage",             "1",    OPTION_BOOLEAN,    "use DirectX texture management" },
	{ "d3dfilter;flt",            "1",    OPTION_BOOLEAN,    "enable bilinear filtering" },

	{ "d3dfeedback",              "0",    OPTION_DEPRECATED, "feedback strength" },
	{ "d3dscan",                  "100",  OPTION_DEPRECATED, "scanline intensity" },
	{ "d3deffectrotate",          "1",    OPTION_DEPRECATED, "enable rotation of effects for rotated games" },
	{ "d3dprescale",              "auto", OPTION_DEPRECATED, "prescale effect" },
	{ "d3deffect",                "none", OPTION_DEPRECATED, "specify the blitting effects" },
	{ "d3dcustom",                NULL,   OPTION_DEPRECATED, "customised blitting effects preset" },
	{ "d3dexpert",                NULL,   OPTION_DEPRECATED, "additional customised settings (undocumented)" },

	{ NULL }
};



//============================================================
//  win_orient_rect
//============================================================

void win_orient_rect(rectangle *rect)
{
	int temp;

	// apply X/Y swap first
	if (blit_swapxy)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}

	// apply X flip
	if (blit_flipx)
	{
		temp = video_width - rect->min_x - 1;
		rect->min_x = video_width - rect->max_x - 1;
		rect->max_x = temp;
	}

	// apply Y flip
	if (blit_flipy)
	{
		temp = video_height - rect->min_y - 1;
		rect->min_y = video_height - rect->max_y - 1;
		rect->max_y = temp;
	}
}



//============================================================
//  win_disorient_rect
//============================================================

void win_disorient_rect(rectangle *rect)
{
	int temp;

	// unapply Y flip
	if (blit_flipy)
	{
		temp = video_height - rect->min_y - 1;
		rect->min_y = video_height - rect->max_y - 1;
		rect->max_y = temp;
	}

	// unapply X flip
	if (blit_flipx)
	{
		temp = video_width - rect->min_x - 1;
		rect->min_x = video_width - rect->max_x - 1;
		rect->max_x = temp;
	}

	// unapply X/Y swap last
	if (blit_swapxy)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}
}



//============================================================
//  monitor_enum_proc
//============================================================

static BOOL CALLBACK monitor_enum_proc(HMONITOR monitor_enum, HDC dc, LPRECT rect, LPARAM data)
{
	MONITORINFOEX monitor_info = { sizeof(MONITORINFOEX) };

	if (!GetMonitorInfo(monitor_enum, (LPMONITORINFO)&monitor_info))
	{
		fprintf(stderr, "ERROR: Failed to get display monitor information\n");
		return FALSE;	// stop enumeration
	}

	if (verbose)
	{
		fprintf(stderr, "Enumerating display monitors... Found: %s %s\n",
				monitor_info.szDevice,
				monitor_info.dwFlags & MONITORINFOF_PRIMARY ? "(primary)" : "");
	}

	// save the primary monitor handle
	if (monitor_info.dwFlags & MONITORINFOF_PRIMARY)
		primary_monitor = monitor_enum;

	// save the current handle if the monitor name matches the requested one
	if (screen_name != NULL && mame_stricmp(monitor_info.szDevice, screen_name) == 0)
		monitor = monitor_enum;

	// enumerate all the available monitors so to list their names in verbose mode
	return TRUE;
}



//============================================================
//  extract_video_config
//============================================================

static int get_cleanstretch(const char *option, int report_error)
{
	const char *stemp = options_get_string(option, report_error);

	// none: never contrain stretching
	if (!strcmp(stemp, "none"))
	{
		return FORCE_INT_STRECT_NONE;
	}
	// full: constrain both width and height to integer ratios
	else if (!strcmp(stemp, "full"))
	{
		return FORCE_INT_STRECT_FULL;
	}
	// auto: let the blitter module decide when/how to constrain stretching
	else if (!strcmp(stemp, "auto"))
	{
		return FORCE_INT_STRECT_AUTO;
	}
	// horizontal: constrain width to integer ratios (relative to game)
	else if (!strncmp(stemp, "horizontal", strlen(stemp)))
	{
		return FORCE_INT_STRECT_HOR;
	}
	// vertical: constrain height to integer ratios (relative to game)
	else if (!strncmp(stemp, "vertical", strlen(stemp)))
	{
		return FORCE_INT_STRECT_VER;
	}
	else
	{
		fprintf(stderr, "error: invalid value for cleanstretch: %s\n", stemp);
	}

	return 0;
}

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

static void get_resolution(const char *name, int report_error)
{
	const char *data = options_get_string(name, report_error);

	win_gfx_width = win_gfx_height = win_gfx_depth = 0;
	if (strcmp(data, "auto") == 0)
		return;
	else if (sscanf(data, "%dx%dx%d", &win_gfx_width, &win_gfx_height, &win_gfx_depth) < 2 && report_error)
		fprintf(stderr, "Illegal resolution value for %s = %s\n", name, data);
}

static void extract_video_config(void)
{
extern int win_d3d_use_filter;
extern int win_d3d_tex_manage;

	// performance options: extract the data
	autoframeskip              = options_get_bool  ("autoframeskip", TRUE);
	frameskip                  = options_get_int   ("frameskip", TRUE);
	throttle                   = options_get_bool  ("throttle", TRUE);
	allow_sleep                = options_get_bool  ("sleep", TRUE);

	// misc options: extract the data
	frames_to_display          = options_get_int   ("frames_to_run", TRUE);
	mngwrite                   = (char *)options_get_string("mngwrite", TRUE);

	// global options: extract the data
	win_window_mode            = options_get_bool  ("window", TRUE);
	win_start_maximized        = options_get_bool  ("maximize", TRUE);

	// per-window options: extract the data
	screen_name                = (char *)options_get_string("screen", TRUE);
	win_screen_aspect          = get_aspect("screen_aspect", TRUE);
	get_resolution("resolution", TRUE);

	// d3d options: extract the data
	win_use_ddraw              = options_get_bool  ("ddraw", TRUE);
	win_use_d3d                = options_get_bool  ("direct3d", TRUE);
	win_wait_vsync             = options_get_bool  ("waitvsync", TRUE);
	win_sync_refresh           = options_get_bool  ("syncrefresh", TRUE);
	win_triple_buffer          = options_get_bool  ("triplebuffer", TRUE);
	win_switch_res             = options_get_bool  ("switchres", TRUE);
	win_gfx_gamma              = options_get_float ("full_screen_gamma", TRUE);

	win_dd_hw_stretch          = options_get_bool  ("hwstretch", TRUE);
	win_force_int_stretch      = get_cleanstretch  ("cleanstretch", TRUE);
	win_gfx_refresh            = options_get_int   ("refresh", TRUE);
	win_old_scanlines          = options_get_bool  ("scanlines", TRUE);
	win_switch_bpp             = options_get_bool  ("switchbpp", TRUE);
	win_keep_aspect            = options_get_bool  ("keepaspect", TRUE);
	win_match_refresh          = options_get_bool  ("matchrefresh", TRUE);
	win_blit_effect            = win_lookup_effect(options_get_string("effect", TRUE));

	win_gfx_zoom               = options_get_int   ("zoom", TRUE);
	win_d3d_tex_manage         = options_get_bool  ("d3dtexmanage", TRUE);
	win_d3d_use_filter         = options_get_bool  ("d3dfilter", TRUE);

	// performance options: sanity check values
	if (frameskip < 0 || frameskip > FRAMESKIP_LEVELS)
	{
		fprintf(stderr, "Invalid frameskip value %d; reverting to 0\n", frameskip);
		frameskip = 0;
	}
	if (options_get_int("priority", TRUE) < -15 || options_get_int("priority", TRUE) > 1)
	{
		fprintf(stderr, "Invalid priority value %d; reverting to 0\n", options_get_int("priority", TRUE));
		options_set_int("priority", 0);
	}

	// misc options: sanity check values

	// global options: sanity check values

	// per-window options: sanity check values

	// d3d options: sanity check values
	if (win_gfx_gamma < 0.0 || win_gfx_gamma > 4.0)
	{
		fprintf(stderr, "Invalid full_screen_gamma value %f; reverting to 1.0\n", win_gfx_gamma);
		win_gfx_gamma = 1.0;
	}

	if (frames_to_display > 0 && frames_to_display < 60*60*5)
		options.skip_warnings = options.skip_gameinfo = options.skip_disclaimer = 1;
}





int winvideo_init(void)
{
extern void win_blit_init(void);

	// extract data from the options
	extract_video_config();

	win_blit_init();

	return win_init_window();
}



//============================================================
//  osd_create_display
//============================================================

int osd_create_display(const osd_create_params *params, UINT32 *rgb_components)
{
	mame_display dummy_display;
	double aspect_ratio;
	int r, g, b;

	logerror("width %d, height %d depth %d\n", params->width, params->height, params->depth);

	// copy the parameters into globals for later use
	video_width			= blit_swapxy ? params->height : params->width;
	video_height		= blit_swapxy ? params->width : params->height;
	video_depth			= (params->depth != 15) ? params->depth : 16;
	video_fps			= params->fps;
	video_attributes	= params->video_attributes;

	// clamp the frameskip value to within range
	if (frameskip < 0)
		frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS)
		frameskip = FRAMESKIP_LEVELS - 1;

	if (!blit_swapxy)
		aspect_ratio = (double)params->aspect_x / (double)params->aspect_y;
	else
		aspect_ratio = (double)params->aspect_y / (double)params->aspect_x;

	// find the monitor to draw on
	monitor = NULL;
	EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, 0);

	if (monitor == NULL)
	{
		if (primary_monitor)
			monitor = primary_monitor;
		else
			fatalerror("ERROR: Unable to find a monitor");

		if (screen_name)
			fprintf(stderr, "WARNING: Screen %s not found, using primary display monitor\n", screen_name);
		else if (verbose)
			fprintf(stderr, "Screen name not specified, using primary display monitor\n");
	}
	else if (verbose)
		fprintf(stderr, "Using %s as specified\n", screen_name);

	// create the window
	if (win_create_window(video_width, video_height, video_depth, video_attributes, aspect_ratio))
		return 1;

	// initialize the palette to a fixed 5-5-5 mapping
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				int idx = (r << 10) | (g << 5) | b;
				int rr = (r << 3) | (r >> 2);
				int gg = (g << 3) | (g >> 2);
				int bb = (b << 3) | (b >> 2);
				palette_16bit_lookup[idx] = win_color16(rr, gg, bb) * 0x10001;
				palette_32bit_lookup[idx] = win_color32(rr, gg, bb);
			}

	// fill in the resulting RGB components
	if (rgb_components)
	{
		if (video_depth == 32)
		{
			rgb_components[0] = win_color32(0xff, 0x00, 0x00);
			rgb_components[1] = win_color32(0x00, 0xff, 0x00);
			rgb_components[2] = win_color32(0x00, 0x00, 0xff);
		}
		else
		{
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;
		}
	}

	// set visible area to nothing just to initialize it - it will be set by the core
	memset(&dummy_display, 0, sizeof(dummy_display));
	update_visible_area(&dummy_display);

	// indicate for later that we're just beginning
	warming_up = 1;

	// start recording movie
	if (mngwrite != NULL)
		record_movie_start(mngwrite);

    return 0;
}



//============================================================
//  osd_close_display
//============================================================

void osd_close_display(void)
{
	// tear down the window
	win_destroy_window();

	// print a final result to the stdout
	if (frames_displayed != 0)
	{
		cycles_t cps = osd_cycles_per_second();
		printf("Average FPS: %f (%d frames)\n", (double)cps / (end_time - start_time) * frames_displayed, frames_displayed);
	}
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

	// display the FPS, frameskip, percent, fps and target fps
	dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
			autoframeskip ? "auto" : "fskp", frameskip,
			(int)(performance->game_speed_percent + 0.5),
			(int)(performance->frames_per_second + 0.5),
			(int)(Machine->refresh_rate[0] + 0.5));

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
}



//============================================================
//  check_inputs
//============================================================

static void check_inputs(void)
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
		frames_displayed = 0;
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
		frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		throttle ^= 1;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
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
//  throttle_speed
//============================================================

static void throttle_speed(void)
{
	static double ticks_per_sleep_msec = 0;
	cycles_t target, curr, cps;

	// if we're only syncing to the refresh, bail now
	if (win_sync_refresh)
		return;

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// get the current time and the target time
	curr = osd_cycles();
	cps = osd_cycles_per_second();
	target = this_frame_base + (int)((double)frameskip_counter * (double)cps / video_fps);

	// sync
	if (curr - target < 0)
	{
		// initialize the ticks per sleep
		if (ticks_per_sleep_msec == 0)
			ticks_per_sleep_msec = (double)(cps / 1000);

		// loop until we reach the target time
		while (curr - target < 0)
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
				curr = next;
			}
			else
			{
				// update the current time
				curr = osd_cycles();
			}
		}
	}

	// idle time done
	profiler_mark(PROFILER_END);
}



//============================================================
//  update_palette
//============================================================

static void update_palette(mame_display *display)
{
	int i, j;

	// loop over dirty colors in batches of 32
	for (i = 0; i < display->game_palette_entries; i += 32)
	{
		UINT32 dirtyflags = palette_lookups_invalid ? ~0 : display->game_palette_dirty[i / 32];
		if (dirtyflags)
		{
			display->game_palette_dirty[i / 32] = 0;

			// loop over all 32 bits and update dirty entries
			for (j = 0; (j < 32) && (i+j < display->game_palette_entries); j++, dirtyflags >>= 1)
				if (dirtyflags & 1)
				{
					// extract the RGB values
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);

					// update both lookup tables
					palette_16bit_lookup[i + j] = win_color16(r, g, b) * 0x10001;
					palette_32bit_lookup[i + j] = win_color32(r, g, b);
				}
		}
	}

	// reset the invalidate flag
	palette_lookups_invalid = 0;
}



//============================================================
//  update_visible_area
//============================================================

static void update_visible_area(mame_display *display)
{
#ifndef NEW_RENDER
	rectangle area = display->game_visible_area;

	// adjust for orientation
	win_orient_rect(&area);

	// track these changes
	logerror("Visible area set to %d-%d %d-%d\n", area.min_x, area.max_x, area.min_y, area.max_y);

	// now adjust the window for the aspect ratio
	if (area.max_x > area.min_x && area.max_y > area.min_y)
		win_adjust_window_for_visible(area.min_x, area.max_x, area.min_y, area.max_y);
#endif
}



//============================================================
//  update_autoframeskip
//============================================================

void update_autoframeskip(void)
{
	// don't adjust frameskip if we're paused or if the debugger was
	// visible this cycle or if we haven't run yet
	if (!game_was_paused && !debugger_was_visible && cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS)
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

	// clear the other states
	game_was_paused = game_is_paused;
	debugger_was_visible = 0;
}



//============================================================
//  render_frame
//============================================================

static void render_frame(mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels)
{
	cycles_t curr;

	// if we're throttling, synchronize
	if (throttle || game_is_paused)
		throttle_speed();

	// at the end, we need the current time
	curr = osd_cycles();

	// update stats for the FPS average calculation
	if (start_time == 0)
	{
		// start the timer going 1 second into the game
		if (timer_get_time() > 1.0)
			start_time = curr;
	}
	else
	{
		frames_displayed++;
		if (frames_displayed == frames_to_display)
		{
			char name[20];
			mame_file *fp;

			// make a filename with an underscore prefix
			sprintf(name, "_%.8s", Machine->gamedrv->name);

#ifndef NEW_RENDER
			// write out the screenshot
			if ((fp = mame_fopen(Machine->gamedrv->name, name, FILETYPE_SCREENSHOT, 1)) != NULL)
			{
				save_screen_snapshot_as(fp, artwork_get_ui_bitmap());
				mame_fclose(fp);
			}
#endif
			mame_schedule_exit();
		}
		end_time = curr;
	}

	// if we're at the start of a frameskip sequence, compute the speed
	if (frameskip_counter == 0)
		last_skipcount0_time = curr;

	// update the bitmap we're drawing
	profiler_mark(PROFILER_BLIT);
	winwindow_video_window_update(bitmap, bounds, vector_dirty_pixels);
	profiler_mark(PROFILER_END);

	// if we're throttling and autoframeskip is on, adjust
	if (throttle && autoframeskip && frameskip_counter == 0)
		update_autoframeskip();
}



//============================================================
//  osd_update_video_and_audio
//============================================================

void osd_update_video_and_audio(mame_display *display)
{
	rectangle updatebounds = display->game_bitmap_update;
	cycles_t cps = osd_cycles_per_second();

	// if this is the first time through, initialize the previous time value
	if (warming_up)
	{
		last_skipcount0_time = osd_cycles() - (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);
		warming_up = 0;
	}

	// if this is the first frame in a sequence, adjust the base time for this frame
	if (frameskip_counter == 0)
		this_frame_base = last_skipcount0_time + (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);

	// if the visible area has changed, update it
	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
		update_visible_area(display);

	// if the refresh rate has changed, update it
	if (display->changed_flags & GAME_REFRESH_RATE_CHANGED)
	{
		video_fps = display->game_refresh_rate;
		sound_update_refresh_rate(display->game_refresh_rate);
	}

#ifndef NEW_DEBUGGER
	// if the debugger focus changed, update it
	if (display->changed_flags & DEBUG_FOCUS_CHANGED)
		debugwin_set_focus(display->debug_focus);
#endif

	// if the game palette has changed, update it
	if (palette_lookups_invalid || (display->changed_flags & GAME_PALETTE_CHANGED))
		update_palette(display);

	// if we're not skipping this frame, draw it
	if (display->changed_flags & GAME_BITMAP_CHANGED)
	{
		win_orient_rect(&updatebounds);

		if (display->changed_flags & VECTOR_PIXELS_CHANGED)
			render_frame(display->game_bitmap, &updatebounds, display->vector_dirty_pixels);
		else
			render_frame(display->game_bitmap, &updatebounds, NULL);
	}

#ifndef NEW_DEBUGGER
	// update the debugger
	if (display->changed_flags & DEBUG_BITMAP_CHANGED)
	{
		debugwin_update_windows(display->debug_bitmap, display->debug_palette);
		debugger_was_visible = 1;
	}
#endif

	// if the LEDs have changed, update them
	if (display->changed_flags & LED_STATE_CHANGED)
		osd_set_leds(display->led_state);

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// check for inputs
	check_inputs();

	// poll the joystick values here
	winwindow_process_events(1);
	wininput_poll();
}


//============================================================
//  osd_override_snapshot
//============================================================

mame_bitmap *osd_override_snapshot(mame_bitmap *bitmap, rectangle *bounds)
{
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
	return copy;
}



//============================================================
//  win_pause
//============================================================

void win_pause(int paused)
{
	// note that we were paused during this autoframeskip cycle
	game_is_paused = paused;
	if (game_is_paused)
		game_was_paused = 1;

	// tell the input system
	win_pause_input(paused);
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

