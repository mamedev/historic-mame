/*
 * directdraw.h:  Main header file for directdraw.c
 */

#ifndef _DIRECTDRAW_H
#define _DIRECTDRAW_H

typedef struct 
{
   int32   width, height;
   int32   pitch;       // Distance between addresses of two vertically adjacent pixels
                        // (may not equal width in some cases).
   Pixel  *bits;        // Raw bits of surface
} Surface;
   

typedef struct  
{
    PALETTEENTRY entries[256]; 
} CKPALETTE; 

BOOL DirectDrawInitialize(void);
void DirectDrawClose(void);
void DirectDrawSetClosestMode(int width,int height);
BOOL DirectDrawSetMode(int32 width, int32 height, int32 depth);
BOOL DirectDrawCreateSurfaces(LPDIRECTDRAWSURFACE *buffer);
BOOL DirectDrawClipToScreen();
BOOL DirectDrawClipToWindow();
void DirectDrawUpdateScreen();
BOOL DirectDrawLock(LPDIRECTDRAWSURFACE buffer, Surface *s);
BOOL DirectDrawUnlock(LPDIRECTDRAWSURFACE buffer, Surface *s);
void DirectDrawSetColor(int index,LPPALETTEENTRY ppe);
BOOL DirectDrawSetPalette(CKPALETTE *palette);
void DirectDrawClearScreen();
void DirectDrawClearBuffer(LPDIRECTDRAWSURFACE ddbuffer, DWORD fill_color);

void DirectDrawGetDC(LPDIRECTDRAWSURFACE ddbuffer);
void DirectDrawReleaseDC(LPDIRECTDRAWSURFACE ddbuffer);

#endif _DIRECTDRAW_H

