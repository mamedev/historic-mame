#ifndef avgdvg_h
#define avgdvg_h

/* defining this produces *lots* of debugging code. Beware! */
#undef VG_DEBUG

/* More accurate? */
#undef CYCLE_COUNTING

#define VECTOR_TEAM \
	"Al Kossow\nHedley Rainnie\nEric Smith\n  (original code)\n" \
	"Brad Oliver\n  (MAME driver)\n" \
	"Bernd Wiebelt\nAllard van der Bas\n  (additional code)\n"
	
/* vector engine types, passed to vg_init */

#define AVGDVG_MIN		1
#define USE_DVG			1		
#define USE_AVG_BZONE	2
#define USE_AVG			3
#define USE_AVG_SWARS	4
#define AVGDVG_MAX		4

extern unsigned char *vectorram;
extern int vectorram_size;

int avgdvg_done (void);
void avgdvg_go (int offset, int data);
void avgdvg_reset (int offset, int data);
void avgdvg_clr_busy (void);
int avgdvg_init(int vgType);

void avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void sw_avg_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void avg_fake_colorram_w (int offset, int data);
void tempest_colorram_w (int offset, int data);
void dvg_screenrefresh (struct osd_bitmap *bitmap);
void avg_screenrefresh (struct osd_bitmap *bitmap);
int dvg_start(void);
int avg_start(void);
int avg_start_starwars(void);
int avg_start_bzone(void);
void dvg_stop(void);
void avg_stop(void);

#endif
