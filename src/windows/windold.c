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
#include "windold.h"
#include "winddold.h"
#include "wind3old.h"
#include "videoold.h"
#include "blit.h"
#include "input.h"
#include "../debug/window.h"

#ifdef NEW_DEBUGGER
#include "debugwin.h"
#endif

#ifdef MESS
#include "menu.h"
#endif /* MESS */



//============================================================
//  IMPORTS
//============================================================

// from input.c
extern void win_pause_input(int pause);
extern int win_is_mouse_captured(void);

// from video.c
extern HMONITOR monitor;

// from wind3dfx.c
int win_d3d_effects_in_use(void);



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
int	win_gfx_width;
int	win_gfx_height;
int win_gfx_depth;
int win_gfx_zoom;
int win_old_scanlines;
int win_switch_res;
int win_switch_bpp;
int win_start_maximized;
int win_keep_aspect;
int win_gfx_refresh;
int win_match_refresh;
int win_sync_refresh;
float win_gfx_gamma;
int win_blit_effect;
float win_screen_aspect = (4.0f / 3.0f);

// windows
HWND win_video_window;
HWND win_debug_window;

// video bounds
double win_aspect_ratio_adjust = 1.0;
int win_default_constraints;

// visible bounds
RECT win_visible_rect;
int win_visible_width;
int win_visible_height;

// 16bpp color conversion
int win_color16_rsrc_shift = 3;
int win_color16_gsrc_shift = 3;
int win_color16_bsrc_shift = 3;
int win_color16_rdst_shift = 10;
int win_color16_gdst_shift = 5;
int win_color16_bdst_shift = 0;

// 32bpp color conversion
int win_color32_rdst_shift = 16;
int win_color32_gdst_shift = 8;
int win_color32_bdst_shift = 0;

// actual physical resolution
int win_physical_width;
int win_physical_height;

// raw mouse support
int win_use_raw_mouse = 0;



//============================================================
//  LOCAL VARIABLES
//============================================================

// config
static int win_use_directx;
#define USE_DDRAW 1
#define USE_D3D 2

// DIB bitmap data
static UINT8 video_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *video_dib_info = (BITMAPINFO *)video_dib_info_data;
static UINT8 *converted_bitmap;

// video bounds
static double aspect_ratio;

// visible bounds
static int visible_area_set;

// event handling
static cycles_t last_event_check;

// derived attributes
static int pixel_aspect_ratio;

// cached bounding rects
static RECT non_fullscreen_bounds;
static RECT non_maximized_bounds;

// debugger
static int in_background;

#ifndef NEW_DEBUGGER
static int debug_focus;
static UINT8 debug_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *debug_dib_info = (BITMAPINFO *)debug_dib_info_data;
#endif

// effects table
static const win_effect_data effect_table[] =
{
	{ "none",    EFFECT_NONE,        1, 1, 3, 4 },
	{ "scan25",  EFFECT_SCANLINE_25, 1, 2, 3, 4 },
	{ "scan50",  EFFECT_SCANLINE_50, 1, 2, 3, 4 },
	{ "scan75",  EFFECT_SCANLINE_75, 1, 2, 3, 4 },
	{ "rgb16",   EFFECT_RGB16,       2, 2, 2, 2 },
	{ "rgb6",    EFFECT_RGB6,        2, 2, 2, 2 },
	{ "rgb4",    EFFECT_RGB4,        2, 2, 2, 2 },
	{ "rgb4v",   EFFECT_RGB4V,       2, 2, 2, 2 },
	{ "rgb3",    EFFECT_RGB3,        2, 2, 2, 2 },
	{ "rgbtiny", EFFECT_RGB_TINY,    2, 2, 2, 2 },
	{ "scan75v", EFFECT_SCANLINE_75V,2, 2, 2, 2 },
	{ "sharp",   EFFECT_SHARP,       2, 2, 2, 2 },
};

static mame_bitmap *last_bitmap;


//============================================================
//  PROTOTYPES
//============================================================

static void compute_multipliers_internal(const RECT *rect, int visible_width, int visible_height, int *xmult, int *ymult);
static void update_system_menu(void);
static void draw_video_contents(HDC dc, mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels, int update);

static void dib_draw_window(HDC dc, mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels, int update);

#ifndef NEW_DEBUGGER
static LRESULT CALLBACK debug_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
static void draw_debug_contents(HDC dc, mame_bitmap *bitmap, const rgb_t *palette);
#endif



//============================================================
//  wnd_extra_width
//============================================================

INLINE int wnd_extra_width(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, win_has_menu(), WINDOW_STYLE_EX);
	return (window.right - window.left) - 100;
}



//============================================================
//  wnd_extra_height
//============================================================

INLINE int wnd_extra_height(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, win_has_menu(), WINDOW_STYLE_EX);
	return (window.bottom - window.top) - 100;
}



//============================================================
//  get_screen_bounds
//============================================================

INLINE void get_screen_bounds(RECT *bounds)
{
	MONITORINFO monitor_info = { sizeof(MONITORINFO) };

	assert_always(GetMonitorInfo(monitor, &monitor_info), "Failed to get monitor info");

	*bounds = monitor_info.rcMonitor;
}



