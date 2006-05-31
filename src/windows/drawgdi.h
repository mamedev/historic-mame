//============================================================
//
//  wingdi.h - Win32 GDI drawing
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WINGDI__
#define __WINGDI__

#include "window.h"


//============================================================
//  PROTOTYPES
//============================================================

int drawgdi_window_init(win_window_info *window);
int drawgdi_window_draw(win_window_info *window, HDC dc, const render_primitive *primlist, int update);
void drawgdi_window_destroy(win_window_info *window);


#endif
