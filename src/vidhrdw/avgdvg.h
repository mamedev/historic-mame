#ifndef avgdvg_h
#define avgdvg_h

/* defining this produces *lots* of debugging code. Beware! */
#undef VG_DEBUG

/* More accurate? */
#undef CYCLE_COUNTING

#define VECTOR_TEAM \
	"Brad Oliver (MAME driver)\n" \
	"Bernd Wiebelt (additional code)\n" \
	"Allard van der Bas (additional code)\n" \
	"Al Kossow (VECSIM)\n" \
	"Hedley Rainnie (VECSIM)\n" \
	"Eric Smith (VECSIM)\n" \
	"Neil Bradley (technical advice)\n"

/* vector engine types, passed to vg_init */

#define AVGDVG_MIN          1
#define USE_DVG             1
#define USE_AVG_BZONE       2
#define USE_AVG             3
#define USE_AVG_TEMPEST     4
#define USE_AVG_MHAVOC      5
#define USE_AVG_SWARS       6
#define USE_AVG_QUANTUM     7
#define AVGDVG_MAX          7

#define MAX_POINTS	5000	/* Maximum # of points we can queue in a vector list */

typedef struct
{
	int			x, y;
	int			color;
} v_point;

typedef struct
{
	v_point		point[MAX_POINTS];
	int			index;
	int			draw;
} v_list;

extern unsigned char *vectorram;
extern int vectorram_size;

int avgdvg_done (void);
void avgdvg_go (int offset, int data);
void avgdvg_reset (int offset, int data);
void avgdvg_clr_busy (void);
int avgdvg_init(int vgType);
void avg_draw_list (v_list *list);
void avg_add_point (v_list *list, int x, int y, int color);

void avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void sw_avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void avg_fake_colorram_w (int offset, int data);
void tempest_colorram_w (int offset, int data);
void mhavoc_colorram_w (int offset, int data);
void quantum_colorram_w (int offset, int data);

void dvg_screenrefresh (struct osd_bitmap *bitmap);
void avg_screenrefresh (struct osd_bitmap *bitmap);
int dvg_start(void);
int avg_start(void);
int avg_start_tempest(void);
int avg_start_mhavoc(void);
int avg_start_starwars(void);
int avg_start_quantum(void);
int avg_start_bzone(void);
void dvg_stop(void);
void avg_stop(void);

#endif
