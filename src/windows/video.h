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
//  PARAMETERS
//============================================================

// maximum video size
#define MAX_VIDEO_WIDTH			1600
#define MAX_VIDEO_HEIGHT		1200



//============================================================
//  GLOBAL VARIABLES
//============================================================

// screen info
extern HMONITOR		monitor;
extern char			*screen_name;


// speed throttling
extern int			throttle;

// palette lookups
extern UINT8		palette_lookups_invalid;
extern UINT32 		palette_16bit_lookup[];
extern UINT32 		palette_32bit_lookup[];

// rotation
extern UINT8		blit_flipx;
extern UINT8		blit_flipy;
extern UINT8		blit_swapxy;



//============================================================
//  PROTOTYPES
//============================================================

void win_pause(int pause);
void win_orient_rect(rectangle *rect);
void win_disorient_rect(rectangle *rect);
void win_set_frameskip(int frameskip);		// <0 = auto
int win_get_frameskip(void);				// <0 = auto

#endif
