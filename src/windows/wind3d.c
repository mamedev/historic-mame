//============================================================
//
//	wind3d.c - Win32 Direct3D 7 (with DirectDraw 7) code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __GNUC__
 #define NONAMELESSUNION
#endif
#include <ddraw.h>
#include <d3d.h>

// additional definitions for d3d
#include "d3d_extra.h"

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"
#include "wind3dfx.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern int verbose;

// from wind3dfx.c
extern UINT32 win_d3d_tfactor;
extern UINT32 win_d3d_preprocess_tfactor;



//============================================================
//	DEBUGGING
//============================================================

#define SHOW_FLIP_TIMES 		0



//============================================================
//	GLOBAL VARIABLES
//============================================================

// effects surfaces
LPDIRECTDRAWSURFACE7 win_d3d_background_surface;
LPDIRECTDRAWSURFACE7 win_d3d_scanline_surface[2];

// effects settings
UINT8 win_d3d_effects_swapxy;
UINT8 win_d3d_effects_flipx;
UINT8 win_d3d_effects_flipy;

int win_d3d_use_filter;
int win_d3d_tex_manage;

UINT8 win_d3d_use_auto_effect;
UINT8 win_d3d_use_rgbeffect;
UINT8 win_d3d_use_scanlines;
UINT8 win_d3d_use_prescale;
UINT8 win_d3d_use_feedback;

// zoom level
int win_d3d_current_zoom;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectDraw7
typedef HRESULT (WINAPI *type_directdraw_create_ex) (GUID FAR *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter);

static HINSTANCE ddraw_dll7;

// DirectDraw objects
static LPDIRECTDRAW7 ddraw7;
static LPDIRECTDRAWGAMMACONTROL gamma_control;
static LPDIRECTDRAWSURFACE7 primary_surface;
static LPDIRECTDRAWSURFACE7 back_surface;
static LPDIRECTDRAWSURFACE7 blit_surface;
static LPDIRECTDRAWCLIPPER primary_clipper;

// DirectDraw object info
static DDSURFACEDESC2 primary_desc;
static DDSURFACEDESC2 blit_desc;
static DDSURFACEDESC2 texture_desc;
static int changed_resolutions;
static int forced_updates;

// Direct3D objects
static LPDIRECT3D7 d3d7;
static LPDIRECT3DDEVICE7 d3d_device7;
static LPDIRECTDRAWSURFACE7 texture_surface;
static LPDIRECTDRAWSURFACE7 preprocess_surface;
static D3DTLVERTEX preprocess_vertex[4];
static D3DTLVERTEX2 vertex[4];

// Direct3D info
static D3DDEVICEDESC7 d3d_device_desc;
static DWORD d3dtop_scanlines;
static DWORD d3dtfg_image;
static DWORD d3dtfg_scanlines;
static DWORD d3dtfn_image;
static DWORD d3dtfn_scanlines;

// video bounds
static int max_width;
static int max_height;
static int pref_depth;
static int effect_min_xscale;
static int effect_min_yscale;
static struct rectangle last_bounds;
static double aspect_ratio;

// mode finding
static double best_score;
static int best_width;
static int best_height;
static int best_depth;
static int best_refresh;

//  misc
static int prev_zoom;

// derived attributes
static int needs_6bpp_per_gun;
static int pixel_aspect_ratio;



//============================================================
//	PROTOTYPES
//============================================================

