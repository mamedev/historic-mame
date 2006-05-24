//============================================================
//
//  video.h - Win32 implementation of MAME video routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN_VIDEO__
#define __WIN_VIDEO__


//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _win_monitor_info win_monitor_info;
struct _win_monitor_info
{
	win_monitor_info  *	next;					// pointer to next monitor in list
	HMONITOR			handle;					// handle to the monitor
	MONITORINFOEX		info;					// most recently retrieved info
	float				aspect;					// computed/configured aspect ratio of the physical device
	int					reqwidth;				// requested width for this monitor
	int					reqheight;				// requested height for this monitor
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

extern win_monitor_info *win_monitor_list;

extern int video_orientation;

extern int win_use_d3d;

// speed throttling
extern int			throttle;



//============================================================
//  PROTOTYPES
//============================================================

int winvideo_init(void);

void winvideo_monitor_refresh(win_monitor_info *monitor);
float winvideo_monitor_get_aspect(win_monitor_info *monitor);
win_monitor_info *winvideo_monitor_from_handle(HMONITOR monitor);

void winvideo_set_frameskip(int frameskip);		// <0 = auto
int winvideo_get_frameskip(void);				// <0 = auto

#endif
