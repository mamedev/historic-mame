#ifndef _SYSTEM8_H_
#define _SYSTEM8_H_

#include "driver.h"

#define SPR_Y_TOP		0
#define SPR_Y_BOTTOM	1
#define SPR_X_LO		2
#define SPR_X_HI		3
#define SPR_BANK		3
#define SPR_WIDTH_LO	4
#define SPR_WIDTH_HI	5
#define SPR_GFXOFS_LO	6
#define SPR_GFXOFS_HI	7
#define SPR_FLIP_X		7

#define SYSTEM8_NO_SPRITEBANKS			0
#define SYSTEM8_SUPPORTS_SPRITEBANKS	1

extern unsigned char	*system8_bg_pagesel;
extern unsigned char 	*system8_scroll_y;
extern unsigned char 	*system8_scroll_x;
extern unsigned char 	*system8_paletteram;
extern unsigned char 	*system8_spritepaletteram;
extern unsigned char 	*system8_backgroundpaletteram;
extern unsigned char 	*system8_videoram;
extern unsigned char 	*system8_spriteram;
extern unsigned char 	*system8_backgroundram;
extern unsigned char 	*system8_sprites_collisionram;
extern unsigned char 	*system8_background_collisionram;
extern unsigned char 	*system8_scrollx_ram;
extern int 	system8_videoram_size;
extern int 	system8_paletteram_size;
extern int 	system8_spritepaletteram_size;
extern int 	system8_backgroundram_size;
extern int 	system8_backgroundpaletteram_size;


int  system8_vh_start(void);
void system8_vh_stop(void);
void system8_define_checkspriteram(void	(*check)(void));
void system8_define_banksupport(int support);
void system8_define_sprite_offset_y(int offset);
void system8_define_spritememsize(int region, int size);
void system8_define_cliparea(int x1, int x2, int y1, int y2);

void pitfall2_clear_spriteram(void);

void system8_soundport_w(int offset, int data);
int  system8_bg_bankselect_r(int offset);
void system8_bg_bankselect_w(int offset, int data);
void system8_videoram_w(int offset,int data);
void system8_paletteram_w(int offset,int data);
void system8_backgroundram_w(int offset,int data);
void system8_backgroundpaletteram_w(int offset,int data);
void system8_vh_screenrefresh(struct osd_bitmap *bitmap);
void system8_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void system8_bankswitch_w(int offset,int data);
int  system8_bankswitch_r(int offset);

void choplifter_scroll_x_w(int offset,int data);
void choplifter_vh_screenrefresh(struct osd_bitmap *bitmap);
void wbml_vh_screenrefresh(struct osd_bitmap *bitmap);

#endif
