/*************************************************************************

  The  System16  machine hardware.   By Mirko Buffoni.

  This files describes the hardware behaviour of a System16 machine.
  It also includes common methods for handling of video hardware.

  Many thanks to Thierry Lescot (Thierry.Lescot@skynet.be),
  Phil Stroffolino (phil@maya.com), and Li Jih Hwa <nao@ms6.hinet.net>
  for precious information about S16 hardware.

*************************************************************************/

struct sys16_sprite_info
{
	int begin_line;
	int end_line;
	int horizontal_position;
	int horizontal_flip;
	int vertical_flip;
	int width;
	int number;
	int bank;
	int priority;
	int color;
	int zoom;
};

extern struct sys16_sprite_info *sys16_sprite;

#define FG_OFFS 0
#define BG_OFFS 1

extern int s16_videoram_size;
extern int s16_soundram_size;
extern int s16_spriteram_size;
extern int s16_backgroundram_size;
extern int system16_sprxoffset;
extern unsigned char *system16_videoram;
extern unsigned char *system16_soundram;
extern unsigned char *system16_spriteram;
extern unsigned char *system16_scrollram;
extern unsigned char *system16_pagesram;
extern unsigned char *system16_colordirty;
extern unsigned char *system16_refreshregister;
extern unsigned char *system16_backgroundram;
extern unsigned char system16_background_bank;
extern unsigned char *system16_bgdirty;

extern unsigned char *goldnaxe_mirror1;
extern unsigned char *goldnaxe_mirror2;

extern int  scrollY[2];
extern int  scrollX[2];
extern unsigned char fg_pages[4];
extern unsigned char bg_pages[4];
extern unsigned char fg_dirty[4];
extern unsigned char bg_dirty[4];
extern int  system16_refreshenable;

extern int *s16_obj_bank;

//struct osd_bitmap *fg_bitmap;
//struct osd_bitmap *bg_bitmap;



/*   Init machine.  Allocates memory for sprites, background,
 *   foreground, and tiles area.
 */

int  system16_init_machine(void);
void system16_done(void);

int  system16_videoram_r(int offset);
void system16_videoram_w(int offset, int data);

int  system16_soundram_r(int offset);
void system16_soundram_w(int offset, int data);

int  system16_spriteram_r(int offset);
void system16_spriteram_w(int offset, int data);

void system16_paletteram_w(int offset, int data);

int  system16_backgroundram_r(int offset);
void system16_backgroundram_w(int offset, int data);

int  system16_refreshenable_r(int offset);
void system16_refreshenable_w(int offset, int data);

void system16_scroll_w(int offset, int data);
void system16_pagesel_w(int offset, int data);

int  system16_vh_start(void);
void system16_vh_stop(void);

void system16_define_bank_vector(int * bank);
void system16_define_sprxoffset(int sprxoffset);

int  system16_interrupt(void);

int  shinobi_control_r (int offset);
int  shinobi_dsw_r (int offset);
void shinobi_scroll_w(int offset, int data);
void shinobi_pagesel_w(int offset, int data);
void shinobi_refreshenable_w(int offset, int data);

int  passshot_control_r (int offset);

int  goldnaxe_mirror1_r (int offset);
int  goldnaxe_mirror2_r (int offset);
void goldnaxe_refreshenable_w(int offset, int data);

void tetris_fgpagesel_w(int offset, int data);
void tetris_bgpagesel_w(int offset, int data);

void passshot_scroll_w(int offset, int data);
void passshot_pagesel_w(int offset, int data);
void passshot_spriteram_w( int offset, int data );