static double compute_mode_score(int width, int height, int depth, int refresh);
static int set_resolution(void);
static int create_surfaces(void);
static int create_blit_surface(void);
static int create_effects_surfaces(void);
static void set_brightness(void);
static int create_clipper(void);
static void erase_surfaces(void);
static int restore_surfaces(void);
static void release_surfaces(void);
static void compute_color_masks(const DDSURFACEDESC2 *desc);
static int render_to_blit(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
static int render_and_flip(LPRECT src, LPRECT dst, int update);




//============================================================
//	erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, LPDIRECTDRAWSURFACE7 surface)
{
	DDBLTFX blitfx = { sizeof(DDBLTFX) };
	RECT clear;

	// erase the blit surface
	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;

	// clear the left edge
	if (inner->left > outer->left)
	{
		clear = *outer;
		clear.right = inner->left;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	win_d3d_test_hardware_caps
//============================================================

static int win_d3d_test_hardware_caps(void)
{
	// scanlines
	if (win_d3d_use_scanlines)
	{
		// ensure the grpahics hardware can handle 2 texture stages
		if (d3d_device_desc.wMaxSimultaneousTextures < 2 || d3d_device_desc.wMaxTextureBlendStages < 2)
		{
			fprintf(stderr, "Error: 3D hardware can't render scanlines (no support for multi-texturing)\n");
			return 1;
		}

		// determine optimal blending method
		d3dtop_scanlines = D3DTOP_ADD;
		if (d3d_device_desc.dwTextureOpCaps & D3DTEXOPCAPS_ADDSMOOTH)
		{
			d3dtop_scanlines = D3DTOP_ADDSMOOTH;
		}
		else if (verbose)
		{
			fprintf(stderr, "Warning: using fall-back method for scanline blending.\n");
		}
	}

	// filtering

	// assume bi-linear filtering as the default
	d3dtfg_image = D3DTFG_LINEAR;
	d3dtfg_scanlines = D3DTFG_LINEAR;
	if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
	{
		d3dtfn_image = D3DTFN_LINEAR;
		d3dtfn_scanlines = D3DTFN_LINEAR;
	}
	else
	{
		d3dtfn_image = D3DTFN_POINT;
		d3dtfn_scanlines = D3DTFN_POINT;
	}

	// override defaults if required
	switch (win_d3d_use_filter)
	{
		// point filtering
		case 0:
		{
			d3dtfg_image = D3DTFG_POINT;
			d3dtfn_image = D3DTFN_POINT;
			break;
		}
		// cubic filtering (flat kernel)
		case 2:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFAFLATCUBIC)
			{
				d3dtfg_image = D3DTFG_FLATCUBIC;
				d3dtfg_scanlines = D3DTFG_FLATCUBIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: flat bi-cubic filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
		// cubic filtering (gaussian kernel)
		case 3:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC)
			{
				d3dtfg_image = D3DTFG_GAUSSIANCUBIC;
				d3dtfg_scanlines = D3DTFG_GAUSSIANCUBIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: gaussian bi-cubic filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
		// anisotropic filtering
		case 4:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
			{
				d3dtfg_image = D3DTFG_ANISOTROPIC;
				d3dtfg_scanlines = D3DTFG_ANISOTROPIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: anisotropic (mag) filtering not supported, falling back to bi-linear\n");
			}
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
			{
				d3dtfn_image = D3DTFN_ANISOTROPIC;
				d3dtfn_scanlines = D3DTFN_ANISOTROPIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: anisotropic (min) filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
	}

	// RGB effects
	switch (win_d3d_use_rgbeffect)
	{
		// multiply mode
		case 1:
		{
			if (!(d3d_device_desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
			{
				if (verbose)
					fprintf(stderr, "Error: RGB effects (multiply mode) not supported\n");

				return 1;
			}
			break;
		}
		// multiply & add mode
		case 2:
		{
			if (!(d3d_device_desc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) ||
				!(d3d_device_desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
			{
				if (verbose)
					fprintf(stderr, "Error: RGB effects (multiply & add mode) not supported\n");

				return 1;
			}
			break;
		}
	}

	return 0;
}

//============================================================
//	adjust_prescale
//============================================================

void adjust_prescale(void)
{
	// disable prescale when the old effects engine is already scaling up the image
	if (effect_min_xscale >= 2)
		win_d3d_use_prescale &= ~240;
	if (effect_min_yscale >= 2)
		win_d3d_use_prescale &= ~15;
}



//============================================================
//	win_d3d_wait_vsync
//============================================================

void win_d3d_wait_vsync(void)
{
	HRESULT result;
	BOOL is_vblank;

	// if we're not already in VBLANK, wait for it
	result = IDirectDraw7_GetVerticalBlankStatus(ddraw7, &is_vblank);
	if (result == DD_OK && !is_vblank)
		result = IDirectDraw7_WaitForVerticalBlank(ddraw7, DDWAITVB_BLOCKBEGIN, 0);
}



//============================================================
//	win_d3d_kill
//============================================================

void win_d3d_kill(void)
{
	if (d3d_device7 != NULL)
		IDirect3DDevice7_Release(d3d_device7);
	d3d_device7 = NULL;

	// release the surfaces
	release_surfaces();

	// restore resolutions & reset cooperative level
	if (ddraw7 != NULL)
		IDirectDraw7_SetCooperativeLevel(ddraw7, NULL, DDSCL_NORMAL);

	// delete the core objects
	if (d3d7 != NULL)
		IDirect3D7_Release(d3d7);
	d3d7 = NULL;

	if (ddraw7 != NULL)
		IDirectDraw7_Release(ddraw7);
	ddraw7 = NULL;

	if (ddraw_dll7)
		FreeLibrary(ddraw_dll7);
	ddraw_dll7 = NULL;
}



//============================================================
//	win_d3d_init
//============================================================

int win_d3d_init(int width, int height, int depth, int attributes, double aspect, const struct win_effect_data *effect)
{
	type_directdraw_create_ex fn_directdraw_create_ex;

	DDSURFACEDESC2 currmode = { sizeof(DDSURFACEDESC2) };
	D3DVIEWPORT7 viewport;
	HRESULT result;

	// since we can't count on DirectX7 being present, load the dll
	ddraw_dll7 = LoadLibrary("ddraw.dll");
	if (ddraw_dll7 == NULL)
	{
		fprintf(stderr, "Error importing ddraw.dll\n");
		goto cant_load_ddraw_dll;
	}
	// and import the DirectDrawCreateEx function manually
	fn_directdraw_create_ex = (type_directdraw_create_ex)GetProcAddress(ddraw_dll7, "DirectDrawCreateEx");
	if (fn_directdraw_create_ex == NULL)
	{
		fprintf(stderr, "Error importing DirectDrawCreateEx() function\n");
		goto cant_create_ddraw7;
	}

	result = fn_directdraw_create_ex(NULL, (void **)&ddraw7, &IID_IDirectDraw7, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating DirectDraw7: %08x\n", (UINT32)result);
		goto cant_create_ddraw7;
	}

	result = IDirectDraw7_QueryInterface(ddraw7, &IID_IDirect3D7, (void **)&d3d7);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating Direct3D7: %08x\n", (UINT32)result);
		goto cant_create_d3d7;
	}

	// set the graphics mode width/height to the window size
	if (width && height && depth)
	{
		max_width = width;
		max_height = height;
		pref_depth = depth;
		needs_6bpp_per_gun	= ((attributes & VIDEO_NEEDS_6BITS_PER_GUN) != 0);
		pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);
	}
	if (effect)
	{
		effect_min_xscale = effect->min_xscale;
		effect_min_yscale = effect->min_yscale;
	}
	if (aspect)
	{
		aspect_ratio = aspect;
	}

	// print available video memory
	if (verbose)
	{
		DDSCAPS2 caps = { 0 };
		DWORD mem_total;
		DWORD mem_free;

		caps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		fprintf(stderr, "Initialising DirectDraw & Direct3D 7 blitter");
		if (IDirectDraw7_GetAvailableVidMem(ddraw7, &caps, &mem_total, &mem_free) == DD_OK)
		{
			fprintf(stderr, " (%.2lfMB video memory available)", (double)mem_total / (1024 * 1024));
		}
		fprintf(stderr, "\n");
	}

	// set the cooperative level
	// for non-window modes, we will use full screen here
	result = IDirectDraw7_SetCooperativeLevel(ddraw7, win_video_window, win_window_mode ? DDSCL_NORMAL : DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting cooperative level: %08x\n", (UINT32)result);
		goto cant_set_coop_level;
	}

	// set initial zoom level
	prev_zoom = win_d3d_current_zoom = 1;

	// init the effects
	win_d3d_effects_init();

	// set contraints on window size
	if (win_d3d_use_scanlines || win_force_int_stretch)
		win_default_constraints = win_d3d_effects_swapxy ? CONSTRAIN_INTEGER_WIDTH : CONSTRAIN_INTEGER_HEIGHT;

	// full screen mode: set the resolution
	changed_resolutions = 0;
	if (set_resolution())
		goto cant_set_resolution;

	// create the screen surfaces
	if (create_surfaces())
		goto cant_create_screen_surfaces;

	result = IDirect3D7_CreateDevice(d3d7, &IID_IDirect3DHALDevice, back_surface, &d3d_device7);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating Direct3D7 device: %08x\n", (UINT32)result);
		goto cant_create_d3d_device7;
	}

	// get device caps
	result = IDirect3DDevice7_GetCaps(d3d_device7, &d3d_device_desc);

	if (win_d3d_test_hardware_caps())
		goto cant_get_hardware;

	// create the blit surfaces
	if (create_blit_surface())
		goto cant_create_blit_surfaces;

	// create the effects surfaces
	if (create_effects_surfaces())
		goto cant_create_effects_surfaces;

	// erase all the surfaces we created
	erase_surfaces();

	// init the surfaces used for the effects
	if (win_d3d_effects_init_surfaces())
		goto cant_init_effects_surfaces;

	// attempt to get the current display mode
	result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
	if (result != DD_OK)
		goto cant_set_viewport;

	// now set up the viewport
	viewport.dwX = 0;
	viewport.dwY = 0;
	viewport.dwWidth = currmode.dwWidth;
	viewport.dwHeight = currmode.dwHeight;

	result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);
	if (result != DD_OK)
		goto cant_set_viewport;

	// set some Direct3D options
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_LIGHTING, FALSE);
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

	// print available video memory
	if (verbose)
	{
		DDSCAPS2 caps = { 0 };
		DWORD mem_total;
		DWORD mem_free;

		caps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		if (IDirectDraw7_GetAvailableVidMem(ddraw7, &caps, &mem_total, &mem_free) == DD_OK)
		{
			fprintf(stderr, "Blitter initialisation complete (%.2lfMB video memory free)\n", (double)mem_free / (1024 * 1024));
		}
	}

	// force update of RGB effects
	prev_zoom = -1;

	// force some updates
	forced_updates = 5;

	return 0;

	// error handling
cant_set_viewport:
cant_init_effects_surfaces:
cant_create_blit_surfaces:
cant_create_effects_surfaces:
cant_get_hardware:
cant_create_d3d_device7:
cant_create_screen_surfaces:
cant_set_resolution:
cant_set_coop_level:
cant_create_d3d7:
cant_create_ddraw7:
cant_load_ddraw_dll:
	win_d3d_kill();
	return 1;
}



//============================================================
//	enum_callback
//============================================================

static HRESULT WINAPI enum_callback(LPDDSURFACEDESC2 desc, LPVOID context)
{
	int refresh = (win_match_refresh || win_gfx_refresh) ? desc->DUMMYUNIONNAMEN(2).dwRefreshRate : 0;
	int depth = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	double score;

	// compute this mode's score
	score = compute_mode_score(desc->dwWidth, desc->dwHeight, depth, refresh);

	// is it the best?
	if (score > best_score)
	{
		// if so, remember it
		best_score = score;
		best_width = desc->dwWidth;
		best_height = desc->dwHeight;
		best_depth = depth;
		best_refresh = refresh;
	}
	return DDENUMRET_OK;
}



//============================================================
//	compute_mode_score
//============================================================

static double compute_mode_score(int width, int height, int depth, int refresh)
{
	static const double depth_matrix[4][2][4] =
	{
			// needs < 24bpp mode		  // needs >= 24bpp mode
		{ { 0.00, 0.75, 0.25, 0.50 },	{ 0.00, 0.25, 0.50, 0.75 } },	// 8bpp source
		{ { 0.00, 1.00, 0.25, 0.50 },	{ 0.00, 0.50, 0.75, 1.00 } },	// 16bpp source
		{ { 0.00, 0.00, 0.00, 0.00 },	{ 0.00, 0.00, 0.00, 0.00 } },	// 24bpp source (doesn't exist)
		{ { 0.00, 0.50, 0.75, 1.00 },	{ 0.00, 0.50, 0.75, 1.00 } }	// 32bpp source
	};

	double size_score, depth_score, refresh_score, final_score;
	int target_width, target_height;

	// select wich depth matrix to use based on the game properties and enabled effects
	int matrix = (needs_6bpp_per_gun || win_d3d_use_feedback) ? 1 : 0;

	// first compute a score based on size

	// determine minimum requirements
	target_width = max_width * effect_min_xscale;
	target_height = max_height * effect_min_yscale;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		target_height *= 2;
	else if (win_old_scanlines)
		target_width *= 2, target_height *= 2;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		target_width *= 2;

	// determine the zoom level based on the number of scanlines of the game
	if (win_keep_aspect && !win_force_int_stretch)
	{
		if (blit_swapxy && win_default_constraints != CONSTRAIN_INTEGER_HEIGHT)
		{
			// adjust width for the desired zoom level
			if (target_width < max_width * win_gfx_zoom)
				target_width = max_width * win_gfx_zoom;

			// match height according to the aspect ratios of the game, screen, and display mode
			target_height = (int)((double)target_width * width / aspect_ratio / height / win_screen_aspect + 0.5);
		}
		else
		{
			// adjust height for the desired zoom level
			if (target_height < max_height * win_gfx_zoom)
				target_height = max_height * win_gfx_zoom;

			// match width according to the aspect ratios of the game, screen, and display mode
			target_width = (int)((double)target_height * width * aspect_ratio / height / win_screen_aspect + 0.5);
		}
	}
	// ignore aspect ratio entirely
	else
	{
		// adjust width for the desired zoom level
		if (target_width < max_width * win_gfx_zoom)
			target_width = max_width * win_gfx_zoom;
		// adjust height for the desired zoom level
		if (target_height < max_height * win_gfx_zoom)
			target_height = max_height * win_gfx_zoom;
	}

	// compute initial score based on difference between target and current
	size_score = 1.0 / (1.0 + fabs(width - target_width) + fabs(height - target_height));

	// if we're looking for a particular mode, make sure it matches
	if (win_gfx_width && win_gfx_height && (width != win_gfx_width || height != win_gfx_height))
		return 0.0;

	// if mode is too small, it's a zero, unless the user specified otherwise
	if ((width < max_width || height < max_height) && (!win_gfx_width || !win_gfx_height))
		return 0.0;

	// if mode is smaller than we'd like, it only scores up to 0.1
	if (width < target_width || height < target_height)
		size_score *= 0.1;

	// next compute depth score
	depth_score = depth_matrix[(pref_depth + 7) / 8 - 1][matrix][(depth + 7) / 8 - 1];

	// only 16bpp and above now supported
	if (depth < 16)
		return 0.0;

	// if we're looking for a particular depth, make sure it matches
	if (win_gfx_depth && depth != win_gfx_depth)
		return 0.0;

	// finally, compute refresh score
	refresh_score = 1.0 / (1.0 + fabs((double)refresh - Machine->drv->frames_per_second));

	// if we're looking for a particular refresh, make sure it matches
	if (win_gfx_refresh && refresh && refresh != win_gfx_refresh)
		return 0.0;

	// if refresh is smaller than we'd like, it only scores up to 0.1
	if ((double)refresh < Machine->drv->frames_per_second)
		refresh_score *= 0.1;

	// weight size highest, followed by depth and refresh
	final_score = (size_score * 100.0 + depth_score * 10.0 + refresh_score) / 111.0;
	return final_score;
}



//============================================================
//	set_resolution
//============================================================

static int set_resolution(void)
{
	DDSURFACEDESC2 currmode = { sizeof(DDSURFACEDESC2) };
	double resolution_aspect;
	HRESULT result;

	// skip if not switching resolution
	if (!win_window_mode && (win_switch_res || win_switch_bpp))
	{
		// if we're only switching depth, set win_gfx_width and win_gfx_height to the current resolution
		if (!win_switch_res || !win_switch_bpp)
		{
			// attempt to get the current display mode
			result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
				goto cant_get_mode;
			}

			// force to the current width/height
			if (!win_switch_res)
			{
				win_gfx_width = currmode.dwWidth;
				win_gfx_height = currmode.dwHeight;
			}
			if (!win_switch_bpp)
				win_gfx_depth = currmode.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
		}

		// enumerate display modes
		best_score = 0.0;
		result = IDirectDraw7_EnumDisplayModes(ddraw7, (win_match_refresh || win_gfx_refresh) ? DDEDM_REFRESHRATES : 0, NULL, NULL, enum_callback);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error enumerating modes: %08x\n", (UINT32)result);
			goto cant_enumerate_modes;
		}

		if (verbose)
		{
			if (best_refresh)
				fprintf(stderr, "Best mode = %dx%dx%d @ %d Hz\n", best_width, best_height, best_depth, best_refresh);
			else
				fprintf(stderr, "Best mode = %dx%dx%d @ default Hz\n", best_width, best_height, best_depth);
		}

		// set it
		if (best_width != 0)
		{
			// Set the refresh rate
			result = IDirectDraw7_SetDisplayMode(ddraw7, best_width, best_height, best_depth, best_refresh, 0);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error setting mode: %08x\n", (UINT32)result);
				goto cant_set_mode;
			}
			changed_resolutions = 1;
		}
	}

	// attempt to get the current display mode
	result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
		goto cant_get_mode;
	}

	// compute the adjusted aspect ratio
	resolution_aspect = (double)currmode.dwWidth / (double)currmode.dwHeight;
	win_aspect_ratio_adjust = resolution_aspect / win_screen_aspect;
	return 0;

	// error handling - non fatal in general
