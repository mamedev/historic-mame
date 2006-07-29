/******************************************************************************
 *
 * vector.c
 *
 *
 * Copyright 1997,1998 by the M.A.M.E. Project
 *
 *        anti-alias code by Andrew Caldwell
 *        (still more to add)
 *
 * 040227 Fixed miny clip scaling which was breaking in mhavoc. AREK
 * 010903 added support for direct RGB modes MLR
 * 980611 use translucent vectors. Thanks to Peter Hirschberg
 *        and Neil Bradley for the inspiration. BW
 * 980307 added cleverer dirty handling. BW, ASG
 *        fixed antialias table .ac
 * 980221 rewrote anti-alias line draw routine
 *        added inline assembly multiply fuction for 8086 based machines
 *        beam diameter added to draw routine
 *        beam diameter is accurate in anti-alias line draw (Tcosin)
 *        flicker added .ac
 * 980203 moved LBO's routines for drawing into a buffer of vertices
 *        from avgdvg.c to this location. Scaling is now initialized
 *        by calling vector_init(...). BW
 * 980202 moved out of msdos.c ASG
 * 980124 added anti-alias line draw routine
 *        modified avgdvg.c and sega.c to support new line draw routine
 *        added two new tables Tinten and Tmerge (for 256 color support)
 *        added find_color routine to build above tables .ac
 *
 **************************************************************************** */

#include <math.h>
#include "osinline.h"
#include "driver.h"
#include "vector.h"
#include "render.h"

#define MAX_POINTS	10000

#define VECTOR_TEAM \
	"-* Vector Heads *-\n" \
	"Brad Oliver\n" \
	"Aaron Giles\n" \
	"Bernd Wiebelt\n" \
	"Allard van der Bas\n" \
	"Al Kossow (VECSIM)\n" \
	"Hedley Rainnie (VECSIM)\n" \
	"Eric Smith (VECSIM)\n" \
	"Neil Bradley (technical advice)\n" \
	"Andrew Caldwell (anti-aliasing)\n" \
	"- *** -\n"


#define VCLEAN  0
#define VDIRTY  1
#define VCLIP   2

/* The vertices are buffered here */
typedef struct
{
	int x; int y;
	rgb_t col;
	int intensity;
	int arg1; int arg2; /* start/end in pixel array or clipping info */
	int status;         /* for dirty and clipping handling */
} point;



UINT8 *vectorram;
size_t vectorram_size;

static int flicker;                              /* beam flicker value     */
static float flicker_correction = 0.0f;

static point *vector_list;
static int vector_index;


/* MLR 990316 new gamma handling added */
void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	flicker = (int)(flicker_correction * 2.55);
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

/*
 * Initializes vector game video emulation
 */

VIDEO_START( vector )
{
	/* Grab the settings for this session */
	vector_set_flicker(options.vector_flicker);

	vector_index = 0;

	/* allocate memory for tables */
	vector_list = auto_malloc (MAX_POINTS * sizeof (vector_list[0]));

	return 0;
}


/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
void vector_add_point (int x, int y, rgb_t color, int intensity)
{
	point *newpoint;

	if (intensity > 0xff)
		intensity = 0xff;

	if (flicker && (intensity > 0))
	{
		intensity += (intensity * (0x80-(rand()&0xff)) * flicker)>>16;
		if (intensity < 0)
			intensity = 0;
		if (intensity > 0xff)
			intensity = 0xff;
	}
	newpoint = &vector_list[vector_index];
	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
	newpoint->status = VDIRTY; /* mark identical lines as clean later */

	vector_index++;
	if (vector_index >= MAX_POINTS)
	{
		vector_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}

/*
 * Add new clipping info to the list
 */
void vector_add_clip (int x1, int yy1, int x2, int y2)
{
	point *newpoint;

	newpoint = &vector_list[vector_index];
	newpoint->x = x1;
	newpoint->y = yy1;
	newpoint->arg1 = x2;
	newpoint->arg2 = y2;
	newpoint->status = VCLIP;

	vector_index++;
	if (vector_index >= MAX_POINTS)
	{
		vector_index--;
		logerror("*** Warning! Vector list overflow!\n");
	}
}


/*
 * The vector CPU creates a new display list. We save the old display list,
 * but only once per refresh.
 */
void vector_clear_list (void)
{
	vector_index = 0;
}


VIDEO_UPDATE( vector )
{
	UINT32 flags = PRIMFLAG_ANTIALIAS(options.antialias ? 1 : 0) | PRIMFLAG_BLENDMODE(BLENDMODE_ADD);
	float xscale = 1.0f / (65536 * (Machine->screen[screen].visarea.max_x - Machine->screen[screen].visarea.min_x));
	float yscale = 1.0f / (65536 * (Machine->screen[screen].visarea.max_y - Machine->screen[screen].visarea.min_y));
	float xoffs = (float)Machine->screen[screen].visarea.min_x;
	float yoffs = (float)Machine->screen[screen].visarea.min_y;
	point *curpoint;
	render_bounds clip;
	int lastx = 0, lasty = 0;
	int i;

	curpoint = vector_list;

	render_screen_add_rect(screen, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0x00,0x00,0x00), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	clip.x0 = clip.y0 = 0.0f;
	clip.x1 = clip.y1 = 1.0f;

	for (i = 0; i < vector_index; i++)
	{
		render_bounds coords;

		if (curpoint->status == VCLIP)
		{
			coords.x0 = ((float)curpoint->x - xoffs) * xscale;
			coords.y0 = ((float)curpoint->y - yoffs) * yscale;
			coords.x1 = ((float)curpoint->arg1 - xoffs) * xscale;
			coords.y1 = ((float)curpoint->arg2 - yoffs) * yscale;

			clip.x0 = (coords.x0 > 0.0f) ? coords.x0 : 0.0f;
			clip.y0 = (coords.y0 > 0.0f) ? coords.y0 : 0.0f;
			clip.x1 = (coords.x1 < 1.0f) ? coords.x1 : 1.0f;
			clip.y1 = (coords.y1 < 1.0f) ? coords.y1 : 1.0f;
		}
		else
		{
			coords.x0 = ((float)lastx - xoffs) * xscale;
			coords.y0 = ((float)lasty - yoffs) * yscale;
			coords.x1 = ((float)curpoint->x - xoffs) * xscale;
			coords.y1 = ((float)curpoint->y - yoffs) * yscale;

			if (curpoint->intensity != 0)
				if (!render_clip_line(&coords, &clip))
					render_screen_add_line(screen, coords.x0, coords.y0, coords.x1, coords.y1,
							(float)options.beam * (1.0f / (65536.0f * 1024.0f)),
							(curpoint->intensity << 24) | (curpoint->col & 0xffffff),
							flags);

			lastx = curpoint->x;
			lasty = curpoint->y;
		}
		curpoint++;
	}
	return 0;
}
