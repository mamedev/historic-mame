/* defined in drivers/psikyosh.c */
#define MASTER_CLOCK 57272700	// main oscillator frequency

extern data32_t *psikyosh_bgram, *psikyosh_zoomram, *psikyosh_vidregs, *psh_ram, *ps4_io_select;
extern data32_t *bgpen_1, *bgpen_2, *screen1_brt, *screen2_brt;

/* defined in vidhrdw/psiykosh.c */
extern data32_t *psikyosh_bgram, *psikyosh_zoomram, *psikyosh_vidregs;

VIDEO_START( psikyosh );
VIDEO_UPDATE( psikyosh );
VIDEO_EOF( psikyosh );

VIDEO_START( psikyo4 );
VIDEO_UPDATE( psikyo4 );

/* Psikyo PS6406B */
#define FLIPSCREEN (((psikyosh_vidregs[3] & 0x0000c000) == 0x0000c000) ? 1:0)
#define BG_LARGE(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00001000 ) ? 1:0)
#define BG_DEPTH_8BPP(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00004000 ) ? 1:0)
#define BG_LAYER_ENABLE(n) (((psikyosh_vidregs[7] << (4*n)) & 0x00008000 ) ? 1:0)

#define BG_TYPE(n) (((psikyosh_vidregs[6] << (8*n)) & 0x7f000000 ) >> 24)
#define BG_NORMAL      0x0a
#define BG_NORMAL_ALT  0x0b /* Same as above but with different location for scroll/priority reg */

/* All below have 0x80 bit set, probably row/linescroll enable/toggle */
#define BG_SCROLL_0C   0x0c /* 224 v/h scroll values in bank 0x0c; Used in daraku, for text */
#define BG_SCROLL_0D   0x0d /* 224 v/h scroll values in bank 0x0d; Used in daraku, for alternate characters of text */
#define BG_SCROLL_0E   0x0e /* 224 v/h scroll values in bank 0x0e; Used in s1945ii */
#define BG_SCROLL_0F   0x0f /* 224 v/h scroll values in bank 0x0f; */
#define BG_SCROLL_10   0x10 /* 224 v/h scroll values in bank 0x10; Used in s1945ii */
#define BG_SCROLL_11   0x11 /* 224 v/h scroll values in bank 0x11; */
#define BG_SCROLL_12   0x12 /* 224 v/h scroll values in bank 0x12; Used in s1945ii */

/* Psikyo PS6807 */
#define DUAL_SCREEN 1 /* Display both screens simultaneously if 1 */
