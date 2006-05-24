//============================================================
//
//  rendsoft.c - Win32 software rendering
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================


int srcshift_r, srcshift_g, srcshift_b;
int dstshift_r, dstshift_g, dstshift_b;



// 32-bpp RGB

#define FUNC_PREFIX(x)		rgb888_##x
#define PIXEL_TYPE			UINT32
#define SRCSHIFT_R			0
#define SRCSHIFT_G			0
#define SRCSHIFT_B			0
#define DSTSHIFT_R			16
#define DSTSHIFT_G			8
#define DSTSHIFT_B			0

#include "rendersw.c"


// 15-bpp RGB

#define FUNC_PREFIX(x)		rgb555_##x
#define PIXEL_TYPE			UINT16
#define SRCSHIFT_R			3
#define SRCSHIFT_G			3
#define SRCSHIFT_B			3
#define DSTSHIFT_R			10
#define DSTSHIFT_G			5
#define DSTSHIFT_B			0

#include "rendersw.c"


// 16-bpp RGB

#define FUNC_PREFIX(x)		rgb565_##x
#define PIXEL_TYPE			UINT16
#define SRCSHIFT_R			3
#define SRCSHIFT_G			2
#define SRCSHIFT_B			3
#define DSTSHIFT_R			11
#define DSTSHIFT_G			5
#define DSTSHIFT_B			0

#include "rendersw.c"



// generic 32-bit

#define FUNC_PREFIX(x)		generic32_##x
#define PIXEL_TYPE			UINT32
#define VARIABLE_SHIFT		1
#define SRCSHIFT_R			srcshift_r
#define SRCSHIFT_G			srcshift_g
#define SRCSHIFT_B			srcshift_b
#define DSTSHIFT_R			dstshift_r
#define DSTSHIFT_G			dstshift_g
#define DSTSHIFT_B			dstshift_b

#include "rendersw.c"



// generic 16-bit

#define FUNC_PREFIX(x)		generic16_##x
#define PIXEL_TYPE			UINT16
#define VARIABLE_SHIFT		1
#define SRCSHIFT_R			srcshift_r
#define SRCSHIFT_G			srcshift_g
#define SRCSHIFT_B			srcshift_b
#define DSTSHIFT_R			dstshift_r
#define DSTSHIFT_G			dstshift_g
#define DSTSHIFT_B			dstshift_b

#include "rendersw.c"
