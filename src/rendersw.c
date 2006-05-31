/***************************************************************************

    raster.c

    Software-only rasterization system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    This file is not to be directly compiled. Rather, the OSD code should
    #define the macros below and then #include this file to generate
    rasterizers that are optimized for a given output format. See
    windows/rendsoft.c for an example.

***************************************************************************/



/***************************************************************************
    USAGE VERIFICATION
***************************************************************************/

#if !defined(FUNC_PREFIX)
#error Must define FUNC_PREFIX!
#endif

#if !defined(PIXEL_TYPE)
#error Must define PIXEL_TYPE!
#endif

#if !defined(SRCSHIFT_R) || !defined(SRCSHIFT_G) || !defined(SRCSHIFT_B)
#error Must define SRCSHIFT_R/SRCSHIFT_G/SRCSHIFT_B!
#endif

#if !defined(DSTSHIFT_R) || !defined(DSTSHIFT_G) || !defined(DSTSHIFT_B)
#error Must define DSTSHIFT_R/DSTSHIFT_G/DSTSHIFT_B!
#endif



/***************************************************************************
    ONE-TIME-ONLY DEFINITIONS
***************************************************************************/

#ifndef FIRST_TIME
#define FIRST_TIME

#include "mamecore.h"
#include "osinline.h"
#include "render.h"
#include <math.h>


/***************************************************************************
    MACROS
***************************************************************************/

#define FSWAP(var1, var2) do { float temp = var1; var1 = var2; var2 = temp; } while (0)

#define Tinten(intensity, col) \
	MAKE_RGB((RGB_RED(col) * (intensity)) >> 8, (RGB_GREEN(col) * (intensity)) >> 8, (RGB_BLUE(col) * (intensity)) >> 8)




/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _quad_setup_data quad_setup_data;
struct _quad_setup_data
{
	INT32			dudx, dvdx, dudy, dvdy;
	INT32			startu, startv;
	INT32			startx, starty;
	INT32			endx, endy;
};



/***************************************************************************
    GLOBALS
***************************************************************************/

static UINT32 cosine_table[2049];



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE float round_nearest(float f)
{
	if (f >= 0)
		return floor(f + 0.5f);
	else
		return floor(f - 0.5f);
}


#ifndef vec_mult
INLINE int vec_mult(int parm1, int parm2)
{
	int temp,result;

	temp     = abs(parm1);
	result   = (temp&0x0000ffff) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp&0x0000ffff) * (parm2>>16       );
	result  += (temp>>16       ) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp>>16       ) * (parm2>>16       );

	if( parm1 < 0 )
		return(-result);
	else
		return( result);
}
#endif

/* can be be replaced by an assembly routine in osinline.h */
#ifndef vec_div
INLINE int vec_div(int parm1, int parm2)
{
	if( (parm2>>12) )
	{
		parm1 = (parm1<<4) / (parm2>>12);
		if( parm1 > 0x00010000 )
			return( 0x00010000 );
		if( parm1 < -0x00010000 )
			return( -0x00010000 );
		return( parm1 );
	}
	return( 0x00010000 );
}
#endif


#endif



/***************************************************************************
    MACROS
***************************************************************************/

/* source 15-bit pixels are in MAME standardized format */
#define SOURCE15_R(pix)		(((pix) >> (7 + SRCSHIFT_R)) & (0xf8 >> SRCSHIFT_R))
#define SOURCE15_G(pix)		(((pix) >> (2 + SRCSHIFT_G)) & (0xf8 >> SRCSHIFT_G))
#define SOURCE15_B(pix)		(((pix) << (3 - SRCSHIFT_B)) & (0xf8 >> SRCSHIFT_B))

/* source 32-bit pixels are in MAME standardized format */
#define SOURCE32_R(pix)		(((pix) >> (16 + SRCSHIFT_R)) & (0xff >> SRCSHIFT_R))
#define SOURCE32_G(pix)		(((pix) >> (8 + SRCSHIFT_G)) & (0xff >> SRCSHIFT_G))
#define SOURCE32_B(pix)		(((pix) >> (0 + SRCSHIFT_B)) & (0xff >> SRCSHIFT_B))

