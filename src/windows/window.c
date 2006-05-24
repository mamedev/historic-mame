//============================================================
//
//  window.c - Win32 window handling
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// Needed for RAW Input
#define _WIN32_WINNT 0x501
#define WM_INPUT 0x00FF

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

// standard C headers
#include <math.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"
#include "input.h"
#include "drawd3d.h"
#include "drawgdi.h"

#ifdef NEW_DEBUGGER
#include "debugwin.h"
#else
#include "../debug/window.h"
#endif

#ifdef MESS
#include "menu.h"
#endif /* MESS */



//============================================================
//  PARAMETERS
//============================================================

// window styles
#define WINDOW_STYLE			WS_OVERLAPPEDWINDOW
#define WINDOW_STYLE_EX			0

// debugger window styles
#define DEBUG_WINDOW_STYLE		WS_OVERLAPPED
#define DEBUG_WINDOW_STYLE_EX	0

// full screen window styles
#define FULLSCREEN_STYLE		WS_POPUP
#define FULLSCREEN_STYLE_EX		WS_EX_TOPMOST

// menu items
#define MENU_FULLSCREEN			1000

// minimum window dimension
#define MIN_WINDOW_DIM			200



//============================================================
//  GLOBAL VARIABLES
//============================================================

// command line config
int	win_window_mode;
int	win_wait_vsync;
int	win_triple_buffer;
int	win_use_ddraw;
int	win_use_d3d;
int	win_dd_hw_stretch;
int win_force_int_stretch;
int win_gfx_depth;
int win_gfx_zoom;
int win_old_scanlines;
int win_switch_res;
int win_switch_bpp;
int win_keep_aspect;
int win_gfx_refresh;
int win_match_refresh;
int win_sync_refresh;
float win_gfx_gamma;
int win_blit_effect;
float win_screen_aspect = (4.0f / 3.0f);

win_window_info *win_window_list;
static win_window_info **last_window_ptr;

// windows
HWND win_debug_window;

// video bounds
double win_aspect_ratio_adjust = 1.0;
int win_default_constraints;

// actual physical resolution
int win_physical_width;
int win_physical_height;

// raw mouse support
int win_use_raw_mouse = 0;



//============================================================
//  LOCAL VARIABLES
//============================================================

// event handling
static cycles_t last_event_check;

// debugger
static int in_background;



//============================================================
//  PROTOTYPES
//============================================================

static void win_window_exit(void);
static void win_window_destroy(win_window_info *window);
static void draw_video_contents(win_window_info *window, HDC dc, int update);

static void constrain_to_aspect_ratio(win_window_info *window, RECT *rect, int adjustment);
static void get_min_bounds(win_window_info *window, RECT *bounds, int constrain);
static void get_max_bounds(win_window_info *window, RECT *bounds, int constrain);

static void toggle_maximize(win_window_info *window, int force_maximize);
static void adjust_window_position_after_major_change(win_window_info *window);

static int create_window_class(void);
static void update_system_menu(win_window_info *window);



//============================================================
//  wnd_extra_width
//============================================================

INLINE int wnd_extra_width(win_window_info *window)
{
	RECT temprect = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
	AdjustWindowRectEx(&temprect, WINDOW_STYLE, win_has_menu(window), WINDOW_STYLE_EX);
	return rect_width(&temprect) - 100;
}



//============================================================
//  wnd_extra_height
//============================================================

INLINE int wnd_extra_height(win_window_info *window)
{
	RECT temprect = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
	AdjustWindowRectEx(&temprect, WINDOW_STYLE, win_has_menu(window), WINDOW_STYLE_EX);
	return rect_height(&temprect) - 100;
}



//============================================================
//  get_window_monitor
//============================================================

