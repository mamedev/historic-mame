/*
 * directdraw.c

 Direct Draw routines.
 
 */

#include "win32.h"

int screen_width = 800;
int screen_height = 600;

static LPDIRECTDRAW        dd;            /* Main DirectDraw object for the screen device */
static LPDIRECTDRAWSURFACE front_buffer;  /* Screen Direct Draw object */
static LPDIRECTDRAWSURFACE back_buffer;   /* Back buffer Direct Draw object */
static LPDIRECTDRAWCLIPPER clipper;       /* Clipper object; used to clip to window */
static LPDIRECTDRAWPALETTE ddpalette;

/************************************************************************/

/*
 * DirectDrawInitialize:  Start up DirectDraw.  Depending on configuration options,
 *   the program might run in a window, or full screen, or in emulation mode.
 *   Return TRUE if initialization is successful.
 */
BOOL DirectDrawInitialize(void)
{
   HRESULT ddrval;

   ddrval = DirectDrawCreate(NULL,&dd,NULL);
   
   if (ddrval != DD_OK)
   {
      printf("DirectDrawCreate failed!\n");
      return FALSE;
   }

   if (is_window)
      ddrval = IDirectDraw2_SetCooperativeLevel(dd,hMain,DDSCL_NORMAL);
   else
      ddrval = IDirectDraw2_SetCooperativeLevel(dd,hMain,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);

   if( ddrval != DD_OK )
   {
      printf("SetCooperativeLevel failed!\n");
      return FALSE;
   }

   /* Create clipper, for use when not full screen */
   ddrval = IDirectDraw2_CreateClipper(dd,0, &clipper, NULL);
   if (ddrval != DD_OK)
   {
      printf("CreateClipper failed!\n");
      return FALSE;
   }

   if (win32_debug >= 2)
   {
      DDCAPS driver_caps,emulate_caps;
      
      dprintf("checking capabilities\n");
      driver_caps.dwSize = sizeof(DDCAPS);
      emulate_caps.dwSize = sizeof(DDCAPS);
      ddrval = IDirectDraw2_GetCaps(dd,&driver_caps,&emulate_caps);
      if (ddrval != DD_OK)
      {
	 dprintf("INVALID GETCAPS %i--%i %i\n",ddrval,DDERR_INVALIDOBJECT,
		 DDERR_INVALIDPARAMS);
      }
      else
      {
	 if (driver_caps.dwCaps & DDCAPS_BLTSTRETCH)
	    dprintf("can stretch in hardware\n");
	 if (emulate_caps.dwCaps & DDCAPS_BLTSTRETCH)
	    dprintf("can stretch in software\n");
	 if (driver_caps.dwCaps & DDCAPS_CANBLTSYSMEM)
	    dprintf("can blt from sys mem\n");
	 if (driver_caps.dwCaps & DDCAPS_CANBLTSYSMEM) 
	    dprintf("can emulate blt from sys mem\n");
	 dprintf("%08x\n",driver_caps.dwFXCaps);
	 dprintf("%08x\n",emulate_caps.dwFXCaps);
      }
   }

   return TRUE;
}
/************************************************************************/
/*
 * DirectDrawClose:  Shut down Direct Draw.
 */
void DirectDrawClose(void)
{
   if (dd != NULL)
      IDirectDraw2_Release(dd);
}

typedef struct screen_size_struct
{
   int width;
   int height;
   struct screen_size_struct *next;
} screen_size_type;

screen_size_type *screen_sizes = NULL;

HRESULT WINAPI DumpMode(LPDDSURFACEDESC dssd,LPVOID joe)
{
   screen_size_type *sst;

   sst = (screen_size_type *)malloc(sizeof(screen_size_type));
   sst->width = dssd->dwWidth;
   sst->height = dssd->dwHeight;
   sst->next = screen_sizes;

   screen_sizes = sst;
}