/* destination pixels are written based on the values of the macros */
#define DEST_ASSEMBLE_RGB(r,g,b)	(((r) << DSTSHIFT_R) | ((g) << DSTSHIFT_G) | ((b) << DSTSHIFT_B))
#define DEST_RGB_TO_PIXEL(r,g,b)	DEST_ASSEMBLE_RGB((r) >> SRCSHIFT_R, (g) >> SRCSHIFT_G, (b) >> SRCSHIFT_B)

/* destination pixel masks are based on the macros as well */
#define DEST_R(pix)			(((pix) >> DSTSHIFT_R) & (0xff >> SRCSHIFT_R))
#define DEST_G(pix)			(((pix) >> DSTSHIFT_G) & (0xff >> SRCSHIFT_G))
#define DEST_B(pix)			(((pix) >> DSTSHIFT_B) & (0xff >> SRCSHIFT_B))

/* direct 15-bit source to destination pixel conversion */
#define SOURCE15_TO_DEST(pix)	DEST_ASSEMBLE_RGB(SOURCE15_R(pix), SOURCE15_G(pix), SOURCE15_B(pix))
#ifndef VARIABLE_SHIFT
#if (SRCSHIFT_R == 3) && (SRCSHIFT_G == 3) && (SRCSHIFT_B == 3) && (DSTSHIFT_R == 10) && (DSTSHIFT_G == 5) && (DSTSHIFT_B == 0)
#undef SOURCE15_TO_DEST
#define SOURCE15_TO_DEST(pix)	(pix)
#endif
#endif

/* direct 32-bit source to destination pixel conversion */
#define SOURCE32_TO_DEST(pix)	DEST_ASSEMBLE_RGB(SOURCE32_R(pix), SOURCE32_G(pix), SOURCE32_B(pix))
#ifndef VARIABLE_SHIFT
#if (SRCSHIFT_R == 0) && (SRCSHIFT_G == 0) && (SRCSHIFT_B == 0) && (DSTSHIFT_R == 16) && (DSTSHIFT_G == 8) && (DSTSHIFT_B == 0)
#undef SOURCE32_TO_DEST
#define SOURCE32_TO_DEST(pix)	(pix)
#endif
#endif




/***************************************************************************

    Software rasterizers

***************************************************************************/

/*-------------------------------------------------
    draw_line - draw a line or point
-------------------------------------------------*/

INLINE void FUNC_PREFIX(draw_aa_pixel)(void *dstdata, UINT32 pitch, int x, int y, const rectangle *clip, rgb_t col)
{
	UINT32 dpix, dr, dg, db;
	PIXEL_TYPE *dest;

	if (x < clip->min_x || x >= clip->max_x)
		return;
	if (y < clip->min_y || y >= clip->max_y)
		return;

	dest = (PIXEL_TYPE *)dstdata + y * pitch + x;
	dpix = *dest;
	dr = RGB_RED(col) + DEST_R(dpix);
	if (dr > 0xff) dr = 0xff;
	dg = RGB_GREEN(col) + DEST_G(dpix);
	if (dg > 0xff) dg = 0xff;
	db = RGB_BLUE(col) + DEST_B(dpix);
	if (db > 0xff) db = 0xff;
	*dest = DEST_ASSEMBLE_RGB(dr, dg, db);
}