INLINE win_monitor_info *get_window_monitor(win_window_info *window, const RECT *proposed)
{
	win_monitor_info *monitor;

	// in window mode, find the nearest
	if (win_window_mode)
	{
		if (proposed != NULL)
			monitor = winvideo_monitor_from_handle(MonitorFromRect(proposed, MONITOR_DEFAULTTONEAREST));
		else
			monitor = winvideo_monitor_from_handle(MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTONEAREST));
	}

	// in full screen, just use the configured monitor
	else
		monitor = window->monitor;

	// make sure we're up-to-date
	winvideo_monitor_refresh(monitor);
	return monitor;
}



//============================================================
//  win_init_window
//============================================================

int win_window_init(void)
{
	// ensure we get called on the way out
	add_exit_callback(win_window_exit);

	// set up window class and register it
	if (create_window_class())
		return 1;

	// initialize
	last_window_ptr = &win_window_list;

	return 0;
}



//============================================================
//  win_window_exit
//============================================================

static void win_window_exit(void)
{
	// free all the windows
	while (win_window_list != NULL)
	{
		win_window_info *temp = win_window_list;
		win_window_list = temp->next;
		win_window_destroy(temp);
	}
}



//============================================================
//  win_window_create
//============================================================

int win_window_create(win_monitor_info *monitor, const char *layout, const char *view, int maxwidth, int maxheight, int switchres)
{
	win_window_info *window;
	int tempwidth, tempheight;
	int result = 0;
	HMENU menu = NULL;
	RECT monitorbounds, client;
	TCHAR title[256];
	HDC dc;

	// allocate a new window object
	window = malloc_or_die(sizeof(*window));
	memset(window, 0, sizeof(*window));
	window->maxwidth = maxwidth;
	window->maxheight = maxheight;
	window->switchres = switchres;
	window->monitor = monitor;

	// add us to the list
	*last_window_ptr = window;
	last_window_ptr = &window->next;

	// load the layout
	window->target = render_target_alloc(layout, FALSE);
	if (window->target == NULL)
		goto error;
	render_target_set_orientation(window->target, video_orientation);

	// set the specific view
	if (view != NULL)
	{
		int viewindex;

		// scan for a matching view name
		for (viewindex = 0; ; viewindex++)
		{
			const char *name = render_target_get_view_name(window->target, viewindex);

			// stop scanning if we hit NULL
			if (name == NULL)
				break;
			if (strcmp(name, view) == 0)
			{
				render_target_set_view(window->target, viewindex);
				break;
			}
		}
	}

	// remember the current values in case they change
	window->targetview = render_target_get_view(window->target);
	window->targetorient = render_target_get_orientation(window->target);

	// get the monitor bounds
	monitorbounds = monitor->info.rcMonitor;

	// make the window title
	sprintf(title, APPNAME ": %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);

	// create the window menu if needed
#if HAS_WINDOW_MENU
	if (win_create_menu(&menu))
		goto error;
#endif

	// create the window, but don't show it yet
	window->hwnd = CreateWindowEx(
						win_window_mode ? WINDOW_STYLE_EX : FULLSCREEN_STYLE_EX,
						TEXT("MAME"),
						title,
						win_window_mode ? WINDOW_STYLE : FULLSCREEN_STYLE,
						monitorbounds.left + 20, monitorbounds.top + 20,
						monitorbounds.left + 100, monitorbounds.top + 100,
						NULL,
						menu,
						GetModuleHandle(NULL),
						NULL);
	if (window->hwnd == NULL)
		goto error;

	// set a pointer back to us
	SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);

	// adjust the window position to the initial width/height
	tempwidth = (maxwidth != 0) ? maxwidth : 640;
	tempheight = (maxheight != 0) ? maxheight : 480;
	SetWindowPos(window->hwnd, NULL, monitorbounds.left + 20, monitorbounds.top + 20,
			monitorbounds.left + tempwidth + wnd_extra_width(window),
			monitorbounds.top + tempheight + wnd_extra_height(window),
			SWP_NOZORDER);

	// maximum or minimize as appropriate
	toggle_maximize(window, TRUE);
	if (!win_start_maximized)
		toggle_maximize(window, FALSE);
	adjust_window_position_after_major_change(window);

	// finish off by trying to initialize DirectX; if we fail, ignore it
	if (win_use_d3d)
		result = win_d3d_init(window);
	result = win_gdi_init(window);

	// show the window
	ShowWindow(window->hwnd, SW_SHOW);

	// clear the window
	dc = GetDC(window->hwnd);
	GetClientRect(window->hwnd, &client);
	FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
	ReleaseDC(window->hwnd, dc);

	// update system menu
	update_system_menu(window);
	return 0;

error:
	win_window_destroy(window);
	return 1;
}