void DirectDrawSetClosestMode(int width,int height)
{
   HRESULT ddrval;
   int best_width;
   int best_height;
   int best_score;
   screen_size_type *sst;

   ddrval = IDirectDraw2_EnumDisplayModes(dd,0,NULL,NULL,DumpMode);
   if (ddrval != DD_OK)
   {
      dprintf("error enuming display modes\n");
      return;
   }

   best_score = 999999;
   sst = screen_sizes;
   while (sst != NULL)
   {
      /* dprintf("%ix%i %ix%i\n",width,height,sst->width,sst->height); */
      if (sst->width >= width & sst->height >= height)
	 if ((sst->width-width + sst->height-height) < best_score)
	 {
	    best_width = sst->width;
	    best_height = sst->height;
	    best_score = sst->width-width + sst->height-height;
	 }
      sst = sst->next;
   }
   if (best_score < 999999)
      DirectDrawSetMode(best_width,best_height,8);

}

/*
 * DirectDrawSetMode:  Set the video mode to width by height, with
 *   depth bits per pixel.
 *   Returns TRUE on success.
 */
BOOL DirectDrawSetMode(int32 width, int32 height, int32 depth)
{
   HRESULT     ddrval;
   
   ig_assert(dd != NULL);

   if (win32_debug >= 2)
      dprintf("setting mode %i,%i\n",width,height);

   ddrval = IDirectDraw_SetDisplayMode(dd,width, height, depth);
   if (ddrval != DD_OK)
   {
      debug(("SetDisplayMode failed, error = %i\n", ddrval));
      return FALSE;
   }

   screen_width = width;
   screen_height = height;

   return TRUE;
}

/*
 * DirectDrawCreateSurfaces:  Create main screen buffer, with a back buffer
 *   for flipping.  Its size is the same as the screen's (if in full screen mode).
 *   Sets buffer to the back buffer.
 *   Returns TRUE on success.
 */
BOOL DirectDrawCreateSurfaces(LPDIRECTDRAWSURFACE *buffer)
{
   DDSURFACEDESC   ddsd;
   DDSCAPS         ddscaps;
   HRESULT         ddrval;
   
   ig_assert(dd != NULL);
   
   /* Create primary surface */
   memset(&ddsd, 0, sizeof(ddsd));
   ddsd.dwSize = sizeof(ddsd);
   
   /* In full screen mode, make a pair of flipping surfaces.
      In windowed mode, just allocate a second surface for the back buffer. */
   
   if (flip)
   {
      ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
      ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
      ddsd.dwBackBufferCount = 1;
   }
   else
   {
      ddsd.dwFlags = DDSD_CAPS;
      ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
   }
   ddrval = IDirectDraw2_CreateSurface(dd,&ddsd, &front_buffer, NULL);
   if (ddrval != DD_OK)
   {
      if (ddrval == DDERR_NOEXCLUSIVEMODE) /* if screen wasn't in 8 bit depth, might happen */
      {
	 dprintf("ERROR with getting back buffer--trying again without flip.\n");
	 flip = FALSE;
	 return DirectDrawCreateSurfaces(buffer);
      }

      debug(("CreateSurface failed to create front buffer, error = %i\n", ddrval));
      dprintf("of %i %i %i %i %i %i %i\n%i %i %i %i %i %i\n%i %i %i %i\n",
	      DDERR_INCOMPATIBLEPRIMARY ,
	      DDERR_INVALIDCAPS ,
	      DDERR_INVALIDOBJECT ,
	      DDERR_INVALIDPARAMS ,
	      DDERR_INVALIDPIXELFORMAT ,
	      DDERR_NOALPHAHW ,
	      DDERR_NOCOOPERATIVELEVELSET ,
	      DDERR_NODIRECTDRAWHW ,
	      DDERR_NOEMULATION ,
	      DDERR_NOEXCLUSIVEMODE ,
	      DDERR_NOFLIPHW ,
	      DDERR_NOMIPMAPHW ,
	      DDERR_NOZBUFFERHW ,
	      DDERR_OUTOFMEMORY ,
	      DDERR_OUTOFVIDEOMEMORY ,
	      DDERR_PRIMARYSURFACEALREADYEXISTS ,
	      DDERR_UNSUPPORTEDMODE);
      return FALSE;
   }

   if (flip)
   {
      /* get a pointer to the back buffer */
      ddscaps.dwCaps = DDSCAPS_BACKBUFFER;

      ddrval = IDirectDrawSurface2_GetAttachedSurface(front_buffer,&ddscaps,&back_buffer);
      if (ddrval != DD_OK)
      {
	 debug(("CreateSurface failed to get back buffer, error = %i\n", ddrval));
	 return FALSE;
      }
   }
   else
   {
      /* Create a separate back buffer to blit with */
      
      ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
      if (rotate)
      {
	 ddsd.dwWidth  = bitmap->height; 
	 ddsd.dwHeight = bitmap->width;
      }
      else
      {
	 ddsd.dwWidth  = bitmap->width; 
	 ddsd.dwHeight = bitmap->height;
      }
      
      
      ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
      if (use_video_memory)
	 ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
      else
	 ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
      ddrval = IDirectDraw2_CreateSurface(dd,&ddsd, &back_buffer, NULL);
      if (ddrval != DD_OK)
      {
	 debug(("CreateSurface failed to create back buffer, error = %i\n", ddrval));
	 return FALSE;
      }
   }

   *buffer = back_buffer;
   
   return TRUE;
}
/************************************************************************/