static void FUNC_PREFIX(draw_line)(const render_primitive *prim, void *dstdata, UINT32 pitch, const render_bounds *fclip, int width, int height)
{
	int dx,dy,sx,sy,cx,cy,bwidth;
	UINT8 a1;
	int x1,x2,y1,y2;
	UINT32 col;
	int xx,yy;
	rectangle clip;
	int beam;

	/* compute the clipping bounds */
	clip.min_x = (int)fclip->x0;
	clip.max_x = (int)fclip->x1;
	clip.min_y = (int)fclip->y0;
	clip.max_y = (int)fclip->y1;

	/* compute the start/end coordinates */
	x1 = (int)(prim->bounds.x0 * 65536.0f);
	y1 = (int)(prim->bounds.y0 * 65536.0f);
	x2 = (int)(prim->bounds.x1 * 65536.0f);
	y2 = (int)(prim->bounds.y1 * 65536.0f);

	/* handle color and intensity */
	col = MAKE_RGB((int)(255.0f * prim->color.r * prim->color.a), (int)(255.0f * prim->color.g * prim->color.a), (int)(255.0f * prim->color.b * prim->color.a));

	if (PRIMFLAG_GET_ANTIALIAS(prim->flags))
	{
		/* build up the cosine table if we haven't yet */
		if (cosine_table[0] == 0)
		{
			int entry;
			for (entry = 0; entry <= 2048; entry++)
				cosine_table[entry] = (int)((double)(1.0 / cos(atan((double)(entry) / 2048.0))) * 0x10000000 + 0.5);
		}

		beam = prim->width * 65536.0f;
		if (beam < 0x00010000)
			beam = 0x00010000;

		/* draw an anti-aliased line */
		dx = abs(x1 - x2);
		dy = abs(y1 - y2);

		if (dx >= dy)
		{
			sx = ((x1 <= x2) ? 1 : -1);
			sy = vec_div(y2 - y1, dx);
			if (sy < 0)
				dy--;
			x1 >>= 16;
			xx = x2 >> 16;
			bwidth = vec_mult(beam << 4, cosine_table[abs(sy) >> 5]);
			y1 -= bwidth >> 1; /* start back half the diameter */
			for (;;)
			{
				dx = bwidth;    /* init diameter of beam */
				dy = y1 >> 16;
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, x1, dy++, &clip, Tinten(0xff & (~y1 >> 8), col));
				dx -= 0x10000 - (0xffff & y1); /* take off amount plotted */
				a1 = (dx >> 8) & 0xff;   /* calc remainder pixel */
				dx >>= 16;                   /* adjust to pixel (solid) count */
				while (dx--)                 /* plot rest of pixels */
					FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, x1, dy++, &clip, col);
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, x1, dy, &clip, Tinten(a1,col));
				if (x1 == xx) break;
				x1 += sx;
				y1 += sy;
			}
		}
		else
		{
			sy = ((y1 <= y2) ? 1: -1);
			sx = vec_div(x2 - x1, dy);
			if (sx < 0)
				dx--;
			y1 >>= 16;
			yy = y2 >> 16;
			bwidth = vec_mult(beam << 4,cosine_table[abs(sx) >> 5]);
			x1 -= bwidth >> 1; /* start back half the width */
			for (;;)
			{
				dy = bwidth;    /* calc diameter of beam */
				dx = x1 >> 16;
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, dx++, y1, &clip, Tinten(0xff & (~x1 >> 8), col));
				dy -= 0x10000 - (0xffff & x1); /* take off amount plotted */
				a1 = (dy >> 8) & 0xff;   /* remainder pixel */
				dy >>= 16;                   /* adjust to pixel (solid) count */
				while (dy--)                 /* plot rest of pixels */
					FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, dx++, y1, &clip, col);
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, dx, y1, &clip, Tinten(a1, col));
				if (y1 == yy) break;
				y1 += sy;
				x1 += sx;
			}
		}
	}
	else /* use good old Bresenham for non-antialiasing 980317 BW */
	{
		x1 = (x1 + 0x8000) >> 16;
		y1 = (y1 + 0x8000) >> 16;
		x2 = (x2 + 0x8000) >> 16;
		y2 = (y2 + 0x8000) >> 16;

		dx = abs(x1 - x2);
		dy = abs(y1 - y2);
		sx = (x1 <= x2) ? 1 : -1;
		sy = (y1 <= y2) ? 1 : -1;
		cx = dx / 2;
		cy = dy / 2;

		if (dx >= dy)
		{
			for (;;)
			{
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, x1, y1, &clip, col);
				if (x1 == x2) break;
				x1 += sx;
				cx -= dy;
				if (cx < 0)
				{
					y1 += sy;
					cx += dx;
				}
			}
		}
		else
		{
			for (;;)
			{
				FUNC_PREFIX(draw_aa_pixel)(dstdata, pitch, x1, y1, &clip, col);
				if (y1 == y2) break;
				y1 += sy;
				cy -= dx;
				if (cy < 0)
				{
					x1 += sx;
					cy += dy;
				}
			}
		}
	}
}


/*-------------------------------------------------
    draw_rect - draw a solid rectangle
-------------------------------------------------*/

