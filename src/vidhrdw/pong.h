/* The video synchronization layout, derived from the schematics */
#define PONG_MAX_H	(256+128+64+4+2)
#define PONG_MAX_V	(256+4+1)
#define PONG_HBLANK (64+16)
#define PONG_VBLANK 16
#define PONG_HSYNC0 32
#define PONG_HSYNC1 64
#define PONG_VSYNC0 4
#define PONG_VSYNC1 8
#define PONG_FPS	60

int pong_vh_start(void);
void pong_vh_stop(void);
void pong_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int pong_vh_scanline(void);
void pong_vh_eof_callback(void);


