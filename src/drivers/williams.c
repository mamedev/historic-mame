/***************************************************************************

Changes:
	Mar 12 98 LBO
	* Added Sinistar speech samples provided by Howie Cohen
	* Fixed clipping circuit with help from Sean Riddle and Pat Lawrence

	Mar 22 98 ASG
	* Fixed Sinistar head drawing, did another major cleanup
	* Rewrote the blitter routines
	* Fixed interrupt timing finally!!!

Do to:  Mystic Marathon
Not sure of the board: Turkey shoot, Joust 2

Note: the visible area (according to the service mode) seems to be
	{ 6, 297-1, 7, 246-1 },
however I use
	{ 6, 298-1, 7, 247-1 },
because the DOS port doesn't support well screen widths which are not multiple of 4.

------- Blaster Bubbles Joust Robotron Sinistar Splat Stargate -------------

0000-8FFF ROM   (for Blaster, 0000-3FFF is a bank of 12 ROMs)
0000-97FF Video  RAM Bank switched with ROM (96FF for Blaster)
9800-BFFF RAM
	0xBB00 Blaster only, Color 0 for each line (256 entry)
	0xBC00 Blaster only, Color 0 flags, latch color only if bit 0 = 1 (256 entry)
	    Do something else with the bit 1, I do not know what
C000-CFFF I/O
D000-FFFF ROM

c000-C00F color_registers  (16 bytes of BBGGGRRR)

c804 widget_pia_dataa (widget = I/O board)
c805 widget_pia_ctrla
c806 widget_pia_datab
c807 widget_pia_ctrlb (CB2 select between player 1 and player 2
		       controls if Table or Joust)
      bits 5-3 = 110 = player 2
      bits 5-3 = 111 = player 1

c80c rom_pia_dataa
c80d rom_pia_ctrla
c80e rom_pia_datab
      bit 0 \
      bit 1 |
      bit 2 |-6 bits to sound board
      bit 3 |
      bit 4 |
      bit 5 /
      bit 6 \
      bit 7 /Plus CA2 and CB2 = 4 bits to drive the LED 7 segment
c80f rom_pia_ctrlb

C900 rom_enable_scr_ctrl  Switch between video ram and rom at 0000-97FF

C940 Blaster only: Select bank in the color Prom for color remap
C980 Blaster only: Select which ROM is at 0000-3FFF
C9C0 Blaster only: bit 0 = enable the color 0 changing each lines
		   bit 1 = erase back each frame


***** Blitter ****** (Stargate and Defender do not have blitter)
CA00 start_blitter    Each bits has a function
      1000 0000 Do not process half the byte 4-7
      0100 0000 Do not process half the byte 0-3
      0010 0000 Shift the shape one pixel right (to display a shape on an odd pixel)
      0001 0000 Remap, if shape != 0 then pixel = mask
      0000 1000 Source  1 = take source 0 = take Mask only
      0000 0100 ?
      0000 0010 Transparent
      0000 0001
CA01 blitter_mask     Not really a mask, more a remap color, see Blitter
CA02 blitter_source   hi
CA03 blitter_source   lo
CA04 blitter_dest     hi
CA05 blitter_dest     lo
CA06 blitter_w_h      H  Do a XOR with 4 to have the real value (Except Splat)
CA07 blitter_w_h      W  Do a XOR with 4 to have the real value (Except Splat)

CB00 6 bits of the video counters bits 2-7

CBFF watchdog

CC00-CFFF 1K X 4 CMOS ram battery backed up (8 bits on Sinistar)




------------------ Robotron -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Move Up
  bit 1  Move Down
  bit 2  Move Left
  bit 3  Move Right
  bit 4  1 Player
  bit 5  2 Players
  bit 6  Fire Up
  bit 7  Fire Down

c806 widget_pia_datab
  bit 0  Fire Left
  bit 1  Fire Right
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Joust -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Move Left   player 1/2
  bit 1  Move Right  player 1/2
  bit 2  Flap        player 1/2
  bit 3
  bit 4  2 Player
  bit 5  1 Players
  bit 6
  bit 7

c806 widget_pia_datab
  bit 0
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Stargate -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Fire
  bit 1  Thrust
  bit 2  Smart Bomb
  bit 3  HyperSpace
  bit 4  2 Players
  bit 5  1 Player
  bit 6  Reverse
  bit 7  Down

c806 widget_pia_datab
  bit 0  Up
  bit 1  Inviso
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7  0 = Upright  1 = Table

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin        (High Score Reset in schematics)
  bit 3  High Score Reset  (Left Coin in schematics)
  bit 4  Left Coin         (Center Coin in schematics)
  bit 5  Center Coin       (Right Coin in schematics)
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Splat -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Walk Up
  bit 1  Walk Down
  bit 2  Walk Left
  bit 3  Walk Right
  bit 4  1 Player
  bit 5  2 Players
  bit 6  Throw Up
  bit 7  Throw Down

c806 widget_pia_datab
  bit 0  Throw Left
  bit 1  Throw Right
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Blaster -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  up/down switch a
  bit 1  up/down switch b
  bit 2  up/down switch c
  bit 3  up/down direction
  bit 4  left/right switch a
  bit 5  left/right switch b
  bit 6  left/right switch c
  bit 7  left/right direction

c806 widget_pia_datab
  bit 0  Thrust (Panel)
  bit 1  Blast
  bit 2  Thrust (Joystick)
  bit 3
  bit 4  1 Player
  bit 5  2 Player
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board

------------------------- Sinistar -------------------------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  up/down switch a
  bit 1  up/down switch b
  bit 2  up/down switch c
  bit 3  up/down direction
  bit 4  left/right switch a
  bit 5  left/right switch b
  bit 6  left/right switch c
  bit 7  left/right direction

c806 widget_pia_datab
  bit 0  Fire
  bit 1  Bomb
  bit 2
  bit 3
  bit 4  1 Player
  bit 5  2 Player
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Bubbles -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Up
  bit 1  Down
  bit 2  Left
  bit 3  Right
  bit 4  2 Players
  bit 5  1 Player
  bit 6
  bit 7

c806 widget_pia_datab
  bit 0
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin        (High Score Reset in schematics)
  bit 3  High Score Reset  (Left Coin in schematics)
  bit 4  Left Coin         (Center Coin in schematics)
  bit 5  Center Coin       (Right Coin in schematics)
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board

------------------------- Defender -------------------------------------
0000-9800 Video RAM
C000-CFFF ROM (4 banks) + I/O
d000-ffff ROM

c000-c00f color_registers  (16 bytes of BBGGGRRR)

C3FC      WatchDog

C400-C4FF CMOS ram battery backed up

C800      6 bits of the video counters bits 2-7

cc00 pia1_dataa (widget = I/O board)
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6
  bit 7
cc01 pia1_ctrla

cc02 pia1_datab
  bit 0 \
  bit 1 |
  bit 2 |-6 bits to sound board
  bit 3 |
  bit 4 |
  bit 5 /
  bit 6 \
  bit 7 /Plus CA2 and CB2 = 4 bits to drive the LED 7 segment
cc03 pia1_ctrlb (CB2 select between player 1 and player 2 controls if Table)

cc04 pia2_dataa
  bit 0  Fire
  bit 1  Thrust
  bit 2  Smart Bomb
  bit 3  HyperSpace
  bit 4  2 Players
  bit 5  1 Player
  bit 6  Reverse
  bit 7  Down
cc05 pia2_ctrla

cc06 pia2_datab
  bit 0  Up
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7
cc07 pia2_ctrlb
  Control the IRQ

d000 Select bank (c000-cfff)
  0 = I/O
  1 = BANK 1
  2 = BANK 2
  3 = BANK 3
  7 = BANK 4


***************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"


/**** from machine/williams.h ****/
extern unsigned char *williams_bank_base;
extern unsigned char *williams_video_counter;