static void FUNC_PREFIX(draw_rect)(const render_primitive *prim, void *dstdata, UINT32 pitch, const render_bounds *clip)
{
	render_bounds fpos = prim->bounds;
	INT32 startx, starty, endx, endy;
	INT32 x, y;

	assert(fpos.x0 <= fpos.x1);
	assert(fpos.y0 <= fpos.y1);

	/* apply X clipping */
	if (fpos.x0 < clip->x0)
		fpos.x0 = clip->x0;
	if (fpos.x1 > clip->x1)
		fpos.x1 = clip->x1;

	/* apply Y clipping */
	if (fpos.x0 < clip->y0)
		fpos.x0 = clip->y0;
	if (fpos.y1 > clip->y1)
		fpos.y1 = clip->y1;

	/* clamp to integers */
	startx = round_nearest(fpos.x0);
	starty = round_nearest(fpos.y0);
	endx = round_nearest(fpos.x1);
	endy = round_nearest(fpos.y1);

	/* bail if nothing left */
	if (fpos.x0 > fpos.x1 || fpos.y0 > fpos.y1)
		return;

	/* only support alpha and "none" blendmodes */
	assert(PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_NONE ||
	       PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_ALPHA);

	/* fast case: no alpha */
	if (PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_NONE || prim->color.a >= 1.0f)
	{
		UINT32 r = (UINT32)(256.0f * prim->color.r);
		UINT32 g = (UINT32)(256.0f * prim->color.g);
		UINT32 b = (UINT32)(256.0f * prim->color.b);
		UINT32 pix;

		/* clamp R,G,B to 0-256 range */
		if (r > 0xff) { if ((INT32)r < 0) r = 0; else r = 0xff; }
		if (g > 0xff) { if ((INT32)g < 0) g = 0; else g = 0xff; }
		if (b > 0xff) { if ((INT32)b < 0) b = 0; else b = 0xff; }
		pix = DEST_RGB_TO_PIXEL(r, g, b);

		/* loop over rows */
		for (y = starty; y < endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + startx;

			/* loop over cols */
			for (x = startx; x < endx; x++)
				*dest++ = pix;
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 rmask = DEST_RGB_TO_PIXEL(0xff,0x00,0x00);
		UINT32 gmask = DEST_RGB_TO_PIXEL(0x00,0xff,0x00);
		UINT32 bmask = DEST_RGB_TO_PIXEL(0x00,0x00,0xff);
		UINT32 r = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 g = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 b = (UINT32)(256.0f * prim->color.b * prim->color.a);
		UINT32 inva = (UINT32)(256.0f * (1.0f - prim->color.a));

		/* clamp R,G,B and inverse A to 0-256 range */
		if (r > 0xff) { if ((INT32)r < 0) r = 0; else r = 0xff; }
		if (g > 0xff) { if ((INT32)g < 0) g = 0; else g = 0xff; }
		if (b > 0xff) { if ((INT32)b < 0) b = 0; else b = 0xff; }
		if (inva > 0x100) { if ((INT32)inva < 0) inva = 0; else inva = 0x100; }

		/* pre-shift the RGBA pieces */
		r = DEST_RGB_TO_PIXEL(r, 0, 0) << 8;
		g = DEST_RGB_TO_PIXEL(0, g, 0) << 8;
		b = DEST_RGB_TO_PIXEL(0, 0, b) << 8;

		/* loop over rows */
		for (y = starty; y < endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + startx;

			/* loop over cols */
			for (x = startx; x < endx; x++)
			{
				UINT32 dpix = *dest;
				UINT32 dr = (r + ((dpix & rmask) * inva)) & (rmask << 8);
				UINT32 dg = (g + ((dpix & gmask) * inva)) & (gmask << 8);
				UINT32 db = (b + ((dpix & bmask) * inva)) & (bmask << 8);
				*dest++ = (dr | dg | db) >> 8;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_palette16_none - perform
    rasterization of a 16bpp palettized texture
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_palette16_none)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT16 *texbase = prim->texture.base;
	rgb_t *palbase = prim->texture.palette;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* ensure all parameters are valid */
	assert(palbase != NULL);

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = palbase[texbase[(curv >> 16) * texrp + (curu >> 16)]];
				*dest++ = SOURCE32_TO_DEST(pix);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* coloring-only case */
	else if (prim->color.a >= 1.0f)
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r);
		UINT32 sg = (UINT32)(256.0f * prim->color.g);
		UINT32 sb = (UINT32)(256.0f * prim->color.b);

		/* clamp R,G,B to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = palbase[texbase[(curv >> 16) * texrp + (curu >> 16)]];
				UINT32 r = (SOURCE32_R(pix) * sr) >> 8;
				UINT32 g = (SOURCE32_G(pix) * sg) >> 8;
				UINT32 b = (SOURCE32_B(pix) * sb) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 sg = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 sb = (UINT32)(256.0f * prim->color.b * prim->color.a);
		UINT32 invsa = (UINT32)(256.0f * (1.0f - prim->color.a));

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }
		if (invsa > 0x100) { if ((INT32)invsa < 0) invsa = 0; else invsa = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = palbase[texbase[(curv >> 16) * texrp + (curu >> 16)]];
				UINT32 dpix = *dest;
				UINT32 r = (SOURCE32_R(pix) * sr + DEST_R(dpix) * invsa) >> 8;
				UINT32 g = (SOURCE32_G(pix) * sg + DEST_G(dpix) * invsa) >> 8;
				UINT32 b = (SOURCE32_B(pix) * sb + DEST_B(dpix) * invsa) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_palette16_add - perform
    rasterization of a 16bpp palettized texture
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_palette16_add)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT16 *texbase = prim->texture.base;
	rgb_t *palbase = prim->texture.palette;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* ensure all parameters are valid */
	assert(palbase != NULL);

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = palbase[texbase[(curv >> 16) * texrp + (curu >> 16)]];
				UINT32 dpix = *dest;
				UINT32 r = SOURCE32_R(pix) + DEST_R(dpix);
				UINT32 g = SOURCE32_G(pix) + DEST_G(dpix);
				UINT32 b = SOURCE32_B(pix) + DEST_B(dpix);
				r = (r | -(r >> 8)) & 0xff;
				g = (g | -(g >> 8)) & 0xff;
				b = (b | -(b >> 8)) & 0xff;
				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 sg = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 sb = (UINT32)(256.0f * prim->color.b * prim->color.a);

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = palbase[texbase[(curv >> 16) * texrp + (curu >> 16)]];
				UINT32 dpix = *dest;
				UINT32 r = ((SOURCE32_R(pix) * sr) >> 8) + DEST_R(dpix);
				UINT32 g = ((SOURCE32_G(pix) * sg) >> 8) + DEST_G(dpix);
				UINT32 b = ((SOURCE32_B(pix) * sb) >> 8) + DEST_B(dpix);
				r = (r | -(r >> 8)) & 0xff;
				g = (g | -(g >> 8)) & 0xff;
				b = (b | -(b >> 8)) & 0xff;
				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_rgb32 - perform rasterization of
    a 32bpp RGB texture
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_rgb32)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT32 *texbase = prim->texture.base;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				*dest++ = SOURCE32_TO_DEST(pix);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* coloring-only case */
	else if (prim->color.a >= 1.0f)
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r);
		UINT32 sg = (UINT32)(256.0f * prim->color.g);
		UINT32 sb = (UINT32)(256.0f * prim->color.b);

		/* clamp R,G,B to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 r = (SOURCE32_R(pix) * sr) >> 8;
				UINT32 g = (SOURCE32_G(pix) * sg) >> 8;
				UINT32 b = (SOURCE32_B(pix) * sb) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 sg = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 sb = (UINT32)(256.0f * prim->color.b * prim->color.a);
		UINT32 invsa = (UINT32)(256.0f * (1.0f - prim->color.a));

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }
		if (invsa > 0x100) { if ((INT32)invsa < 0) invsa = 0; else invsa = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 dpix = *dest;
				UINT32 r = (SOURCE32_R(pix) * sr + DEST_R(dpix) * invsa) >> 8;
				UINT32 g = (SOURCE32_G(pix) * sg + DEST_G(dpix) * invsa) >> 8;
				UINT32 b = (SOURCE32_B(pix) * sb + DEST_B(dpix) * invsa) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_rgb15 - perform rasterization of
    a 15bpp RGB texture
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_rgb15)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT16 *texbase = prim->texture.base;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				*dest++ = SOURCE15_TO_DEST(pix);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* coloring-only case */
	else if (prim->color.a >= 1.0f)
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r);
		UINT32 sg = (UINT32)(256.0f * prim->color.g);
		UINT32 sb = (UINT32)(256.0f * prim->color.b);

		/* clamp R,G,B to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 r = (SOURCE15_R(pix) * sr) >> 8;
				UINT32 g = (SOURCE15_G(pix) * sg) >> 8;
				UINT32 b = (SOURCE15_B(pix) * sb) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 sg = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 sb = (UINT32)(256.0f * prim->color.b * prim->color.a);
		UINT32 invsa = (UINT32)(256.0f * (1.0f - prim->color.a));

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }
		if (invsa > 0x100) { if ((INT32)invsa < 0) invsa = 0; else invsa = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 dpix = *dest;
				UINT32 r = (SOURCE15_R(pix) * sr + DEST_R(dpix) * invsa) >> 8;
				UINT32 g = (SOURCE15_G(pix) * sg + DEST_G(dpix) * invsa) >> 8;
				UINT32 b = (SOURCE15_B(pix) * sb + DEST_B(dpix) * invsa) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_argb32_alpha - perform
    rasterization using standard alpha blending
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_argb32_alpha)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT32 *texbase = prim->texture.base;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 ta = pix >> 24;
				if (ta != 0)
				{
					UINT32 dpix = *dest;
					UINT32 invta = 0x100 - ta;
					UINT32 r = (SOURCE32_R(pix) * ta + DEST_R(dpix) * invta) >> 8;
					UINT32 g = (SOURCE32_G(pix) * ta + DEST_G(dpix) * invta) >> 8;
					UINT32 b = (SOURCE32_B(pix) * ta + DEST_B(dpix) * invta) >> 8;

					*dest = DEST_ASSEMBLE_RGB(r, g, b);
				}
				dest++;
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r);
		UINT32 sg = (UINT32)(256.0f * prim->color.g);
		UINT32 sb = (UINT32)(256.0f * prim->color.b);
		UINT32 sa = (UINT32)(256.0f * prim->color.a);

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }
		if (sa > 0x100) { if ((INT32)sa < 0) sa = 0; else sa = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 ta = (pix >> 24) * sa;
				if (ta)
				{
					UINT32 dpix = *dest;
					UINT32 invsta = (0x10000 - ta) << 8;
					UINT32 r = (SOURCE32_R(pix) * sr * ta + DEST_R(dpix) * invsta) >> 24;
					UINT32 g = (SOURCE32_G(pix) * sg * ta + DEST_G(dpix) * invsta) >> 24;
					UINT32 b = (SOURCE32_B(pix) * sb * ta + DEST_B(dpix) * invsta) >> 24;

					*dest = DEST_ASSEMBLE_RGB(r, g, b);
				}
				dest++;
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    draw_quad_argb32_multiply - perform
    rasterization using RGB multiply
-------------------------------------------------*/

