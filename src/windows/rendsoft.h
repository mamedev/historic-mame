//============================================================
//
//  rendsoft.h - Win32 software rendering
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================


//============================================================
//  GLOBAL VARIABLES
//============================================================

extern int srcshift_r, srcshift_g, srcshift_b;
extern int dstshift_r, dstshift_g, dstshift_b;



//============================================================
//  PROTOTYPES
//============================================================

void rgb888_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void rgb555_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void rgb565_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void generic32_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
void generic16_draw_primitives(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch);
