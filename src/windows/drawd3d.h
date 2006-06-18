//============================================================
//
//  drawd3d.c - Win32 Direct3D implementation
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

int drawd3d_init(void);
void drawd3d_exit(void);

int drawd3d_window_init(win_window_info *window);
int drawd3d_window_draw(win_window_info *window, HDC dc, const render_primitive_list *primlist, int update);
void drawd3d_window_destroy(win_window_info *window);


#endif