static void FUNC_PREFIX(draw_quad_argb32_multiply)(const render_primitive *prim, void *dstdata, UINT32 pitch, quad_setup_data *setup)
{
	UINT32 *texbase = prim->texture.base;
	UINT32 texrp = prim->texture.rowpixels;
	INT32 dudx = setup->dudx;
	INT32 dvdx = setup->dvdx;
	INT32 endx = setup->endx;
	INT32 x, y;

	/* fast case: no coloring, no alpha */
	if (prim->color.r >= 1.0f && prim->color.g >= 1.0f && prim->color.b >= 1.0f && prim->color.a >= 1.0f)
	{
		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 dpix = *dest;
				UINT32 r = (SOURCE32_R(pix) * DEST_R(dpix)) >> 8;
				UINT32 g = (SOURCE32_G(pix) * DEST_G(dpix)) >> 8;
				UINT32 b = (SOURCE32_B(pix) * DEST_B(dpix)) >> 8;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}

	/* alpha and/or coloring case */
	else
	{
		UINT32 sr = (UINT32)(256.0f * prim->color.r * prim->color.a);
		UINT32 sg = (UINT32)(256.0f * prim->color.g * prim->color.a);
		UINT32 sb = (UINT32)(256.0f * prim->color.b * prim->color.a);

		/* clamp R,G,B and inverse A to 0-256 range */
		if (sr > 0x100) { if ((INT32)sr < 0) sr = 0; else sr = 0x100; }
		if (sg > 0x100) { if ((INT32)sg < 0) sg = 0; else sg = 0x100; }
		if (sb > 0x100) { if ((INT32)sb < 0) sb = 0; else sb = 0x100; }

		/* loop over rows */
		for (y = setup->starty; y < setup->endy; y++)
		{
			PIXEL_TYPE *dest = (PIXEL_TYPE *)dstdata + y * pitch + setup->startx;
			INT32 curu = setup->startu + (y - setup->starty) * setup->dudy;
			INT32 curv = setup->startv + (y - setup->starty) * setup->dvdy;

			/* loop over cols */
			for (x = setup->startx; x < endx; x++)
			{
				UINT32 pix = texbase[(curv >> 16) * texrp + (curu >> 16)];
				UINT32 dpix = *dest;
				UINT32 r = (SOURCE32_R(pix) * sr * DEST_R(dpix)) >> 16;
				UINT32 g = (SOURCE32_G(pix) * sg * DEST_G(dpix)) >> 16;
				UINT32 b = (SOURCE32_B(pix) * sb * DEST_B(dpix)) >> 16;

				*dest++ = DEST_ASSEMBLE_RGB(r, g, b);
				curu += dudx;
				curv += dvdx;
			}
		}
	}
}


