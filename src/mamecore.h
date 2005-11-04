/***************************************************************************

    mamecore.h

    General core utilities and macros used throughout MAME.

***************************************************************************/

#pragma once

#ifndef __MAMECORE_H__
#define __MAMECORE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "osd_cpu.h"



/***************************************************************************

    Common types

***************************************************************************/

/* FPTR is a type that can be used to cast a pointer to a scalar */
/* 64-bit platforms should define __LP64__ */
#ifdef __LP64__
typedef UINT64 FPTR;
#else
typedef UINT32 FPTR;
#endif


/* ----- for generic function pointers ----- */
typedef void genf(void);



/* These are forward struct declarations that are used to break
   circular dependencies in the code */

typedef struct _mame_display mame_display;
typedef struct _game_driver game_driver;
typedef struct _machine_config machine_config;
typedef struct _rom_load_data rom_load_data;
typedef struct _xml_data_node xml_data_node;
typedef struct _performance_info performance_info;


typedef struct _osd_file osd_file;


/* pen_t is used to represent pixel values in mame_bitmaps */
typedef UINT32 pen_t;


/* mame_bitmaps are used throughout the code */
struct _mame_bitmap
{
	int width,height;	/* width and height of the bitmap */
	int depth;			/* bits per pixel */
	void **line;		/* pointers to the start of each line - can be UINT8 **, UINT16 ** or UINT32 ** */

	/* alternate way of accessing the pixels */
	void *base;			/* pointer to pixel (0,0) (adjusted for padding) */
	int rowpixels;		/* pixels per row (including padding) */
	int rowbytes;		/* bytes per row (including padding) */

	/* functions to render in the correct orientation */
	void (*plot)(struct _mame_bitmap *bitmap,int x,int y,pen_t pen);
	pen_t (*read)(struct _mame_bitmap *bitmap,int x,int y);
	void (*plot_box)(struct _mame_bitmap *bitmap,int x,int y,int width,int height,pen_t pen);
};
typedef struct _mame_bitmap mame_bitmap;


/* rectangles are used throughout the code */
struct _rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};
typedef struct _rectangle rectangle;



/***************************************************************************
 * Union of UINT8, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
***************************************************************************/
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3; } b;
	struct { UINT16 l,h; } w;
#else
	struct { UINT8 h3,h2,h,l; } b;
	struct { UINT16 h,l; } w;
#endif
	UINT32 d;
} PAIR;




/***************************************************************************
 * Union of UINT8, UINT16, UINT32, and UINT64 in native endianess of
 * the target.  This is used to access bytes and words in a machine
 * independent manner.
***************************************************************************/
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3,h4,h5,h6,h7; } b;
	struct { UINT16 l,h,h2,h3; } w;
	struct { UINT32 l,h; } d;
#else
	struct { UINT8 h7,h6,h5,h4,h3,h2,h,l; } b;
	struct { UINT16 h3,h2,h,l; } w;
	struct { UINT32 h,l; } d;
#endif
	UINT64 lw;
} PAIR64;



/***************************************************************************

    Common constants

***************************************************************************/

/* Ensure that TRUE/FALSE are defined */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif



/***************************************************************************

    Common macros

***************************************************************************/

/* Standard MIN/MAX macros */
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

/* Highly useful macro for compile-time knowledge of an array size */
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))


/* U64 and S64 are used to wrap long integer constants. */
#ifdef __GNUC__
#define U64(val) val##ULL
#define S64(val) val##LL
#else
#define U64(val) val
#define S64(val) val
#endif


/* Macros for normalizing data into big or little endian formats */
#define FLIPENDIAN_INT16(x)	(((((UINT16) (x)) >> 8) | ((x) << 8)) & 0xffff)
#define FLIPENDIAN_INT32(x)	((((UINT32) (x)) << 24) | (((UINT32) (x)) >> 24) | \
	(( ((UINT32) (x)) & 0x0000ff00) << 8) | (( ((UINT32) (x)) & 0x00ff0000) >> 8))
#define FLIPENDIAN_INT64(x)	\
	(												\
		(((((UINT64) (x)) >> 56) & ((UINT64) 0xFF)) <<  0)	|	\
		(((((UINT64) (x)) >> 48) & ((UINT64) 0xFF)) <<  8)	|	\
		(((((UINT64) (x)) >> 40) & ((UINT64) 0xFF)) << 16)	|	\
		(((((UINT64) (x)) >> 32) & ((UINT64) 0xFF)) << 24)	|	\
		(((((UINT64) (x)) >> 24) & ((UINT64) 0xFF)) << 32)	|	\
		(((((UINT64) (x)) >> 16) & ((UINT64) 0xFF)) << 40)	|	\
		(((((UINT64) (x)) >>  8) & ((UINT64) 0xFF)) << 48)	|	\
		(((((UINT64) (x)) >>  0) & ((UINT64) 0xFF)) << 56)		\
	)