cant_set_mode:
cant_get_mode:
cant_enumerate_modes:
	return 0;
}



//============================================================
//	print_surface_size & print_surface_colour
//============================================================

static int print_surface_size(LPDIRECTDRAWSURFACE7 surface, char *string)
{
	DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

	if (IDirectDrawSurface7_GetSurfaceDesc(surface, &desc) != DD_OK)
		return 1;

	sprintf(string, "%dx%dx%d",
				(int)desc.dwWidth,
				(int)desc.dwHeight,
				(int)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount);

	return 0;
}

static int print_surface_colour(LPDIRECTDRAWSURFACE7 surface, char *string)
{
	DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

	if (IDirectDrawSurface7_GetSurfaceDesc(surface, &desc) != DD_OK)
		return 1;

	sprintf(string, "R=%08x G=%08x B=%08x",
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask,
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask,
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask);


	return 0;
}


//============================================================
//	create_surfaces
//============================================================

static int create_surfaces(void)
{
	HRESULT result;

	// make a description of the primary surface
	memset(&primary_desc, 0, sizeof(primary_desc));
	primary_desc.dwSize = sizeof(primary_desc);
	primary_desc.dwFlags = DDSD_CAPS;
	primary_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	// for full screen mode, allocate flipping surfaces
	if (!win_window_mode)
	{
		primary_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		primary_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
		if (win_triple_buffer)
			primary_desc.dwBackBufferCount = 2;
		else
			primary_desc.dwBackBufferCount = 1;
	}

	// then create the primary surface
	result = IDirectDraw7_CreateSurface(ddraw7, &primary_desc, &primary_surface, NULL);
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Error creating primary surface: %08x\n", (UINT32)result);
		goto cant_create_primary;
	}

	// get a description of the primary surface
	result = IDirectDrawSurface7_GetSurfaceDesc(primary_surface, &primary_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting primary surface desc: %08x\n", (UINT32)result);
		goto cant_get_primary_desc;
	}

	// print out the good stuff
	if (verbose)
	{
		char size[80];
		char colour[80];

		if (win_window_mode)
			fprintf(stderr, "Rendering to an off-screen surface and using blt()\n");
		else
			fprintf(stderr, "Using a %s buffer and pageflipping\n", win_triple_buffer ? "triple" : "double");

		print_surface_size(primary_surface, size);
		print_surface_colour(primary_surface, colour);

		fprintf(stderr, "Primary surface created: %s (%s)\n", size, colour);
	}

	// determine the color masks and force the palette to recalc
	compute_color_masks(&primary_desc);

	// if this is a full-screen mode, attempt to create a color control object
	if (!win_window_mode && win_gfx_brightness != 0.0)
		set_brightness();

	// window mode: allocate the back surface seperately
	if (win_window_mode)
	{
        DDSURFACEDESC2 back_desc = { sizeof(DDSURFACEDESC2) };

		back_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		back_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;

		back_desc.dwWidth = primary_desc.dwWidth;
		back_desc.dwHeight = primary_desc.dwHeight;

		// then create the primary surface
		result = IDirectDraw7_CreateSurface(ddraw7, &back_desc, &back_surface, NULL);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error creating back surface: %08x\n", (UINT32)result);
			goto cant_get_back_surface;
		}
	}
	// full screen mode: get the back surface
	else
	{
		DDSCAPS2 caps = { DDSCAPS_BACKBUFFER };
		result = IDirectDrawSurface7_GetAttachedSurface(primary_surface, &caps, &back_surface);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error getting attached back surface: %08x\n", (UINT32)result);
			goto cant_get_back_surface;
		}
	}

	// create a clipper for windowed mode
	if (win_window_mode)
	{
		if (create_clipper())
			goto cant_init_clipper;
	}

	return 0;

	// error handling