//============================================================
//  erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, HDC dc)
{
	HBRUSH brush = GetStockObject(BLACK_BRUSH);
	RECT clear;

	// clear the left edge
	if (inner->left > outer->left)
	{
		clear = *outer;
		clear.right = inner->left;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
		if (dc)
			FillRect(dc, &clear, brush);
	}
}



//============================================================
//  get_work_area
//============================================================

INLINE void get_work_area(RECT *maximum)
{
	int tempwidth = blit_swapxy ? win_gfx_height : win_gfx_width;
	int tempheight = blit_swapxy ? win_gfx_width : win_gfx_height;

	MONITORINFO monitor_info = { sizeof(MONITORINFO) };

	assert_always(GetMonitorInfo(monitor, &monitor_info), "Failed to get monitor info");

	*maximum = monitor_info.rcWork;

	// clamp to the width specified
	if (tempwidth && (maximum->right - maximum->left) > (tempwidth + wnd_extra_width()))
	{
		int diff = (maximum->right - maximum->left) - (tempwidth + wnd_extra_width());
		if (diff > 0)
		{
			maximum->left += diff / 2;
			maximum->right -= diff - (diff / 2);
		}
	}

	// clamp to the height specified
	if (tempheight && (maximum->bottom - maximum->top) > (tempheight + wnd_extra_height()))
	{
		int diff = (maximum->bottom - maximum->top) - (tempheight + wnd_extra_height());
		if (diff > 0)
		{
			maximum->top += diff / 2;
			maximum->bottom -= diff - (diff / 2);
		}
	}
}



//============================================================
//  win_init_window
//============================================================