void robotron_init_machine(void);
void joust_init_machine(void);
void stargate_init_machine(void);
void bubbles_init_machine(void);
void sinistar_init_machine(void);
void defender_init_machine(void);
void splat_init_machine(void);
void blaster_init_machine(void);
void colony7_init_machine(void);
void lottofun_init_machine(void);

int williams_interrupt(void);
void williams_vram_select_w(int offset,int data);

extern unsigned char *robotron_catch;
int robotron_catch_loop_r(int offset);

extern unsigned char *stargate_catch;
int stargate_catch_loop_r(int offset);

extern unsigned char *defender_catch;
extern unsigned char *defender_bank_base;
int defender_catch_loop_r(int offset);
void defender_bank_select_w(int offset,int data);

void colony7_bank_select_w(int offset,int data);

extern unsigned char *blaster_catch;
extern unsigned char *blaster_bank2_base;
int blaster_catch_loop_r(int offset);
void blaster_bank_select_w(int offset,int data);
void blaster_vram_select_w(int offset,int data);

extern unsigned char *splat_catch;
int splat_catch_loop_r(int offset);


/**** from vidhrdw/williams.h ****/
extern unsigned char *williams_blitterram;
extern unsigned char *williams_remap_select;

void williams_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int williams_vh_start(void);
int williams_vh_start_sc2(void);
void williams_vh_stop(void);
void williams_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void williams_videoram_w(int offset,int data);
void williams_blitter_w(int offset,int data);
void williams_remap_select_w(int offset, int data);

extern unsigned char *blaster_color_zero_table;
extern unsigned char *blaster_color_zero_flags;
extern unsigned char *blaster_video_bits;
void blaster_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void blaster_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void blaster_video_bits_w(int offset, int data);

int sinistar_vh_start(void);

void defender_videoram_w(int offset,int data);



/***************************************************************************

  Memory handlers

***************************************************************************/

/*
 *   Read/Write mem for Robotron Joust Stargate Bubbles
 */

static struct MemoryReadAddress robotron_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9810, 0x9810, robotron_catch_loop_r, &robotron_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress joust_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress stargate_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9c39, 0x9c39, stargate_catch_loop_r, &stargate_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bubbles_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress williams_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },           /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                  /* CMOS                      */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Splat
 */

static struct MemoryReadAddress splat_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x984b, 0x984b, splat_catch_loop_r, &splat_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                           /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress splat_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },         /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram },  /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Sinistar
 */

static struct MemoryReadAddress sinistar_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xdfff, MRA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sinistar_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },             /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram },   /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                    /* CMOS         */
	{ 0xd000, 0xdfff, MWA_RAM },                                    /* for Sinistar */
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Blaster
 */

static struct MemoryReadAddress blaster_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1, &williams_bank_base },
	{ 0x4000, 0x96ff, MRA_BANK2, &blaster_bank2_base },