cant_init_clipper:
cant_get_back_surface:
cant_get_primary_desc:
cant_create_primary:
	return 1;
}



//============================================================
//	create_effects_surfaces
//============================================================

static int create_effects_surfaces(void)
{
	HRESULT result = DD_OK;

	// create a surface that will hold the RGB effects pattern, based on the primary surface
	if (win_d3d_use_rgbeffect)
	{
        DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

		desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

		// we can save video memory because we only need to partially cover the screen
		if (!win_window_mode && win_keep_aspect)
		{
			desc.dwWidth = primary_desc.dwWidth;
			desc.dwHeight = (int)((double)primary_desc.dwHeight / aspect_ratio * win_screen_aspect + 0.5);
			if (desc.dwHeight > primary_desc.dwHeight)
			{
				desc.dwHeight = primary_desc.dwHeight;
				desc.dwWidth = (int)((double)primary_desc.dwWidth * aspect_ratio / win_screen_aspect + 0.5);
			}
		}
		else
		{
			desc.dwWidth = primary_desc.dwWidth;
			desc.dwHeight = primary_desc.dwHeight;
		}

		// create the RGB effects surface
		result = IDirectDraw7_CreateSurface(ddraw7, &desc, &win_d3d_background_surface, NULL);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error creating RGB effects surface: %08x\n", (UINT32)result);
			goto cant_get_background_surface;
		}

		if (verbose)
		{
			char size[80];

			print_surface_size(win_d3d_background_surface, size);
			fprintf(stderr, "RGB effects surface created: %s\n", size);
		}
	}

	// create 2 surfaces that will hold different sizes of scanlines, based on the primary surface
	if (win_d3d_use_scanlines)
	{
		int i;

		for (i = 0; i < 2; i++)
		{
	        DDSURFACEDESC2 desc =  { sizeof(DDSURFACEDESC2) };

			desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
			desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;

			desc.dwWidth = 2 << i;
			desc.dwHeight = 2 << i;

			if (desc.dwWidth < d3d_device_desc.dwMinTextureWidth || desc.dwHeight < d3d_device_desc.dwMinTextureHeight)
			{
				fprintf(stderr, "Error creating scanline surface[%d]: %08x\n", i, (UINT32)result);
				goto cant_get_scanline_surface;
			}

			// create the surface
			result = IDirectDraw7_CreateSurface(ddraw7, &desc, &win_d3d_scanline_surface[i], NULL);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error creating scanline surface[%d]: %08x\n", i, (UINT32)result);
				goto cant_get_scanline_surface;
			}

			if (verbose)
			{
				char size[80];

				print_surface_size(win_d3d_scanline_surface[i], size);
				fprintf(stderr, "Scanline surface[%i] created: %s\n", i, size);
			}
		}
	}

	return 0;

cant_get_background_surface:
cant_get_scanline_surface:
	return 1;
}



//============================================================
//	callback for IDirect3DDevice7_EnumTextureFormats
//============================================================

