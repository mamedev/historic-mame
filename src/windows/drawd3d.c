//============================================================
//
//  wind3d.c - Win32 Direct3D 7 (with DirectDraw 7) code
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// Useful info:
//  Windows XP/2003 shipped with DirectX 8.1
//  Windows 2000 shipped with DirectX 7a
//  Windows 98SE shipped with DirectX 6.1a
//  Windows 98 shipped with DirectX 5
//  Windows NT shipped with DirectX 3.0a
//  Windows 95 shipped with DirectX 2

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ddraw.h>
#include <d3d8.h>
#include <mmsystem.h>

#ifndef D3DCAPS2_DYNAMICTEXTURES
#define D3DCAPS2_DYNAMICTEXTURES 0x20000000L
#endif

// standard C headers
#include <math.h>

#include "driver.h"
#include "render.h"
#include "video.h"
#include "window.h"
#include "drawd3d.h"
#include "profiler.h"



// future caps to handle:
//    if (d3d->caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA)




extern int verbose;


//============================================================
//  DEBUGGING
//============================================================

#define DEBUG_MODE_SCORES	0



//============================================================
//  CONSTANTS
//============================================================

#define VERTEX_FORMAT		(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define VERTEX_BUFFER_SIZE	(2048*4)



//============================================================
//  MACROS
//============================================================

#define FSWAP(var1, var2) do { float temp = var1; var1 = var2; var2 = temp; } while (0)



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef IDirect3D8 *(WINAPI *direct3dcreate8_ptr)(UINT SDKVersion);


/* a d3d_texture holds information about a texture */
typedef struct _d3d_texture d3d_texture;
struct _d3d_texture
{
	d3d_texture *			next;
	UINT32					hash;
	UINT32					flags;
	float					uscale, vscale;
	render_texinfo			texinfo;
	IDirect3DTexture8 *		d3dtex;
};


/* a d3d_poly holds information about a single polygon/d3d primitive */
typedef struct _d3d_poly d3d_poly;
struct _d3d_poly
{
	 D3DPRIMITIVETYPE		type;
	 UINT32					count;
	 UINT32					numverts;
	 UINT32					flags;
	 d3d_texture *			texture;
};


/* a d3d_vertex describes a single vertex */
typedef struct _d3d_vertex d3d_vertex;
struct _d3d_vertex
{
	float					x, y, z;
	float					rhw;
	D3DCOLOR 				color;
	float					u0, v0;
	float					padding;
};


/* d3d_info is the information about Direct3D for the current screen */
typedef struct _d3d_info d3d_info;
struct _d3d_info
{
	int						adapter;					// ordinal adapter number
	int						width, height;				// current width, height
	int						refresh;					// current refresh rate

	IDirect3DDevice8 *		device;						// pointer to the Direct3DDevice object
	D3DCAPS8				caps;						// device capabilities
	D3DDISPLAYMODE			origmode;					// original display mode for the adapter
	D3DFORMAT				pixformat;					// pixel format we are using

	IDirect3DVertexBuffer8 *vertexbuf;					// pointer to the vertex buffer object
	d3d_vertex *			lockedbuf;					// pointer to the locked vertex buffer
	int						numverts;					// number of accumulated vertices

	d3d_poly				poly[VERTEX_BUFFER_SIZE / 3];// array to hold polygons as they are created
	int						numpolys;					// number of accumulated polygons

	d3d_texture *			texlist;					// list of active textures
	int						dynamic_supported;			// are dynamic textures supported?
	D3DFORMAT				screen_format;				// format to use for screen textures
};


/* line_aa_step is used for drawing antialiased lines */
typedef struct _line_aa_step line_aa_step;
struct _line_aa_step
{
	float		xoffs;
	float		yoffs;
	float		weight;
};



//============================================================
//  GLOBALS
//============================================================

static IDirect3D8 *			d3d8;						// pointer to the root Direct3D object
static HINSTANCE			d3d8dll;					// handle to the DLL

static const line_aa_step line_aa_1step[] =
{
	{  0.00f,  0.00f,  1.00f  },
	{ 0 }
};

static const line_aa_step line_aa_4step[] =
{
	{ -0.25f,  0.00f,  0.25f  },
	{  0.25f,  0.00f,  0.25f  },
	{  0.00f, -0.25f,  0.25f  },
	{  0.00f,  0.25f,  0.25f  },
	{ 0 }
};



//============================================================
//  INLINES
//============================================================

INLINE UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags)
{
	return (UINT32)texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}



//============================================================
//  PROTOTYPES
//============================================================

static int device_create(win_window_info *window);
static void device_delete(d3d_info *d3d);
static int device_verify_caps(d3d_info *d3d);

// video modes
static int config_adapter_mode(win_window_info *window);
static int get_adapter_for_monitor(d3d_info *d3d, win_monitor_info *monitor);
static void pick_best_mode(win_window_info *window);
static int update_window_size(win_window_info *window);

// drawing
static void draw_line(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip);
static void draw_quad(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip);

// primitives
static d3d_vertex *primitive_alloc(d3d_info *d3d, int numverts);
static void primitive_flush_pending(d3d_info *d3d);

// textures
static d3d_texture *texture_create(d3d_info *d3d, const render_texinfo *texsource, UINT32 flags);
static void texture_set_data(d3d_info *d3d, IDirect3DTexture8 *texture, const render_texinfo *texsource, UINT32 flags);



//============================================================
//  drawd3d_init
//============================================================