int win_init_window(void)
{
	static int classes_created = FALSE;
	TCHAR title[256];
	HMENU menu = NULL;

	// if we already have a window, just leave it alone
	if (win_video_window)
		return 0;

#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options.mame_debug)
		win_window_mode = 1;
#endif

	// disable win_old_scanlines if a win_blit_effect is active
	if (win_blit_effect != 0)
		win_old_scanlines = 0;

	// set up window class and register it
	if (!classes_created)
	{
		WNDCLASS wc = { 0 };

		// initialize the description of the window class
		wc.lpszClassName 	= TEXT("MAME");
		wc.hInstance 		= GetModuleHandle(NULL);
#ifdef MESS
		wc.lpfnWndProc		= win_mess_window_proc;
#else
		wc.lpfnWndProc		= winwindow_video_window_proc;
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

	// make the window title
	sprintf(title, APPNAME ": %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);

#if HAS_WINDOW_MENU
	if (win_create_menu(&menu))
		return 1;
#endif

	// create the window, but don't show it yet
	win_video_window = CreateWindowEx(win_window_mode ? WINDOW_STYLE_EX : FULLSCREEN_STYLE_EX,
			TEXT("MAME"), title, win_window_mode ? WINDOW_STYLE : FULLSCREEN_STYLE,
			20, 20, 100, 100, NULL, menu, GetModuleHandle(NULL), NULL);
	if (!win_video_window)
		return 1;

	// possibly create the debug window, but don't show it yet
#ifdef MAME_DEBUG
	if (options.mame_debug)
		if (debugwin_init_windows())
			return 1;
#endif

	// update system menu
	update_system_menu();
	return 0;
}



//============================================================
//  win_create_window
//============================================================

int win_create_window(int width, int height, int depth, int attributes, double aspect)
{
	int i, result = 0;

	// clear the initial state
	last_bitmap = NULL;
	visible_area_set = 0;

	// extract useful parameters from the attributes
	pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);

	// handle failure if we couldn't create the video window
	if (!win_video_window)
		return 1;

	// adjust the window position
	SetWindowPos(win_video_window, NULL, 20, 20,
			width + wnd_extra_width() + 2, height + wnd_extra_height() + 2,
			SWP_NOZORDER);

	// make sure we paint the window once here
	winwindow_video_window_update(NULL, NULL, NULL);

	// fill in the bitmap info header
	video_dib_info->bmiHeader.biSize			= sizeof(video_dib_info->bmiHeader);
	video_dib_info->bmiHeader.biPlanes			= 1;
	video_dib_info->bmiHeader.biCompression		= BI_RGB;
	video_dib_info->bmiHeader.biSizeImage		= 0;
	video_dib_info->bmiHeader.biXPelsPerMeter	= 0;
	video_dib_info->bmiHeader.biYPelsPerMeter	= 0;
	video_dib_info->bmiHeader.biClrUsed			= 0;
	video_dib_info->bmiHeader.biClrImportant	= 0;

	// initialize the palette to a gray ramp
	for (i = 0; i < 255; i++)
	{
		video_dib_info->bmiColors[i].rgbRed			= i;
		video_dib_info->bmiColors[i].rgbGreen		= i;
		video_dib_info->bmiColors[i].rgbBlue		= i;
		video_dib_info->bmiColors[i].rgbReserved	= i;
	}

	win_default_constraints = 0;

	// Determine which DirectX components to use
	if (win_use_d3d)
		win_use_directx = USE_D3D;
	else if (win_use_ddraw)
		win_use_directx = USE_DDRAW;

	// determine the aspect ratio: hardware stretch case
	if (win_force_int_stretch != FORCE_INT_STRECT_FULL && (win_use_directx == USE_D3D || (win_use_directx == USE_DDRAW && win_dd_hw_stretch)))
	{
		aspect_ratio = aspect;
	}
	// determine the aspect ratio: software stretch / full cleanstretch case
	else
	{
		aspect_ratio = (double)width / (double)height;
		if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		{
			if (!blit_swapxy)
				aspect_ratio *= 2.0;
			else
				aspect_ratio /= 2.0;
		}
		else if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		{
			if (!blit_swapxy)
				aspect_ratio /= 2.0;
			else
				aspect_ratio *= 2.0;
		}
	}

	win_default_constraints = 0;
	switch (win_force_int_stretch)
	{
		// contrain height for full cleanstretch
		case FORCE_INT_STRECT_FULL:
			win_default_constraints = blit_swapxy ? CONSTRAIN_INTEGER_HEIGHT : CONSTRAIN_INTEGER_WIDTH;
			break;
		// contrain width (relative to the game)
		case FORCE_INT_STRECT_HOR:
			win_default_constraints = blit_swapxy ? CONSTRAIN_INTEGER_HEIGHT : CONSTRAIN_INTEGER_WIDTH;
			break;
		// contrain height (relative to the game)
		case FORCE_INT_STRECT_VER:
			win_default_constraints = blit_swapxy ? CONSTRAIN_INTEGER_WIDTH : CONSTRAIN_INTEGER_HEIGHT;
			break;
	}

	// finish off by trying to initialize DirectX
	if (win_use_directx)
	{
		if (win_use_directx == USE_D3D)
			result = win_d3d_init(width, height, depth, attributes, aspect_ratio, &effect_table[win_blit_effect]);
		else
			result = win_ddraw_init(width, height, depth, attributes, &effect_table[win_blit_effect]);
	}

	// warn the user if effects for an inactive/possibly inapropriate effects engine are selected
	if (win_use_directx == USE_D3D)
	{
		if (win_blit_effect)
			fprintf(stderr, "Warning: non-hardware-accelerated blitting-effects engine enabled\n         use the -d3deffect option to enable hardware acceleration\n");
	}
	else
	{
		if (win_d3d_effects_in_use())
			fprintf(stderr, "Warning: hardware-accelerated blitting-effects selected, but currently disabled\n         use the -direct3d option to enable hardware acceleration\n");
	}

	// return directx initialisation status
	if (win_use_directx)
		return result;

	return 0;
}



//============================================================
//  win_destroy_window
//============================================================

void win_destroy_window(void)
{
	// possibly kill the debug window
#ifdef MAME_DEBUG
	if (options.mame_debug)
		debugwin_destroy_windows();
#endif

	// kill directdraw
	if (win_use_directx)
	{
		if (win_use_directx == USE_D3D)
			win_d3d_kill();
		else
			win_ddraw_kill();
	}

	// kill the window if it still exists
	if (win_video_window)
		DestroyWindow(win_video_window);

	// free converted bitmap, if allocated
	if (converted_bitmap)
	{
		free(converted_bitmap);
		converted_bitmap = NULL;
	}
}



//============================================================
//  winwindow_update_cursor_state
//============================================================

void winwindow_update_cursor_state(void)
{
	static POINT last_cursor_pos = {-1,-1};
	RECT bounds;	// actual screen area of game video
	POINT video_ul;	// client area upper left corner
	POINT video_lr;	// client area lower right corner

	// store the cursor if just initialized
	if (win_use_raw_mouse && last_cursor_pos.x == -1 && last_cursor_pos.y == -1) GetCursorPos(&last_cursor_pos);

	if ((win_window_mode || win_has_menu()) && !win_is_mouse_captured())
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
			GetClientRect(win_video_window, &bounds);
			video_ul.x = bounds.left;
			video_ul.y = bounds.top;
			video_lr.x = bounds.right;
			video_lr.y = bounds.bottom;
			ClientToScreen(win_video_window, &video_ul);
			ClientToScreen(win_video_window, &video_lr);
			SetRect(&bounds, video_ul.x, video_ul.y, video_lr.x, video_lr.y);
			ClipCursor(&bounds);
		}
	}
}



//============================================================
//  update_system_menu
//============================================================

static void update_system_menu(void)
{
	HMENU menu;

	// revert the system menu
	GetSystemMenu(win_video_window, TRUE);

	// add to the system menu
	menu = GetSystemMenu(win_video_window, FALSE);
	if (menu)
		AppendMenu(menu, MF_ENABLED | MF_STRING, MENU_FULLSCREEN, "Full Screen\tAlt+Enter");
}



//============================================================
//  winwindow_video_window_update
//============================================================

void winwindow_video_window_update(mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels)
{
	// get the client DC and draw to it
	if (win_video_window)
	{
		HDC dc = GetDC(win_video_window);
		draw_video_contents(dc, bitmap, bounds, vector_dirty_pixels, 0);
		ReleaseDC(win_video_window, dc);
	}
}



//============================================================
//  draw_video_contents
//============================================================

