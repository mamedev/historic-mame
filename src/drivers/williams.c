/***************************************************************************

Do to:  Mystic Marathon
Not sure of the board: Turkey shoot, Joust 2


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
C980 Blaster only: Select witch ROM is at 0000-3FFF
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
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"


/**** from machine/williams.h ****/
extern unsigned char *williams_port_select;
extern unsigned char *williams_video_counter;
extern unsigned char *williams_bank_base;
void williams_init_machine(void);
int williams_interrupt(void);
void williams_vram_select_w(int offset,int data);
int williams_input_port_0_1(int offset);
int williams_input_port_2_3(int offset);

extern unsigned char *robotron_catch;
int robotron_catch_loop_r(int offset); /* JB 970823 */

extern unsigned char *stargate_catch;
int stargate_interrupt(void);
int stargate_catch_loop_r(int offset); /* JB 970823 */
int stargate_input_port_0_r(int offset);


extern unsigned char *defender_irq_enable;
extern unsigned char *defender_catch;
extern unsigned char *defender_bank_base;
int defender_interrupt(void);
int defender_bank_r(int offset);
void defender_bank_w(int offset,int data);
void defender_bank_select_w(int offset,int data);
int defender_catch_loop_r(int offset); /* JB 970823 */

extern unsigned char *blaster_color_zero_table;
extern unsigned char *blaster_color_zero_flags;

extern unsigned char *blaster_catch;
extern unsigned char *blaster_bank2_base;
int blaster_catch_loop_r(int offset); /* JB 970823 */
void blaster_bank_select_w(int offset,int data);
void blaster_vram_select_w(int offset,int data);
int blaster_input_port_0(int offset);
int sinistar_input_port_0(int offset);

extern unsigned char *splat_catch;
int splat_catch_loop_r(int offset); /* JB 970823 */


/**** from sndhrdw/williams.h ****/
extern unsigned char *williams_dac;
void williams_sh_w(int offset,int data);
void williams_digital_out_w(int offset,int data);
int williams_sh_start(void);
void williams_sh_stop(void);
void williams_sh_update(void);


/**** from vidhrdw/williams.h ****/
extern unsigned char *williams_paletteram;
extern unsigned char *williams_blitterram;
extern int williams_paletteram_size;
void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int williams_vh_start(void);
void williams_vh_stop(void);
void williams_vh_screenrefresh(struct osd_bitmap *bitmap);
int williams_videoram_r(int offset);
void williams_palette_w(int offset,int data);
void williams_videoram_w(int offset,int data);
void williams_blitter_w(int offset,int data);

extern unsigned char *blaster_remap_select;
extern unsigned char *blaster_video_bits;
int blaster_videoram_r(int offset);
void blaster_blitter_w(int offset,int data);
void blaster_vh_screenrefresh(struct osd_bitmap *bitmap);
void blaster_vh_convert_color_prom (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void splat_blitter_w(int offset,int data);

void defender_videoram_w(int offset,int data);



/*
 *   Read mem for Robotron Joust Stargate Bubbles
 */

static struct MemoryReadAddress robotron_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9810, 0x9810, robotron_catch_loop_r, &robotron_catch }, /* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress joust_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, williams_input_port_0_1 }, /* IN0-1 */
	{ 0xc806, 0xc806, input_port_2_r },          /* IN2 */
	{ 0xc80c, 0xc80c, input_port_3_r },          /* IN3 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress stargate_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9c39, 0x9c39, stargate_catch_loop_r, &stargate_catch }, /* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, stargate_input_port_0_r }, /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bubbles_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Robotron Joust Stargate Bubbles
 */

static struct MemoryWriteAddress williams_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, williams_palette_w, &williams_paletteram, &williams_paletteram_size },
	{ 0xc807, 0xc807, MWA_RAM, &williams_port_select },
	{ 0xc80e, 0xc80e, williams_sh_w },                            /* Sound */
	{ 0xc900, 0xc900, williams_vram_select_w },           /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, MWA_NOP },                                  /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                  /* CMOS                      */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Splat
 */