static HRESULT CALLBACK enum_textures_callback(LPDDPIXELFORMAT pixelformat, LPVOID preferred_pixelformat)
{
    if (pixelformat->dwFlags == DDPF_RGB && pixelformat->DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
    {
		// use RGB:555 format if supported by the 3D hardware
		if (pixelformat->DUMMYUNIONNAMEN(2).dwRBitMask == 0x7C00 &&
			pixelformat->DUMMYUNIONNAMEN(3).dwGBitMask == 0x03E0 &&
			pixelformat->DUMMYUNIONNAMEN(4).dwBBitMask == 0x001F)
		{
			memcpy(preferred_pixelformat, pixelformat, sizeof(DDPIXELFORMAT));

			return D3DENUMRET_CANCEL;
		}
	}

	// use RGB:565 format otherwise
	if (pixelformat->DUMMYUNIONNAMEN(2).dwRBitMask == 0xF800 &&
		pixelformat->DUMMYUNIONNAMEN(3).dwGBitMask == 0x07E0 &&
		pixelformat->DUMMYUNIONNAMEN(4).dwBBitMask == 0x001F)
	{
			memcpy(preferred_pixelformat, pixelformat, sizeof(DDPIXELFORMAT));
    }

    return D3DENUMRET_OK;
}



//============================================================
//	create_blit_surface
//============================================================

static int create_blit_surface(void)
{
	DDPIXELFORMAT preferred_pixelformat = { 0 };
	DDSURFACEDESC2 desc;
	int width, height;
	int texture_width = 256, texture_height = 256;
	HRESULT result;
	int done = 0;
	int i;

	// determine the width/height of the blit surface
	while (!done)
	{
		done = 1;

		// first compute the ideal size
		width = (max_width * effect_min_xscale) + 16;
		height = (max_height * effect_min_yscale) + 0;

		// if it's okay, keep it
		if (width <= primary_desc.dwWidth && height <= primary_desc.dwHeight)
			break;

		// reduce the width
		if (width > primary_desc.dwWidth)
		{
			if (effect_min_xscale > 1)
			{
				done = 0;
				effect_min_xscale--;
			}
		}

		// reduce the height
		if (height > primary_desc.dwHeight)
		{
			if (effect_min_yscale > 1)
			{
				done = 0;
				effect_min_yscale--;
			}
		}
	}

	// adjust prescale settings now we know effect_min_xscale/effect_min_xscale
	adjust_prescale();

	// determine the width/height of the texture surface (assume ^2 sizes are required)
	while (texture_width < (width - 16))
		texture_width <<= 1;
	while (texture_height < (height - 0))
		texture_height <<= 1;

	if (win_d3d_tex_manage && texture_width < width)
		texture_width <<= 1;

	if (texture_width < d3d_device_desc.dwMinTextureWidth)
		texture_width = d3d_device_desc.dwMinTextureWidth;
	if (texture_height < d3d_device_desc.dwMinTextureHeight)
		texture_height = d3d_device_desc.dwMinTextureHeight;

	if (d3d_device_desc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		if (texture_width > texture_height)
		{
			texture_height = texture_width;
		}
		else if (texture_height > texture_width)
		{
			texture_width = texture_height;
		}
	}

	if (texture_width > d3d_device_desc.dwMaxTextureWidth || texture_height > d3d_device_desc.dwMaxTextureHeight)
	{
		if (verbose)
			fprintf(stderr, "Error: required texture size too large (max: %ix%i, need %ix%i)\n", (int)d3d_device_desc.dwMaxTextureWidth, (int)d3d_device_desc.dwMaxTextureHeight, texture_width, texture_height);
		goto cant_create_preprocess;
	}

	// texture used for prescale and feedback (always the same format as the primary surface)
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
		memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);
		desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;

		desc.dwWidth = texture_width;
		desc.dwHeight = texture_height;

		desc.dwWidth <<= (win_d3d_use_prescale >> 4);
		desc.dwHeight <<= (win_d3d_use_prescale & 15);

		if (desc.dwWidth > d3d_device_desc.dwMaxTextureWidth || desc.dwHeight > d3d_device_desc.dwMaxTextureHeight)
		{
			if (verbose)
				fprintf(stderr, "Warning: required texture size too large for prescale, disabling prescale\n");

			if (desc.dwWidth > d3d_device_desc.dwMaxTextureWidth)
			{
				desc.dwWidth = texture_width;
				win_d3d_use_prescale &= 15;
			}
			if (desc.dwHeight > d3d_device_desc.dwMaxTextureHeight)
			{
				desc.dwHeight = texture_height;
				win_d3d_use_prescale &= 240;
			}
		}
	}
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
		// create the pre-processing surface
		result = IDirectDraw7_CreateSurface(ddraw7, &desc, &preprocess_surface, NULL);
		if (result != DD_OK)
		{
			if (verbose)
				fprintf(stderr, "Error creating preprocess surface: %08x\n", (UINT32)result);
			goto cant_create_preprocess;
		}
	}

	// now make a description of our blit surface, based on the primary surface
	blit_desc = primary_desc;
	blit_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;

	// determine the preferred blit/texture surface colour format
	if (!needs_6bpp_per_gun)
		IDirect3DDevice7_EnumTextureFormats(d3d_device7, &enum_textures_callback, (LPVOID)&preferred_pixelformat);
	// if we have a preferred colour format, use it
	if (preferred_pixelformat.dwSize)
		blit_desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat = preferred_pixelformat;

	if (win_d3d_tex_manage)
	{
		// the blit surface will also be the texture surface
		blit_desc.dwWidth = texture_width;
		blit_desc.dwHeight = texture_height;
		blit_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
		blit_desc.ddsCaps.dwCaps2 = DDSCAPS2_HINTDYNAMIC | DDSCAPS2_TEXTUREMANAGE;
	}
	else
	{
		// use separate blit and texture surfaces
		blit_desc.dwWidth = width;
		blit_desc.dwHeight = height;
		blit_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
	}

	// then create the blit surface
	result = IDirectDraw7_CreateSurface(ddraw7, &blit_desc, &blit_surface, NULL);
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Error creating blit surface: %08x\n", (UINT32)result);
		goto cant_create_blit;
	}

	// make a description of our texture surface, based on the blit surface
	texture_desc = blit_desc;
	if (!win_d3d_tex_manage)
	{
		// fill in the differences
		texture_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
		texture_desc.dwWidth = texture_width;
		texture_desc.dwHeight = texture_height;

		// then create the texture surface
		result = IDirectDraw7_CreateSurface(ddraw7, &texture_desc, &texture_surface, NULL);
		if (result != DD_OK)
		{
			if (verbose)
				fprintf(stderr, "Error creating texture surface: %08x\n", (UINT32)result);
			goto cant_create_texture;
		}
	}

	// get a description of the blit surface
	result = IDirectDrawSurface7_GetSurfaceDesc(blit_surface, &blit_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting blit surface desc: %08x\n", (UINT32)result);
		goto cant_get_surface_desc;
	}

	// and if we have a separate one, of the texture surface
	if (texture_surface)
	{
		result = IDirectDrawSurface7_GetSurfaceDesc(texture_surface, &texture_desc);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error getting texture surface desc: %08x\n", (UINT32)result);
			goto cant_get_surface_desc;
		}
	}

	// set up the vertices (x/y position and texture coordinates will updated when rendering the image)
	for (i = 0; i < 4; i++)
	{
		vertex[i].rhw = 1;
		vertex[i].color = 0xFFFFFFFF;
		vertex[i].specular = 0xFFFFFFFF;

		if (win_d3d_use_prescale || win_d3d_use_feedback)
		{
			preprocess_vertex[i].rhw = 1;
			preprocess_vertex[i].color = 0xFFFFFFFF;
			preprocess_vertex[i].specular = 0xFFFFFFFF;
		}
	}

	// blit surface color masks override the primary surface color masks
	compute_color_masks(&blit_desc);

	// print out the good stuff
	if (verbose)
	{
		char size[80];
		char colour[80];

		print_surface_size(blit_surface, size);
		print_surface_colour(blit_surface, colour);

		fprintf(stderr, "%s surface created: %s (%s)\n", win_d3d_tex_manage ? "Managed texture" : "Blit", size, colour);

		if (texture_surface)
		{
			print_surface_size(texture_surface, size);
			fprintf(stderr, "Texture surface created: %s\n", size);
		}
		if (preprocess_surface)
		{
			print_surface_size(preprocess_surface, size);
			fprintf(stderr, "Pre-process texture surface created: %s\n", size);
		}
	}

	return 0;

	// error handling
cant_get_surface_desc:
cant_create_texture:
cant_create_blit:
cant_create_preprocess:
	return 1;
}



//============================================================
//	set_brightness
//============================================================

static void set_brightness(void)
{
	HRESULT result;

	// see if we can get a GammaControl object
	result = IDirectDrawSurface7_QueryInterface(primary_surface, &IID_IDirectDrawGammaControl, (void **)&gamma_control);
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Warning: could not create gamma control to change brightness: %08x\n", (UINT32)result);
		gamma_control = NULL;
	}

	// if we got it, proceed
	if (gamma_control)
	{
		DDGAMMARAMP ramp;
		int i;

		// fill the gamma ramp
		for (i = 0; i < 256; i++)
		{
			double val = ((float)i / 255.0) * win_gfx_brightness;
			if (val > 1.0)
				val = 1.0;
			ramp.red[i] = ramp.green[i] = ramp.blue[i] = (WORD)(val * 65535.0);
		}

		// attempt to get the current settings
		result = IDirectDrawGammaControl_SetGammaRamp(gamma_control, 0, &ramp);
		if (result != DD_OK)
			fprintf(stderr, "Error setting gamma ramp: %08x\n", (UINT32)result);
	}
}



//============================================================
//	create_clipper
//============================================================

static int create_clipper(void)
{
	HRESULT result;

	// create a clipper for the primary surface
	result = IDirectDraw7_CreateClipper(ddraw7, 0, &primary_clipper, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating clipper: %08x\n", (UINT32)result);
		goto cant_create_clipper;
	}

	// set the clipper's hwnd
	result = IDirectDrawClipper_SetHWnd(primary_clipper, 0, win_video_window);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting clipper hwnd: %08x\n", (UINT32)result);
		goto cant_set_hwnd;
	}

	// set the clipper on the primary surface
	result = IDirectDrawSurface_SetClipper(primary_surface, primary_clipper);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting clipper on primary surface: %08x\n", (UINT32)result);
		goto cant_set_surface;
	}
	return 0;

	// error handling