//============================================================
//  win_window_destroy
//============================================================

static void win_window_destroy(win_window_info *window)
{
	win_window_info **prevptr;

	// remove us from the list
	for (prevptr = &win_window_list; *prevptr != NULL; prevptr = &(*prevptr)->next)
		if (*prevptr == window)
		{
			*prevptr = window->next;
			break;
		}

	// if we have a window, destroy it
	if (window->hwnd != NULL)
		DestroyWindow(window->hwnd);

	// free the render target
	if (window->target != NULL)
		render_target_free(window->target);

	// free the window itself
	free(window);
}



//============================================================
//  window_update_cursor_state
//============================================================

void window_update_cursor_state(void)
{
	static POINT last_cursor_pos = {-1,-1};
	RECT bounds;	// actual screen area of game video
	POINT video_ul;	// client area upper left corner
	POINT video_lr;	// client area lower right corner

	// store the cursor if just initialized
	if (win_use_raw_mouse && last_cursor_pos.x == -1 && last_cursor_pos.y == -1)
		GetCursorPos(&last_cursor_pos);

	if ((win_window_mode || win_has_menu(win_window_list)) && !win_is_mouse_captured())
	{
		// show cursor
		while (ShowCursor(TRUE) < 0) ;

		if (win_use_raw_mouse)
		{
			// allow cursor to move freely
			ClipCursor(NULL);
			// restore cursor to last position
			SetCursorPos(last_cursor_pos.x, last_cursor_pos.y);
		}
	}
	else
	{
		// hide cursor
		while (ShowCursor(FALSE) >= 0) ;

		if (win_use_raw_mouse)
		{
			// store the cursor position
			GetCursorPos(&last_cursor_pos);

			// clip cursor to game video window
			GetClientRect(win_window_list->hwnd, &bounds);
			video_ul.x = bounds.left;
			video_ul.y = bounds.top;
			video_lr.x = bounds.right;
			video_lr.y = bounds.bottom;
			ClientToScreen(win_window_list->hwnd, &video_ul);
			ClientToScreen(win_window_list->hwnd, &video_lr);
			SetRect(&bounds, video_ul.x, video_ul.y, video_lr.x, video_lr.y);
			ClipCursor(&bounds);
		}
	}
}



//============================================================
//  win_update_video_window
//============================================================

void win_update_video_window(win_window_info *window)
{
	int targetview, targetorient;

	// see if the target has changed significantly in window mode
	targetview = render_target_get_view(window->target);
	targetorient = render_target_get_orientation(window->target);
	if (targetview != window->targetview || targetorient != window->targetorient)
	{
		window->targetview = targetview;
		window->targetorient = targetorient;

		// in window mode, remaximize
		if (win_window_mode && window->ismaximized)
			toggle_maximize(window, TRUE);
	}

	// track whether or not we are maximized
	if (win_window_mode)
	{
		RECT bounds, maxbounds;

		// compare the maximum bounds versus the current bounds
		get_max_bounds(window, &maxbounds, TRUE);
		GetWindowRect(window->hwnd, &bounds);

		// if either the width or height matches, we were maximized
		window->ismaximized = (rect_width(&bounds) == rect_width(&maxbounds) ||
								rect_height(&bounds) == rect_height(&maxbounds));
	}

	// get the client DC and draw to it
	if (window->hwnd)
	{
		HDC dc = GetDC(window->hwnd);
		draw_video_contents(window, dc, FALSE);
		ReleaseDC(window->hwnd, dc);
	}
}