static struct MemoryReadAddress splat_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x984b, 0x984b, splat_catch_loop_r, &splat_catch },    /* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, williams_input_port_0_1 },           /* IN0-1 */
	{ 0xc806, 0xc806, williams_input_port_2_3 },           /* IN2-3*/
	{ 0xc80c, 0xc80c, input_port_4_r },                    /* IN4 */
	{ 0xc80e, 0xc80e, MRA_RAM },                           /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                           /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Splat
 */

static struct MemoryWriteAddress splat_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, williams_palette_w, &williams_paletteram, &williams_paletteram_size },
	{ 0xc807, 0xc807, MWA_RAM, &williams_port_select },
	{ 0xc80e, 0xc80e, williams_sh_w},                           /* Sound */
	{ 0xc900, 0xc900, williams_vram_select_w },         /* bank  */
	{ 0xca00, 0xca07, splat_blitter_w, &williams_blitterram },  /* blitter */
	{ 0xcbff, 0xcbff, MWA_NOP },                                /* WatchDog     */
	{ 0xcc00, 0xcfff, MWA_RAM },                                /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Sinistar
 */

static struct MemoryReadAddress sinistar_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, sinistar_input_port_0 },    /* IN0 Special joystick */
	{ 0xc806, 0xc806, input_port_1_r },          /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xdfff, MRA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Sinistar
 */

static struct MemoryWriteAddress sinistar_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, williams_palette_w, &williams_paletteram, &williams_paletteram_size },
	{ 0xc80e, 0xc80e, williams_sh_w },                            /* Sound */
	{ 0xc900, 0xc900, williams_vram_select_w },             /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram },   /* blitter */
	{ 0xcbff, 0xcbff, MWA_NOP },                                    /* WatchDog     */
	{ 0xcc00, 0xcfff, MWA_RAM },                                    /* CMOS         */
	{ 0xd000, 0xdfff, MWA_RAM },                                    /* for Sinistar */
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Blaster
 */

static struct MemoryReadAddress blaster_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1, &williams_bank_base },
	{ 0x4000, 0x96ff, MRA_BANK2, &blaster_bank2_base },
/*      { 0x9700, 0x9700, blaster_catch_loop_r, &blaster_catch },*/             /* JB 970823 */
	{ 0x9700, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, blaster_input_port_0 },    /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },          /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS      */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Blaster
 */

static struct MemoryWriteAddress blaster_writemem[] =
{
	{ 0x0000, 0x96ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9700, 0xbaff, MWA_RAM },
	{ 0xbb00, 0xbbff, MWA_RAM, &blaster_color_zero_table },     /* Color 0 for each line */
	{ 0xbc00, 0xbcff, MWA_RAM, &blaster_color_zero_flags },     /* Color 0 flags, latch color only if bit 0 = 1 */
	{ 0xbd00, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, williams_palette_w, &williams_paletteram, &williams_paletteram_size },
	{ 0xc80e, 0xc80e, williams_sh_w},                            /* Sound */
	{ 0xc900, 0xc900, blaster_vram_select_w },          			 /* VRAM bank  */
	{ 0xc940, 0xc940, MWA_RAM, &blaster_remap_select },          /* select remap colors in Remap prom */
	{ 0xc980, 0xc980, blaster_bank_select_w },                   /* Bank Select */
	{ 0xc9C0, 0xc9C0, MWA_RAM, &blaster_video_bits },            /* Video related bits */
	{ 0xca00, 0xca07, blaster_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, MWA_NOP },                                 /* WatchDog     */
	{ 0xCC00, 0xCFFF, MWA_RAM },                                 /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Defender
 */

static struct MemoryReadAddress defender_readmem[] =
{
	{ 0xa05d, 0xa05d, defender_catch_loop_r, &defender_catch }, /* JB 970823 */
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },          /* i/o / rom */
	{ 0xcc07, 0xcc07, MRA_BANK1, &defender_irq_enable },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Defender
 */

static struct MemoryWriteAddress defender_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &williams_paletteram, &williams_paletteram_size },   /* here only to initialize the pointers */
	{ 0xd000, 0xd000, defender_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};




/* memory handlers for the audio CPU */
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0400, 0x0400, MRA_RAM },
	{ 0x0402, 0x0402, soundlatch_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0400, 0x0400, williams_digital_out_w, &williams_dac },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};





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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
INPUT_PORTS_END