static void draw_video_contents(HDC dc, mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels, int update)
{
	// if no bitmap, use the last one we got
	if (bitmap == NULL)
		bitmap = last_bitmap;

	// if no bitmap, just fill
	if (bitmap == NULL)
	{
		RECT fill;
		GetClientRect(win_video_window, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}
	last_bitmap = bitmap;

	// if we're iconic, don't bother
	if (IsIconic(win_video_window))
		return;

	// if we have a blit surface, use that

	if (win_use_directx)
	{
		if (win_use_directx == USE_D3D)
		{
			if (win_d3d_draw(bitmap, bounds, vector_dirty_pixels, update))
				return;
		}
		else
		{
			if (win_ddraw_draw(bitmap, bounds, vector_dirty_pixels, update))
				return;
		}
	}

	// draw to the window with a DIB
	dib_draw_window(dc, bitmap, bounds, vector_dirty_pixels, update);
}



//============================================================
//  winwindow_video_window_proc
//============================================================

LRESULT CALLBACK winwindow_video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
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
 			if (win_video_window)
  				draw_video_contents(hdc, NULL, NULL, NULL, 1);
 			if (win_has_menu())
 				DrawMenuBar(win_video_window);
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
		case WM_ENTERMENULOOP:
		case WM_ENTERSIZEMOVE:
			osd_sound_enable(0);
			win_timer_enable(0);
			break;

		// resume sound and timer if we dome with resizing or a menu
		case WM_EXITMENULOOP:
		case WM_EXITSIZEMOVE:
			osd_sound_enable(1);
			win_timer_enable(1);
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
				win_constrain_to_aspect_ratio(rect, wparam, 0, COORDINATES_DESKTOP);
			InvalidateRect(win_video_window, NULL, FALSE);
			break;
		}

		// syscommands: catch win_start_maximized
		case WM_SYSCOMMAND:
		{
			// prevent screensaver or monitor power events
			if (wparam == SC_MONITORPOWER || wparam == SC_SCREENSAVE)
				return 1;

			InvalidateRect(win_video_window, NULL, FALSE);
			if ((wparam & 0xfff0) == SC_MAXIMIZE)
			{
				win_toggle_maximize(0);
				break;
			}
			else if (wparam == MENU_FULLSCREEN)
			{
				winwindow_toggle_full_screen();
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
			if (win_use_directx)
			{
				if (win_use_directx == USE_D3D)
				{
					win_d3d_kill();
				}
				else
				{
					win_ddraw_kill();
				}
			}
			win_video_window = 0;
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
//  win_constrain_to_aspect_ratio
//============================================================

void win_constrain_to_aspect_ratio(RECT *rect, int adjustment, int constraints, int coordinate_system)
{
	double adjusted_ratio = aspect_ratio;
	int extrawidth = wnd_extra_width();
	int extraheight = wnd_extra_height();
	int reqwidth, reqheight;
	int adjwidth, adjheight;
	RECT minrect, maxrect, temp;
	RECT rectcopy = *rect;

	// adjust if hardware stretching
	if (win_force_int_stretch != FORCE_INT_STRECT_FULL && (win_use_directx == USE_D3D || (win_use_directx == USE_DDRAW && win_dd_hw_stretch)))
		adjusted_ratio *= win_aspect_ratio_adjust;

	// determine the minimum rect
	minrect = rectcopy;
	if (win_visible_width < win_visible_height)
	{
		minrect.right = minrect.left + MIN_WINDOW_DIM + extrawidth;
		minrect.bottom = minrect.top + (int)((double)MIN_WINDOW_DIM / adjusted_ratio + 0.5) + extraheight;
	}
	else
	{
		minrect.right = minrect.left + (int)((double)MIN_WINDOW_DIM * adjusted_ratio + 0.5) + extrawidth;
		minrect.bottom = minrect.top + MIN_WINDOW_DIM + extraheight;
	}

	// determine the maximum rect
	if (win_window_mode)
		get_work_area(&maxrect);
	else
	{
		get_screen_bounds(&maxrect);
		if (coordinate_system == COORDINATES_DISPLAY)
		{
			// normalize the rect back to the top left at 0,0
			maxrect.right -= maxrect.left;
			maxrect.left = 0;
			maxrect.bottom -= maxrect.top;
			maxrect.top = 0;
		}
	}

	// expand the initial rect past the minimum
	temp = rectcopy;
	UnionRect(&rectcopy, &temp, &minrect);

	// clamp the initial rect to its maxrect box
	temp = rectcopy;
	IntersectRect(&rectcopy, &temp, &maxrect);

	// if we're not forcing the aspect ratio, just return the intersection
	if (!win_keep_aspect)
		return;

	if (constraints == CONSTRAIN_INTEGER_WIDTH)
	{
		int maxwidth = (rectcopy.right - rectcopy.left - extrawidth) / win_visible_width;

		while (maxwidth > 1 && maxrect.bottom - maxrect.top < (int)((double)maxwidth * win_visible_width / adjusted_ratio + 0.5) + extraheight)
			maxwidth--;
		if (maxrect.right - maxrect.left > maxwidth * win_visible_width + extrawidth)
			maxrect.right = maxrect.left + maxwidth * win_visible_width + extrawidth;
	}
	else if (constraints == CONSTRAIN_INTEGER_HEIGHT)
	{
		int maxheight = (rectcopy.bottom - rectcopy.top - extraheight) / win_visible_height;

		while (maxheight > 1 && maxrect.right - maxrect.left < (int)((double)maxheight * win_visible_height * adjusted_ratio + 0.5) + extrawidth)
			maxheight--;
		if (maxrect.bottom - maxrect.top > maxheight * win_visible_height + extraheight)
			maxrect.bottom = maxrect.top + maxheight * win_visible_height + extraheight;
	}

	// compute the maximum requested width/height
	switch (adjustment)
	{
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
			reqwidth = rectcopy.right - rectcopy.left - extrawidth;
			reqheight = (int)((double)reqwidth / adjusted_ratio + 0.5);
			break;

		case WMSZ_TOP:
		case WMSZ_BOTTOM:
			reqheight = rectcopy.bottom - rectcopy.top - extraheight;
			reqwidth = (int)((double)reqheight * adjusted_ratio + 0.5);
			break;

		default:
			reqwidth = rectcopy.right - rectcopy.left - extrawidth;
			reqheight = (int)((double)reqwidth / adjusted_ratio + 0.5);
			if (reqheight < (rectcopy.bottom - rectcopy.top - extraheight))
			{
				reqheight = rectcopy.bottom - rectcopy.top - extraheight;
				reqwidth = (int)((double)reqheight * adjusted_ratio + 0.5);
			}
			break;
	}

	// scale up if too small
	if (reqwidth + extrawidth < minrect.right - minrect.left)
	{
		reqwidth = minrect.right - minrect.left - extrawidth;
		reqheight = (int)((double)reqwidth / adjusted_ratio + 0.5);
	}
	if (reqheight + extraheight < minrect.bottom - minrect.top)
	{
		reqheight = minrect.bottom - minrect.top - extraheight;
		reqwidth = (int)((double)reqheight * adjusted_ratio + 0.5);
	}

	// scale down if too big
	if (reqwidth + extrawidth > maxrect.right - maxrect.left)
	{
		reqwidth = maxrect.right - maxrect.left - extrawidth;
		reqheight = (int)((double)reqwidth / adjusted_ratio + 0.5);
	}
	if (reqheight + extraheight > maxrect.bottom - maxrect.top)
	{
		reqheight = maxrect.bottom - maxrect.top - extraheight;
		reqwidth = (int)((double)reqheight * adjusted_ratio + 0.5);
	}

	// compute the adjustments we need to make
	adjwidth = (reqwidth + extrawidth) - (rect->right - rect->left);
	adjheight = (reqheight + extraheight) - (rect->bottom - rect->top);

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
//  adjust_window_for_visible
//============================================================

void win_adjust_window_for_visible(int min_x, int max_x, int min_y, int max_y)
{
	int old_visible_width = win_visible_rect.right - win_visible_rect.left;
	int old_visible_height = win_visible_rect.bottom - win_visible_rect.top;

	// set the new values
	win_visible_rect.left = min_x;
	win_visible_rect.top = min_y;
	win_visible_rect.right = max_x + 1;
	win_visible_rect.bottom = max_y + 1;
	win_visible_width = win_visible_rect.right - win_visible_rect.left;
	win_visible_height = win_visible_rect.bottom - win_visible_rect.top;

	// if we're not using hardware stretching, recompute the aspect ratio
	if (win_use_directx != USE_D3D && (win_use_directx != USE_DDRAW || !win_dd_hw_stretch))
	{
		aspect_ratio = (double)win_visible_width / (double)win_visible_height;
		if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		{
			if (!blit_swapxy)
				aspect_ratio *= 2.0;
			else
				aspect_ratio /= 2.0;
		}
		else if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		{
			if (!blit_swapxy)
				aspect_ratio /= 2.0;
			else
				aspect_ratio *= 2.0;
		}
	}

 	// if we are adjusting the size in windowed mode without stretch, use our own way of changing the window size
 	if (visible_area_set && win_window_mode && win_use_directx != USE_D3D && (win_use_directx != USE_DDRAW || !win_dd_hw_stretch))
 	{
 		RECT r;
 		int xmult, ymult;

 		GetClientRect(win_video_window, &r);
 		compute_multipliers_internal(&r, old_visible_width, old_visible_height, &xmult, &ymult);

 		GetWindowRect(win_video_window, &r);
 		r.right += (win_visible_width - old_visible_width) * xmult;
 		r.bottom += (win_visible_height - old_visible_height) * ymult;
 		SetWindowPos(win_video_window, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOMOVE);
 	}
 	else
 	{
  		// adjust the window
  		win_adjust_window();
 	}

	// first time through here, we need to show the window
	if (!visible_area_set)
	{
		// let's also win_start_maximized the window
		if (win_window_mode)
		{
			RECT bounds, work;

			// compute the non-maximized bounds here
			get_work_area(&work);
			GetWindowRect(win_video_window, &bounds);
			non_maximized_bounds.left = work.left + ((work.right - work.left) - (bounds.right - bounds.left)) / 2;
			non_maximized_bounds.top = work.top + ((work.bottom - work.top) - (bounds.bottom - bounds.top)) / 2;
			non_maximized_bounds.right = non_maximized_bounds.left + bounds.right - bounds.left;
			non_maximized_bounds.bottom = non_maximized_bounds.top + bounds.bottom - bounds.top;

			// if maximizing, toggle it
			if (win_start_maximized)
				win_toggle_maximize(0);

			// otherwise, just enforce the bounds
			else
				SetWindowPos(win_video_window, NULL, non_maximized_bounds.left, non_maximized_bounds.top,
						non_maximized_bounds.right - non_maximized_bounds.left,
						non_maximized_bounds.bottom - non_maximized_bounds.top,
						SWP_NOZORDER);
		}

		// kludge to fix full screen mode for the non-ddraw case
		if (!win_use_directx && !win_window_mode)
		{
			win_window_mode = 1;
			winwindow_toggle_full_screen();
			memset(&non_fullscreen_bounds, 0, sizeof(non_fullscreen_bounds));
		}

		// show the result
		ShowWindow(win_video_window, SW_SHOW);
		SetForegroundWindow(win_video_window);
		winwindow_video_window_update(NULL, NULL, NULL);

		// update the cursor state
		winwindow_update_cursor_state();

		// unpause the input devices
		win_pause_input(0);
		visible_area_set = 1;
	}
}



//============================================================
//  win_toggle_maximize
//============================================================

void win_toggle_maximize(int force_maximize)
{
	RECT current, constrained, maximum;
	int xoffset, yoffset;
	int center_window = 0;

	// get the current position
	GetWindowRect(win_video_window, &current);

	// get the desktop work area
	get_work_area(&maximum);

	// get the maximum constrained area
	constrained = maximum;
	if (win_default_constraints)
	{
		win_constrain_to_aspect_ratio(&constrained, WMSZ_BOTTOMRIGHT, win_default_constraints, COORDINATES_DESKTOP);
	}

	if (force_maximize)
	{
		current = constrained;
		center_window = 1;
	}
	else if (win_default_constraints)
	{
		// toggle between maximised, contrained, and normal sizes
		if ((current.right - current.left) >= (maximum.right - maximum.left) ||
			(current.bottom - current.top) >= (maximum.bottom - maximum.top))
		{
			current = non_maximized_bounds;
		}
		else if ((current.right - current.left) == (constrained.right - constrained.left) &&
				 (current.bottom - current.top) == (constrained.bottom - constrained.top))
		{
			current = maximum;

			win_constrain_to_aspect_ratio(&current, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DESKTOP);
			center_window = 1;
		}
		else if ((current.right - current.left) > (constrained.right - constrained.left) &&
				 (current.bottom - current.top) > (constrained.bottom - constrained.top))
		{
			// save the current location
			non_maximized_bounds = current;

			current = maximum;

			win_constrain_to_aspect_ratio(&current, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DESKTOP);
			center_window = 1;
		}
		else
		{
			// save the current location
			non_maximized_bounds = current;

			current = constrained;
			center_window = 1;
		}
	}
	else
	{
		// toggle between maximised and mormal sizes
		if ((current.right - current.left) >= (maximum.right - maximum.left) ||
			(current.bottom - current.top) >= (maximum.bottom - maximum.top))
		{
			current = non_maximized_bounds;
		}
		else
		{
			// save the current location
			non_maximized_bounds = current;

			current = maximum;
			center_window = 1;
		}

		win_constrain_to_aspect_ratio(&current, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DESKTOP);
	}

	if (center_window == 1)
	{
		// if we're not stretching, compute the multipliers
		if (win_use_directx != USE_D3D && (win_use_directx != USE_DDRAW || !win_dd_hw_stretch))
		{
			int xmult, ymult;

			current.right -= wnd_extra_width() + 2;
			current.bottom -= wnd_extra_height() + 2;
			win_compute_multipliers(&current, &xmult, &ymult);
			current.right = current.left + win_visible_width * xmult + wnd_extra_width() + 2;
			current.bottom = current.top + win_visible_height * ymult + wnd_extra_height() + 2;
		}

		// center it
		xoffset = ((maximum.right - maximum.left) - (current.right - current.left)) / 2;
		yoffset = ((maximum.bottom - maximum.top) - (current.bottom - current.top)) / 2;
		current.left += xoffset;
		current.right += xoffset;
		current.top += yoffset;
		current.bottom += yoffset;
	}

	// set the new position
	SetWindowPos(win_video_window, NULL, current.left, current.top,
			current.right - current.left, current.bottom - current.top,
			SWP_NOZORDER);
}



//============================================================
//  winwindow_toggle_full_screen
//============================================================

void winwindow_toggle_full_screen(void)
{
#ifdef MAME_DEBUG
	// if we are in debug mode, never go full screen
	if (options.mame_debug)
		return;
#endif

	// rip down DirectDraw
	if (win_use_directx)
	{
		if (win_use_directx == USE_D3D)
		{
			win_d3d_kill();
		}
		else
		{
			win_ddraw_kill();
		}
	}

	// hide the window
	ShowWindow(win_video_window, SW_HIDE);
#ifdef MAME_DEBUG
	if (win_window_mode)
		debugwin_show(SW_HIDE);
#endif

	// toggle the window mode
	win_window_mode = !win_window_mode;

	// adjust the window style and z order
	if (win_window_mode)
	{
		// adjust the style
		SetWindowLong(win_video_window, GWL_STYLE, WINDOW_STYLE);
		SetWindowLong(win_video_window, GWL_EXSTYLE, WINDOW_STYLE_EX);
		SetWindowPos(win_video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// force to the bottom, then back on top
		SetWindowPos(win_video_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(win_video_window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// adjust the bounds
		if (non_fullscreen_bounds.right != non_fullscreen_bounds.left)
			SetWindowPos(win_video_window, HWND_TOP, non_fullscreen_bounds.left, non_fullscreen_bounds.top,
						non_fullscreen_bounds.right - non_fullscreen_bounds.left, non_fullscreen_bounds.bottom - non_fullscreen_bounds.top,
						SWP_NOZORDER);
		else
		{
			SetWindowPos(win_video_window, HWND_TOP, 0, 0, win_visible_width + 2, win_visible_height + 2, SWP_NOZORDER);
			win_toggle_maximize(1);
		}
	}
	else
	{
		// save the bounds
		GetWindowRect(win_video_window, &non_fullscreen_bounds);

		// adjust the style
		SetWindowLong(win_video_window, GWL_STYLE, FULLSCREEN_STYLE);
		SetWindowLong(win_video_window, GWL_EXSTYLE, FULLSCREEN_STYLE_EX);
		SetWindowPos(win_video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// set topmost
		SetWindowPos(win_video_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	// adjust the window to compensate for the change
	win_adjust_window();
	update_system_menu();

	// show and adjust the window
	ShowWindow(win_video_window, SW_SHOW);
#ifdef MAME_DEBUG
	if (win_window_mode)
		debugwin_show(SW_SHOW);
#endif

	// reinit
	if (win_use_directx)
	{
		if (win_use_directx == USE_D3D)
		{
			if (win_d3d_init(0, 0, 0, 0, 0, NULL))
				exit(1);
		}
		else
		{
			if (win_ddraw_init(0, 0, 0, 0, NULL))
				exit(1);
		}
	}

	// make sure the window is properly readjusted
	win_adjust_window();
}



//============================================================
//  win_adjust_window
//============================================================

void win_adjust_window(void)
{
	RECT original, window;

	// get the current size
	GetWindowRect(win_video_window, &original);

	// adjust the window size so the client area is what we want
	if (win_window_mode)
	{
		// constrain the existing size to the aspect ratio
		window = original;
		win_constrain_to_aspect_ratio(&window, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DESKTOP);
	}

	// in full screen, make sure it covers the primary display
	else
		get_screen_bounds(&window);

	// adjust the position if different
	if (original.left != window.left ||
		original.top != window.top ||
		original.right != window.right ||
		original.bottom != window.bottom)
		SetWindowPos(win_video_window, win_window_mode ? HWND_TOP : HWND_TOPMOST,
				window.left, window.top,
				window.right - window.left, window.bottom - window.top, 0);

	// take note of physical window size (used for lightgun coordinate calculation)
	win_physical_width=window.right - window.left;
	win_physical_height=window.bottom - window.top;

	logerror("Physical width %d, height %d\n",win_physical_width,win_physical_height);

	// update the cursor state
	winwindow_update_cursor_state();
}



//============================================================
//  winwindow_process_events_periodic
//============================================================

void winwindow_process_events_periodic(void)
{
	cycles_t curr = osd_cycles();
	if (curr - last_event_check < osd_cycles_per_second() / 8)
		return;
	winwindow_process_events(1);
}



//============================================================
//  winwindow_process_events
//============================================================

void winwindow_process_events(int ingame)
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
				input_mouse_button_down(0,GET_X_LPARAM(message.lParam),GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;
			case WM_RBUTTONDOWN:
				input_mouse_button_down(1,GET_X_LPARAM(message.lParam),GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;
			case WM_MBUTTONDOWN:
				input_mouse_button_down(2,GET_X_LPARAM(message.lParam),GET_Y_LPARAM(message.lParam));
				dispatch = is_debugger_visible;
				break;
			case WM_XBUTTONDOWN:
				input_mouse_button_down(3,GET_X_LPARAM(message.lParam),GET_Y_LPARAM(message.lParam));
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
}



//============================================================
//  win_prepare_palette
//============================================================

UINT32 *win_prepare_palette(win_blit_params *params)
{
	// 16bpp source only needs a palette if RGB direct or modifiable
	if (params->srcdepth == 15 || params->srcdepth == 16)
		return (params->dstdepth == 16) ? palette_16bit_lookup : palette_32bit_lookup;

	// nobody else needs it
	return NULL;
}



//============================================================
//  dib_draw_window
//============================================================

static void dib_draw_window(HDC dc, mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int depth = (bitmap->depth == 15) ? 16 : bitmap->depth;
	win_blit_params params;
	int xmult, ymult;
	RECT client;
	int cx, cy;

	// compute the multipliers
	GetClientRect(win_video_window, &client);
	win_compute_multipliers(&client, &xmult, &ymult);

	// allocate a temporary bitmap in case we need it
	if (!converted_bitmap)
		converted_bitmap = malloc(MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT * 4);
	if (!converted_bitmap)
		return;

	// blit to our temporary bitmap
	params.dstdata		= (void *)(((UINT32)converted_bitmap + 15) & ~15);
	params.dstpitch		= (((win_visible_width * xmult) + 3) & ~3) * depth / 8;
	params.dstdepth		= depth;
	params.dstxoffs		= 0;
	params.dstyoffs		= 0;
	params.dstxscale	= xmult;
	params.dstyscale	= (!win_old_scanlines || ymult == 1) ? ymult : ymult - 1;
	params.dstyskip		= (!win_old_scanlines || ymult == 1) ? 0 : 1;
	params.dsteffect	= win_determine_effect(&params);

	params.srcdata		= bitmap->base;
	params.srcpitch		= bitmap->rowbytes;
	params.srcdepth		= bitmap->depth;
	params.srclookup	= win_prepare_palette(&params);
	params.srcxoffs		= win_visible_rect.left;
	params.srcyoffs		= win_visible_rect.top;
	params.srcwidth		= win_visible_width;
	params.srcheight	= win_visible_height;

	params.vecdirty		= vector_dirty_pixels;

	params.flipx		= blit_flipx;
	params.flipy		= blit_flipy;
	params.swapxy		= blit_swapxy;

	// adjust for more optimal bounds
	if (bounds && !update && !vector_dirty_pixels)
	{
		params.dstxoffs += (bounds->min_x - win_visible_rect.left) * xmult;
		params.dstyoffs += (bounds->min_y - win_visible_rect.top) * ymult;
		params.srcxoffs += bounds->min_x - win_visible_rect.left;
		params.srcyoffs += bounds->min_y - win_visible_rect.top;
		params.srcwidth = bounds->max_x - bounds->min_x + 1;
		params.srcheight = bounds->max_y - bounds->min_y + 1;
	}

	win_perform_blit(&params, update);

	// fill in bitmap-specific info
	video_dib_info->bmiHeader.biWidth = params.dstpitch / (depth / 8);
	video_dib_info->bmiHeader.biHeight = -win_visible_height * ymult;
	video_dib_info->bmiHeader.biBitCount = depth;

	// compute the center position
	cx = client.left + ((client.right - client.left) - win_visible_width * xmult) / 2;
	cy = client.top + ((client.bottom - client.top) - win_visible_height * ymult) / 2;

	// blit to the screen
	StretchDIBits(dc, cx, cy, win_visible_width * xmult, win_visible_height * ymult,
				0, 0, win_visible_width * xmult, win_visible_height * ymult,
				converted_bitmap, video_dib_info, DIB_RGB_COLORS, SRCCOPY);

	// erase the edges if updating
	if (update)
	{
		RECT inner;

		inner.left = cx;
		inner.top = cy;
		inner.right = cx + win_visible_width * xmult;
		inner.bottom = cy + win_visible_height * ymult;
		erase_outer_rect(&client, &inner, dc);
	}
}



//============================================================
//  lookup_effect
//============================================================

int win_lookup_effect(const char *arg)
{
	int effindex;

	// loop through all the effects and find a match
	for (effindex = 0; effindex < sizeof(effect_table) / sizeof(effect_table[0]); effindex++)
		if (!strcmp(arg, effect_table[effindex].name))
			return effindex;

	return -1;
}



//============================================================
//  win_determine_effect
//============================================================

int win_determine_effect(const win_blit_params *params)
{
	// default to what was selected
	int result = effect_table[win_blit_effect].effect;

	// if we're out of range, revert to NONE
	if (params->dstxscale < effect_table[win_blit_effect].min_xscale ||
		params->dstxscale > effect_table[win_blit_effect].max_xscale ||
		params->dstyscale < effect_table[win_blit_effect].min_yscale ||
		params->dstyscale > effect_table[win_blit_effect].max_yscale)
		result = EFFECT_NONE;

	return result;
}



//============================================================
//  compute_multipliers_internal
//============================================================

static void compute_multipliers_internal(const RECT *rect, int visible_width, int visible_height, int *xmult, int *ymult)
{
	// first compute simply
	*xmult = (rect->right - rect->left) / visible_width;
	*ymult = (rect->bottom - rect->top) / visible_height;

	// clamp to the hardcoded max
	if (*xmult > MAX_X_MULTIPLY)
		*xmult = MAX_X_MULTIPLY;
	if (*ymult > MAX_Y_MULTIPLY)
		*ymult = MAX_Y_MULTIPLY;

	// clamp to the effect max
	if (*xmult > effect_table[win_blit_effect].max_xscale)
		*xmult = effect_table[win_blit_effect].max_xscale;
	if (*ymult > effect_table[win_blit_effect].max_yscale)
		*ymult = effect_table[win_blit_effect].max_yscale;

	// adjust for pixel aspect ratio
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
	{
		if (!blit_swapxy)
		{
			if (*ymult > 1)
				*ymult &= ~1;
		}
		else
		{
			if (*xmult > 1)
				*xmult &= ~1;
		}
	}
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
	{
		if (!blit_swapxy)
		{
			if (*xmult > 1)
				*xmult &= ~1;
		}
		else
		{
			if (*ymult > 1)
				*ymult &= ~1;
		}
	}

	// make sure we have at least 1
	if (*xmult < 1)
		*xmult = 1;
	if (*ymult < 1)
		*ymult = 1;
}



//============================================================
//  win_compute_multipliers
//============================================================

void win_compute_multipliers(const RECT *rect, int *xmult, int *ymult)
{
	compute_multipliers_internal(rect, win_visible_width, win_visible_height, xmult, ymult);
}