//============================================================
//  draw_video_contents
//============================================================

static void draw_video_contents(win_window_info *window, HDC dc, int update)
{
	// if we're iconic, don't bother
	if (window->hwnd == NULL || IsIconic(window->hwnd))
		return;

	// get the primitives list
	if (window->target != NULL && window->resize_state == RESIZE_STATE_NORMAL)
	{
		// ensure the target bounds are up-to-date, and then get the primitives
		RECT client;
		GetClientRect(window->hwnd, &client);
		render_target_set_bounds(window->target, rect_width(&client), rect_height(&client), winvideo_monitor_get_aspect(window->monitor));
		window->primlist = render_target_get_primitives(window->target);
	}

	// if no bitmap, just fill
	if (window->primlist == NULL)
	{
		RECT fill;
		GetClientRect(window->hwnd, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}

	// if we have a blit surface, use that
	if (win_use_d3d && win_d3d_draw(window, dc, window->primlist, update) == 0)
		return;
	win_gdi_draw(window, dc, window->primlist, update);
}



//============================================================
//  win_video_window_proc
//============================================================

LRESULT CALLBACK win_video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	LONG_PTR ptr = GetWindowLongPtr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	extern void win_timer_enable(int enabled);

	// handle a few messages
	switch (message)
	{
		// input: handle the raw mouse input
		case WM_INPUT:
		{
			if (win_use_raw_mouse)
				win_raw_mouse_update((HRAWINPUT)lparam);
			break;
		}

		// paint: redraw the last bitmap
		case WM_PAINT:
		{
			PAINTSTRUCT pstruct;
			HDC hdc = BeginPaint(wnd, &pstruct);
			draw_video_contents(window, hdc, TRUE);
 			if (win_has_menu(window))
 				DrawMenuBar(window->hwnd);
			EndPaint(wnd, &pstruct);
			break;
		}

#if !HAS_WINDOW_MENU
		// non-client paint: punt if full screen
		case WM_NCPAINT:
			if (win_window_mode)
				return DefWindowProc(wnd, message, wparam, lparam);
			break;
#endif /* !HAS_WINDOW_MENU */

		// suspend sound and timer if we are resizing or a menu is coming up
		case WM_ENTERSIZEMOVE:
			window->resize_state = RESIZE_STATE_RESIZING;
		case WM_ENTERMENULOOP:
			osd_sound_enable(FALSE);
			win_timer_enable(FALSE);
			break;

		// resume sound and timer if we dome with resizing or a menu
		case WM_EXITSIZEMOVE:
			window->resize_state = RESIZE_STATE_PENDING;
		case WM_EXITMENULOOP:
			osd_sound_enable(TRUE);
			win_timer_enable(TRUE);
			break;

		// get min/max info: set the minimum window size
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO *minmax = (MINMAXINFO *)lparam;
			minmax->ptMinTrackSize.x = MIN_WINDOW_DIM;
			minmax->ptMinTrackSize.y = MIN_WINDOW_DIM;
			break;
		}

		// sizing: constrain to the aspect ratio unless control key is held down
		case WM_SIZING:
		{
			RECT *rect = (RECT *)lparam;
			if (win_keep_aspect && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
				constrain_to_aspect_ratio(window, rect, wparam);
			InvalidateRect(wnd, NULL, FALSE);
			break;
		}

		// syscommands: catch win_start_maximized
		case WM_SYSCOMMAND:
		{
			// prevent screensaver or monitor power events
			if (wparam == SC_MONITORPOWER || wparam == SC_SCREENSAVE)
				return 1;

			// most SYSCOMMANDs require us to invalidate the window
			InvalidateRect(wnd, NULL, FALSE);

			// handle maximize
			if ((wparam & 0xfff0) == SC_MAXIMIZE)
			{
				toggle_maximize(window, FALSE);
				break;
			}

			// handle full screen mode
			else if (wparam == MENU_FULLSCREEN)
			{
				win_toggle_full_screen();
				break;
			}
			return DefWindowProc(wnd, message, wparam, lparam);
		}

		// close: handle clicks on the close box
		case WM_CLOSE:
			mame_schedule_exit();
			break;

		// destroy: close down the app
		case WM_DESTROY:
			if (win_use_d3d)
				win_d3d_kill(window);
			win_gdi_kill(window);
			window->hwnd = NULL;
			break;

		// track whether we are in the foreground
		case WM_ACTIVATEAPP:
			in_background = !wparam;
			break;

		// everything else: defaults
		default:
			return DefWindowProc(wnd, message, wparam, lparam);
	}

	return 0;
}