/*-------------------------------------------------
    setup_and_draw_textured_quad - perform setup
    and then dispatch to a texture-mode-specific
    drawing routine
-------------------------------------------------*/

static void FUNC_PREFIX(setup_and_draw_textured_quad)(const render_primitive *prim, void *dstdata, UINT32 pitch, const render_bounds *clip)
{
	quad_setup_data setup;

	/* temporary fp U/V coordinates */
	float fdudx, fdvdx, fdudy, fdvdy;
	float u0 = 0.0f, v0 = 0.0f;		/* top-left */
	float u1 = 1.0f, v1 = 0.0f;		/* top-right */
	float u2 = 0.0f, v2 = 1.0f;		/* bottom-left */
	float u3 = 1.0f, v3 = 1.0f;		/* bottom-right */
	render_bounds fpos;

	/* apply orientation to the U/V coordinates */
	if (prim->flags & ORIENTATION_SWAP_XY) { FSWAP(u1, u2); FSWAP(v1, v2); }
	if (prim->flags & ORIENTATION_FLIP_X) { FSWAP(u0, u1); FSWAP(v0, v1); FSWAP(u2, u3); FSWAP(v2, v3); }
	if (prim->flags & ORIENTATION_FLIP_Y) { FSWAP(u0, u2); FSWAP(v0, v2); FSWAP(u1, u3); FSWAP(v1, v3); }

	/* build a local copy of the bounds */
	fpos = prim->bounds;
	assert(fpos.x0 <= fpos.x1);
	assert(fpos.y0 <= fpos.y1);

	/* determine U/V deltas */
	fdudx = (u1 - u0) / (fpos.x1 - fpos.x0);
	fdvdx = (v1 - v0) / (fpos.x1 - fpos.x0);
	fdudy = (u2 - u0) / (fpos.y1 - fpos.y0);
	fdvdy = (v2 - v0) / (fpos.y1 - fpos.y0);

	/* apply X clipping */
	if (fpos.x0 < clip->x0)
	{
		u0 += fdudx * (clip->x0 - fpos.x0);
		v0 += fdvdx * (clip->x0 - fpos.x0);
		fpos.x0 = clip->x0;
	}
	if (fpos.x1 > clip->x1)
		fpos.x1 = clip->x1;

	/* apply Y clipping */
	if (fpos.y0 < clip->y0)
	{
		u0 += fdudy * (clip->y0 - fpos.y0);
		v0 += fdvdy * (clip->y0 - fpos.y0);
		fpos.y0 = clip->y0;
	}
	if (fpos.y1 > clip->y1)
		fpos.y1 = clip->y1 + 1;

	/* bail if nothing left */
	if (fpos.x0 > fpos.x1 || fpos.y0 > fpos.y1)
		return;

	/* clamp to integers */
	setup.startx = round_nearest(fpos.x0);
	setup.starty = round_nearest(fpos.y0);
	setup.endx = round_nearest(fpos.x1);
	setup.endy = round_nearest(fpos.y1);

	/* compute start and delta U,V coordinates now */
	setup.dudx = round_nearest(65536.0f * (float)prim->texture.width * fdudx);
	setup.dvdx = round_nearest(65536.0f * (float)prim->texture.height * fdvdx);
	setup.dudy = round_nearest(65536.0f * (float)prim->texture.width * fdudy);
	setup.dvdy = round_nearest(65536.0f * (float)prim->texture.height * fdvdy);
	setup.startu = round_nearest(65536.0f * (float)prim->texture.width * u0);
	setup.startv = round_nearest(65536.0f * (float)prim->texture.height * v0);

	/* advance U/V to the middle of the first pixel */
	setup.startu += (setup.dudx + setup.dudy) / 2;
	setup.startv += (setup.dvdx + setup.dvdy) / 2;

	/* render based on the texture coordinates */
	switch (prim->flags & (PRIMFLAG_TEXFORMAT_MASK | PRIMFLAG_BLENDMODE_MASK))
	{
		case PRIMFLAG_TEXFORMAT(TEXFORMAT_PALETTE16) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE):
			FUNC_PREFIX(draw_quad_palette16_none)(prim, dstdata, pitch, &setup);
			break;

		case PRIMFLAG_TEXFORMAT(TEXFORMAT_PALETTE16) | PRIMFLAG_BLENDMODE(BLENDMODE_ADD):
			FUNC_PREFIX(draw_quad_palette16_add)(prim, dstdata, pitch, &setup);
			break;

		case PRIMFLAG_TEXFORMAT(TEXFORMAT_RGB15) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE):
			FUNC_PREFIX(draw_quad_rgb15)(prim, dstdata, pitch, &setup);
			break;

		case PRIMFLAG_TEXFORMAT(TEXFORMAT_RGB32) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE):
		case PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32) | PRIMFLAG_BLENDMODE(BLENDMODE_NONE):
			FUNC_PREFIX(draw_quad_rgb32)(prim, dstdata, pitch, &setup);
			break;

		case PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32) | PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA):
			FUNC_PREFIX(draw_quad_argb32_alpha)(prim, dstdata, pitch, &setup);
			break;

		case PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32) | PRIMFLAG_BLENDMODE(BLENDMODE_RGB_MULTIPLY):
			FUNC_PREFIX(draw_quad_argb32_multiply)(prim, dstdata, pitch, &setup);
			break;

		default:
			fatalerror("Unknown texformat(%d)/blendmode(%d) combo\n", PRIMFLAG_GET_TEXFORMAT(prim->flags), PRIMFLAG_GET_BLENDMODE(prim->flags));
			break;
	}
}



