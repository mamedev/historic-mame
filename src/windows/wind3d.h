//============================================================
//
//	wind3d.c - Win32 Direct3D 7 (with DirectDraw 7) code
//
//============================================================

#ifndef __WIN32_D3D__
#define __WIN32_D3D__

#include "window.h"


//============================================================
//	PROTOTYPES
//============================================================

int win_d3d_init(int width, int height, int depth, int attributes, const struct win_effect_data *effect);
void win_d3d_kill(void);
int win_d3d_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
void win_d3d_wait_vsync(void);



#endif