//============================================================
//  constrain_to_aspect_ratio
//============================================================

static void constrain_to_aspect_ratio(win_window_info *window, RECT *rect, int adjustment)
{
	win_monitor_info *monitor = get_window_monitor(window, rect);
	INT32 extrawidth = wnd_extra_width(window);
	INT32 extraheight = wnd_extra_height(window);
	INT32 propwidth, propheight;
	INT32 minwidth, minheight;
	INT32 maxwidth, maxheight;
	INT32 viswidth, visheight;
	INT32 adjwidth, adjheight;
	float pixel_aspect;

	// get the pixel aspect ratio for the target monitor
	pixel_aspect = winvideo_monitor_get_aspect(monitor);

	// determine the proposed width/height
	propwidth = rect_width(rect) - extrawidth;
	propheight = rect_height(rect) - extraheight;

	// based on which edge we are adjusting, take either the width, height, or both as gospel
	// and scale to fit using that as our parameter
	switch (adjustment)
	{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
			render_target_compute_visible_area(window->target, 10000, propheight, pixel_aspect, video_orientation, &propwidth, &propheight);
			break;

		case WMSZ_LEFT:
		case WMSZ_RIGHT:
			render_target_compute_visible_area(window->target, propwidth, 10000, pixel_aspect, video_orientation, &propwidth, &propheight);
			break;

		default:
			render_target_compute_visible_area(window->target, propwidth, propheight, pixel_aspect, video_orientation, &propwidth, &propheight);
			break;
	}

	// get the minimum width/height for the current layout
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// clamp against the absolute minimum
	if (propwidth < MIN_WINDOW_DIM)
		propwidth = MIN_WINDOW_DIM;
	if (propheight < MIN_WINDOW_DIM)
		propheight = MIN_WINDOW_DIM;

	// clamp against the minimum width and height
	if (propwidth < minwidth)
		propwidth = minwidth;
	if (propheight < minheight)
		propheight = minheight;

	// clamp against the maximum (fit on one screen for full screen mode)
	if (!win_window_mode)
	{
		maxwidth = rect_width(&monitor->info.rcMonitor) - extrawidth;
		maxheight = rect_height(&monitor->info.rcMonitor) - extraheight;
	}
	else
	{
		maxwidth = rect_width(&monitor->info.rcWork) - extrawidth;
		maxheight = rect_height(&monitor->info.rcWork) - extraheight;

		// further clamp to the maximum width/height in the window
		if (window->maxwidth != 0)
		{
			int temp = window->maxwidth + wnd_extra_width(window);
			maxwidth = MIN(maxwidth, temp);
		}
		if (window->maxheight != 0)
		{
			int temp = window->maxheight + wnd_extra_height(window);
			maxheight = MIN(maxheight, temp);
		}
	}

	// clamp to the maximum
	if (propwidth > maxwidth)
		propwidth = maxwidth;
	if (propheight > maxheight)
		propheight = maxheight;

	// compute the visible area based on the proposed rectangle
	render_target_compute_visible_area(window->target, propwidth, propheight, pixel_aspect, video_orientation, &viswidth, &visheight);

	// compute the adjustments we need to make
	adjwidth = (viswidth + extrawidth) - rect_width(rect);
	adjheight = (visheight + extraheight) - rect_height(rect);

	// based on which corner we're adjusting, constrain in different ways
	switch (adjustment)
	{
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
		case WMSZ_RIGHT:
			rect->right += adjwidth;
			rect->bottom += adjheight;
			break;

		case WMSZ_BOTTOMLEFT:
			rect->left -= adjwidth;
			rect->bottom += adjheight;
			break;

		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_TOP:
			rect->left -= adjwidth;
			rect->top -= adjheight;
			break;

		case WMSZ_TOPRIGHT:
			rect->right += adjwidth;
			rect->top -= adjheight;
			break;
	}
}