/*-------------------------------------------------
    draw_primitives - draw a series of primitives
    using a software rasterizer
-------------------------------------------------*/

void FUNC_PREFIX(draw_primitives)(const render_primitive *primlist, void *dstdata, UINT32 width, UINT32 height, UINT32 pitch)
{
	const render_primitive *prim;
	render_bounds clipstack[8];
	render_bounds *clip = &clipstack[0];

	/* set up the initial cliprect */
	clip->x0 = clip->y0 = 0;
	clip->x1 = (float)width;
	clip->y1 = (float)height;

	/* loop over the list and render each element */
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
				if (clip->x1 > (float)width) clip->x1 = (float)width;
				if (clip->y1 > (float)height) clip->y1 = (float)height;
				break;

			case RENDER_PRIMITIVE_CLIP_POP:
				clip--;
				assert(clip >= clipstack);
				break;

			case RENDER_PRIMITIVE_LINE:
				FUNC_PREFIX(draw_line)(prim, dstdata, pitch, clip, width, height);
				break;

			case RENDER_PRIMITIVE_QUAD:
				if (!prim->texture.base)
					FUNC_PREFIX(draw_rect)(prim, dstdata, pitch, clip);
				else
					FUNC_PREFIX(setup_and_draw_textured_quad)(prim, dstdata, pitch, clip);
				break;
		}
}



/***************************************************************************
    MACRO UNDOING
***************************************************************************/

#undef SOURCE15_R
#undef SOURCE15_G
#undef SOURCE15_B

#undef SOURCE32_R
#undef SOURCE32_G
#undef SOURCE32_B

#undef DEST_ASSEMBLE_RGB
#undef DEST_RGB_TO_PIXEL

#undef DEST_R
#undef DEST_G
#undef DEST_B

#undef SOURCE15_TO_DEST
#undef SOURCE32_TO_DEST

#undef FUNC_PREFIX
#undef PIXEL_TYPE

#undef SRCSHIFT_R
#undef SRCSHIFT_G
#undef SRCSHIFT_B

#undef DSTSHIFT_R
#undef DSTSHIFT_G
#undef DSTSHIFT_B

#undef VARIABLE_SHIFT