/*
 *   Joust buttons
 */

INPUT_PORTS_START( joust_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

/*Not really a hardware port, this is just to map the buttons */
	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
INPUT_PORTS_END


/*
 *   Stargate buttons
 */

INPUT_PORTS_START( stargate_input_ports )
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
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON5, "Inviso", OSD_KEY_SPACE, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)

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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", OSD_KEY_COLON, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust", OSD_KEY_L, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Smart Bomb", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4, "Hyperspace", OSD_KEY_N, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON6, "Reverse", OSD_KEY_ALT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY, "Move Down", OSD_KEY_S, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY, "Move Up", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
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

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER1, "Walk Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER1, "Walk Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER1, "Walk Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Walk Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
/*      PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )*/
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER1, "Throw Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER1, "Throw Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER2, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER1, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
INPUT_PORTS_END


/*
 *   Blaster buttons
 */

INPUT_PORTS_START( blaster_input_ports )
	PORT_START      /* IN0 */
/* Not really the hardware bits, see blaster_input_port_0() */
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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_5, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_6, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
INPUT_PORTS_END


/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the background color table. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	4,      /* 4 bits per pixel */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0, &fakelayout, 0, 1 },
	{ -1 } /* end of array */
};



static struct MachineDriver robotron_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			robotron_readmem,       /* MemoryReadAddress */ /* JB 970823 */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


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
			4                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};



static struct MachineDriver stargate_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			stargate_readmem,       /* MemoryReadAddress */ /* JB 970823 */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			stargate_interrupt,     /* interrupt routine */
			2                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
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
			1                       /* interrupts per frame (should be 4ms but?) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


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
			defender_interrupt,     /* interrupt routine */
			2                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


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
			4                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


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
			4                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


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
			4                       /* interrupts per frame (should be 4ms) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60,                                     /* frames per second */
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	256,                                  /* total colors */
	16,                                      /* color table length */
	blaster_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	blaster_vh_screenrefresh,               /* vh_update routine */

	/* sound hardware */
	0,                                      /* sh_init routine */
	williams_sh_start,                      /* sh_start routine */
	williams_sh_stop,                       /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};



/***************************************************************************

  High score/CMOS save/load

***************************************************************************/

static int cmos_load(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}
	else
	{
		struct DisplayText dt[2];
		dt[0].text =
      "THIS IS THE FIRST TIME YOU START THIS GAME\n"
      "\n"
      "AFTER THE RAM TEST THE GAME WILL\n"
      "DISPLAY  FACTORY SETTINGS RESTORED\n"
      "IT WILL WAIT FOR YOU TO PRESS\n"
      "THE ADVANCE BUTTON THAT IS KEY 6\n"
      "\n"
      "PRESS THIS BUTTON LATER TO GO TO\n"
      "THE TESTS AND OPERATOR MENU\n"
      "\n"
      "TO SKIP THE TESTS HOLD THE AUTO UP\n"
      "BUTTON BEFORE PRESSING ADVANCE\n"
      "AUTO UP BUTTON IS KEY 5\n"
      "\n"
      "\n"
			"PRESS ADVANCE TO CONTINUE\n";

		dt[0].color = DT_COLOR_RED;
		dt[0].x = 10;
		dt[0].y = 20;
		dt[1].text = 0;
		displaytext (dt, 0);

		while (!osd_key_pressed (OSD_KEY_6));    /* wait for key press */
		while (osd_key_pressed (OSD_KEY_6));     /* wait for key release */
	}

	return 1;
}

static void cmos_save(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}
}