//============================================================
//  get_min_bounds
//============================================================

static void get_min_bounds(win_window_info *window, RECT *bounds, int constrain)
{
	INT32 minwidth, minheight;

	// get the minimum target size
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// expand to our minimum dimensions
	if (minwidth < MIN_WINDOW_DIM)
		minwidth = MIN_WINDOW_DIM;
	if (minheight < MIN_WINDOW_DIM)
		minheight = MIN_WINDOW_DIM;

	// account for extra window stuff
	minwidth += wnd_extra_width(window);
	minheight += wnd_extra_height(window);

	// if we want it constrained, figure out which one is larger
	if (constrain)
	{
		RECT test1, test2;

		// first constrain with no height limit
		test1.top = test1.left = 0;
		test1.right = minwidth;
		test1.bottom = 10000;
		constrain_to_aspect_ratio(window, &test1, WMSZ_BOTTOMRIGHT);

		// then constrain with no width limit
		test2.top = test2.left = 0;
		test2.right = 10000;
		test2.bottom = minheight;
		constrain_to_aspect_ratio(window, &test2, WMSZ_BOTTOMRIGHT);

		// pick the larger
		if (rect_width(&test1) > rect_width(&test2))
		{
			minwidth = rect_width(&test1);
			minheight = rect_height(&test1);
		}
		else
		{
			minwidth = rect_width(&test2);
			minheight = rect_height(&test2);
		}
	}

	// get the client rect
	GetWindowRect(window->hwnd, bounds);

	// now adjust
	bounds->right = bounds->left + minwidth;
	bounds->bottom = bounds->top + minheight;
}



//============================================================
//  get_max_bounds
//============================================================

static void get_max_bounds(win_window_info *window, RECT *bounds, int constrain)
{
	RECT maximum;

	// compute the maximum client area
	winvideo_monitor_refresh(window->monitor);
	maximum = window->monitor->info.rcWork;

	// clamp to the window's max
	if (window->maxwidth != 0)
	{
		int temp = window->maxwidth + wnd_extra_width(window);
		if (temp < rect_width(&maximum))
			maximum.right = maximum.left + temp;
	}
	if (window->maxheight != 0)
	{
		int temp = window->maxheight + wnd_extra_height(window);
		if (temp < rect_height(&maximum))
			maximum.bottom = maximum.top + temp;
	}

	// constrain to fit
	if (constrain)
		constrain_to_aspect_ratio(window, &maximum, WMSZ_BOTTOMRIGHT);
	else
	{
		maximum.right -= wnd_extra_width(window);
		maximum.bottom -= wnd_extra_height(window);
	}

	// center within the work area
	bounds->left = window->monitor->info.rcWork.left + (rect_width(&window->monitor->info.rcWork) - rect_width(&maximum)) / 2;
	bounds->top = window->monitor->info.rcWork.top + (rect_height(&window->monitor->info.rcWork) - rect_height(&maximum)) / 2;
	bounds->right = bounds->left + rect_width(&maximum);
	bounds->bottom = bounds->top + rect_height(&maximum);
}



//============================================================
//  toggle_maximize
//============================================================

static void toggle_maximize(win_window_info *window, int force_maximize)
{
	RECT cursize, newsize;

	// get the current position
	GetWindowRect(window->hwnd, &cursize);

	// get the max size
	get_max_bounds(window, &newsize, TRUE);

	// if we're forcing maximum, we're done
	if (!force_maximize && rect_width(&cursize) == rect_width(&newsize) && rect_height(&cursize) == rect_height(&newsize))
		get_min_bounds(window, &newsize, TRUE);

	// set the new position
	SetWindowPos(window->hwnd, NULL, newsize.left, newsize.top,
			rect_width(&newsize), rect_height(&newsize),
			SWP_NOZORDER);
}