BOOL DirectDrawClipToScreen()
{
   HRESULT     ddrval;
   struct 
   {
      RGNDATAHEADER rdh;
      RECT cliprect;
   } rgndata;

   ig_assert(dd != NULL);
   ig_assert(clipper != NULL);
   ig_assert(front_buffer != NULL);

   /*
   ddrval = IDirectDrawClipper_SetHWnd(clipper,0, hwnd);
   */
   rgndata.rdh.dwSize = sizeof(RGNDATAHEADER);
   rgndata.rdh.iType = RDH_RECTANGLES;
   rgndata.rdh.nCount = 1;
   rgndata.rdh.nRgnSize = sizeof(RECT);
   rgndata.rdh.rcBound.left = 0;
   rgndata.rdh.rcBound.top = 0;
   rgndata.rdh.rcBound.right = screen_width;
   rgndata.rdh.rcBound.bottom = screen_height;
   
   rgndata.cliprect.left = 0;
   rgndata.cliprect.top = 0;
   rgndata.cliprect.right = screen_width;
   rgndata.cliprect.bottom = screen_height;

   ddrval = IDirectDrawClipper_SetClipList(clipper,(LPRGNDATA)&rgndata,0);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawClipToScreen couldn't set clip list\n"));
      return FALSE;
   }

   ddrval = IDirectDrawSurface2_SetClipper(front_buffer,clipper);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawClipToScreen couldn't set clipper\n"));
      return FALSE;
   }

   ddrval = IDirectDrawSurface2_SetClipper(back_buffer,clipper);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawClipToScreen couldn't 2 set clipper\n"));
      return FALSE;
   }

   return TRUE;
}

BOOL DirectDrawClipToWindow()
{
   HRESULT     ddrval;
   
   ig_assert(dd != NULL);
   ig_assert(clipper != NULL);
   ig_assert(front_buffer != NULL);

   ddrval = IDirectDrawClipper_SetHWnd(clipper,0,hMain);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawClipToWindow couldn't set window\n"));
      return FALSE;
   }

   ddrval = IDirectDrawSurface2_SetClipper(front_buffer,clipper);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawClipToWindow couldn't set clipper\n"));
      return FALSE;
   }

   
   return TRUE;
}

