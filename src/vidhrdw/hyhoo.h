extern void hyhoo_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern void hyhoo_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern int hyhoo_vh_start(void);
extern void hyhoo_vh_stop(void);

extern WRITE_HANDLER( hyhoo_palette_w );
extern void hyhoo_radrx_w(int data);
extern void hyhoo_radry_w(int data);
extern void hyhoo_sizex_w(int data);
extern void hyhoo_sizey_w(int data);
extern void hyhoo_dispflag_w(int data);
extern void hyhoo_dispflag2_w(int data);
extern void hyhoo_drawx_w(int data);
extern void hyhoo_drawy_w(int data);
extern void hyhoo_romsel_w(int data);