//============================================================
//  win_toggle_full_screen
//============================================================

void win_toggle_full_screen(void)
{
	win_window_info *window;

#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options.mame_debug)
		return;
#endif

	// hide all the windows
	for (window = win_window_list; window != NULL; window = window->next)
	{
		if (win_use_d3d)
			win_d3d_kill(window);
		win_gdi_kill(window);
		ShowWindow(window->hwnd, SW_HIDE);
	}

#ifdef MAME_DEBUG
	if (win_window_mode)
		debugwin_show(SW_HIDE);
#endif

	// toggle the window mode
	win_window_mode = !win_window_mode;

	// adjust the window style and z order while everyone is hidden
	for (window = win_window_list; window != NULL; window = window->next)
	{
		if (win_window_mode)
		{
			// adjust the style
			SetWindowLong(window->hwnd, GWL_STYLE, WINDOW_STYLE);
			SetWindowLong(window->hwnd, GWL_EXSTYLE, WINDOW_STYLE_EX);
			SetWindowPos(window->hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			// force to the bottom, then back on top
			SetWindowPos(window->hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(window->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			// if we have previous non-fullscreen bounds, use those
			if (window->non_fullscreen_bounds.right != window->non_fullscreen_bounds.left)
			{
				SetWindowPos(window->hwnd, HWND_TOP, window->non_fullscreen_bounds.left, window->non_fullscreen_bounds.top,
							rect_width(&window->non_fullscreen_bounds), rect_height(&window->non_fullscreen_bounds),
							SWP_NOZORDER);
			}

			// otherwise, set a small size and maximize from there
			else
			{
				SetWindowPos(window->hwnd, HWND_TOP, 0, 0, MIN_WINDOW_DIM, MIN_WINDOW_DIM, SWP_NOZORDER);
				toggle_maximize(window, TRUE);
			}
		}
		else
		{
			// save the bounds
			GetWindowRect(window->hwnd, &window->non_fullscreen_bounds);

			// adjust the style
			SetWindowLong(window->hwnd, GWL_STYLE, FULLSCREEN_STYLE);
			SetWindowLong(window->hwnd, GWL_EXSTYLE, FULLSCREEN_STYLE_EX);
			SetWindowPos(window->hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			// set topmost
			SetWindowPos(window->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		// adjust the window to compensate for the change
		adjust_window_position_after_major_change(window);
	}

	// show all the windows
	for (window = win_window_list; window != NULL; window = window->next)
	{
		ShowWindow(window->hwnd, SW_SHOW);
		if (win_use_d3d && win_d3d_init(window))
			exit(1);
		if (win_gdi_init(window))
			exit(1);
	}

#ifdef MAME_DEBUG
	if (win_window_mode)
		debugwin_show(SW_SHOW);
#endif

	// make sure the window is properly readjusted
	for (window = win_window_list; window != NULL; window = window->next)
	{
		adjust_window_position_after_major_change(window);

		// update the system menu
		update_system_menu(window);
	}
}



//============================================================
//  adjust_window_position_after_major_change
//============================================================

static void adjust_window_position_after_major_change(win_window_info *window)
{
	RECT oldrect, newrect;

	// get the current size
	GetWindowRect(window->hwnd, &oldrect);

	// adjust the window size so the client area is what we want
	if (win_window_mode)
	{
		// constrain the existing size to the aspect ratio
		newrect = oldrect;
		constrain_to_aspect_ratio(window, &newrect, WMSZ_BOTTOMRIGHT);
	}

	// in full screen, make sure it covers the primary display
	else
	{
		win_monitor_info *monitor = get_window_monitor(window, NULL);
		newrect = monitor->info.rcMonitor;
	}

	// adjust the position if different
	if (oldrect.left != newrect.left || oldrect.top != newrect.top ||
		oldrect.right != newrect.right || oldrect.bottom != newrect.bottom)
		SetWindowPos(window->hwnd, win_window_mode ? HWND_TOP : HWND_TOPMOST,
				newrect.left, newrect.top,
				rect_width(&newrect), rect_height(&newrect), 0);

	// take note of physical window size (used for lightgun coordinate calculation)
	if (window == win_window_list)
	{
		win_physical_width = rect_width(&newrect);
		win_physical_height = rect_height(&newrect);
		logerror("Physical width %d, height %d\n",win_physical_width,win_physical_height);
	}

	// update the cursor state
	window_update_cursor_state();
}



//============================================================
//  winwindow_process_events_periodic
//============================================================

void winwindow_process_events_periodic(void)
{
	cycles_t curr = osd_cycles();
	if (curr - last_event_check < osd_cycles_per_second() / 8)
		return;
	winwindow_process_events(TRUE);
}



//============================================================
//  winwindow_process_events
//============================================================

int winwindow_process_events(int ingame)
{
	int is_debugger_visible = 0;
	MSG message;

	// if we're running, disable some parts of the debugger
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	if (ingame)
	{
		is_debugger_visible = (options.mame_debug && debugwin_is_debugger_visible());
		debugwin_update_during_game();
	}
#endif

	// remember the last time we did this
	last_event_check = osd_cycles();

	// loop over all messages in the queue
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
	{
		int dispatch = 1;

		switch (message.message)
		{
			// special case for quit
			case WM_QUIT:
				exit(0);
				break;

			// ignore keyboard messages
			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
#ifndef MESS
			case WM_KEYUP:
			case WM_KEYDOWN:
			case WM_CHAR:
#endif
				dispatch = is_debugger_visible;
				break;

			case WM_LBUTTONDOWN:
				input_mouse_button_down(0, GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;

			case WM_RBUTTONDOWN:
				input_mouse_button_down(1, GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;

			case WM_MBUTTONDOWN:
				input_mouse_button_down(2, GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;

			case WM_XBUTTONDOWN:
				input_mouse_button_down(3, GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;

			case WM_LBUTTONUP:
				input_mouse_button_up(0);
				dispatch = is_debugger_visible;
				break;

			case WM_RBUTTONUP:
				input_mouse_button_up(1);
				dispatch = is_debugger_visible;
				break;

			case WM_MBUTTONUP:
				input_mouse_button_up(2);
				dispatch = is_debugger_visible;
				break;

			case WM_XBUTTONUP:
				input_mouse_button_up(3);
				dispatch = is_debugger_visible;
				break;
		}

		// dispatch if necessary
		if (dispatch)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	// return 1 if we slept this frame
	return 0;
}



//============================================================
//  create_window_class
//============================================================

static int create_window_class(void)
{
	static int classes_created = FALSE;

	if (!classes_created)
	{
		WNDCLASS wc = { 0 };

		// initialize the description of the window class
		wc.lpszClassName 	= TEXT("MAME");
		wc.hInstance 		= GetModuleHandle(NULL);
#ifdef MESS
		wc.lpfnWndProc		= win_mess_window_proc;
#else
		wc.lpfnWndProc		= win_video_window_proc;
#endif
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
		wc.lpszMenuName		= NULL;
		wc.hbrBackground	= NULL;
		wc.style			= 0;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;

		// register the class; fail if we can't
		if (!RegisterClass(&wc))
			return 1;
		classes_created = TRUE;
	}

	return 0;
}



//============================================================
//  update_system_menu
//============================================================

static void update_system_menu(win_window_info *window)
{
	HMENU menu;

	// revert the system menu
	GetSystemMenu(window->hwnd, TRUE);

	// add to the system menu
	menu = GetSystemMenu(window->hwnd, FALSE);
	if (menu)
		AppendMenu(menu, MF_ENABLED | MF_STRING, MENU_FULLSCREEN, "Full Screen\tAlt+Enter");
}