void DirectDrawUpdateScreen()
{
    HRESULT ddrval;
    RECT rect;
    POINT p;
    int diff;

    ig_assert(dd != NULL);
    ig_assert(front_buffer != NULL);

    while (1)
    {
       if (flip)
	  ddrval = IDirectDrawSurface2_Flip(front_buffer,NULL,0);
       else
       {
	  p.x = p.y = 0;
	  if (is_window)
	     ClientToScreen(hMain, &p); 
	  
	  rect.left = p.x;
	  rect.top = p.y;
	  if (rotate)
	  {
	     rect.right = rect.left + bitmap->height*scale;
	     rect.bottom = rect.top + bitmap->width*scale;
	  }
	  else
	  {
	     rect.right = rect.left + bitmap->width*scale;
	     rect.bottom = rect.top + bitmap->height*scale;
	  }
	  if (!is_window)
	  {
	     if (screen_width > (rect.right - rect.left))
	     {
		diff = (screen_width - (rect.right-rect.left))/2 / 4 * 4;
		rect.left += diff;
		rect.right += diff;
	     }
	     
	     if (screen_height > (rect.bottom - rect.top))
	     {
		diff = (screen_height - (rect.bottom-rect.top))/2 / 4 * 4;
		rect.top += diff;
		rect.bottom += diff;
	     }
	  }

	  ddrval = IDirectDrawSurface2_Blt(front_buffer,&rect,back_buffer,NULL,0,NULL);
       }
       
       if (ddrval == DD_OK)
	  break;

       if (ddrval == DDERR_SURFACELOST)
	  if (!DirectDrawRestoreSurfaces())
	     return;

       if (ddrval != DDERR_WASSTILLDRAWING)
       {
	  debug(("DirectDrawUpdateScreen got unexpected error %i\n", ddrval));
	  if (ddrval == DDERR_SURFACEBUSY)
	     dprintf("surface busy\n");

	  dprintf("%i %i %i %i %i %i %i %i %i x%i\n%i %i %i %i %i %i %i %i\n",
		  DDERR_GENERIC,DDERR_INVALIDCLIPLIST,DDERR_INVALIDOBJECT ,
		  DDERR_INVALIDPARAMS ,DDERR_INVALIDRECT,DDERR_NOALPHAHW, 
		  DDERR_NOBLTHW ,DDERR_NOCLIPLIST ,DDERR_NODDROPSHW ,
		  DDERR_NOMIRRORHW ,DDERR_NORASTEROPHW ,DDERR_NOROTATIONHW ,
		  DDERR_NOSTRETCHHW ,DDERR_NOZBUFFERHW ,DDERR_SURFACEBUSY ,
		  DDERR_SURFACELOST ,DDERR_UNSUPPORTED);
	  break;
       }
    }
}

void DirectDrawClearScreen()
{
   DirectDrawClearBuffer(front_buffer,0);
}

/*
 * DirectDrawClearBuffer:  Clear given Direct Draw buffer with given RGB color.
 */
void DirectDrawClearBuffer(LPDIRECTDRAWSURFACE ddbuffer, DWORD fill_color)
{
   DDBLTFX     ddbltfx;
   HRESULT     ddrval;
   
   ig_assert(dd != NULL);
   ig_assert(ddbuffer != NULL);
    
   ddbltfx.dwSize = sizeof(ddbltfx);
   ddbltfx.dwFillColor = fill_color;
   
   while (1)
   {
      ddrval = IDirectDrawSurface2_Blt(ddbuffer,NULL,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);
      
      if (ddrval == DD_OK )
	 break;
      
      if (ddrval == DDERR_SURFACELOST )
	 DirectDrawRestoreSurfaces();
      else 
	 if (ddrval != DDERR_WASSTILLDRAWING)
	 {
	    debug(("DirectDrawClearBuffer got unexpected error %i\n", ddrval));
	    return;
	 }
   }
}

BOOL DirectDrawLock(LPDIRECTDRAWSURFACE buffer, Surface *s)
{
   DDSURFACEDESC ddsd;
   HRESULT ddrval;
   POINT p;

   ig_assert(dd != NULL);
   ig_assert(buffer != NULL);

   if (flip)
   {
      DirectDrawClearBuffer(buffer,0);
   }

   ddsd.dwSize = sizeof(ddsd);
   while (1)
   {
      ddrval = IDirectDrawSurface2_Lock(buffer,NULL, &ddsd, DDLOCK_WAIT, NULL);
      if (ddrval == DD_OK)
	 break;

      if (ddrval == DDERR_SURFACELOST)
      {
	 ig_assert(front_buffer != NULL);
	 
	 ddrval = IDirectDrawSurface2_Restore(front_buffer);
	 if (ddrval != DD_OK)
	 {
	    debug(("DirectDrawRestoreSurfaces failed to restore surfaces\n"));
	 }
	 continue;
      }

      dprintf("Error locking %iXX %i %i %i %i %i %i\n",ddrval,
	      DDERR_INVALIDOBJECT,DDERR_INVALIDPARAMS,DDERR_OUTOFMEMORY,
	      DDERR_SURFACEBUSY,DDERR_SURFACELOST,DDERR_WASSTILLDRAWING);
      return FALSE;
   }

   s->height = ddsd.dwHeight;
   s->width  = ddsd.dwWidth;
   s->bits   = (Pixel *) ddsd.lpSurface;
   s->pitch  = ddsd.lPitch;

   if (flip)
   {
      int width,height;

      if (rotate)
      {
	 width = bitmap->height;
	 height = bitmap->width;
      }
      else
      {
	 width = bitmap->width;
	 height = bitmap->height;
      }
      if (s->width > width)
      {
	 s->bits += ((s->width-width)/2) / 4 * 4; /* round down to nearest dword */
      }
      if (s->height > height)
      {
	 s->bits += ((s->height-height)/2) * s->pitch;
      }
   }
   return TRUE;
}

