extern unsigned char *lwings_fgvideoram;
extern unsigned char *lwings_bg1videoram;

WRITE_HANDLER( lwings_fgvideoram_w );
WRITE_HANDLER( lwings_bg1videoram_w );
WRITE_HANDLER( lwings_bg1_scrollx_w );
WRITE_HANDLER( lwings_bg1_scrolly_w );
WRITE_HANDLER( lwings_bg2_scrollx_w );
WRITE_HANDLER( lwings_bg2_image_w );
int  lwings_vh_start(void);
int  trojan_vh_start(void);
int  avengers_vh_start(void);
void lwings_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void trojan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void lwings_eof_callback(void);
