#ifndef _MAMEW32_H
#define _MAMEW32_H

#include <windows.h>
#include <ddraw.h>
#include <dsound.h>

#include "driver.h"
#undef stricmp

typedef UCHAR uint8;
typedef DWORD int32;

typedef uint8 Pixel;

#define debug(a) (dprintf a)
#define ig_assert(x) \
  if (!(x)) \
    debug(("Assertion failed in %s, line %d\n", __FILE__, __LINE__));

void dprintf(char *fmt,...);

#include "directdraw.h"

typedef unsigned char byte;

extern HWND hMain;
extern int is_window;
extern int use_video_memory;
extern int flip;
extern int scale;
extern int rotate;
extern struct osd_bitmap *bitmap;
extern HDC hdc_vector;
extern int win32_debug;

#endif