cant_set_surface:
cant_set_hwnd:
cant_create_clipper:
	return 1;
}



//============================================================
//	erase_surfaces
//============================================================

static void erase_surfaces(void)
{
	DDBLTFX blitfx = { sizeof(DDBLTFX) };
	HRESULT result = DD_OK;
	int i;

	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;

	// erase the texture surface
	if (texture_surface)
		result = IDirectDrawSurface7_Blt(texture_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// erase the blit surface
	if (blit_surface)
		result = IDirectDrawSurface7_Blt(blit_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// erase the preprocess surface
	if (preprocess_surface)
		result = IDirectDrawSurface7_Blt(preprocess_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	win_d3d_effects_init();

	if (!win_window_mode)
	{
		// loop through enough to get all the back buffers
		for (i = 0; i < 5; i++)
		{
			// first flip
			result = IDirectDrawSurface7_Flip(primary_surface, NULL, DDFLIP_NOVSYNC | DDFLIP_WAIT);

			// then do a color fill blit
			result = IDirectDrawSurface7_Blt(back_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		}
	}
	else
	{
		// do a color fill blit on both primary adn back buffer
		result = IDirectDrawSurface7_Blt(primary_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		result = IDirectDrawSurface7_Blt(back_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	restore_surfaces
//============================================================

static int restore_surfaces(void)
{
	HRESULT result;

#define RESTORE_SURFACE(s) if (s) { result = IDirectDrawSurface7_Restore(s); if (result != DD_OK) { return 1; } }

	// restore the texture surface
	RESTORE_SURFACE(texture_surface);

	// restore the blit surface
	RESTORE_SURFACE(blit_surface);

	// restore the preprocess surface
	RESTORE_SURFACE(preprocess_surface);

	// restore the scanline surfaces
	RESTORE_SURFACE(win_d3d_scanline_surface[0]);
	RESTORE_SURFACE(win_d3d_scanline_surface[1]);

	// restore the rgb-effects surface
	RESTORE_SURFACE(win_d3d_background_surface);

	// restore the back surface seperately if needed
	if (back_surface && win_window_mode)
	{
		RESTORE_SURFACE(back_surface);
	}
	// restore the primary surface
	RESTORE_SURFACE(primary_surface);

	// all surfaces are successfully restored

	// erase all surfaces
	erase_surfaces();

	return 0;

#undef RESTORE_SURFACE

}

//============================================================
//	release_surfaces
//============================================================

static void release_surfaces(void)
{

#define RELEASE_SURFACE(s) if (s) { IDirectDrawSurface7_Release(s); s = NULL; }

	// release the texture surface
	RELEASE_SURFACE(texture_surface);

	// release the blit surface
	RELEASE_SURFACE(blit_surface);

	// release the preprocess surface
	RELEASE_SURFACE(preprocess_surface);

	// release the clipper
	if (primary_clipper)
		IDirectDrawClipper_Release(primary_clipper);
	primary_clipper = NULL;

	// release the color controls
	if (gamma_control)
		IDirectDrawColorControl_Release(gamma_control);
	gamma_control = NULL;

	// release the scanline surfaces
	RELEASE_SURFACE(win_d3d_scanline_surface[0]);
	RELEASE_SURFACE(win_d3d_scanline_surface[1]);

	// release the background surface
	RELEASE_SURFACE(win_d3d_background_surface);

	// release the back surface seperately if needed
	if (win_window_mode)
	{
		RELEASE_SURFACE(back_surface);
	}
	else
		back_surface = NULL;

	// release the primary surface
	RELEASE_SURFACE(primary_surface);

#undef RELEASE_SURFACE

}



//============================================================
//	compute_color_masks
//============================================================

static void compute_color_masks(const DDSURFACEDESC2 *desc)
{
	// 16bpp case
	if (desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
	{
		int temp;

		// red
		win_color16_rdst_shift = win_color16_rsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_rdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_rsrc_shift++;

		// green
		win_color16_gdst_shift = win_color16_gsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_gdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_gsrc_shift++;

		// blue
		win_color16_bdst_shift = win_color16_bsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_bdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_bsrc_shift++;
	}

	// 24/32bpp case
	else if (desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 24 ||
			 desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 32)
	{
		int temp;

		// red
		win_color32_rdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_rdst_shift++;

		// green
		win_color32_gdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_gdst_shift++;

		// blue
		win_color32_bdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_bdst_shift++;
	}

	// mark the lookups invalid
	palette_lookups_invalid = 1;
}



//============================================================
//	win_d3d_draw
//============================================================

int win_d3d_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int result;

	// handle forced updates
	if (forced_updates)
	{
		forced_updates--;
		update = 1;
	}

#if 1
	// if the surfaces are lost, restore them
	if (IDirectDrawSurface7_IsLost(primary_surface) == DDERR_SURFACELOST)
		restore_surfaces();

	// render to the blit surface,
	result = render_to_blit(bitmap, bounds, vector_dirty_pixels, update);


	return result;
#else
	return 0;
#endif
}



//============================================================
//	lock_must_succeed
//============================================================

static int lock_must_succeed(const struct rectangle *bounds, void *vector_dirty_pixels)
{
	// determine up front if this lock must succeed; by default, it depends on
	// whether or not we're throttling
	int result = throttle;

	// if we're using dirty pixels, we must succeed as well, or else we will leave debris
	if (vector_dirty_pixels)
		result = 1;

	// if we're blitting a different source rect than before, we also must
	// succeed, or else we will miss some areas
	if (bounds)
	{
		if (bounds->min_x != last_bounds.min_x || bounds->min_y != last_bounds.min_y ||
			bounds->max_x != last_bounds.max_x || bounds->max_y != last_bounds.max_y)
			result = 1;
		last_bounds = *bounds;
	}

	return result;
}



//============================================================
//	render_to_blit
//============================================================

static int render_to_blit(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int dstdepth = blit_desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	int wait_for_lock = lock_must_succeed(bounds, vector_dirty_pixels);
	struct win_blit_params params;
	HRESULT result;
	RECT src, dst;
	int dstxoffs;

tryagain:
	if (win_d3d_tex_manage)
	{
		// we only need to lock a part of the surface
		src.left = 0;
		src.top = 0;
		src.right = win_visible_width * effect_min_xscale + 16;
		src.bottom = win_visible_height * effect_min_yscale;

		// attempt to lock the blit surface
		result = IDirectDrawSurface7_Lock(blit_surface, &src, &blit_desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS | (wait_for_lock ? DDLOCK_WAIT : 0), NULL);
	}
	else
	{
		// attempt to lock the blit surface
		result = IDirectDrawSurface7_Lock(blit_surface, NULL, &blit_desc, DDLOCK_SURFACEMEMORYPTR | (wait_for_lock ? DDLOCK_WAIT : 0), NULL);
	}
	if (result == DDERR_SURFACELOST)
		goto surface_lost;

	// if it was busy (and we're not throttling), just punt
	if (result == DDERR_SURFACEBUSY || result == DDERR_WASSTILLDRAWING)
		return 1;
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Unable to lock blit_surface: %08x\n", (UINT32)result);
		return 0;
	}

	// align the destination to 16 bytes
	dstxoffs = (((UINT32)blit_desc.lpSurface + 16) & ~15) - (UINT32)blit_desc.lpSurface;
	dstxoffs /= (dstdepth / 8);

	// perform the low-level blit
	params.dstdata		= blit_desc.lpSurface;
	params.dstpitch		= blit_desc.DUMMYUNIONNAMEN(1).lPitch;
	params.dstdepth		= dstdepth;
	params.dstxoffs		= dstxoffs;
	params.dstyoffs		= 0;
	params.dstxscale	= effect_min_xscale;
	params.dstyscale	= effect_min_yscale;
	params.dstyskip		= 0;
	params.dsteffect	= win_determine_effect(&params);

	params.srcdata		= bitmap->base;
	params.srcpitch		= bitmap->rowbytes;
	params.srcdepth		= bitmap->depth;
	params.srclookup	= win_prepare_palette(&params);
	params.srcxoffs		= win_visible_rect.left;
	params.srcyoffs		= win_visible_rect.top;
	params.srcwidth		= win_visible_width;
	params.srcheight	= win_visible_height;

	params.vecdirty		= vector_dirty_pixels;

	params.flipx		= blit_flipx;
	params.flipy		= blit_flipy;
	params.swapxy		= blit_swapxy;

	// adjust for more optimal bounds
	if (bounds && !update && !vector_dirty_pixels)
	{
		params.dstxoffs += (bounds->min_x - win_visible_rect.left) * effect_min_xscale;
		params.dstyoffs += (bounds->min_y - win_visible_rect.top) * effect_min_yscale;
		params.srcxoffs += bounds->min_x - win_visible_rect.left;
		params.srcyoffs += bounds->min_y - win_visible_rect.top;
		params.srcwidth = bounds->max_x - bounds->min_x + 1;
		params.srcheight = bounds->max_y - bounds->min_y + 1;
	}

	win_perform_blit(&params, 0);

	// unlock the surface
	IDirectDrawSurface7_Unlock(blit_surface, NULL);

	// make the src rect
	src.left = dstxoffs;
	src.top = 0;
	src.right = (dstxoffs + win_visible_width * effect_min_xscale);
	src.bottom = (win_visible_height * effect_min_yscale);

	// window mode
	if (win_window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(win_video_window, &dst);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[0]);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[1]);
	}

	// full screen mode
	else
	{
		// constrain to the screen/window size
		dst.left = dst.top = 0;
		dst.right = primary_desc.dwWidth;
		dst.bottom = primary_desc.dwHeight;

		win_constrain_to_aspect_ratio(&dst, WMSZ_BOTTOMRIGHT, win_default_constraints);

		// center
		dst.left += (primary_desc.dwWidth - (dst.right - dst.left)) / 2;
		dst.top += (primary_desc.dwHeight - (dst.bottom - dst.top)) / 2;
		dst.right += dst.left;
		dst.bottom += dst.top;
   }

	// render and flip
	if (!render_and_flip(&src, &dst, update))
		return 0;

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Restoring surfaces\n");

	// go ahead and adjust the window
	win_adjust_window();

	// restore the surfaces
	if (!restore_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	render_and_flip
//============================================================

static int render_and_flip(LPRECT src, LPRECT dst, int update)
{
	LPDIRECTDRAWSURFACE7 texture;
	D3DVIEWPORT7 viewport;
	HRESULT result;

	// determine the current zoom level
	win_d3d_current_zoom = win_d3d_effects_swapxy ? (dst->right - dst->left) / win_visible_width :
													(dst->bottom - dst->top) / win_visible_height;

	if (prev_zoom != win_d3d_current_zoom)
	{
		prev_zoom = win_d3d_current_zoom;

		// kludge to force the -effect auto effect to the proper size
		if (win_d3d_use_auto_effect)
		{
			win_d3d_effects_init();
			win_d3d_effects_init_surfaces();
		}
	}

tryagain:
	// if the surface is lost, bail an try again
	result = IDirectDrawSurface7_IsLost(back_surface);
	if (result == DDERR_SURFACELOST)
		goto surface_lost;

	if (win_d3d_tex_manage)
	{
		// Adjust texture coordinates to the source
		vertex[0].tu = vertex[2].tu = (float)src->left / blit_desc.dwWidth;
		vertex[1].tu = vertex[3].tu = (float)src->right / blit_desc.dwWidth;
		vertex[0].tv = vertex[1].tv = (float)src->top / blit_desc.dwHeight;
		vertex[2].tv = vertex[3].tv = (float)src->bottom / blit_desc.dwHeight;
	}
	else
	{
		// blit the image to the texture surface ourselves when texture management in't used
		result = IDirectDrawSurface7_BltFast(texture_surface, 0, 0, blit_surface, src, DDBLTFAST_WAIT);
		if (result == DDERR_SURFACELOST)
			goto surface_lost;
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt blit_surface to texture_surface: %08x\n", (UINT32)result);
			return 0;
		}

		// set texture coordinates
		vertex[0].tu = vertex[2].tu = 0;
		vertex[1].tu = vertex[3].tu = (float)(src->right - src->left) / texture_desc.dwWidth;
		vertex[0].tv = vertex[1].tv = 0;
		vertex[2].tv = vertex[3].tv = (float)(src->bottom - src->top) / texture_desc.dwHeight;
	}

	// set texture coordinates for scanlines
	if (win_d3d_effects_swapxy)
	{
		vertex[2].tu1 = vertex[3].tu1 = win_d3d_effects_flipx ? (float)win_visible_height : 0;
		vertex[0].tu1 = vertex[1].tu1 = win_d3d_effects_flipx ? 0 : (float)win_visible_height;
		vertex[2].tv1 = vertex[0].tv1 = win_d3d_effects_flipy ? (float)win_visible_width : 0;
		vertex[3].tv1 = vertex[1].tv1 = win_d3d_effects_flipy ? 0 : (float)win_visible_width;
	}
	else
	{
		vertex[0].tu1 = vertex[2].tu1 = win_d3d_effects_flipx ? (float)win_visible_width : 0;
		vertex[1].tu1 = vertex[3].tu1 = win_d3d_effects_flipx ? 0 : (float)win_visible_width;
		vertex[0].tv1 = vertex[1].tv1 = win_d3d_effects_flipy ? (float)win_visible_height : 0;
		vertex[2].tv1 = vertex[3].tv1 = win_d3d_effects_flipy ? 0 : (float)win_visible_height;
	}

	// set the vertex coordinates
	if (win_window_mode)
	{
		// render to the upper left of the back surface
		vertex[0].sx = -0.5f;									vertex[0].sy = -0.5f;
		vertex[1].sx = -0.5f + (float)(dst->right - dst->left); vertex[1].sy = -0.5f;
		vertex[2].sx = -0.5f;									vertex[2].sy = -0.5f + (float)(dst->bottom - dst->top);
		vertex[3].sx = -0.5f + (float)(dst->right - dst->left); vertex[3].sy = -0.5f + (float)(dst->bottom - dst->top);
	}
	else
	{
		vertex[0].sx = -0.5f + (float)dst->left;  vertex[0].sy = -0.5f + (float)dst->top;
		vertex[1].sx = -0.5f + (float)dst->right; vertex[1].sy = -0.5f + (float)dst->top;
		vertex[2].sx = -0.5f + (float)dst->left;  vertex[2].sy = -0.5f + (float)dst->bottom;
		vertex[3].sx = -0.5f + (float)dst->right; vertex[3].sy = -0.5f + (float)dst->bottom;
	}

	// determine if we need to render to the pre-process texture first
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
		RECT rect;

		result = IDirect3DDevice7_SetRenderTarget(d3d_device7, preprocess_surface, 0);

		// match the coordinates to the transfer method used
		if (win_d3d_tex_manage)
		{
			rect.left = src->left << (win_d3d_use_prescale >> 4);
			rect.right = src->right << (win_d3d_use_prescale >> 4);
			rect.top = src->top << (win_d3d_use_prescale & 15);
			rect.bottom = src->bottom << (win_d3d_use_prescale & 15);
		}
		else
		{
			rect.left = 0;
			rect.right = (src->right - src->left) << (win_d3d_use_prescale >> 4);
			rect.top = 0;
			rect.bottom = (src->bottom - src->top) << (win_d3d_use_prescale & 15);
		}

		// prepare x/y coordinates
		viewport.dwX = 0;
		viewport.dwY = 0;
		viewport.dwWidth = texture_desc.dwWidth << (win_d3d_use_prescale >> 4);
		viewport.dwHeight = texture_desc.dwHeight << (win_d3d_use_prescale & 15);

		// set vertex coordinates for rendering to the pre-process texture
		preprocess_vertex[0].sx = -0.5f + rect.left;  preprocess_vertex[0].sy = -0.5f + rect.top;
		preprocess_vertex[1].sx = -0.5f + rect.right; preprocess_vertex[1].sy = -0.5f + rect.top;
		preprocess_vertex[2].sx = -0.5f + rect.left;  preprocess_vertex[2].sy = -0.5f + rect.bottom;
		preprocess_vertex[3].sx = -0.5f + rect.right; preprocess_vertex[3].sy = -0.5f + rect.bottom;

		// set texture coordinates
		preprocess_vertex[0].tu = preprocess_vertex[2].tu = vertex[0].tu;
		preprocess_vertex[1].tu = preprocess_vertex[3].tu = vertex[1].tu;
		preprocess_vertex[0].tv = preprocess_vertex[1].tv = vertex[0].tv;
		preprocess_vertex[2].tv = preprocess_vertex[3].tv = vertex[2].tv;

		// set up the viewport
		result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);

		result = IDirect3DDevice7_BeginScene(d3d_device7);

		if (win_d3d_use_feedback)
		{
			// use the needed colour
			preprocess_vertex[0].color = win_d3d_preprocess_tfactor;
			preprocess_vertex[1].color = win_d3d_preprocess_tfactor;
			preprocess_vertex[2].color = win_d3d_preprocess_tfactor;
			preprocess_vertex[3].color = win_d3d_preprocess_tfactor;

			// alpha blend the new image with the previous one
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCALPHA);

			// stage 0 holds the new image, the previous one is still on the texture
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		}
		else
		{
			// just do a simple copy
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

			// stage 0 holds the image
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}

		// never use texture filtering
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, D3DTFN_POINT);

		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 0);

		// stage 1 isn't used
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		IDirect3DDevice7_SetTexture(d3d_device7, 0, win_d3d_tex_manage ? blit_surface : texture_surface);

		// now render the image using 3d hardware
		result = IDirect3DDevice7_DrawPrimitive(d3d_device7, D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, (void*)preprocess_vertex, 4, 0);
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to render to preprocess texture: %08x\n", (UINT32)result);
			return 0;
		}

		IDirect3DDevice7_SetTexture(d3d_device7, 0, NULL);

		result = IDirect3DDevice7_EndScene(d3d_device7);

		texture = preprocess_surface;
	}
	else
	{
		texture = win_d3d_tex_manage ? blit_surface : texture_surface;
	}

	result = IDirect3DDevice7_SetRenderTarget(d3d_device7, back_surface, 0);

	// set up the viewport
	viewport.dwX = 0;
	viewport.dwY = 0;
	viewport.dwWidth = primary_desc.dwWidth;
	viewport.dwHeight = primary_desc.dwHeight;

	result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);

	result = IDirect3DDevice7_BeginScene(d3d_device7);

	// set the Diret3D state

	switch (win_d3d_use_rgbeffect)
	{
		case 1:
		{
			// RGB effects multiply
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		}
		case 2:
		{
			// RGB effects multiply & add
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		}
		default:
		{
			// no RGB effects
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
		}
	}

	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_TEXTUREFACTOR, win_d3d_tfactor);

	// determine if we should render scanlines
	if (!win_d3d_use_scanlines || win_d3d_current_zoom < 2)
	{
		// no scanlines

		// indicate how to filter the image texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, d3dtfg_image);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MINFILTER, d3dtfn_image);

		// stage 0 holds the image
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 0);

		// stage 1 isn't used
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		IDirect3DDevice7_SetTexture(d3d_device7, 0, texture);
	}
	else
	{
		// render scanlines

		// indicate how to filter the image texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_MAGFILTER, d3dtfg_image);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_MINFILTER, d3dtfn_image);

		// indicate how to filter the scanline texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, d3dtfg_scanlines);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MINFILTER, d3dtfn_scanlines);

		// stage 0 holds the scanlines
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, d3dtop_scanlines);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 1);

		// stage 1 holds the image
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_TEXCOORDINDEX, 0);

		// use the best scanline texture for the current zoom level
		IDirect3DDevice7_SetTexture(d3d_device7, 0, win_d3d_scanline_surface[win_d3d_current_zoom >= 4 ? 1 : 0]);
		IDirect3DDevice7_SetTexture(d3d_device7, 1, texture);
	}

	// now render the image using 3d hardware
	result = IDirect3DDevice7_DrawPrimitive(d3d_device7, D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX2, (void*)vertex, 4, 0);
	if (result != DD_OK)
	{
		// error, print the error and fall back
		if (verbose)
			fprintf(stderr, "Unable to render to back_surface: %08x\n", (UINT32)result);
		return 0;
	}

	IDirect3DDevice7_SetTexture(d3d_device7, 0, NULL);
	IDirect3DDevice7_SetTexture(d3d_device7, 1, NULL);

	result = IDirect3DDevice7_EndScene(d3d_device7);

	// sync to VBLANK?
	if ((win_wait_vsync || win_sync_refresh) && throttle && mame_get_performance_info()->game_speed_percent > 95)
	{
		BOOL is_vblank;

		// this counts as idle time
		profiler_mark(PROFILER_IDLE);

		result = IDirectDraw7_GetVerticalBlankStatus(ddraw7, &is_vblank);
		if (!is_vblank)
			result = IDirectDraw7_WaitForVerticalBlank(ddraw7, DDWAITVB_BLOCKBEGIN, 0);

		// idle time done
		profiler_mark(PROFILER_END);
	}

	// erase the edges if updating
	if (update)
	{
		RECT outer;
		outer.top = outer.left = 0;
		outer.right = primary_desc.dwWidth;
		outer.bottom = primary_desc.dwHeight;
		erase_outer_rect(&outer, dst, win_window_mode ? primary_surface : back_surface);
	}

	// if in windowed mode, blit the image from the back surface to the primary surface
	if (win_window_mode)
	{
		// blit from the upper left of the back surface to the correct position on the screen
		RECT rect = { 0, 0, dst->right - dst->left, dst->bottom - dst->top };

		result = IDirectDrawSurface7_Blt(primary_surface, dst, back_surface, &rect, DDBLT_ASYNC, NULL);
		if (result == DDERR_SURFACELOST)
			goto surface_lost;
		if (result != DD_OK)
		{
			// otherwise, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt back_surface to primary_surface: %08x\n", (UINT32)result);
			return 0;
		}
	}
	// full screen mode: flip
	else
	{
#if SHOW_FLIP_TIMES
		static cycles_t total;
		static int count;
		cycles_t start = osd_cycles(), stop;
#endif

		result = IDirectDrawSurface7_Flip(primary_surface, NULL, DDFLIP_WAIT | ((!win_triple_buffer || !throttle) ?  DDFLIP_NOVSYNC : 0));

#if SHOW_FLIP_TIMES
		stop = osd_cycles();
		if (++count > 100)
		{
			total += stop - start;
			usrintf_showmessage("Avg Flip = %d", (int)(total / (count - 100)));
		}
#endif
	}

	// blit the pattern for the RGB effects
	if (win_d3d_use_rgbeffect)
	{
		RECT rect = { 0, 0, dst->right - dst->left, dst->bottom - dst->top };

		result = IDirectDrawSurface7_BltFast(back_surface, win_window_mode ? 0 : dst->left, win_window_mode ? 0 : dst->top, win_d3d_background_surface, &rect, DDBLTFAST_WAIT);
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt RGB effect to back_surface: %08x\n",	(UINT32)result);
			return 0;
		}
	}

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Restoring surfaces\n");

	// go ahead and adjust the window
	win_adjust_window();

	// restore the surfaces
	if (!restore_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}
