/* appoooh.h */

extern unsigned char *spriteram,*spriteram_2;

/* vidhrdw */
extern unsigned char *appoooh_fg_videoram,*appoooh_fg_colorram;
extern unsigned char *appoooh_bg_videoram,*appoooh_bg_colorram;
WRITE_HANDLER( appoooh_fg_videoram_w );
WRITE_HANDLER( appoooh_fg_colorram_w );
WRITE_HANDLER( appoooh_bg_videoram_w );
WRITE_HANDLER( appoooh_bg_colorram_w );
void appoooh_vh_convert_color_prom(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom);
WRITE_HANDLER( appoooh_scroll_w );
WRITE_HANDLER( appoooh_out_w );
int appoooh_vh_start(void);
void appoooh_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