int drawd3d_init(void)
{
	direct3dcreate8_ptr direct3dcreate8;

	// dynamically grab the create function from d3d8.dll
	d3d8dll = LoadLibrary("d3d8.dll");
	if (d3d8dll == NULL)
	{
		fprintf(stderr, "Unable to access d3d8.dll\n");
		return 1;
	}

	// import the create function
	direct3dcreate8 = (direct3dcreate8_ptr)GetProcAddress(d3d8dll, "Direct3DCreate8");
	if (direct3dcreate8 == NULL)
	{
		fprintf(stderr, "Unable to find Direct3DCreate8\n");
		FreeLibrary(d3d8dll);
		d3d8dll = NULL;
		return 1;
	}

	// create our core direct 3d object
	d3d8 = (*direct3dcreate8)(D3D_SDK_VERSION);
	if (d3d8 == NULL)
	{
		fprintf(stderr, "Error attempting to initialize Direct3D8\n");
		FreeLibrary(d3d8dll);
		d3d8dll = NULL;
		return 1;
	}
	return 0;
}



//============================================================
//  drawd3d_exit
//============================================================

void drawd3d_exit(void)
{
	// relase the D3D8 object itself
	if (d3d8 != NULL)
		IDirect3D8_Release(d3d8);

	// release our handle to the DLL
	if (d3d8dll != NULL)
		FreeLibrary(d3d8dll);
}



//============================================================
//  drawd3d_window_init
//============================================================

int drawd3d_window_init(win_window_info *window)
{
	d3d_info *d3d;

	// allocate memory for our structures
	d3d = malloc_or_die(sizeof(*d3d));
	memset(d3d, 0, sizeof(*d3d));
	window->dxdata = d3d;

	// configure the adapter for the mode we want
	if (config_adapter_mode(window))
		goto error;

	// create the device immediately for the full screen case (defer for window mode)
	if (!video_config.windowed && device_create(window))
		goto error;

	return 0;

error:
	drawd3d_window_destroy(window);
	return 1;
}



//============================================================
//  drawd3d_window_destroy
//============================================================

void drawd3d_window_destroy(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;

	// skip if nothing
	if (d3d == NULL)
		return;

	// delete the device
	device_delete(d3d);

	// free the memory in the window
	free(d3d);
	window->dxdata = NULL;
}



//============================================================
//  drawd3d_window_draw
//============================================================

int drawd3d_window_draw(win_window_info *window, HDC dc, const render_primitive *primlist, int update)
{
	render_bounds clipstack[8];
	render_bounds *clip = &clipstack[0];
	d3d_info *d3d = window->dxdata;
	const render_primitive *prim;
	HRESULT result;

	// if we haven't been created, just punt
	if (d3d == NULL)
		return 1;

	// in window mode, we need to track the window size
	if (video_config.windowed || d3d->device == NULL)
	{
		// if the size changes, skip this update since the render target will be out of date
		if (update_window_size(window))
			return 0;

		// if we have no device, after updating the size, return an error so GDI can try
		if (d3d->device == NULL)
			return 1;
	}

	// set up the initial clipping rect
	clip->x0 = clip->y0 = 0;
	clip->x1 = (float)d3d->width;
	clip->y1 = (float)d3d->height;

	// begin the scene
	result = IDirect3DDevice8_BeginScene(d3d->device);
	assert(result == D3D_OK);
	result = IDirect3DDevice8_SetTexture(d3d->device, 0, NULL);
	assert(result == D3D_OK);

	d3d->lockedbuf = NULL;

	// loop over primitives
	for (prim = primlist; prim != NULL; prim = prim->next)
		switch (prim->type)
		{
			case RENDER_PRIMITIVE_CLIP_PUSH:
				clip++;
				assert(clip - clipstack < ARRAY_LENGTH(clipstack));

				/* extract the new clip */
				*clip = prim->bounds;

				/* clip against the main bounds */
				if (clip->x0 < 0) clip->x0 = 0;
				if (clip->y0 < 0) clip->y0 = 0;
				if (clip->x1 > (float)d3d->width) clip->x1 = (float)d3d->width;
				if (clip->y1 > (float)d3d->height) clip->y1 = (float)d3d->height;
				break;

			case RENDER_PRIMITIVE_CLIP_POP:
				clip--;
				assert(clip >= clipstack);
				break;

			case RENDER_PRIMITIVE_LINE:
				draw_line(d3d, prim, clip);
				break;

			case RENDER_PRIMITIVE_QUAD:
				draw_quad(d3d, prim, clip);
				break;
		}

	// flush any pending polygons
	primitive_flush_pending(d3d);

	// finish the scene
	result = IDirect3DDevice8_EndScene(d3d->device);
	assert(result == D3D_OK);

	// explicitly wait for VBLANK
	if ((video_config.waitvsync || video_config.syncrefresh) && video_config.throttle && !(!video_config.windowed && video_config.triplebuf))
	{
		// this counts as idle time
		profiler_mark(PROFILER_IDLE);
		while (1)
		{
			D3DRASTER_STATUS status;

			// get the raster status, and break only when we hit VBLANK
			result = IDirect3DDevice8_GetRasterStatus(d3d->device, &status);
			assert(result == D3D_OK);
			if (status.InVBlank)
				break;
		}
		profiler_mark(PROFILER_END);
	}

	// finally present the scene
	result = IDirect3DDevice8_Present(d3d->device, NULL, NULL, NULL, NULL);
	assert(result == D3D_OK);

	return 0;
}



//============================================================
//  device_create
//============================================================

