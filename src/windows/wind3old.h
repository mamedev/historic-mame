//============================================================
//
//  wind3old.h - Win32 Direct3D 7 (with DirectDraw 7) code
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN32_D3D__
#define __WIN32_D3D__

#include "windold.h"


//============================================================
//  GLOBAL VARIABLES
//============================================================

extern int win_d3d_use_filter;
extern int win_d3d_tex_manage;

extern UINT8 win_d3d_effects_swapxy;
extern UINT8 win_d3d_effects_flipx;
extern UINT8 win_d3d_effects_flipy;



//============================================================
//  PROTOTYPES
//============================================================

int win_d3d_init(int width, int height, int depth, int attributes, double aspect, const win_effect_data *effect);
void win_d3d_kill(void);
int win_d3d_draw(mame_bitmap *bitmap, const rectangle *bounds, void *vector_dirty_pixels, int update);



#endif
