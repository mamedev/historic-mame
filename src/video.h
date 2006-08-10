/***************************************************************************

    video.h

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "mamecore.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* maximum number of screens for one game */
#define MAX_SCREENS					8



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/*-------------------------------------------------
    screen_state - current live state of a screen
-------------------------------------------------*/

typedef struct _screen_state screen_state;
struct _screen_state
{
	int				width, height;				/* total width/height (HTOTAL, VTOTAL) */
	rectangle		visarea;					/* visible area (HBLANK end/start, VBLANK end/start) */
	float			refresh;					/* refresh rate */
	double			vblank;						/* duration of a VBLANK */
};


/*-------------------------------------------------
    screen_config - configuration of a single
    screen
-------------------------------------------------*/

typedef struct _screen_config screen_config;
struct _screen_config
{
	const char *	tag;						/* nametag for the screen */
	UINT32			palette_base;				/* base palette entry for this screen */
	screen_state	defstate;					/* default state */
};


/*-------------------------------------------------
    performance_info - information about the
    current performance
-------------------------------------------------*/

struct _performance_info
{
	double			game_speed_percent;			/* % of full speed */
	double			frames_per_second;			/* actual rendered fps */
	int				vector_updates_last_second; /* # of vector updates last second */
	int				partial_updates_this_frame; /* # of partial updates last frame */
};
/* In mamecore.h: typedef struct _performance_info performance_info; */



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- screen rendering and management ----- */

/* core initialization */
int video_init(void);

/* set the resolution of a screen */
void configure_screen(int scrnum, int width, int height, const rectangle *visarea, float refresh);

/* set the visible area of a screen; this is a subset of configure_screen */
void set_visible_area(int scrnum, int min_x, int max_x, int min_y, int max_y);

/* force a partial update of the screen up to and including the requested scanline */
void force_partial_update(int scrnum, int scanline);

/* reset the partial updating for a frame; generally only called by cpuexec.c */
void reset_partial_updates(void);

/* update the screen, handling frame skipping and rendering */
void video_frame_update(void);

/* are we skipping the current frame? */
int skip_this_frame(void);

/* return current performance data */
const performance_info *mame_get_performance_info(void);

/*
  Save a screen shot of the game display. It is suggested to use the core
  function snapshot_save_all_screens() or snapshot_save_screen_indexed(), so the format
  of the screen shots will be consistent across ports. This hook is provided
  only to allow the display of a file requester to let the user choose the
  file name. This isn't scrictly necessary, so you can just call
  snapshot_save_all_screens() to let the core automatically pick a default name.
*/
void snapshot_save_screen_indexed(mame_file *fp, int scrnum);
void snapshot_save_all_screens(void);

/* Movie recording */
void record_movie_start(const char *name);
void record_movie_stop(void);
void record_movie_toggle(void);
void record_movie_frame(int scrnum);

/* bitmap allocation */
#define bitmap_alloc(w,h) bitmap_alloc_depth(w, h, Machine->color_depth)
#define auto_bitmap_alloc(w,h) auto_bitmap_alloc_depth(w, h, Machine->color_depth)
mame_bitmap *bitmap_alloc_depth(int width, int height, int depth);
mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth);
void bitmap_free(mame_bitmap *bitmap);

#endif	/* __VIDEO_H__ */