static int device_create(win_window_info *window)
{
	D3DPRESENT_PARAMETERS presentation;
	d3d_info *d3d = window->dxdata;
	HRESULT result;
	int verify;

	// if a device exists, free it
	if (d3d->device != NULL)
		device_delete(d3d);

	// get the caps
	memset(&d3d->caps, 0, sizeof(d3d->caps));
	result = IDirect3D8_GetDeviceCaps(d3d8, d3d->adapter, D3DDEVTYPE_HAL, &d3d->caps);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Unable to get the device capabilities (%08X)\n", (UINT32)result);
		return 1;
	}

	// verify the caps
	verify = device_verify_caps(d3d);
	if (verify == 2)
	{
		fprintf(stderr, "Error: Device does not meet minimum requirements for Direct3D rendering\n");
		return 1;
	}
	if (verify == 1)
		fprintf(stderr, "Warning: Device may not perform well for Direct3D rendering\n");

	// verify texture formats
	result = IDirect3D8_CheckDeviceFormat(d3d8, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error: A8R8G8B8 format textures not supported\n");
		return 1;
	}

try_again:
	// try for XRGB first
	d3d->screen_format = D3DFMT_X8R8G8B8;
	result = IDirect3D8_CheckDeviceFormat(d3d8, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0, D3DRTYPE_TEXTURE, d3d->screen_format);
	if (result != D3D_OK)
	{
		// if not, try for ARGB
		d3d->screen_format = D3DFMT_A8R8G8B8;
		result = IDirect3D8_CheckDeviceFormat(d3d8, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0, D3DRTYPE_TEXTURE, d3d->screen_format);
		if (result != D3D_OK && d3d->dynamic_supported)
		{
			d3d->dynamic_supported = FALSE;
			goto try_again;
		}
		if (result != D3D_OK)
		{
			fprintf(stderr, "Error: unable to configure a screen texture format\n");
			return 1;
		}
	}

	// initialize the D3D presentation parameters
	memset(&presentation, 0, sizeof(presentation));
	presentation.BackBufferWidth					= d3d->width;
	presentation.BackBufferHeight					= d3d->height;
	presentation.BackBufferFormat					= d3d->pixformat;
	presentation.BackBufferCount					= video_config.triplebuf ? 2 : 1;
	presentation.MultiSampleType					= D3DMULTISAMPLE_NONE;
	presentation.SwapEffect							= D3DSWAPEFFECT_DISCARD;
	presentation.hDeviceWindow						= window->hwnd;
	presentation.Windowed							= video_config.windowed;
	presentation.EnableAutoDepthStencil				= FALSE;
	presentation.AutoDepthStencilFormat				= D3DFMT_D16;
	presentation.Flags								= 0;
	presentation.FullScreen_RefreshRateInHz			= d3d->refresh;
	presentation.FullScreen_PresentationInterval	= video_config.windowed ? D3DPRESENT_INTERVAL_DEFAULT :
													  video_config.triplebuf ? D3DPRESENT_INTERVAL_ONE :
													  D3DPRESENT_INTERVAL_IMMEDIATE;
	// create the D3D device
	result = IDirect3D8_CreateDevice(d3d8, d3d->adapter, D3DDEVTYPE_HAL, win_window_list->hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentation, &d3d->device);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Unable to create the Direct3D device (%08X)\n", (UINT32)result);
		return 1;
	}

	// allocate a vertex buffer to use
	result = IDirect3DDevice8_CreateVertexBuffer(d3d->device,
				sizeof(d3d_vertex) * VERTEX_BUFFER_SIZE,
				D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY,
				VERTEX_FORMAT, D3DPOOL_DEFAULT, &d3d->vertexbuf);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error creating vertex buffer (%08X)", (UINT32)result);
		return 1;
	}

	// set the vertex format
	result = IDirect3DDevice8_SetVertexShader(d3d->device, VERTEX_FORMAT);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error setting vertex shader (%08X)", (UINT32)result);
		return 1;
	}

	// set the fixed render state
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ZENABLE, D3DZB_FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_FILLMODE, D3DFILL_SOLID);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SHADEMODE, D3DSHADE_FLAT);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ZWRITEENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHATESTENABLE, TRUE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_LASTPIXEL, TRUE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_CULLMODE, D3DCULL_NONE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ZFUNC, D3DCMP_LESS);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHAREF, 0);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_DITHERENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_FOGENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SPECULARENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ZBIAS, 0);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_STENCILENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_WRAP0, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_CLIPPING, TRUE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_LIGHTING, FALSE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_COLORVERTEX, TRUE);
	result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_BLENDOP, D3DBLENDOP_ADD);

	result = IDirect3DDevice8_SetTextureStageState(d3d->device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	result = IDirect3DDevice8_SetTextureStageState(d3d->device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	// clear the buffer
	result = IDirect3DDevice8_Clear(d3d->device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0,0,0), 0, 0);
	result = IDirect3DDevice8_Present(d3d->device, NULL, NULL, NULL, NULL);

	if (verbose)
		printf("Device created at %dx%d\n", d3d->width, d3d->height);
	return 0;
}



//============================================================
//  device_delete
//============================================================

static void device_delete(d3d_info *d3d)
{
	// free all textures
	while (d3d->texlist != NULL)
	{
		d3d_texture *tex = d3d->texlist;
		d3d->texlist = tex->next;
		if (tex->d3dtex != NULL)
			IDirect3DTexture8_Release(tex->d3dtex);
		free(tex);
	}

	// free the vertex buffer
	if (d3d->vertexbuf != NULL)
		IDirect3DVertexBuffer8_Release(d3d->vertexbuf);

	// free the device itself
	if (d3d->device != NULL)
		IDirect3DDevice8_Release(d3d->device);
}



//============================================================
//  device_verify_caps
//============================================================

