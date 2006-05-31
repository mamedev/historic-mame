//============================================================
//
//  wingdi.c - Win32 GDI drawing
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "driver.h"
#include "video.h"
#include "window.h"
#include "rendsoft.h"
#include "drawgdi.h"



//============================================================
//  TYPE DEFINITIONS
//============================================================

/* gdi_info is the information for the current screen */
typedef struct _gdi_info gdi_info;
struct _gdi_info
{
	BITMAPINFO				bminfo;
	RGBQUAD					colors[256];
	UINT8 *					bmdata;
	size_t					bmsize;
};



//============================================================
//  drawgdi_window_init
//============================================================

int drawgdi_window_init(win_window_info *window)
{
	gdi_info *gdi;
	int i;

	// allocate memory for our structures
	gdi = malloc_or_die(sizeof(*gdi));
	memset(gdi, 0, sizeof(*gdi));
	window->gdidata = gdi;

	// fill in the bitmap info header
	gdi->bminfo.bmiHeader.biSize			= sizeof(gdi->bminfo.bmiHeader);
	gdi->bminfo.bmiHeader.biPlanes			= 1;
	gdi->bminfo.bmiHeader.biCompression		= BI_RGB;
	gdi->bminfo.bmiHeader.biSizeImage		= 0;
	gdi->bminfo.bmiHeader.biXPelsPerMeter	= 0;
	gdi->bminfo.bmiHeader.biYPelsPerMeter	= 0;
	gdi->bminfo.bmiHeader.biClrUsed			= 0;
	gdi->bminfo.bmiHeader.biClrImportant	= 0;

	// initialize the palette to a gray ramp
	for (i = 0; i < 256; i++)
	{
		gdi->bminfo.bmiColors[i].rgbRed			= i;
		gdi->bminfo.bmiColors[i].rgbGreen		= i;
		gdi->bminfo.bmiColors[i].rgbBlue		= i;
		gdi->bminfo.bmiColors[i].rgbReserved	= i;
	}

	return 0;
}



//============================================================
//  drawgdi_window_destroy
//============================================================

void drawgdi_window_destroy(win_window_info *window)
{
	gdi_info *gdi = window->gdidata;

	// skip if nothing
	if (gdi == NULL)
		return;

	// free the bitmap memory
	if (gdi->bmdata != NULL)
		free(gdi->bmdata);
	free(gdi);
	window->gdidata = NULL;
}



//============================================================
//  drawgdi_window_draw
//============================================================

int drawgdi_window_draw(win_window_info *window, HDC dc, const render_primitive *primlist, int update)
{
	gdi_info *gdi = window->gdidata;
	int width, height, pitch;
	RECT bounds;

	// we don't have any special resize behaviors
	if (window->resize_state == RESIZE_STATE_PENDING)
		window->resize_state = RESIZE_STATE_NORMAL;

	// get the target bounds
	GetClientRect(window->hwnd, &bounds);

	// compute width/height/pitch of target
	width = rect_width(&bounds);
	height = rect_height(&bounds);
	pitch = (width + 3) & ~3;

	// make sure our temporary bitmap is big enough
	if (pitch * height * 4 > gdi->bmsize)
	{
		gdi->bmsize = pitch * height * 4 * 2;
		gdi->bmdata = realloc(gdi->bmdata, gdi->bmsize);
	}

	// draw the primitives to the bitmap
	rgb888_draw_primitives(primlist, gdi->bmdata, width, height, pitch);
//  rgb555_draw_primitives(primlist, gdi->bmdata, width, height, pitch);

	// fill in bitmap-specific info
	gdi->bminfo.bmiHeader.biWidth = pitch;
	gdi->bminfo.bmiHeader.biHeight = -height;
	gdi->bminfo.bmiHeader.biBitCount = 32;
//  gdi->bminfo.bmiHeader.biBitCount = 16;

	// blit to the screen
	StretchDIBits(dc, 0, 0, width, height,
				0, 0, width, height,
				gdi->bmdata, &gdi->bminfo, DIB_RGB_COLORS, SRCCOPY);
	return 0;
}