#ifdef LSB_FIRST
#define BIG_ENDIANIZE_INT16(x)		(FLIPENDIAN_INT16(x))
#define BIG_ENDIANIZE_INT32(x)		(FLIPENDIAN_INT32(x))
#define BIG_ENDIANIZE_INT64(x)		(FLIPENDIAN_INT64(x))
#define LITTLE_ENDIANIZE_INT16(x)	(x)
#define LITTLE_ENDIANIZE_INT32(x)	(x)
#define LITTLE_ENDIANIZE_INT64(x)	(x)
#else
#define BIG_ENDIANIZE_INT16(x)		(x)
#define BIG_ENDIANIZE_INT32(x)		(x)
#define BIG_ENDIANIZE_INT64(x)		(x)
#define LITTLE_ENDIANIZE_INT16(x)	(FLIPENDIAN_INT16(x))
#define LITTLE_ENDIANIZE_INT32(x)	(FLIPENDIAN_INT32(x))
#define LITTLE_ENDIANIZE_INT64(x)	(FLIPENDIAN_INT64(x))
#endif /* LSB_FIRST */


/* Useful macros to deal with bit shuffling encryptions */
#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP32(val,B31,B30,B29,B28,B27,B26,B25,B24,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B31) << 31) | \
		 (BIT(val,B30) << 30) | \
		 (BIT(val,B29) << 29) | \
		 (BIT(val,B28) << 28) | \
		 (BIT(val,B27) << 27) | \
		 (BIT(val,B26) << 26) | \
		 (BIT(val,B25) << 25) | \
		 (BIT(val,B24) << 24) | \
		 (BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))



/***************************************************************************

    Common functions

***************************************************************************/

/* since stricmp is not part of the standard, we use this instead */
INLINE int my_stricmp(const char *dst, const char *src)
{
	while (*src && *dst)
	{
		if (tolower(*src) != tolower(*dst))
			return *dst - *src;
		src++;
		dst++;
	}
	return *dst - *src;
}

/* compute the intersection of two rectangles */
INLINE void sect_rect(rectangle *dst, const rectangle *src)
{
	if (src->min_x > dst->min_x) dst->min_x = src->min_x;
	if (src->max_x < dst->max_x) dst->max_x = src->max_x;
	if (src->min_y > dst->min_y) dst->min_y = src->min_y;
	if (src->max_y < dst->max_y) dst->max_y = src->max_y;
}

/* convert a series of 32 bits into a float */
INLINE float u2f(UINT32 v)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.vv = v;
	return u.ff;
}

/* convert a float into a series of 32 bits */
INLINE UINT32 f2u(float f)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.ff = f;
	return u.vv;
}

/* convert a series of 64 bits into a double */
INLINE float u2d(UINT64 v)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.vv = v;
	return u.dd;
}

/* convert a double into a series of 64 bits */
INLINE UINT64 d2u(double d)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.dd = d;
	return u.vv;
}



/***************************************************************************

    Compiler-specific nastiness

***************************************************************************/

/* Suppress warnings about redefining the macro 'PPC' on LinuxPPC. */
#ifdef PPC
#undef PPC
#endif



/* Some optimizations/warnings cleanups for GCC */
#if defined(__GNUC__) && (__GNUC__ >= 3)
#define ATTR_UNUSED			__attribute__((__unused__))
#define ATTR_NORETURN		__attribute__((noreturn))
#define ATTR_PRINTF(x,y)	__attribute__((format(printf, x, y)))
#define ATTR_MALLOC			__attribute__((malloc))
#define ATTR_PURE			__attribute__((pure))
#define ATTR_CONST			__attribute__((const))
#define UNEXPECTED(exp)		__builtin_expect((exp), 0)
#else
#define ATTR_UNUSED
#define ATTR_NORETURN
#define ATTR_PRINTF(x,y)
#define ATTR_MALLOC
#define ATTR_PURE
#define ATTR_CONST
#define UNEXPECTED(exp)		(exp)
#endif



/* And some MSVC optimizations/warnings */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#define DECL_NORETURN		__declspec(noreturn)
#else
#define DECL_NORETURN
#endif




#endif	/* __MAMECORE_H__ */
