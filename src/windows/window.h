//============================================================
//
//  window.h - Win32 window handling
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN_WINDOW__
#define __WIN_WINDOW__

#include "blit.h"
#include "video.h"

#include "render.h"


//============================================================
//  PARAMETERS
//============================================================

#ifndef MESS
#define HAS_WINDOW_MENU			FALSE
#else
#define HAS_WINDOW_MENU			TRUE
#endif



//============================================================
//  CONSTANTS
//============================================================

#define RESIZE_STATE_NORMAL		0
#define RESIZE_STATE_RESIZING	1
#define RESIZE_STATE_PENDING	2



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _win_window_info win_window_info;
struct _win_window_info
{
	win_window_info *	next;

	// window handle and info
	HWND				hwnd;
	RECT				non_fullscreen_bounds;
	int					isminimized;
	int					ismaximized;
	int					resize_state;

	// monitor info
	win_monitor_info *	monitor;
	int					maxwidth, maxheight;
	int					depth;
	int					refresh;
	float				aspect;

	// rendering info
	render_target *		target;
	int					targetview;
	int					targetorient;
	const render_primitive *primlist;

	// drawing data
	void *				gdidata;
	void *				dxdata;
};


typedef struct _win_effect_data win_effect_data;
struct _win_effect_data
{
	const char *name;
	int effect;
	int min_xscale;
	int min_yscale;
	int max_xscale;
	int max_yscale;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

// windows
extern win_window_info *win_window_list;

// windows
extern HWND			win_debug_window;

// video bounds
extern double		win_aspect_ratio_adjust;
extern int 			win_default_constraints;

// visible bounds
extern RECT			win_visible_rect;
extern int			win_visible_width;
extern int			win_visible_height;

// raw mouse support
extern int			win_use_raw_mouse;



//============================================================
//  DEFINES
//============================================================

// win_constrain_to_aspect_ratio() constraints parameter
#define CONSTRAIN_INTEGER_WIDTH 1
#define CONSTRAIN_INTEGER_HEIGHT 2

// win_constrain_to_aspect_ratio() coordinate_system parameter
// desktop coordinates--when not in any full screen mode
#define COORDINATES_DESKTOP 1
// display coordinates, when each display is 0,0 at the top left
#define COORDINATES_DISPLAY 2


// win_force_int_stretch values
#define FORCE_INT_STRECT_NONE 0
#define FORCE_INT_STRECT_FULL 1
#define FORCE_INT_STRECT_AUTO 2
#define FORCE_INT_STRECT_HOR 3
#define FORCE_INT_STRECT_VER 4



//============================================================
//  PROTOTYPES
//============================================================

// core initialization
int winwindow_init(void);

// creation/deletion of windows
int winwindow_video_window_create(int index, win_monitor_info *monitor, const win_window_config *config);

void winwindow_update_cursor_state(void);
void winwindow_video_window_update(win_window_info *window);

LRESULT CALLBACK winwindow_video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
void winwindow_toggle_full_screen(void);

void winwindow_process_events_periodic(void);
int winwindow_process_events(int ingame);

#if HAS_WINDOW_MENU
int win_create_menu(HMENU *menus);
#endif

#ifndef NEW_DEBUGGER
int debugwin_init_windows(void);
void debugwin_update_windows(mame_bitmap *bitmap, const rgb_t *palette);
void debugwin_show(int type);
void debugwin_set_focus(int focus);
#endif


//============================================================
//  win_has_menu
//============================================================

INLINE BOOL win_has_menu(win_window_info *window)
{
	return GetMenu(window->hwnd) ? TRUE : FALSE;
}


//============================================================
//  rect_width / rect_height
//============================================================

INLINE int rect_width(const RECT *rect)
{
	return rect->right - rect->left;
}


INLINE int rect_height(const RECT *rect)
{
	return rect->bottom - rect->top;
}

#endif
