#ifndef __VECTOR__
#define __VECTOR__

#include "artwork.h"

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

#define MAX_POINTS 5000	/* Maximum # of points we can queue in a vector list */

#define MAX_PIXELS 850000  /* Maximum of pixels we can remember */

#define VECTOR_COLOR111(c) \
(((c) & 1)? 0x0000ff : 0) \
|(((c) & 2)? 0x00ff00: 0) \
|(((c) & 4)? 0xff0000: 0)

#define VECTOR_COLOR222(c) \
(((c) & 3) * 0x55) \
|(((((c) >> 2) & 3) * 0x55) << 8) \
|(((((c) >> 4) & 3) * 0x55) << 16)

#define VECTOR_COLOR444(c) \
(((c) & 0xf) * 0x11) \
|(((((c) >> 4) & 0xf) * 0x11) << 8) \
|(((((c) >> 8) & 0xf) * 0x11) << 16)

extern int translucency;  /* translucent vectors  */

extern unsigned char *vectorram;
extern size_t vectorram_size;

int  vector_vh_start (void);
void vector_vh_stop (void);
void vector_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void vector_clear_list (void);
void vector_draw_to (int x2, int y2, int col, int intensity, int dirty);
void vector_add_point (int x, int y, int color, int intensity);
void vector_add_clip (int minx, int miny, int maxx, int maxy);
void vector_set_flip_x (int flip);
void vector_set_flip_y (int flip);
void vector_set_swap_xy (int swap);
void vector_set_shift (int shift);
void vector_set_intensity(float _intensity);
float vector_get_intensity(void);
void vector_set_flicker(float _flicker);
float vector_get_flicker(void);
void vector_set_gamma(float _gamma);
float vector_get_gamma(void);

#endif