static int device_verify_caps(d3d_info *d3d)
{
	int result = 0;

	// verify presentation capabilities
	if (!(d3d->caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE))
	{
		fprintf(stderr, "Error: Device does not support immediate presentations\n");
		result = 2;
	}
	if (!(d3d->caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE))
	{
		fprintf(stderr, "Error: Device does not support per-refresh presentations\n");
		result = 2;
	}

	// verify device capabilities
	if (!(d3d->caps.DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP))
	{
		fprintf(stderr, "Warning: Device does not support queued rendering after a page flip\n");
		result = 1;
	}
	if (!(d3d->caps.DevCaps & D3DDEVCAPS_HWRASTERIZATION))
	{
		fprintf(stderr, "Warning: Device does not support hardware rasterization\n");
		result = 1;
	}

	// verify source blend capabilities
	if (!(d3d->caps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA))
	{
		fprintf(stderr, "Error: Device does not support source alpha blending with source alpha\n");
		result = 2;
	}
	if (!(d3d->caps.SrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR))
	{
		fprintf(stderr, "Error: Device does not support source alpha blending with destination color\n");
		result = 2;
	}

	// verify destination blend capabilities
	if (!(d3d->caps.DestBlendCaps & D3DPBLENDCAPS_ZERO))
	{
		fprintf(stderr, "Error: Device does not support dest alpha blending with zero\n");
		result = 2;
	}
	if (!(d3d->caps.DestBlendCaps & D3DPBLENDCAPS_ONE))
	{
		fprintf(stderr, "Error: Device does not support dest alpha blending with one\n");
		result = 2;
	}
	if (!(d3d->caps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA))
	{
		fprintf(stderr, "Error: Device does not support dest alpha blending with inverted source alpha\n");
		result = 2;
	}

	// verify texture capabilities
	if (!(d3d->caps.TextureCaps & D3DPTEXTURECAPS_ALPHA))
	{
		fprintf(stderr, "Error: Device does not support texture alpha\n");
		result = 2;
	}

	// verify texture filter capabilities
	if (!(d3d->caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		fprintf(stderr, "Warning: Device does not support point-sample texture filtering for magnification\n");
		result = 1;
	}
	if (!(d3d->caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		fprintf(stderr, "Warning: Device does not support bilinear texture filtering for magnification\n");
		result = 1;
	}
	if (!(d3d->caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		fprintf(stderr, "Warning: Device does not support point-sample texture filtering for minification\n");
		result = 1;
	}
	if (!(d3d->caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		fprintf(stderr, "Warning: Device does not support bilinear texture filtering for minification\n");
		result = 1;
	}

	// verify texture addressing capabilities
	if (!(d3d->caps.TextureFilterCaps & D3DPTADDRESSCAPS_CLAMP))
	{
		fprintf(stderr, "Warning: Device does not support texture clamping\n");
		result = 1;
	}
	if (!(d3d->caps.TextureFilterCaps & D3DPTADDRESSCAPS_WRAP))
	{
		fprintf(stderr, "Warning: Device does not support texture wrapping\n");
		result = 1;
	}

	// verify texture operation capabilities
	if (!(d3d->caps.TextureFilterCaps & D3DTEXOPCAPS_MODULATE))
	{
		fprintf(stderr, "Warning: Device does not support texture modulation\n");
		result = 1;
	}

	// set a simpler flag to indicate we can use dynamic textures
	d3d->dynamic_supported = ((d3d->caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) != 0);

	return result;
}



//============================================================
//  config_adapter_mode
//============================================================

static int config_adapter_mode(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;
	HRESULT result;

	// choose the monitor number
	d3d->adapter = get_adapter_for_monitor(d3d, window->monitor);

	// get the current display mode
	result = IDirect3D8_GetAdapterDisplayMode(d3d8, d3d->adapter, &d3d->origmode);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error getting mode for adapter #%d\n", d3d->adapter);
		return 1;
	}

	// choose a resolution: window mode case
	if (video_config.windowed)
	{
		RECT client;

		// bounds are from the window client rect
		GetClientRect(window->hwnd, &client);
		d3d->width = client.right - client.left;
		d3d->height = client.bottom - client.top;

		// pix format is from the current mode
		d3d->pixformat = d3d->origmode.Format;
		d3d->refresh = 0;

		// make sure it's a pixel format we can get behind
		if (d3d->pixformat != D3DFMT_X1R5G5B5 && d3d->pixformat != D3DFMT_R5G6B5 && d3d->pixformat != D3DFMT_X8R8G8B8)
		{
			fprintf(stderr, "Device %s currently in an unsupported mode\n", window->monitor->info.szDevice);
			return 1;
		}
	}

	// choose a resolution: full screen mode case
	else
	{
		// default to the current mode exactly
		d3d->width = d3d->origmode.Width;
		d3d->height = d3d->origmode.Height;
		d3d->pixformat = d3d->origmode.Format;
		d3d->refresh = d3d->origmode.RefreshRate;

		// if we're allowed to switch resolutions, override with something better
		if (video_config.switchres)
			pick_best_mode(window);
	}

	// see if we can handle the device type
	result = IDirect3D8_CheckDeviceType(d3d8, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->pixformat, video_config.windowed);
	if (result != DD_OK)
	{
		fprintf(stderr, "Proposed video mode not supported on device %s\n", window->monitor->info.szDevice);
		return 1;
	}
	return 0;
}



//============================================================
//  get_adapter_for_monitor
//============================================================

static int get_adapter_for_monitor(d3d_info *d3d, win_monitor_info *monitor)
{
	int maxadapter = IDirect3D8_GetAdapterCount(d3d8);
	int adapternum;

	// iterate over adapters until we error or find a match
	for (adapternum = 0; adapternum < maxadapter; adapternum++)
	{
		HMONITOR curmonitor;

		// get the monitor for this adapter
		curmonitor = IDirect3D8_GetAdapterMonitor(d3d8, adapternum);

		// if we match the proposed monitor, this is it
		if (curmonitor == monitor->handle)
			return adapternum;
	}

	// default to the default
	return D3DADAPTER_DEFAULT;
}



//============================================================
//  pick_best_mode
//============================================================

static void pick_best_mode(win_window_info *window)
{
	INT32 target_width, target_height;
	d3d_info *d3d = window->dxdata;
	INT32 minwidth, minheight;
	float best_score = 0.0;
	int maxmodes;
	int modenum;

	// determine the minimum width/height for the selected target
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// use those as the target for now
	target_width = minwidth;
	target_height = minheight;

	// determine the maximum number of modes
	maxmodes = IDirect3D8_GetAdapterModeCount(d3d8, d3d->adapter);

	// enumerate all the video modes and find the best match
	for (modenum = 0; modenum < maxmodes; modenum++)
	{
		float size_score, refresh_score, final_score;
		D3DDISPLAYMODE mode;
		HRESULT result;

		// check this mode
		result = IDirect3D8_EnumAdapterModes(d3d8, d3d->adapter, modenum, &mode);
		if (result != D3D_OK)
			break;

		// skip non-32 bit modes
		if (mode.Format != D3DFMT_X8R8G8B8)
			continue;

		// compute initial score based on difference between target and current
		size_score = 1.0f / (1.0f + fabs(mode.Width - target_width) + fabs(mode.Height - target_height));

		// if we're looking for a particular mode, that's a winner
		if (mode.Width == window->maxwidth && mode.Height == window->maxheight)
			size_score = 1.0f;

		// if the mode is too small, give a big penalty
		if (mode.Width < minwidth || mode.Height < minheight)
			size_score *= 0.01f;

		// if mode is smaller than we'd like, it only scores up to 0.1
		if (mode.Width < target_width || mode.Height < target_height)
			size_score *= 0.1f;

		// compute refresh score
		refresh_score = 1.0f / (1.0f + fabs((double)mode.RefreshRate - Machine->refresh_rate[0]));

		// if we're looking for a particular refresh, make sure it matches
		if (mode.RefreshRate == window->refresh)
			refresh_score = 1.0f;

		// if refresh is smaller than we'd like, it only scores up to 0.1
		if ((double)mode.RefreshRate < Machine->refresh_rate[0])
			refresh_score *= 0.1;

		// weight size highest, followed by depth and refresh
		final_score = (size_score * 100.0 + refresh_score) / 101.0;

#if DEBUG_MODE_SCORES
		printf("%4dx%4d@%3d -> %f\n", mode.Width, mode.Height, mode.RefreshRate, final_score);
#endif

		// best so far?
		if (final_score > best_score)
		{
			best_score = final_score;
			d3d->width = mode.Width;
			d3d->height = mode.Height;
			d3d->pixformat = mode.Format;
			d3d->refresh = mode.RefreshRate;
		}
	}
}



//============================================================
//  update_window_size
//============================================================

static int update_window_size(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;
	RECT client;

	// get the current window bounds
	GetClientRect(window->hwnd, &client);

	// if we have a device and matching width/height, nothing to do
	if (d3d->device != NULL && rect_width(&client) == d3d->width && rect_height(&client) == d3d->height)
	{
		// clear out any pending resizing if the area didn't change
		if (window->resize_state == RESIZE_STATE_PENDING)
			window->resize_state = RESIZE_STATE_NORMAL;
		return FALSE;
	}

	// if we're in the middle of resizing, leave it alone as well
	if (window->resize_state == RESIZE_STATE_RESIZING)
		return FALSE;

	// set the new bounds and create the device again
	d3d->width = rect_width(&client);
	d3d->height = rect_height(&client);
	if (device_create(window))
		return FALSE;

	// reset the resize state to normal, and indicate we made a change
	window->resize_state = RESIZE_STATE_NORMAL;
	return TRUE;
}



//============================================================
//  draw_line
//============================================================

static void draw_line(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip)
{
	const line_aa_step *step = line_aa_4step;
	float unitx, unity, effwidth;
	render_bounds bounds;
	d3d_vertex *vertex;
	d3d_poly *poly;
	DWORD color;
	int i;

	/*
        High-level logic -- due to math optimizations, this info is lost below.

        Imagine a thick line of width (w), drawn from (p0) to (p1), with a unit
        vector (u) indicating the direction from (p0) to (p1).

          B                                                          C
            +-----------------------  ...   -----------------------+
            |                                               ^      |
            |                                               |(w)   |
            |                                               v      |
            |<---->* (p0)        ------------>         (p1) *      |
            |  (w)                    (u)                          |
            |                                                      |
            |                                                      |
            +-----------------------  ...   -----------------------+
          A                                                          D

        To convert this into a quad, we need to compute the four points A, B, C
        and D.

        Starting with point A. We first multiply the unit vector by (w) and then
        rotate the result 135 degrees. This points us in the right direction, but
        needs to be scaled by a factor of sqrt(2) to reach A. Thus, we have:

            A.x = p0.x + w * u.x * cos(135) * sqrt(2) - w * u.y * sin(135) * sqrt(2)
            A.y = p0.y + w * u.y * sin(135) * sqrt(2) + w * u.y * cos(135) * sqrt(2)

        Conveniently, sin(135) = 1/sqrt(2), and cos(135) = -1/sqrt(2), so this
        simplifies to:

            A.x = p0.x - w * u.x - w * u.y
            A.y = p0.y + w * u.y - w * u.y

        Working clockwise around the polygon, the same fallout happens all around as
        we rotate the unit vector by -135 (B), -45 (C), and 45 (D) degrees:

            B.x = p0.x - w * u.x + w * u.y
            B.y = p0.y - w * u.x - w * u.y

            C.x = p1.x + w * u.x + w * u.y
            C.y = p1.y - w * u.x + w * u.y

            D.x = p1.x + w * u.x - w * u.y
            D.y = p1.y + w * u.x + w * u.y
    */

	// first we need to compute the clipped line
	bounds = prim->bounds;
	if (render_clip_line(&bounds, clip))
		return;

	// compute a vector from point 0 to point 1
	unitx = bounds.x1 - bounds.x0;
	unity = bounds.y1 - bounds.y0;

	// points just use a +1/+1 unit vector; this gives a nice diamond pattern
	if (unitx == 0 && unity == 0)
	{
		unitx = 0.70710678f;
		unity = 0.70710678f;
	}

	// lines need to be divided by their length
	else
	{
		float invlength = 1.0f / sqrt(unitx * unitx + unity * unity);
		unitx *= invlength;
		unity *= invlength;
	}

	// compute the effective width based on the direction of the line
	effwidth = prim->width;
	if (effwidth < 0.5f)
		effwidth = 0.5f;

	// prescale unitx and unity by the length
	unitx *= effwidth;
	unity *= effwidth;

	// iterate over AA steps
	for (step = PRIMFLAG_GET_ANTIALIAS(prim->flags) ? line_aa_4step : line_aa_1step; step->weight != 0; step++)
	{
		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;

		// rotate the unit vector by 135 degrees and add to point 0
		vertex[0].x = bounds.x0 - unitx - unity + step->xoffs;
		vertex[0].y = bounds.y0 + unitx - unity + step->yoffs;

		// rotate the unit vector by -135 degrees and add to point 0
		vertex[1].x = bounds.x0 - unitx + unity + step->xoffs;
		vertex[1].y = bounds.y0 - unitx - unity + step->yoffs;

		// rotate the unit vector by -45 degrees and add to point 1
		vertex[2].x = bounds.x1 + unitx + unity + step->xoffs;
		vertex[2].y = bounds.y1 - unitx + unity + step->yoffs;

		// rotate the unit vector by 45 degrees and add to point 1
		vertex[3].x = bounds.x1 + unitx - unity + step->xoffs;
		vertex[3].y = bounds.y1 + unitx + unity + step->yoffs;

		// set the color, Z parameters to standard values
		color = D3DCOLOR_ARGB((DWORD)(prim->color.a * 255.0f * step->weight), (DWORD)(prim->color.r * 255.0f), (DWORD)(prim->color.g * 255.0f), (DWORD)(prim->color.b * 255.0f));
		for (i = 0; i < 4; i++)
		{
			vertex[i].z = 0.0f;
			vertex[i].color = color;
		}

		// now add a polygon entry
		poly = &d3d->poly[d3d->numpolys++];
		poly->type = D3DPT_TRIANGLEFAN;
		poly->count = 2;
		poly->numverts = 4;
		poly->flags = prim->flags;
		poly->texture = NULL;
	}
}



//============================================================
//  draw_quad
//============================================================

static void draw_quad(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip)
{
	d3d_texture *texture = NULL;
	d3d_vertex *vertex;
	render_bounds bounds;
	d3d_poly *poly;
	DWORD color;
	int i;

	// if we have no texture, make sure we're all clear
	if (prim->texture.base != NULL)
	{
		// find a match
		UINT32 texhash = texture_compute_hash(&prim->texture, prim->flags);
		for (texture = d3d->texlist; texture != NULL; texture = texture->next)
			if (texture->hash == texhash &&
				texture->texinfo.base == prim->texture.base &&
				texture->texinfo.width == prim->texture.width &&
				texture->texinfo.height == prim->texture.height &&
				((texture->flags ^ prim->flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
				break;

		// if we didn't find one, create a new texture
		if (texture == NULL)
			texture = texture_create(d3d, &prim->texture, prim->flags);

		// if we found it, but with a different seqid, copy the data
		if (texture->texinfo.seqid != prim->texture.seqid)
		{
			texture_set_data(d3d, texture->d3dtex, &prim->texture, prim->flags);
			texture->texinfo.seqid = prim->texture.seqid;
		}
	}

	// make a copy of the bounds
	bounds = prim->bounds;

	// non-textured case
	if (texture == NULL)
	{
		// apply clipping
		if (render_clip_quad(&bounds, clip, NULL, NULL))
			return;

		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;
	}

	// textured case
	else
	{
		float u[4], v[4];

		// set the default coordinates
		u[0] = v[0] = 0.0f;
		u[1] = texture->uscale; v[1] = 0.0f;
		u[2] = texture->uscale; v[2] = texture->vscale;
		u[3] = 0.0f; v[3] = texture->vscale;

		// apply orientation to the U/V coordinates
		if (prim->flags & ORIENTATION_SWAP_XY) { FSWAP(u[1], u[3]); FSWAP(v[1], v[3]); }
		if (prim->flags & ORIENTATION_FLIP_X) { FSWAP(u[0], u[1]); FSWAP(v[0], v[1]); FSWAP(u[2], u[3]); FSWAP(v[2], v[3]); }
		if (prim->flags & ORIENTATION_FLIP_Y) { FSWAP(u[0], u[3]); FSWAP(v[0], v[3]); FSWAP(u[1], u[2]); FSWAP(v[1], v[2]); }

		// apply clipping
		if (render_clip_quad(&bounds, clip, u, v))
			return;

		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;

		// set the final coordinates
		vertex[0].u0 = u[0];
		vertex[0].v0 = v[0];
		vertex[1].u0 = u[1];
		vertex[1].v0 = v[1];
		vertex[2].u0 = u[2];
		vertex[2].v0 = v[2];
		vertex[3].u0 = u[3];
		vertex[3].v0 = v[3];
	}

	// fill in the vertexes clockwise
	vertex[0].x = bounds.x0 - 0.5f;
	vertex[0].y = bounds.y0 - 0.5f;
	vertex[1].x = bounds.x1 - 0.5f;
	vertex[1].y = bounds.y0 - 0.5f;
	vertex[2].x = bounds.x1 - 0.5f;
	vertex[2].y = bounds.y1 - 0.5f;
	vertex[3].x = bounds.x0 - 0.5f;
	vertex[3].y = bounds.y1 - 0.5f;

	// set the color, Z parameters to standard values
	color = D3DCOLOR_ARGB((DWORD)(prim->color.a * 255.0f), (DWORD)(prim->color.r * 255.0f), (DWORD)(prim->color.g * 255.0f), (DWORD)(prim->color.b * 255.0f));
	for (i = 0; i < 4; i++)
	{
		vertex[i].z = 0.0f;
		vertex[i].rhw = 1.0f;
		vertex[i].color = color;
	}

	// now add a polygon entry
	poly = &d3d->poly[d3d->numpolys++];
	poly->type = D3DPT_TRIANGLEFAN;
	poly->count = 2;
	poly->numverts = 4;
	poly->flags = prim->flags;
	poly->texture = texture;
}



//============================================================
//  primitive_alloc
//============================================================

static d3d_vertex *primitive_alloc(d3d_info *d3d, int numverts)
{
	HRESULT result;

	// if we're going to overflow, flush
	if (d3d->lockedbuf != NULL && d3d->numverts + numverts >= VERTEX_BUFFER_SIZE)
		primitive_flush_pending(d3d);

	// if we don't have a lock, grab it now
	if (d3d->lockedbuf == NULL)
	{
		result = IDirect3DVertexBuffer8_Lock(d3d->vertexbuf, 0, 0, (BYTE **)&d3d->lockedbuf, D3DLOCK_DISCARD);
		if (result != D3D_OK)
			return NULL;
	}

	// if we already have the lock and enough room, just return a pointer
	if (d3d->lockedbuf != NULL && d3d->numverts + numverts < VERTEX_BUFFER_SIZE)
	{
		int oldverts = d3d->numverts;
		d3d->numverts += numverts;
		return &d3d->lockedbuf[oldverts];
	}
	return NULL;
}



//============================================================
//  primitive_flush_pending
//============================================================

static void primitive_flush_pending(d3d_info *d3d)
{
	d3d_texture *prevtex = (d3d_texture *)~0;
	int blendmode = -1, filter = -1;
	HRESULT result;
	int polynum;
	int vertnum;

	// ignore if we're not locked
	if (d3d->lockedbuf == NULL)
		return;

	// unlock the buffer
	result = IDirect3DVertexBuffer8_Unlock(d3d->vertexbuf);
	assert(result == D3D_OK);
	d3d->lockedbuf = NULL;

	// set the stream
	result = IDirect3DDevice8_SetStreamSource(d3d->device, 0, d3d->vertexbuf, sizeof(d3d_vertex));
	assert(result == D3D_OK);

	// now do the polys
	for (polynum = vertnum = 0; polynum < d3d->numpolys; polynum++)
	{
		d3d_poly *poly = &d3d->poly[polynum];
		int newfilter;

		// set the texture if different
		if (poly->texture != prevtex)
		{
			prevtex = poly->texture;
			result = IDirect3DDevice8_SetTexture(d3d->device, 0, (prevtex == NULL) ? NULL : (IDirect3DBaseTexture8 *)prevtex->d3dtex);
			assert(result == D3D_OK);
		}

		// set filtering if different
		newfilter = FALSE;
		if (PRIMFLAG_GET_SCREENTEX(poly->flags))
			newfilter = video_config.filter;
		if (newfilter != filter)
		{
			filter = newfilter;
			result = IDirect3DDevice8_SetTextureStageState(d3d->device, 0, D3DTSS_MINFILTER, newfilter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
			assert(result == D3D_OK);
			result = IDirect3DDevice8_SetTextureStageState(d3d->device, 0, D3DTSS_MAGFILTER, newfilter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
			assert(result == D3D_OK);
		}

		// set the blendmode if different
		if (PRIMFLAG_GET_BLENDMODE(poly->flags) != blendmode)
		{
			int prevmode = blendmode;
			blendmode = PRIMFLAG_GET_BLENDMODE(poly->flags);
			switch (blendmode)
			{
				case BLENDMODE_NONE:
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHABLENDENABLE, FALSE);
					assert(result == D3D_OK);
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					assert(result == D3D_OK);
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					assert(result == D3D_OK);
					break;

				case BLENDMODE_ALPHA:
					if (prevmode == BLENDMODE_NONE)
					{
						result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHABLENDENABLE, TRUE);
						assert(result == D3D_OK);
					}
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					assert(result == D3D_OK);
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					assert(result == D3D_OK);
					break;

				case BLENDMODE_RGB_MULTIPLY:
					if (prevmode == BLENDMODE_NONE)
					{
						result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHABLENDENABLE, TRUE);
						assert(result == D3D_OK);
					}
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
					assert(result == D3D_OK);
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_DESTBLEND, D3DBLEND_ZERO);
					assert(result == D3D_OK);
					break;

				case BLENDMODE_ADD:
					if (prevmode == BLENDMODE_NONE)
					{
						result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_ALPHABLENDENABLE, TRUE);
						assert(result == D3D_OK);
					}
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					assert(result == D3D_OK);
					result = IDirect3DDevice8_SetRenderState(d3d->device, D3DRS_DESTBLEND, D3DBLEND_ONE);
					assert(result == D3D_OK);
					break;
			}
		}

		// add the primitives
		assert(vertnum + poly->numverts <= d3d->numverts);
		result = IDirect3DDevice8_DrawPrimitive(d3d->device, poly->type, vertnum, poly->count);
		assert(result == D3D_OK);
		vertnum += poly->numverts;
	}

	// reset the vertex count
	d3d->numverts = 0;
	d3d->numpolys = 0;
}



//============================================================
//  texture_create
//============================================================

static d3d_texture *texture_create(d3d_info *d3d, const render_texinfo *texsource, UINT32 flags)
{
	int texwidth = texsource->width;
	int texheight = texsource->height;
	d3d_texture *texture;
	HRESULT result;

//printf("texture_create(%p, %dx%d, %03x)\n", texsource->base, texwidth, texheight, flags);

	// allocate a new texture
	texture = malloc_or_die(sizeof(*texture));
	memset(texture, 0, sizeof(*texture));

	// fill in the core data
	texture->hash = texture_compute_hash(texsource, flags);
	texture->flags = flags;
	texture->texinfo = *texsource;

	// round width/height up to nearest power of 2 if we need to
	if (!(d3d->caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
	{
		texwidth |= texwidth >> 1;
		texwidth |= texwidth >> 2;
		texwidth |= texwidth >> 4;
		texwidth |= texwidth >> 8;
		texheight |= texheight >> 1;
		texheight |= texheight >> 2;
		texheight |= texheight >> 4;
		texheight |= texheight >> 8;
		texwidth++;
		texheight++;
	}

	// round up to square if we need to
	if (d3d->caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		if (texwidth < texheight)
			texwidth = texheight;
		else
			texheight = texwidth;
	}

	// adjust the aspect ratio if we need to
	while (texwidth < texheight && texheight / texwidth > d3d->caps.MaxTextureAspectRatio)
		texwidth *= 2;
	while (texheight < texwidth && texwidth / texheight > d3d->caps.MaxTextureAspectRatio)
		texheight *= 2;

	// if we're above the max width/height, do what?
	if (texwidth > d3d->caps.MaxTextureWidth || texheight > d3d->caps.MaxTextureHeight)
	{
		static int printed = FALSE;
		if (!printed) fprintf(stderr, "Texture too big! (wanted: %dx%d, max is %dx%d)\n", texwidth, texheight, (int)d3d->caps.MaxTextureWidth, (int)d3d->caps.MaxTextureHeight);
		printed = TRUE;
	}

	// compute the U/V scale factors
	texture->uscale = (float)texsource->width / (float)texwidth;
	texture->vscale = (float)texsource->height / (float)texheight;

//if (d3d->caps.Caps2 & D3DCAPS2_CANMANAGERESOURCE)

	// create a new texture
	if (PRIMFLAG_GET_SCREENTEX(flags))
		result = IDirect3DDevice8_CreateTexture(d3d->device, texwidth, texheight, 1,
					d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0,
					d3d->screen_format,
					d3d->dynamic_supported ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &texture->d3dtex);
	else
		result = IDirect3DDevice8_CreateTexture(d3d->device, texwidth, texheight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture->d3dtex);
	if (result != D3D_OK)
		goto error;

	// copy the data to the texture
	texture_set_data(d3d, texture->d3dtex, texsource, flags);

	// add us to the texture list
	texture->next = d3d->texlist;
	d3d->texlist = texture;
	return texture;

error:
	free(texture);
	return NULL;
}



//============================================================
//  texture_set_data
//============================================================

static void texture_set_data(d3d_info *d3d, IDirect3DTexture8 *texture, const render_texinfo *texsource, UINT32 flags)
{
	D3DLOCKED_RECT rect;
	HRESULT result;
	int y;

	// lock the texture
	if (PRIMFLAG_GET_SCREENTEX(flags) && d3d->dynamic_supported)
		result = IDirect3DTexture8_LockRect(texture, 0, &rect, NULL, D3DLOCK_DISCARD);
	else
		result = IDirect3DTexture8_LockRect(texture, 0, &rect, NULL, 0);
	if (result != D3D_OK)
		return;

	// loop over Y
	for (y = 0; y < texsource->height; y++)
	{
		UINT32 *dst32 = (UINT32 *)((BYTE *)rect.pBits + y * rect.Pitch);
		UINT32 *src32;
		UINT16 *src16;
		int x;

		// switch off of the format and
		switch (PRIMFLAG_GET_TEXFORMAT(flags))
		{
			case TEXFORMAT_ARGB32:
				src32 = (UINT32 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = *src32++;
				break;

			case TEXFORMAT_PALETTE16:
				src16 = (UINT16 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = texsource->palette[*src16++] | 0xff000000;
				break;

			case TEXFORMAT_RGB15:
				src16 = (UINT16 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
				{
					UINT16 pix = *src16++;
					UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
					*dst32++ = color | ((color >> 5) & 0x070707) | 0xff000000;
				}
				break;

			case TEXFORMAT_RGB32:
				src32 = (UINT32 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = *src32++ | 0xff000000;
				break;

			default:
				fprintf(stderr, "Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
				break;
		}
	}

	// unlock
	IDirect3DTexture8_UnlockRect(texture, 0);
}