/*
 * DirectDrawUnlock:  Unlock the given Direct Draw surface, where s->bits
 *   points to the memory of the surface that was locked.
 */
BOOL DirectDrawUnlock(LPDIRECTDRAWSURFACE buffer,Surface *s)
{
   HRESULT ddrval;

   ig_assert(buffer != NULL);
   
   ddrval = IDirectDrawSurface2_Unlock(buffer,s->bits);
   if (ddrval != DD_OK)
      return FALSE;

   return TRUE;
}

void DirectDrawSetColor(int index,LPPALETTEENTRY ppe)
{
   HRESULT     ddrval;

   ig_assert(ddpalette != NULL);

   ddrval = IDirectDrawPalette_SetEntries(ddpalette,0,index,1,ppe);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawSetColor couldn't set color, error = %i\n", ddrval));
      return;
   }
}

/*
 * DirectDrawSetPalette:  Set palette of the display.
 *   Returns TRUE on success.
 */
BOOL DirectDrawSetPalette(CKPALETTE *palette)
{
   HRESULT     ddrval;

   ig_assert(dd != NULL);

   ddrval = IDirectDraw2_CreatePalette(dd,DDPCAPS_8BIT | DDPCAPS_ALLOW256,
				       (struct tagPALETTEENTRY *)palette,&ddpalette, NULL);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawSetPalette couldn't create palette, error = %i\n", ddrval));
      return FALSE;
   }

   while (1)
   {
      ddrval = IDirectDrawSurface2_SetPalette(front_buffer,ddpalette);
      if (ddrval == DD_OK)
	 break;

      if (ddrval == DDERR_SURFACELOST)
      {
	 if (!DirectDrawRestoreSurfaces())
	 {
	    dprintf("Direct draw error setting palette/restoring surface\n");
	    return FALSE;
	 }
	 continue;
      }
      
      debug(("DirectDrawSetPalette couldn't set palette, error = %i\n", ddrval));
      dprintf("%i %i %i %i %i\n%i %i %i %i %i\n",
	      DDERR_GENERIC , DDERR_INVALIDOBJECT , DDERR_INVALIDPARAMS ,
	      DDERR_INVALIDSURFACETYPE , DDERR_NOEXCLUSIVEMODE ,
	      DDERR_NOPALETTEATTACHED ,DDERR_NOPALETTEHW ,DDERR_NOT8BITCOLOR ,
	      DDERR_SURFACELOST , DDERR_UNSUPPORTED);
      return FALSE;
   }
   
   return TRUE;
}

/*
 * DirectDrawRestoreSurfaces:  Restore surfaces when they've been lost
 *   (e.g. when the program is minimized and reactivated).
 *   Returns TRUE on success.
 */
BOOL DirectDrawRestoreSurfaces(void)
{
   HRESULT     ddrval;

   ig_assert(front_buffer != NULL);
   
   ddrval = IDirectDrawSurface2_Restore(front_buffer);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawRestoreSurfaces failed to restore surfaces\n"));
      return FALSE;
   }

   return TRUE;
}

void DirectDrawGetDC(LPDIRECTDRAWSURFACE ddbuffer)
{
   HRESULT     ddrval;

   ig_assert(ddbuffer != NULL);
   
   ddrval = IDirectDrawSurface2_GetDC(ddbuffer,&hdc_vector);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawGetDC  failed to get DC\n"));
   }
}

void DirectDrawReleaseDC(LPDIRECTDRAWSURFACE ddbuffer)
{
   HRESULT     ddrval;

   ig_assert(ddbuffer != NULL);
   
   ddrval = IDirectDrawSurface2_ReleaseDC(ddbuffer,hdc_vector);
   if (ddrval != DD_OK)
   {
      debug(("DirectDrawReleaseDC  failed to release DC\n"));
   }
   hdc_vector = NULL;
}