static int defender_cmos_load(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}
	else
	{
		struct DisplayText dt[2];
		dt[0].text =
      "THIS IS THE FIRST TIME YOU START DEFENDER\n"
      "\n"
      "AFTER THE RAM TEST THE GAME WILL\n"
      "GO IN THE OPERATOR MENU\n"
      "THIS IS NORMAL AND WILL OCCUR\n"
      "ONLY THIS TIME\n"
      "WHILE IN THE MENU IF AUTO UP\n"
      "BUTTON IS RELEASED YOU WILL\n"
      "DECREASE VALUES HOLD IT PRESSED\n"
      "TO INCREASE VALUES\n"
      "TO EXIT THE MENU YOU HAVE TO\n"
      "PASS FROM LINE 28 TO LINE 0\n"
      "SO HOLD AUTO UP AND PRESS ADVANCE\n"
      "MANY TIMES\n"
      "THE AUTO UP BUTTON IS KEY 5\n"
      "THE ADVANCE BUTTON IS KEY 6\n"
      "\n"
      "PRESS ADVANCE BUTTON LATER TO GO\n"
      "TO THE TESTS AND OPERATOR MENU\n"
      "\n"
      "TO SKIP THE TESTS HOLD THE AUTO UP\n"
      "BUTTON BEFORE PRESSING ADVANCE\n"
      "\n"
      "\n"
			"   PRESS ADVANCE TO CONTINUE\n";

		dt[0].color = DT_COLOR_RED;
		dt[0].x = 10;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext (dt, 0);

		while (!osd_key_pressed (OSD_KEY_6));    /* wait for key press */
		while (osd_key_pressed (OSD_KEY_6));     /* wait for key release */
	}

	return 1;
}

