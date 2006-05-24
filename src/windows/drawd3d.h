//============================================================
//
//  wind3d.h - Win32 Direct3D 7 (with DirectDraw 7) code
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIND3D__
#define __WIND3D__

#include "window.h"


//============================================================
//  PROTOTYPES
//============================================================

int win_d3d_init(win_window_info *window);
int win_d3d_draw(win_window_info *window, HDC dc, const render_primitive *primlist, int update);
void win_d3d_kill(win_window_info *window);


#endif
