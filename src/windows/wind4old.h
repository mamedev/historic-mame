//============================================================
//
//  wind4old.h - Win32 Direct3D 7 (with DirectDraw 7) effects
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __WIN32_D3D_FX__
#define __WIN32_D3D_FX__

#include "windold.h"


//============================================================
//  CONSTANTS
//============================================================

// maximum prescale level
#define MAX_PRESCALE			4

// maximum size for the auto effect
#define MAX_AUTOEFFECT_ZOOM	5



//============================================================
//  PROTOTYPES
//============================================================

int win_d3d_effects_init(int attributes);
int win_d3d_effects_init_surfaces(void);



#endif