static void defender_cmos_save(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

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
	ROM_LOAD( "ROBOTRON.SBa", 0xd000, 0x1000, 0x17b8fc1e )
	ROM_LOAD( "ROBOTRON.SBb", 0xe000, 0x1000, 0xe816f8e6 )
	ROM_LOAD( "ROBOTRON.SBc", 0xf000, 0x1000, 0xcfc2d9aa )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed bacause the main */
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
	ROM_LOAD( "ROBOTRON.YOa", 0xd000, 0x1000, 0x1ac7fceb )
	ROM_LOAD( "ROBOTRON.YOb", 0xe000, 0x1000, 0xe615fee7 )
	ROM_LOAD( "ROBOTRON.YOc", 0xf000, 0x1000, 0xd4bdec59 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "ROBOTRON.SND", 0xf000, 0x1000, 0x000f85e5 )
ROM_END


struct GameDriver robotron_driver =
{
	"Robotron",
	"robotron",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&robotron_machine_driver,       /* MachineDriver * */

	robotron_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,robotron_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver robotryo_driver =
{
	"Robotron (Yellow/Orange label)",
	"robotryo",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	&robotron_machine_driver,       /* MachineDriver * */

	robotryo_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,robotron_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

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
	/* empty memory region - not used by the game, but needed bacause the main */
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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "JOUST.SND", 0xf000, 0x1000, 0x5799e563 )
ROM_END

ROM_START( joustg_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "JOUST.SG1", 0x0000, 0x1000, 0x56ea72c8 )
	ROM_LOAD( "JOUST.SG2", 0x1000, 0x1000, 0xe32af9c4 )
	ROM_LOAD( "JOUST.SG3", 0x2000, 0x1000, 0x21ec9c8e )
	ROM_LOAD( "JOUST.SG4", 0x3000, 0x1000, 0xad230361 )
	ROM_LOAD( "JOUST.SG5", 0x4000, 0x1000, 0xd37f04e9 )
	ROM_LOAD( "JOUST.SG6", 0x5000, 0x1000, 0x727b5c05 )
	ROM_LOAD( "JOUST.SG7", 0x6000, 0x1000, 0x81aa3756 )
	ROM_LOAD( "JOUST.SG8", 0x7000, 0x1000, 0x8d1829b6 )
	ROM_LOAD( "JOUST.SG9", 0x8000, 0x1000, 0xcbfcd9a6 )
	ROM_LOAD( "JOUST.SGa", 0xd000, 0x1000, 0xf102016a )
	ROM_LOAD( "JOUST.SGb", 0xe000, 0x1000, 0x11b3700d )
	ROM_LOAD( "JOUST.SGc", 0xf000, 0x1000, 0x0cd46bb8 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed bacause the main */
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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "JOUST.SND", 0xf000, 0x1000, 0x5799e563 )
ROM_END


struct GameDriver joust_driver =
{
	"Joust (White/Green label)",
	"joust",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	&joust_machine_driver,          /* MachineDriver * */

	joust_rom,                      /* White/Green version, latest */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,joust_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustr_driver =
{
	"Joust (Red label)",
	"joustr",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&joust_machine_driver,          /* MachineDriver * */

	joustr_rom,                     /* Solid Red version, has pterodactyl bug */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,joust_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustg_driver =
{
	"Joust (Green label)",
	"joustg",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&joust_machine_driver,          /* MachineDriver * */

	joustg_rom,                     /* Solid Red version, has pterodactyl bug */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,joust_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustwr_driver =
{
	"Joust (White/Red label)",
	"joustwr",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&joust_machine_driver,          /* MachineDriver * */

	joustwr_rom,                     /* Solid Red version, has pterodactyl bug */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,joust_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "SINISTAR.SND", 0xf000, 0x1000, 0x7400ae74 )
ROM_END

struct GameDriver sinistar_driver =
{
	"Sinistar",
	"sinistar",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&sinistar_machine_driver,       /* MachineDriver * */

	sinistar_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,sinistar_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	cmos_load, cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "BUBBLES.SND", 0xf000, 0x1000, 0x82ae5994 )
ROM_END

ROM_START( bubblesr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "BUBBLES.1B",  0x0000, 0x1000, 0x90409106 )
	ROM_LOAD( "BUBBLES.2B",  0x1000, 0x1000, 0x2eae0950 )
	ROM_LOAD( "BUBBLES.3B",  0x2000, 0x1000, 0xf6ad9f95 )
	ROM_LOAD( "BUBBLES.4B",  0x3000, 0x1000, 0x945d4b73 )
	ROM_LOAD( "BUBBLES.5B",  0x4000, 0x1000, 0xb002076e )
	ROM_LOAD( "BUBBLES.6B",  0x5000, 0x1000, 0x4617e675 )
	ROM_LOAD( "BUBBLES.7B",  0x6000, 0x1000, 0x16d20c98 )
	ROM_LOAD( "BUBBLES.8B",  0x7000, 0x1000, 0xfb8bad59 )
	ROM_LOAD( "BUBBLES.9B",  0x8000, 0x1000, 0xc5ca8ce2 )
	ROM_LOAD( "BUBBLES.10B", 0xd000, 0x1000, 0x8cc42496 )
	ROM_LOAD( "BUBBLES.11B", 0xe000, 0x1000, 0xfd0252a2 )
	ROM_LOAD( "BUBBLES.12B", 0xf000, 0x1000, 0xfcb24da2 )

	ROM_REGION(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "BUBBLES.SND", 0xf000, 0x1000, 0x82ae5994 )
ROM_END


struct GameDriver bubbles_driver =
{
	"Bubbles",
	"bubbles",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&bubbles_machine_driver,        /* MachineDriver * */

	bubbles_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,bubbles_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver bubblesr_driver =
{
	"Bubbles (Red Label)",
	"bubblesr",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	&bubbles_machine_driver,        /* MachineDriver * */

	bubblesr_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,bubbles_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "sg.snd", 0xf800, 0x0800, 0xe18a8e66 )
ROM_END

struct GameDriver stargate_driver =
{
	"Stargate",
	"stargate",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&stargate_machine_driver,       /* MachineDriver * */

	stargate_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,stargate_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "defend.snd", 0xf800, 0x0800, 0xa7f601a4 )
ROM_END

struct GameDriver defender_driver =
{
	"Defender",
	"defender",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&defender_machine_driver,       /* MachineDriver * */

	defender_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,defender_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "SPLAT.SND", 0xf000, 0x1000, 0x999bcb87 )
ROM_END


struct GameDriver splat_driver =
{
	"Splat",
	"splat",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&splat_machine_driver,          /* MachineDriver * */

	splat_rom,                      /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,splat_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

  cmos_load, cmos_save
};




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
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "blaster.18", 0xf000, 0x1000, 0x42c2fc80 )
ROM_END


struct GameDriver blaster_driver =
{
	"Blaster",
	"blaster",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	&blaster_machine_driver,        /* MachineDriver * */

	blaster_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	0/*TBR*/,blaster_input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