/*      { 0x9700, 0x9700, blaster_catch_loop_r, &blaster_catch },*/
	{ 0x9700, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS      */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress blaster_writemem[] =
{
	{ 0x0000, 0x96ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9700, 0xbaff, MWA_RAM },
	{ 0xbb00, 0xbbff, MWA_RAM, &blaster_color_zero_table },     /* Color 0 for each line */
	{ 0xbc00, 0xbcff, MWA_RAM, &blaster_color_zero_flags },     /* Color 0 flags, latch color only if bit 0 = 1 */
	{ 0xbd00, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, blaster_vram_select_w },          			 /* VRAM bank  */
	{ 0xc940, 0xc940, williams_remap_select_w, &williams_remap_select }, /* select remap colors in Remap prom */
	{ 0xc980, 0xc980, blaster_bank_select_w },                   /* Bank Select */
	{ 0xc9C0, 0xc9c0, blaster_video_bits_w, &blaster_video_bits },/* Video related bits */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                 /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Defender
 */

static struct MemoryReadAddress defender_readmem[] =
{
	{ 0xa05d, 0xa05d, defender_catch_loop_r, &defender_catch },
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },          /* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress defender_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },   /* here only to initialize the pointers */
	{ 0xd000, 0xd000, defender_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Colony7
 */

static struct MemoryReadAddress colony7_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },		/* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress colony7_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },	/* here only to initialize the pointers */
	{ 0xd000, 0xd000, colony7_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   memory handlers for the audio CPU
 */

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0400, 0x0403, pia_3_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0400, 0x0403, pia_3_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   memory handlers for the Colony 7 audio CPU
 */

static struct MemoryReadAddress colony7_sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x8400, 0x8403, pia_3_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress colony7_sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x8400, 0x8403, pia_3_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/***************************************************************************

  I/O port definitions

***************************************************************************/

/*
 *   Robotron buttons
 */

INPUT_PORTS_START( robotron_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP, "Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN, "Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT, "Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT, "Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP, "Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN, "Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT, "Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT, "Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Joust buttons
 */

INPUT_PORTS_START( joust_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 (muxed with IN0) */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/*
 *   Stargate buttons
 */

INPUT_PORTS_START( stargate_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", OSD_KEY_COLON, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust", OSD_KEY_L, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Smart Bomb", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON6, "Hyperspace", OSD_KEY_N, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON4, "Reverse", OSD_KEY_ALT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY, "Move Down", OSD_KEY_S, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY, "Move Up", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON5, "Inviso", OSD_KEY_SPACE, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 - fake port for better joystick control */
	/* This fake port is handled via stargate_input_port_0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT )
INPUT_PORTS_END


/*
 *   Defender buttons
 */

INPUT_PORTS_START( defender_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", OSD_KEY_COLON, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust", OSD_KEY_L, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Smart Bomb", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4, "Hyperspace", OSD_KEY_N, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON6, "Reverse", OSD_KEY_ALT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY, "Move Down", OSD_KEY_S, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY, "Move Up", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 - fake port for better joystick control */
	/* This fake port is handled via defender_input_port_1 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT )
INPUT_PORTS_END


/*
 *   Sinistar buttons
 */

INPUT_PORTS_START( sinistar_input_ports )
	PORT_START      /* IN0 */
/* Not really the hardware bits, see sinistar_input_port_0() */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Bubbles buttons
 */

INPUT_PORTS_START( bubbles_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Splat buttons
 */

INPUT_PORTS_START( splat_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER2, "Walk Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER2, "Walk Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER2, "Walk Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER2, "Walk Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER2, "Throw Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER2, "Throw Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER2, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER1, "Walk Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER1, "Walk Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER1, "Walk Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Walk Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER1, "Throw Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER1, "Throw Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER1, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*
 *   Blaster buttons
 */

INPUT_PORTS_START( blaster_input_ports )
	PORT_START      /* IN0 */
/* Not really the hardware bits */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust panel", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, "Blast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Thrust joystick", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Colony7 buttons
 */

INPUT_PORTS_START( colony7_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*
 *   Lotto Fun buttons
 */

INPUT_PORTS_START( lottofun_input_ports )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Used by ticket dispenser */

	PORT_START		/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPF_TOGGLE, "Memory Protect", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // COIN1.5? :)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Sound board handshake
INPUT_PORTS_END


/***************************************************************************

  Machine drivers

***************************************************************************/

/*
 *   Sound interface
 */

static struct DACinterface dac_interface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};


/*
 *   Robotron driver
 */

static struct MachineDriver robotron_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			robotron_readmem,       /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	robotron_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Joust driver
 */

static struct MachineDriver joust_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			joust_readmem,          /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	joust_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



/*
 *   Stargate driver
 */

static struct MachineDriver stargate_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			stargate_readmem,       /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	stargate_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Bubbles driver
 */

static struct MachineDriver bubbles_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			bubbles_readmem,        /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	bubbles_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Sinistar driver
 */

static struct Samplesinterface sinistar_samples_interface =
{
	1	/* 1 channel */
};

static struct MachineDriver sinistar_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */ /*Sinistar do not like 1 mhz*/
			0,                      /* memory region */
			sinistar_readmem,       /* MemoryReadAddress */
			sinistar_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	sinistar_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	sinistar_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_SAMPLES,
			&sinistar_samples_interface
		}
	}
};


/*
 *   Defender driver
 */

static struct MachineDriver defender_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz. Collect at least 9 humans, when you depose them, the game stuck */
			0,                      /* memory region */
			defender_readmem,       /* MemoryReadAddress */
			defender_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	defender_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Splat driver
 */

static struct MachineDriver splat_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			splat_readmem,          /* MemoryReadAddress */
			splat_writemem,         /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	splat_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start_sc2,                  /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Blaster driver
 */

static struct MachineDriver blaster_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			blaster_readmem,        /* MemoryReadAddress */
			blaster_writemem,       /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	blaster_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	256,                                  /* total colors */
	0,                                      /* color table length */
	blaster_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start_sc2,                  /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	blaster_vh_screenrefresh,               /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Colony 7 driver
 */

static struct MachineDriver colony7_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* 1 Mhz */
			0,                      /* memory region */
			colony7_readmem,       /* MemoryReadAddress */
			colony7_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750,   /* 0.89475 Mhz (3.579 / 4) */
			2,        /* memory region #2 */
			colony7_sound_readmem,colony7_sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	colony7_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *	 Lotto Fun driver
 */

static struct MachineDriver lottofun_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,				/* ? Mhz */
			0,						/* memory region */
			bubbles_readmem,		/* MemoryReadAddress */
			williams_writemem,		/* MemoryWriteAddress */
			0,						/* IOReadPort */
			0,						/* IOWritePort */
			williams_interrupt, 	/* interrupt routine */
			64						/* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,		/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1		/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	lottofun_init_machine,			/* init machine routine */

	/* video hardware */
	304, 256,								/* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,							/* GfxDecodeInfo * */
	16, 								 /* total colors */
	0,										/* color table length */
	williams_vh_convert_color_prom, 		/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,										/* vh_init routine */
	williams_vh_start,						/* vh_start routine */
	williams_vh_stop,						/* vh_stop routine */
	williams_vh_screenrefresh,				/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/***************************************************************************

  High score/CMOS save/load

***************************************************************************/

static int cmos_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}

	return 1;
}

static void cmos_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}
}


static int defender_cmos_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}

	return 1;
}

static void defender_cmos_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}
}



/***************************************************************************

  Color PROM data

***************************************************************************/

