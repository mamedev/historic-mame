//============================================================
//
//	blit.h - Win32 blit handling
//
//============================================================

#ifndef __WIN32_BLIT__
#define __WIN32_BLIT__


//============================================================
//	PARAMETERS
//============================================================

// maximum X/Y scale values
#define MAX_X_MULTIPLY		3
#define MAX_Y_MULTIPLY		3



//============================================================
//	TYPE DEFINITIONS
//============================================================

struct blit_params
{
	void *		dstdata;
	int			dstpitch;
	int			dstdepth;
	int			dstxoffs;
	int			dstyoffs;
	int			dstxscale;
	int			dstyscale;
	int			dstyskip;
	
	void *		srcdata;
	int			srcpitch;
	int			srcdepth;
	UINT32 *	srclookup;
	int			srcxoffs;
	int			srcyoffs;
	int			srcwidth;
	int			srcheight;
	
	void *		dirtydata;
	int			dirtypitch;
};



//============================================================
//	PROTOTYPES
//============================================================

int perform_blit(const struct blit_params *blit, int update);

#endif
