//============================================================
//
//	blit.c - Win32 blit handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "driver.h"
#include "blit.h"
#include "window.h"



//============================================================
//	GLOBAL VARIABLES
//============================================================

void *asmblit_srcdata;
UINT32 asmblit_srcpitch;
UINT32 asmblit_srcwidth;
UINT32 asmblit_srcheight;
void *asmblit_srclookup;

void *asmblit_dstdata;
UINT32 asmblit_dstpitch;

void *asmblit_dirtydata;
UINT32 asmblit_dirtypitch;



//============================================================
//	PROTOTYPES
//============================================================

#define BLITTER(srcbpp,dstbpp,xscale,yscale,scan,dirty) \
	asmblit_##srcbpp##_to_##dstbpp##_##xscale##x##yscale##_sl##scan##_dirty##dirty

#define DECLARE_BLITTER(srcbpp,dstbpp,xscale,yscale,scan,dirty) \
	extern void BLITTER(srcbpp,dstbpp,xscale,yscale,scan,dirty)(void);

#define DECLARE_BLITTER_FAMILY(srcbpp,dstbpp,xscale,yscale) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 0, 0) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 0, 1) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 1, 0) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 1, 1) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 2, 0) \
	DECLARE_BLITTER(srcbpp, dstbpp, xscale, yscale, 2, 1)

#define DECLARE_BLITTER_ALL_YSCALE(srcbpp,dstbpp,xscale) \
	DECLARE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 1) \
	DECLARE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 2) \
	DECLARE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 3) \
	DECLARE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 4)

#define DECLARE_BLITTER_ALL_XYSCALE(srcbpp,dstbpp) \
	DECLARE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 1) \
	DECLARE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 2) \
	DECLARE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 3) \
	DECLARE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 4)

DECLARE_BLITTER_ALL_XYSCALE(8,8)
DECLARE_BLITTER_ALL_XYSCALE(8,16)
DECLARE_BLITTER_ALL_XYSCALE(8,24)
DECLARE_BLITTER_ALL_XYSCALE(8,32)

DECLARE_BLITTER_ALL_XYSCALE(16,16)
DECLARE_BLITTER_ALL_XYSCALE(16,24)
DECLARE_BLITTER_ALL_XYSCALE(16,32)

DECLARE_BLITTER_ALL_XYSCALE(32,16)
DECLARE_BLITTER_ALL_XYSCALE(32,24)
DECLARE_BLITTER_ALL_XYSCALE(32,32)



//============================================================
//	FUNCTION TABLE
//============================================================

#define TABLE_BLITTER_FAMILY(srcbpp,dstbpp,xscale,yscale) \
	{ { BLITTER(srcbpp, dstbpp, xscale, yscale, 0, 0), BLITTER(srcbpp, dstbpp, xscale, yscale, 0, 1) }, \
	  { BLITTER(srcbpp, dstbpp, xscale, yscale, 1, 0), BLITTER(srcbpp, dstbpp, xscale, yscale, 1, 1) }, \
	  { BLITTER(srcbpp, dstbpp, xscale, yscale, 2, 0), BLITTER(srcbpp, dstbpp, xscale, yscale, 2, 1) } }

#define TABLE_BLITTER_ALL_YSCALE(srcbpp,dstbpp,xscale) \
	{ TABLE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 1), \
	  TABLE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 2), \
	  TABLE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 3), \
	  TABLE_BLITTER_FAMILY(srcbpp, dstbpp, xscale, 4) }

#define TABLE_BLITTER_ALL_XYSCALE(srcbpp,dstbpp) \
	{ TABLE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 1), \
	  TABLE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 2), \
	  TABLE_BLITTER_ALL_YSCALE(srcbpp, dstbpp, 3) }

static void (*blit_table[4][4][4][4][3][2])(void) =
{
	{
		TABLE_BLITTER_ALL_XYSCALE(8,8),
		TABLE_BLITTER_ALL_XYSCALE(8,16),
		TABLE_BLITTER_ALL_XYSCALE(8,24),
		TABLE_BLITTER_ALL_XYSCALE(8,32)
	},

	{
		{{{{ 0 }}}},
		TABLE_BLITTER_ALL_XYSCALE(16,16),
		TABLE_BLITTER_ALL_XYSCALE(16,24),
		TABLE_BLITTER_ALL_XYSCALE(16,32)
	},
	
	{{{{{ 0 }}}}},

	{
		{{{{ 0 }}}},
		TABLE_BLITTER_ALL_XYSCALE(32,16),
		TABLE_BLITTER_ALL_XYSCALE(32,24),
		TABLE_BLITTER_ALL_XYSCALE(32,32)
	},
};
	



//============================================================
//	perform_blit
//============================================================

int perform_blit(const struct blit_params *blit, int update)
{
	int srcdepth_index = (blit->srcdepth + 7) / 8 - 1;
	int dstdepth_index = (blit->dstdepth + 7) / 8 - 1;
	int scans_index = blit->dstyskip ? (update ? 2 : 1) : 0;
	int dirty_index = (blit->dirtydata && !update) ? 1 : 0;
	void (*func)(void);

	asmblit_srcdata = (UINT8 *)blit->srcdata + blit->srcpitch * blit->srcyoffs + (srcdepth_index + 1) * blit->srcxoffs;
	asmblit_srcpitch = blit->srcpitch;
	asmblit_srcwidth = blit->srcwidth;
	asmblit_srcheight = blit->srcheight;
	asmblit_srclookup = blit->srclookup;
	
	asmblit_dstdata = (UINT8 *)blit->dstdata + blit->dstpitch * blit->dstyoffs + (dstdepth_index + 1) * blit->dstxoffs;
	asmblit_dstpitch = blit->dstpitch;

	asmblit_dirtydata = blit->dirtydata;
	asmblit_dirtypitch = blit->dirtypitch;

	func = blit_table[srcdepth_index][dstdepth_index][blit->dstxscale - 1][blit->dstyscale - 1][scans_index][dirty_index];
	if (func)
	{
		(*func)();
		return 1;
	}
	else
	{
		static int first_time = 1;
		if (first_time)
			fprintf(stderr, "blit_table[%d][%d][%d][%d][%d][%d]\n", srcdepth_index, dstdepth_index, blit->dstxscale - 1, blit->dstyscale - 1, scans_index, 0);
		first_time = 0;
		return 0;
	}
	return 1;
}