static unsigned char blaster_remap_prom[] =
{
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x04,0x01,0x03,0x09,0x0A,0x01,0x0C,0x0D,0x0F,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x0D,0x0A,0x09,0x0A,0x0C,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0C,0x0F,0x01,0x01,0x09,0x0A,0x07,0x0C,0x0D,0x06,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x01,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x04,0x0C,0x0D,0x05,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x09,0x09,0x0E,0x01,0x0A,0x09,0x0C,0x0D,0x09,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0C,0x0A,0x09,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x0B,0x0E,0x0C,0x0B,0x06,0x0E,0x0D,0x07,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x06,0x0E,0x0C,0x06,0x01,0x0E,0x0D,0x03,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x01,0x0E,0x0C,0x01,0x0B,0x0E,0x0D,0x0D,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x0B,0x0E,0x0C,0x0B,0x0C,0x0E,0x0D,0x0D,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x04,0x0A,0x05,0x0C,0x0D,0x01,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x05,0x0C,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x05,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x06,0x03,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0C,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0E,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0E,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x01,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x01,0x04,0x03,0x09,0x0B,0x0C,0x0C,0x0D,0x0B,0x0F,
     0x00,0x0F,0x02,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,
     0x00,0x03,0x02,0x07,0x09,0x0B,0x0D,0x0F,0x05,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x01,
     0x00,0x03,0x02,0x0E,0x07,0x01,0x0F,0x09,0x0D,0x06,0x08,0x0A,0x0C,0x05,0x04,0x0B,
     0x00,0x0B,0x02,0x05,0x0C,0x0A,0x04,0x06,0x0D,0x09,0x0F,0x01,0x07,0x0E,0x08,0x03,
     0x00,0x06,0x02,0x0A,0x0C,0x05,0x04,0x0B,0x03,0x08,0x0E,0x07,0x01,0x0F,0x09,0x0D,
     0x00,0x07,0x02,0x0D,0x0F,0x0C,0x04,0x0B,0x05,0x0A,0x06,0x09,0x01,0x03,0x08,0x0E,
     0x00,0x09,0x02,0x05,0x03,0x07,0x01,0x0D,0x0A,0x04,0x08,0x0F,0x06,0x0C,0x0B,0x0E,
     0x00,0x01,0x02,0x01,0x01,0x01,0x01,0x01,0x05,0x09,0x0C,0x01,0x0C,0x01,0x01,0x05,
     0x00,0x05,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x05,
     0x00,0x05,0x02,0x05,0x01,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x02,0x05,0x05,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x02,0x01,0x05,0x05,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x05,0x01,0x01,0x05,0x05,0x01,0x01,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x01,0x05,0x01,0x01,0x01,0x05,0x05,0x01,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x05,0x05,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x09,0x07,0x08,0x09,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x09,0x0B,0x0C,0x0D,0x09,0x0F,
     0x00,0x09,0x02,0x03,0x09,0x05,0x00,0x07,0x08,0x0D,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x09,0x07,0x08,0x09,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x09,0x09,0x0C,0x0D,0x09,0x0F,
     0x00,0x09,0x02,0x03,0x09,0x05,0x00,0x07,0x08,0x0D,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x0D,0x02,0x03,0x01,0x04,0x05,0x04,0x01,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x03,
     0x00,0x03,0x02,0x0D,0x03,0x01,0x04,0x05,0x04,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x01,
     0x00,0x01,0x02,0x03,0x0D,0x03,0x01,0x04,0x05,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x04,
     0x00,0x04,0x02,0x01,0x03,0x0D,0x03,0x01,0x04,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x05,
     0x00,0x05,0x02,0x04,0x01,0x03,0x0D,0x03,0x01,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x04,
     0x00,0x04,0x02,0x05,0x04,0x01,0x03,0x0D,0x03,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x01,
     0x00,0x01,0x02,0x04,0x05,0x04,0x01,0x03,0x0D,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x03,
     0x00,0x03,0x02,0x01,0x04,0x05,0x04,0x01,0x03,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x0D,
     0x00,0x0B,0x02,0x03,0x04,0x01,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0C,0x02,0x03,0x04,0x01,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x05,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x08,0x08,0x08,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x09,0x02,0x0B,0x09,0x05,0x02,0x02,0x02,0x09,0x0A,0x0B,0x09,0x0A,0x0E,0x0E,
     0x00,0x04,0x02,0x03,0x01,0x05,0x05,0x05,0x05,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x0F,
     0x00,0x01,0x02,0x0D,0x01,0x05,0x0B,0x0B,0x0B,0x09,0x0A,0x0B,0x05,0x01,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x01,0x05,0x09,0x09,0x09,0x09,0x0A,0x0B,0x0F,0x01,0x0E,0x09,
     0x00,0x01,0x02,0x0D,0x01,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x05,0x04,0x0E,0x0F,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x02,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0B,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x02,0x09,0x09,0x0A,0x0B,0x0C,0x0D,0x09,0x0B,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x09,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x09,
     0x00,0x09,0x02,0x03,0x04,0x05,0x09,0x02,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0B,
     0x00,0x0E,0x02,0x0A,0x0B,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0B,0x02,0x0A,0x01,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0C,0x0E,0x0F,
     0x00,0x07,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x04,0x0E,0x0F,
     0x00,0x01,0x02,0x0B,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0C,
     0x00,0x05,0x02,0x07,0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x08,0x0E,0x09,
     0x00,0x0F,0x02,0x01,0x0C,0x0B,0x06,0x07,0x08,0x0D,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x08,0x02,0x06,0x07,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0A,0x0E,0x05,
     0x00,0x0C,0x02,0x08,0x03,0x04,0x0E,0x0E,0x0E,0x06,0x08,0x0B,0x0C,0x08,0x07,0x0F,
     0x00,0x0B,0x02,0x0A,0x0D,0x0C,0x0E,0x0E,0x0A,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0C,
     0x00,0x06,0x02,0x0D,0x02,0x0A,0x06,0x07,0x08,0x0C,0x01,0x0B,0x0C,0x0D,0x0F,0x0F,
     0x00,0x05,0x02,0x03,0x01,0x04,0x05,0x04,0x01,0x0F,0x03,0x0B,0x0C,0x0D,0x0A,0x0F,
     0x00,0x01,0x02,0x02,0x08,0x06,0x05,0x04,0x01,0x05,0x07,0x0B,0x0C,0x0D,0x0A,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x01,0x0E,0x09,0x01,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x06,0x0E,0x09,0x06,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x01,0x0E,0x09,0x01,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x0B,0x0E,0x09,0x0B,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x0F,0x0C,0x0D,0x05,0x05,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x05,0x04,0x03,0x0C,0x0C,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0F,0x04,0x01,0x03,0x06,0x06,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x0E,0x0A,0x0D,0x01,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0A,0x02,0x0D,0x0C,0x0E,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x07,0x02,0x08,0x06,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0C,0x02,0x03,0x04,0x06,0x0E,0x07,0x08,0x05,0x0A,0x0B,0x01,0x03,0x06,0x04,
     0x00,0x0C,0x02,0x03,0x04,0x06,0x02,0x07,0x08,0x05,0x0A,0x0B,0x01,0x03,0x06,0x04,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x05,0x05,0x05,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x02,0x02,0x02,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x09,0x02,0x0B,0x09,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x09,0x0A,0x0E,0x0E,
     0x00,0x04,0x02,0x03,0x01,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x0F,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x02,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x01,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x02,0x05,0x02,0x0E,0x01,0x0C,0x01,0x05,0x01,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x05,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x05,
     0x00,0x05,0x02,0x03,0x04,0x05,0x05,0x02,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x01,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x02,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0C,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x02,0x0F,0x02,0x0A,0x0C,0x0C,0x0A,0x0F,0x0C,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x09,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0F,
     0x00,0x09,0x02,0x0A,0x09,0x0E,0x0F,0x02,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0C,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0B,0x0A,0x09,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x03,0x04,0x05,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0B,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x01,0x04,0x03,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x05,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x06,0x07,0x08,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x05,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x05,0x04,0x03,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0A,
     0x00,0x01,0x02,0x03,0x04,0x06,0x05,0x04,0x03,0x0E,0x0A,0x0B,0x01,0x0D,0x0E,0x04,
     0x00,0x01,0x02,0x03,0x04,0x0C,0x01,0x01,0x03,0x09,0x0E,0x0B,0x07,0x08,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x09,0x09,0x0A,0x09,0x0A,0x0B,0x01,0x0D,0x0E,0x0C,
     0x00,0x01,0x02,0x03,0x04,0x09,0x05,0x05,0x03,0x09,0x0A,0x0B,0x04,0x04,0x0E,0x03,
     0x00,0x01,0x02,0x03,0x04,0x01,0x0C,0x0C,0x0D,0x01,0x0D,0x0B,0x0A,0x0D,0x0E,0x09,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x06,0x07,0x08,0x0E,0x0A,0x0B,0x04,0x04,0x0E,0x03,
     0x00,0x01,0x02,0x03,0x04,0x0F,0x04,0x01,0x03,0x0F,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
 *   Robotron
 */

ROM_START( robotron_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROBOTRON.SB1", 0x0000, 0x1000, 0xd5778c45 )
	ROM_LOAD( "ROBOTRON.SB2", 0x1000, 0x1000, 0x51fb054b )
	ROM_LOAD( "ROBOTRON.SB3", 0x2000, 0x1000, 0x293ce6c2 )
	ROM_LOAD( "ROBOTRON.SB4", 0x3000, 0x1000, 0x7c9557c1 )
	ROM_LOAD( "ROBOTRON.SB5", 0x4000, 0x1000, 0x3dd77e9b )
	ROM_LOAD( "ROBOTRON.SB6", 0x5000, 0x1000, 0x8c53f6cd )
	ROM_LOAD( "ROBOTRON.SB7", 0x6000, 0x1000, 0x746ecd96 )
	ROM_LOAD( "ROBOTRON.SB8", 0x7000, 0x1000, 0x5e442b24 )
	ROM_LOAD( "ROBOTRON.SB9", 0x8000, 0x1000, 0x294d9c17 )
	ROM_LOAD( "ROBOTRON.SBA", 0xd000, 0x1000, 0x17b8fc1e )
	ROM_LOAD( "ROBOTRON.SBB", 0xe000, 0x1000, 0xe816f8e6 )
	ROM_LOAD( "ROBOTRON.SBC", 0xf000, 0x1000, 0xcfc2d9aa )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "ROBOTRON.SND", 0xf000, 0x1000, 0x000f85e5 )
ROM_END

ROM_START( robotryo_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ROBOTRON.YO1", 0x0000, 0x1000, 0xd5778c45 )
	ROM_LOAD( "ROBOTRON.YO2", 0x1000, 0x1000, 0x51fb054b )
	ROM_LOAD( "ROBOTRON.YO3", 0x2000, 0x1000, 0x8f4af95e )
	ROM_LOAD( "ROBOTRON.YO4", 0x3000, 0x1000, 0xeef123ef )
	ROM_LOAD( "ROBOTRON.YO5", 0x4000, 0x1000, 0x6327d431 )
	ROM_LOAD( "ROBOTRON.YO6", 0x5000, 0x1000, 0x8b53f7cd )
	ROM_LOAD( "ROBOTRON.YO7", 0x6000, 0x1000, 0x8f8cbcee )
	ROM_LOAD( "ROBOTRON.YO8", 0x7000, 0x1000, 0x5e442b24 )
	ROM_LOAD( "ROBOTRON.YO9", 0x8000, 0x1000, 0x294d9c17 )
	ROM_LOAD( "ROBOTRON.YOA", 0xd000, 0x1000, 0x1ac7fceb )
	ROM_LOAD( "ROBOTRON.YOB", 0xe000, 0x1000, 0xe615fee7 )
	ROM_LOAD( "ROBOTRON.YOC", 0xf000, 0x1000, 0xd4bdec59 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "ROBOTRON.SND", 0xf000, 0x1000, 0x000f85e5 )
ROM_END


struct GameDriver robotron_driver =
{
	__FILE__,
	0,
	"robotron",
	"Robotron (Solid Blue label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&robotron_machine_driver,       /* MachineDriver * */

	robotron_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	robotron_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver robotryo_driver =
{
	__FILE__,
	&robotron_driver,
	"robotryo",
	"Robotron (Yellow/Orange label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&robotron_machine_driver,       /* MachineDriver * */

	robotryo_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	robotron_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};



/*
 *   Joust
 */

ROM_START( joust_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "JOUST.WG1", 0x0000, 0x1000, 0x56ea72c8 )
	ROM_LOAD( "JOUST.WG2", 0x1000, 0x1000, 0xe32af9c4 )
	ROM_LOAD( "JOUST.WG3", 0x2000, 0x1000, 0x21ec9c8e )
	ROM_LOAD( "JOUST.WG4", 0x3000, 0x1000, 0xad230361 )
	ROM_LOAD( "JOUST.WG5", 0x4000, 0x1000, 0xd37f04e9 )
	ROM_LOAD( "JOUST.WG6", 0x5000, 0x1000, 0x727b5c05 )
	ROM_LOAD( "JOUST.WG7", 0x6000, 0x1000, 0x81aa3756 )
	ROM_LOAD( "JOUST.WG8", 0x7000, 0x1000, 0x8d1829b6 )
	ROM_LOAD( "JOUST.WG9", 0x8000, 0x1000, 0xcbfcd9a6 )
	ROM_LOAD( "JOUST.WGa", 0xd000, 0x1000, 0xf102016a )
	ROM_LOAD( "JOUST.WGb", 0xe000, 0x1000, 0x11b3700d )
	ROM_LOAD( "JOUST.WGc", 0xf000, 0x1000, 0x0cd46bb8 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "JOUST.SND", 0xf000, 0x1000, 0x5799e563 )
ROM_END

ROM_START( joustr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "JOUST.SR1", 0x0000, 0x1000, 0x56ea72c8 )
	ROM_LOAD( "JOUST.SR2", 0x1000, 0x1000, 0xe32af9c4 )
	ROM_LOAD( "JOUST.SR3", 0x2000, 0x1000, 0x21ec9c8e )
	ROM_LOAD( "JOUST.SR4", 0x3000, 0x1000, 0xbd041382 )
	ROM_LOAD( "JOUST.SR5", 0x4000, 0x1000, 0xd37f04e9 )
	ROM_LOAD( "JOUST.SR6", 0x5000, 0x1000, 0x4e2153d3 )
	ROM_LOAD( "JOUST.SR7", 0x6000, 0x1000, 0x8e102052 )
	ROM_LOAD( "JOUST.SR8", 0x7000, 0x1000, 0xd092c808 )
	ROM_LOAD( "JOUST.SR9", 0x8000, 0x1000, 0x341a43c8 )
	ROM_LOAD( "JOUST.SRa", 0xd000, 0x1000, 0xc7d5d145 )
	ROM_LOAD( "JOUST.SRb", 0xe000, 0x1000, 0x54b23102 )
	ROM_LOAD( "JOUST.SRc", 0xf000, 0x1000, 0x0cd46bd8 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "JOUST.SND", 0xf000, 0x1000, 0x5799e563 )
ROM_END

ROM_START( joustwr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "JOUST.WR1", 0x0000, 0x1000, 0x56ea72c8 )
	ROM_LOAD( "JOUST.WR2", 0x1000, 0x1000, 0xe32af9c4 )
	ROM_LOAD( "JOUST.WR3", 0x2000, 0x1000, 0x21ec9c8e )
	ROM_LOAD( "JOUST.WR4", 0x3000, 0x1000, 0xad230361 )
	ROM_LOAD( "JOUST.WR5", 0x4000, 0x1000, 0xd37f04e9 )
	ROM_LOAD( "JOUST.WR6", 0x5000, 0x1000, 0x727b5c05 )
	ROM_LOAD( "JOUST.WR7", 0x6000, 0x1000, 0x81aacd2c )
	ROM_LOAD( "JOUST.WR8", 0x7000, 0x1000, 0x8d1829b6 )
	ROM_LOAD( "JOUST.WR9", 0x8000, 0x1000, 0xcbfcd9a6 )
	ROM_LOAD( "JOUST.WRa", 0xd000, 0x1000, 0x4292d584 )
	ROM_LOAD( "JOUST.WRb", 0xe000, 0x1000, 0x11b3700d )
	ROM_LOAD( "JOUST.WRc", 0xf000, 0x1000, 0x0cd46bb8 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "JOUST.SND", 0xf000, 0x1000, 0x5799e563 )
ROM_END


struct GameDriver joust_driver =
{
	__FILE__,
	0,
	"joust",
	"Joust (White/Green label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&joust_machine_driver,          /* MachineDriver * */

	joust_rom,                      /* White/Green version, latest */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustr_driver =
{
	__FILE__,
	&joust_driver,
	"joustr",
	"Joust (Solid Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&joust_machine_driver,          /* MachineDriver * */

	joustr_rom,                     /* Solid Red version, has pterodactyl bug */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustwr_driver =
{
	__FILE__,
	&joust_driver,
	"joustwr",
	"Joust (White/Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&joust_machine_driver,          /* MachineDriver * */

	joustwr_rom,
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Sinistar
 */

ROM_START( sinistar_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "SINISTAR.01", 0x0000, 0x1000, 0xea8b43ef )
	ROM_LOAD( "SINISTAR.02", 0x1000, 0x1000, 0x4be17573 )
	ROM_LOAD( "SINISTAR.03", 0x2000, 0x1000, 0xcd47871d )
	ROM_LOAD( "SINISTAR.04", 0x3000, 0x1000, 0xe7ceefca )
	ROM_LOAD( "SINISTAR.05", 0x4000, 0x1000, 0xe708bd4c )
	ROM_LOAD( "SINISTAR.06", 0x5000, 0x1000, 0x115f07eb )
	ROM_LOAD( "SINISTAR.07", 0x6000, 0x1000, 0x97ba8096 )
	ROM_LOAD( "SINISTAR.08", 0x7000, 0x1000, 0x3c5c3dc4 )
	ROM_LOAD( "SINISTAR.09", 0x8000, 0x1000, 0x4a7953bf )
	ROM_LOAD( "SINISTAR.10", 0xe000, 0x1000, 0xaf1de107 )
	ROM_LOAD( "SINISTAR.11", 0xf000, 0x1000, 0x81481d1a )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "SINISTAR.SND", 0xf000, 0x1000, 0x7400ae74 )
ROM_END

ROM_START( oldsin_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "SINOLD01.ROM", 0x0000, 0x1000, 0xebf346e5 )
	ROM_LOAD( "SINOLD02.ROM", 0x1000, 0x1000, 0x4be17573 )
	ROM_LOAD( "SINOLD03.ROM", 0x2000, 0x1000, 0x9ec1fddb )
	ROM_LOAD( "SINOLD04.ROM", 0x3000, 0x1000, 0x8b9a77d2 )
	ROM_LOAD( "SINOLD05.ROM", 0x4000, 0x1000, 0x46289976 )
	ROM_LOAD( "SINOLD06.ROM", 0x5000, 0x1000, 0xe6e4b9b6 )
	ROM_LOAD( "SINOLD07.ROM", 0x6000, 0x1000, 0x0c0bee55 )
	ROM_LOAD( "SINOLD08.ROM", 0x7000, 0x1000, 0xba851ef5 )
	ROM_LOAD( "SINOLD09.ROM", 0x8000, 0x1000, 0x2e4ff0b9 )
	ROM_LOAD( "SINOLD10.ROM", 0xe000, 0x1000, 0xd9a9f2fb )
	ROM_LOAD( "SINOLD11.ROM", 0xf000, 0x1000, 0xa5ea8a3e )

	ROM_REGION(0x1000) /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* 64k for the sound CPU */
	ROM_LOAD( "SINISTAR.SND", 0xf000, 0x1000, 0x7400ae74 )
ROM_END

/* Sinistar speech samples */
const char *sinistar_sample_names[]=
{
	"*sinistar",
	"ilive.sam",
	"ihunger.sam",
	"roar.sam",
	"runrun.sam",
	"iamsin.sam",
	"bewarcow.sam",
	"ihungerc.sam",
	"runcow.sam",
	0 /*array end*/
};

struct GameDriver sinistar_driver =
{
	__FILE__,
	0,
	"sinistar",
	"Sinistar",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nHowie Cohen\nSean Riddle\nPat Lawrence",
	0,
	&sinistar_machine_driver,       /* MachineDriver * */

	sinistar_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	sinistar_sample_names,          /* samplenames */
	0,      /* sound_prom */

	sinistar_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	cmos_load, cmos_save
};

struct GameDriver oldsin_driver =
{
	__FILE__,
	&sinistar_driver,
	"oldsin",
	"Sinistar (prototype version)",
	"1982",
	"Williams",
	"\nSinistar team:\nMarc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nHowie Cohen\nSean Riddle\nPat Lawrence\nBrian Deuel (prototype driver)\n\nSpecial thanks to Peter Freeman",
	0,
	&sinistar_machine_driver, /* MachineDriver * */

	oldsin_rom, /* RomModule * */
	0, 0, /* ROM decrypt routines; not sure if these are needed here */
	sinistar_sample_names, /* samplenames */
	0, /* sound_prom */

	sinistar_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	cmos_load, cmos_save
};




/*
 *   Bubbles
 */

ROM_START( bubbles_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "BUBBLES.1B",  0x0000, 0x1000, 0xc6383a74 )
	ROM_LOAD( "BUBBLES.2B",  0x1000, 0x1000, 0xdb996db3 )
	ROM_LOAD( "BUBBLES.3B",  0x2000, 0x1000, 0x40f5f815 )
	ROM_LOAD( "BUBBLES.4B",  0x3000, 0x1000, 0xd0f4b4f0 )
	ROM_LOAD( "BUBBLES.5B",  0x4000, 0x1000, 0x620a3f98 )
	ROM_LOAD( "BUBBLES.6B",  0x5000, 0x1000, 0x4617e675 )
	ROM_LOAD( "BUBBLES.7B",  0x6000, 0x1000, 0x16d20c98 )
	ROM_LOAD( "BUBBLES.8B",  0x7000, 0x1000, 0xfb83ad41 )
	ROM_LOAD( "BUBBLES.9B",  0x8000, 0x1000, 0xc5ca8ce2 )
	ROM_LOAD( "BUBBLES.10B", 0xd000, 0x1000, 0x5eb0c568 )
	ROM_LOAD( "BUBBLES.11B", 0xe000, 0x1000, 0x31598c5d )
	ROM_LOAD( "BUBBLES.12B", 0xf000, 0x1000, 0xfdb24d56 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "BUBBLES.SND", 0xf000, 0x1000, 0x82ae5994 )
ROM_END

ROM_START( bubblesr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "BUBBLESR.1B",  0x0000, 0x1000, 0x90409106 )
	ROM_LOAD( "BUBBLESR.2B",  0x1000, 0x1000, 0x2eae0950 )
	ROM_LOAD( "BUBBLESR.3B",  0x2000, 0x1000, 0xf6ad9f95 )
	ROM_LOAD( "BUBBLESR.4B",  0x3000, 0x1000, 0x945d4b73 )
	ROM_LOAD( "BUBBLESR.5B",  0x4000, 0x1000, 0xb002076e )
	ROM_LOAD( "BUBBLESR.6B",  0x5000, 0x1000, 0x4617e675 )
	ROM_LOAD( "BUBBLESR.7B",  0x6000, 0x1000, 0x16d20c98 )
	ROM_LOAD( "BUBBLESR.8B",  0x7000, 0x1000, 0xfb8bad59 )
	ROM_LOAD( "BUBBLESR.9B",  0x8000, 0x1000, 0xc5ca8ce2 )
	ROM_LOAD( "BUBBLESR.10B", 0xd000, 0x1000, 0x8cc42496 )
	ROM_LOAD( "BUBBLESR.11B", 0xe000, 0x1000, 0xfd0252a2 )
	ROM_LOAD( "BUBBLESR.12B", 0xf000, 0x1000, 0xfcb24da2 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "BUBBLES.SND", 0xf000, 0x1000, 0x82ae5994 )
ROM_END


struct GameDriver bubbles_driver =
{
	__FILE__,
	0,
	"bubbles",
	"Bubbles",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&bubbles_machine_driver,        /* MachineDriver * */

	bubbles_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	bubbles_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver bubblesr_driver =
{
	__FILE__,
	&bubbles_driver,
	"bubblesr",
	"Bubbles (Solid Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&bubbles_machine_driver,        /* MachineDriver * */

	bubblesr_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	bubbles_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Stargate
 */

ROM_START( stargate_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "01", 0x0000, 0x1000, 0xe5fecedc )
	ROM_LOAD( "02", 0x1000, 0x1000, 0xbd525dec )
	ROM_LOAD( "03", 0x2000, 0x1000, 0xb5fed8d8 )
	ROM_LOAD( "04", 0x3000, 0x1000, 0x41ba0bc0 )
	ROM_LOAD( "05", 0x4000, 0x1000, 0xdde98c57 )
	ROM_LOAD( "06", 0x5000, 0x1000, 0x1b795abd )
	ROM_LOAD( "07", 0x6000, 0x1000, 0xe45af9ca )
	ROM_LOAD( "08", 0x7000, 0x1000, 0xa1026964 )
	ROM_LOAD( "09", 0x8000, 0x1000, 0xad03f4f1 )
	ROM_LOAD( "10", 0xd000, 0x1000, 0x001c6dec )
	ROM_LOAD( "11", 0xe000, 0x1000, 0xd33c4d64 )
	ROM_LOAD( "12", 0xf000, 0x1000, 0x94afba0b )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "sg.snd", 0xf800, 0x0800, 0xe18a8e66 )
ROM_END

struct GameDriver stargate_driver =
{
	__FILE__,
	0,
	"stargate",
	"Stargate",
	"1981",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&stargate_machine_driver,       /* MachineDriver * */

	stargate_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	stargate_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Defender
 */

ROM_START( defender_rom )
	ROM_REGION(0x14000)
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "defend.9", 0x10000, 0x0800, 0x3e2646ae )
	ROM_LOAD( "defend.12",0x10800, 0x0800, 0xd13eeb4a )
	ROM_LOAD( "defend.8", 0x11000, 0x0800, 0x67afa299 )
	ROM_LOAD( "defend.11",0x11800, 0x0800, 0x287572ed )
	ROM_LOAD( "defend.7", 0x12000, 0x0800, 0x344c9bd0 )
	ROM_LOAD( "defend.10",0x12800, 0x0800, 0xee30b06e )
	ROM_LOAD( "defend.6", 0x13000, 0x0800, 0x8c7b2da3 )
	ROM_LOAD( "defend.10",0x13800, 0x0800, 0xee30b06e )
	ROM_LOAD( "defend.1", 0xd000, 0x0800, 0x4aa8c614 )
	ROM_LOAD( "defend.4", 0xd800, 0x0800, 0x99e9bb31 )
	ROM_LOAD( "defend.2", 0xe000, 0x1000, 0x8991dceb )
	ROM_LOAD( "defend.3", 0xf000, 0x1000, 0x3f6e9fe2 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "defend.snd", 0xf800, 0x0800, 0xa7f601a4 )
ROM_END

struct GameDriver defender_driver =
{
	__FILE__,
	0,
	"defender",
	"Defender",
	"1980",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&defender_machine_driver,       /* MachineDriver * */

	defender_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};




/*
 *   Splat!
 */

ROM_START( splat_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "SPLAT.01", 0x0000, 0x1000, 0x43a37fff )
	ROM_LOAD( "SPLAT.02", 0x1000, 0x1000, 0x6cfb15f1 )
	ROM_LOAD( "SPLAT.03", 0x2000, 0x1000, 0xf55c6b64 )
	ROM_LOAD( "SPLAT.04", 0x3000, 0x1000, 0x1130f8ba )
	ROM_LOAD( "SPLAT.05", 0x4000, 0x1000, 0xb5e71503 )
	ROM_LOAD( "SPLAT.06", 0x5000, 0x1000, 0x8ceda6f7 )
	ROM_LOAD( "SPLAT.07", 0x6000, 0x1000, 0x1150f5f0 )
	ROM_LOAD( "SPLAT.08", 0x7000, 0x1000, 0x91cd6cd5 )
	ROM_LOAD( "SPLAT.09", 0x8000, 0x1000, 0xb78db4bf )
	ROM_LOAD( "SPLAT.10", 0xd000, 0x1000, 0x148a9cbc )
	ROM_LOAD( "SPLAT.11", 0xe000, 0x1000, 0x77ca0200 )
	ROM_LOAD( "SPLAT.12", 0xf000, 0x1000, 0x9f4667ba )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "SPLAT.SND", 0xf000, 0x1000, 0x999bcb87 )
ROM_END


struct GameDriver splat_driver =
{
	__FILE__,
	0,
	"splat",
	"Splat",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&splat_machine_driver,          /* MachineDriver * */

	splat_rom,                      /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	splat_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

  cmos_load, cmos_save
};




/*
 *   Blaster
 */

ROM_START( blaster_rom )
	ROM_REGION(0x3c000)
	ROM_LOAD( "blaster.11", 0x04000, 0x2000, 0xdc7831b2 )
	ROM_LOAD( "blaster.12", 0x06000, 0x2000, 0x68244eb6 )
	ROM_LOAD( "blaster.17", 0x08000, 0x1000, 0xecbf6a51 )
	ROM_LOAD( "blaster.16", 0x0d000, 0x1000, 0xd6e04ee2 )
	ROM_LOAD( "blaster.13", 0x0e000, 0x2000, 0x376fc541 )

	ROM_LOAD( "blaster.15", 0x00000, 0x4000, 0x1c345c06 )
	ROM_LOAD( "blaster.8",  0x10000, 0x4000, 0xc297d5ab )
	ROM_LOAD( "blaster.9",  0x14000, 0x4000, 0xe88478a0 )
	ROM_LOAD( "blaster.10", 0x18000, 0x4000, 0xc68a1386 )
	ROM_LOAD( "blaster.6",  0x1c000, 0x4000, 0x3142941e )
	ROM_LOAD( "blaster.5",  0x20000, 0x4000, 0x2ebd56e5 )
	ROM_LOAD( "blaster.14", 0x24000, 0x4000, 0xe0267262 )
	ROM_LOAD( "blaster.7",  0x28000, 0x4000, 0x17bff9a5 )
	ROM_LOAD( "blaster.1",  0x2c000, 0x4000, 0x37d9abe5 )
	ROM_LOAD( "blaster.2",  0x30000, 0x4000, 0xd99ff133 )
	ROM_LOAD( "blaster.4",  0x34000, 0x4000, 0x8d86011c )
	ROM_LOAD( "blaster.3",  0x38000, 0x4000, 0x86ddd013 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "blaster.18", 0xf000, 0x1000, 0x42c2fc80 )
ROM_END


struct GameDriver blaster_driver =
{
	__FILE__,
	0,
	"blaster",
	"Blaster",
	"1983",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&blaster_machine_driver,        /* MachineDriver * */

	blaster_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	blaster_input_ports,

	blaster_remap_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Colony 7
 */

ROM_START( colony7_rom )
	ROM_REGION(0x14000)
	ROM_LOAD( "cs03.bin", 0x0d000, 0x1000, 0xd3fcbf64 )
	ROM_LOAD( "cs02.bin", 0x0e000, 0x1000, 0x741ecfe4 )
	ROM_LOAD( "cs01.bin", 0x0f000, 0x1000, 0xdfb0181c )
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin", 0x10000, 0x0800, 0xb4d158c1 )
	ROM_LOAD( "cs04.bin", 0x10800, 0x0800, 0x69594a21 )
	ROM_LOAD( "cs07.bin", 0x11000, 0x0800, 0xc168b5da )
	ROM_LOAD( "cs05.bin", 0x11800, 0x0800, 0x49741fe0 )
	ROM_LOAD( "cs08.bin", 0x12000, 0x0800, 0x7838bc72 )
	ROM_RELOAD(           0x12800, 0x0800 )

	ROM_REGION(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin", 0xf800, 0x0800, 0x54ba49bc ) /* Sound ROM */
ROM_END

ROM_START( colony7a_rom )
	ROM_REGION(0x14000)
	ROM_LOAD( "cs03a.bin", 0x0d000, 0x1000, 0x124fcfb1 )
	ROM_LOAD( "cs02a.bin", 0x0e000, 0x1000, 0x9acb45f3 )
	ROM_LOAD( "cs01a.bin", 0x0f000, 0x1000, 0xe3508de4 )
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin",  0x10000, 0x0800, 0xb4d158c1 )
	ROM_LOAD( "cs04.bin",  0x10800, 0x0800, 0x69594a21 )
	ROM_LOAD( "cs07.bin",  0x11000, 0x0800, 0xc168b5da )
	ROM_LOAD( "cs05.bin",  0x11800, 0x0800, 0x49741fe0 )
	ROM_LOAD( "cs08.bin",  0x12000, 0x0800, 0x7838bc72 )
	ROM_RELOAD(            0x12800, 0x0800 )

	ROM_REGION(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin", 0xf800, 0x0800, 0x54ba49bc ) /* Sound ROM */
ROM_END

struct GameDriver colony7_driver =
{
	__FILE__,
	0,
	"colony7",
	"Colony 7 (set 1)",
	"1981",
	"Taito",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	0,
	&colony7_machine_driver,       /* MachineDriver * */

	colony7_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	colony7_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	defender_cmos_load, defender_cmos_save
};

struct GameDriver colony7a_driver =
{
	__FILE__,
	&colony7_driver,
	"colony7a",
	"Colony 7 (set 2)",
	"1981",
	"Taito",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	0,
	&colony7_machine_driver,       /* MachineDriver * */

	colony7a_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	colony7_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	defender_cmos_load, defender_cmos_save
};




/*
 *   Lotto Fun
 */

ROM_START( lottofun_rom )
	ROM_REGION(0x10000) 	/* 64k for code */
	ROM_LOAD( "vl4e.dat", 0x0000, 0x1000, 0x8c22de60 )
	ROM_LOAD( "vl4c.dat", 0x1000, 0x1000, 0x76ffdc39 )
	ROM_LOAD( "vl4a.dat", 0x2000, 0x1000, 0xaad204c2 )
	ROM_LOAD( "vl5e.dat", 0x3000, 0x1000, 0x8cd1bc8b )
	ROM_LOAD( "vl5c.dat", 0x4000, 0x1000, 0x61ca8076 )
	ROM_LOAD( "vl5a.dat", 0x5000, 0x1000, 0x52705d22 )
	ROM_LOAD( "vl6e.dat", 0x6000, 0x1000, 0xc8f2a4d6 )
	ROM_LOAD( "vl6c.dat", 0x7000, 0x1000, 0x80f34965 )
	ROM_LOAD( "vl6a.dat", 0x8000, 0x1000, 0x44305588 )
	ROM_LOAD( "vl7a.dat", 0xd000, 0x1000, 0x041338f9 )
	ROM_LOAD( "vl7c.dat", 0xe000, 0x1000, 0x864747f3 )
	ROM_LOAD( "vl7e.dat", 0xf000, 0x1000, 0x820df961 )

	ROM_REGION(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) 	/* 64k for the sound CPU */
	ROM_LOAD( "vl2532.snd", 0xf000, 0x1000, 0x79b62d96 )
ROM_END



struct GameDriver lottofun_driver =
{
	__FILE__,
	0,
	"lottofun",
	"Lotto Fun",
	"1987",
	"H.A.R. Management",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour (Ticket Dispenser)",
	0,
	&lottofun_machine_driver,		/* MachineDriver * */

	lottofun_rom,					 /* RomModule * */
	0, 0,							/* ROM decrypt routines */
	0,								/* samplenames */
	0,		/* sound_prom */

	lottofun_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};
