/*
 * The normal height for a Williams game is 256, but not all the
 * lines are used. 240 lines is just enough and more compatible
 * with the PC resolutions
 * Dont forget HEIGHT_240 in VIDHRDW\WILLIAMS.C
*/

#define HEIGHT_240

/***************************************************************************

NOTE3: Blaster is not finished, when entering a zone in the game where
       the display is not erased, press '=' to toggle the 'erase each frame'
       mode.
       Bubble do not start, I am working on it.
       Sinistar start but do weird thing when he try to display the game
       screen, I am about to give up this game, I do not know what to do
       anymore.

Do to:  Mystic Marathon
Not sure of the board: Turkey shoot, Joust 2


------- Blaster Bubbles Joust Robotron Sinistar Splat Stargate -------------

0000-8FFF ROM   (for Blaster, 0000-3FFF is a bank of 12 ROMs)
0000-97FF Video  RAM Bank switched with ROM (96FF for Blaster)
9800-BFFF RAM
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
C9C0 Blaster only:?


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
#include "M6809.h"

void williams_sh_w(int offset,int data);
void williams_sh_update(void);
void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

void williams_init_machine(void);
void williams_nofastop_init_machine(void);

extern unsigned char *robotron_videoram;
void williams_videoram_w(int offset,int data);
void blaster_videoram_w(int offset,int data);
int williams_videoram_r(int offset);
int blaster_videoram_r(int offset);
void williams_StartBlitter(int offset,int data);
void williams_StartBlitter2(int offset,int data);
void williams_BlasterStartBlitter(int offset,int data);

int williams_vh_start(void);
void williams_vh_stop(void);
void williams_vh_screenrefresh(struct osd_bitmap *bitmap);
void blaster_vh_screenrefresh(struct osd_bitmap *bitmap);
void Williams_Palette_w(int offset,int data);

int defender_catch_loop_r(int offset); /* JB 970823 */
int stargate_catch_loop_r(int offset); /* JB 970823 */
int joust_catch_loop_r(int offset); /* JB 970823 */
int robotron_catch_loop_r(int offset); /* JB 970823 */
int blaster_catch_loop_r(int offset); /* JB 970823 */
int splat_catch_loop_r(int offset); /* JB 970823 */

int defender_bank_r(int offset);
void defender_bank_w(int offset,int data);
void defender_bank_select_w(int offset,int data);
void defender_videoram_w(int offset,int data);

void blaster_bank_select_w(int offset,int data);
int Williams_Interrupt(void);
int Stargate_Interrupt(void);
int Defender_Interrupt(void);

int video_counter_r(int offset);
int test_r(int offset);
void test_w(int offset,int data);
int input_port_0_1(int offset);
int input_port_2_3(int offset);
int blaster_input_port_0(int offset);


/*	JB 970823 - separate Stargate for busy loop optimization
 *   Read mem for Stargate
 */

static struct MemoryReadAddress stargate_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9c39, 0x9c39, stargate_catch_loop_r }, /* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*	JB 970823 - separate Robotron for busy loop optimization
 *   Read mem for Robotron
 */

static struct MemoryReadAddress robotron_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9810, 0x9810, robotron_catch_loop_r }, /* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

/*
 *   Read mem for [Robotron] [Stargate] Bubbles Splat
 */

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Robotron Joust Stargate Bubbles
 */

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, Williams_Palette_w },
	{ 0xc80e, 0xc80e, williams_sh_w},            /* Sound */
	{ 0xc900, 0xc900, MWA_RAM },                 /* bank  */
	{ 0xca00, 0xca00, williams_StartBlitter },   /* StartBlitter */
	{ 0xca01, 0xca07, MWA_RAM },                 /* Blitter      */
	{ 0xcbff, 0xcbff, MWA_NOP },                 /* WatchDog (have to be $39) */
	{ 0xCC00, 0xCFFF, MWA_RAM },                 /* CMOS                      */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Joust
 */

static struct MemoryReadAddress joust_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_1 },          /* IN0-1 */
	{ 0xc806, 0xc806, input_port_2_r },          /* IN2 */
	{ 0xc80c, 0xc80c, input_port_3_r },          /* IN3 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
//	{ 0xe0f2, 0xe0f2, joust_catch_loop_r },		/* JB 970823 */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Splat
 */

static struct MemoryReadAddress splat_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x984b, 0x984b, splat_catch_loop_r },		/* JB 970823 */
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_1 },          /* IN0-1 */
	{ 0xc806, 0xc806, input_port_2_3 },          /* IN2-3*/
	{ 0xc80c, 0xc80c, input_port_4_r },          /* IN4 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Splat
 */

static struct MemoryWriteAddress splat_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, Williams_Palette_w },
	{ 0xc80e, 0xc80e, williams_sh_w},            /* Sound */
	{ 0xc900, 0xc900, MWA_RAM },                 /* bank  */
	{ 0xca00, 0xca00, williams_StartBlitter2 },  /* StartBlitter */
	{ 0xca01, 0xca07, MWA_RAM },                 /* Blitter      */
	{ 0xcbff, 0xcbff, MWA_NOP },                 /* WatchDog     */
	{ 0xCC00, 0xCFFF, MWA_RAM },                 /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Sinistar
 */

static struct MemoryReadAddress sinistar_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },          /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },          /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xdfff, MRA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Sinistar
 */

static struct MemoryWriteAddress sinistar_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, Williams_Palette_w },
	{ 0xc900, 0xc900, MWA_RAM },                 /* bank         */
	{ 0xca00, 0xca00, williams_StartBlitter },   /* StartBlitter */
	{ 0xca01, 0xca07, MWA_RAM },                 /* Blitter      */
	{ 0xcbff, 0xcbff, MWA_NOP },                 /* WatchDog     */
	{ 0xCC00, 0xCFFF, MWA_RAM },                 /* CMOS         */
	{ 0xd000, 0xdfff, MWA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Blaster
 */

static struct MemoryReadAddress blaster_readmem[] =
{
	{ 0x0000, 0x96ff, blaster_videoram_r },
//	{ 0x9700, 0x9700, blaster_catch_loop_r },		/* JB 970823 */
	{ 0x9700, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, blaster_input_port_0 },    /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },          /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS      */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Blaster
 */

static struct MemoryWriteAddress blaster_writemem[] =
{
	{ 0x0000, 0x96ff, blaster_videoram_w },
	{ 0x9700, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, Williams_Palette_w },
	{ 0xc80e, 0xc80e, williams_sh_w},                 /* Sound */
	{ 0xc900, 0xc900, MWA_RAM },                      /* bank  */
	{ 0xc940, 0xc940, MWA_RAM },                      /* select remap colors in Remap prom */
	{ 0xc980, 0xc980, blaster_bank_select_w },        /* Bank Select */
/*        { 0xc9c0, 0xc9c0, MWA_RAM }, */
	{ 0xca00, 0xca00, williams_BlasterStartBlitter }, /* StartBlitter */
	{ 0xca01, 0xca07, MWA_RAM },                      /* Blitter      */
	{ 0xcbff, 0xcbff, MWA_NOP },                      /* WatchDog     */
	{ 0xCC00, 0xCFFF, MWA_RAM },                      /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read mem for Defender
 */

static struct MemoryReadAddress defender_readmem[] =
{
	{ 0xa05d, 0xa05d, defender_catch_loop_r }, /* JB 970823 */
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, defender_bank_r },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Write mem for Defender
 */

static struct MemoryWriteAddress defender_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, defender_bank_w },
	{ 0xd000, 0xd000, defender_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};




/* Dip switch for all games
    -There is no Dip switches on the Williams games
      but this has to be here because
      the program will bug when TAB is pressed */

static struct DSW williams_dsw[] =
{
	{ 2, 0x80, "NO DIP SWITCHES", { "" } },
	{ -1 }
};



/*
 *   Robotron buttons
 */

static struct InputPort robotron_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_DOWN },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE3 }      /* V.V */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0,0,0 },
		{ OSD_JOY_FIRE1, OSD_JOY_FIRE4, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{ -1 }  /* end of table */
};


static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet robotron_keys[] =
{
  { 0, 0, "MOVE UP" },
  { 0, 1, "MOVE DOWN" },
  { 0, 2, "MOVE LEFT" },
  { 0, 3, "MOVE RIGHT" },
  { 0, 6, "FIRE UP" },
  { 0, 7, "FIRE DOWN" },
  { 1, 0, "FIRE LEFT" },
  { 1, 1, "FIRE RIGHT" },
/*
  { 0, 4, "1 PLAYER" },
  { 0, 5, "2 PLAYERS" },
  { 2, 2, "COIN IN" },
*/
  { 2, 0, "AUTO UP" },
  { 2, 1, "ADVANCE" },
  { 2, 3, "HIGHSCORE RESET" },
  { -1 }
};



/*
 *   Joust buttons
 */

static struct InputPort joust_input_ports[] =
{
	{       /* IN0 Player 2 */
		0x00,   /* default_value */
		{ OSD_KEY_F, OSD_KEY_G, OSD_KEY_S, 0, OSD_KEY_2, OSD_KEY_1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN0 Player 1 */
		0x00,   /* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, 0, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},

	{ -1 }  /* end of table */
};

static struct KEYSet joust_keys[] =
{
  { 1, 0, "PLAYER 1 MOVE LEFT" },
  { 1, 1, "PLAYER 1 MOVE RIGHT" },
  { 1, 2, "PLAYER 1 FLAP" },
  { 0, 0, "PLAYER 2 MOVE LEFT" },
  { 0, 1, "PLAYER 2 MOVE RIGHT" },
  { 0, 2, "PLAYER 2 FLAP" },
/*
  { 0, 5, "1 PLAYER" },
  { 0, 4, "2 PLAYERS" },
  { 3, 2, "COIN IN" },
*/
  { 3, 0, "AUTO UP" },
  { 3, 1, "ADVANCE" },
  { 3, 3, "HIGHSCORE RESET" },
  { -1 }
};


/*
 *   Stargate buttons
 */

static struct InputPort stargate_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_X, OSD_KEY_C, OSD_KEY_2, OSD_KEY_1, OSD_KEY_LEFT, OSD_KEY_DOWN },
		{ OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_FIRE3, OSD_JOY_FIRE4, 0, 0, OSD_JOY_LEFT, OSD_JOY_DOWN }      /* V.V */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_UP, OSD_KEY_V, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_FIRE2, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},

	{ -1 }  /* end of table */
};

static struct KEYSet stargate_keys[] =
{
  { 1, 0, "MOVE UP" },
  { 0, 7, "MOVE DOWN" },
  { 0, 6, "REVERSE" },
  { 0, 0, "FIRE" },
  { 0, 1, "THRUST" },
  { 0, 2, "SMART BOMB" },
  { 1, 1, "INVISO" },
  { 0, 3, "HYPERSPACE" },
/*
  { 0, 5, "1 PLAYER" },
  { 0, 4, "2 PLAYERS" },
  { 2, 2, "COIN IN" },
*/
  { 2, 0, "AUTO UP" },
  { 2, 1, "ADVANCE" },
  { 2, 3, "HIGHSCORE RESET" },
  { -1 }
};



/*
 *   Defender buttons
 */

static struct InputPort defender_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_X, OSD_KEY_C, OSD_KEY_2, OSD_KEY_1, OSD_KEY_LEFT, OSD_KEY_DOWN },
		{ OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_FIRE2, OSD_JOY_FIRE3, 0, 0, OSD_JOY_LEFT, OSD_JOY_DOWN }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_UP, 0,0,0,0,0,0,0 },
		{ OSD_JOY_UP, 0, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},

	{ -1 }  /* end of table */
};

static struct KEYSet defender_keys[] =
{
  { 2, 0, "MOVE UP" },
  { 1, 7, "MOVE DOWN" },
  { 1, 6, "REVERSE" },
  { 1, 0, "FIRE" },
  { 1, 1, "THRUST" },
  { 1, 2, "SMART BOMB" },
  { 1, 3, "HYPERSPACE" },
/*
  { 1, 5, "1 PLAYER" },
  { 1, 4, "2 PLAYERS" },
  { 0, 2, "COIN IN" },
*/
  { 0, 0, "AUTO UP" },
  { 0, 1, "ADVANCE" },
  { 0, 3, "HIGHSCORE RESET" },
  { -1 }
};



/*
 *   Sinistar buttons
 */

static struct InputPort sinistar_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},

	{ -1 }  /* end of table */
};



/*
 *   Bubbles buttons
 */

static struct InputPort bubbles_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},
	{ -1 }  /* end of table */
};



/*
 *   Splat buttons
 */

static struct InputPort splat_input_ports[] =
{
	{       /* IN0 player 2*/
		0x00,   /* default_value */
		{ OSD_KEY_I, OSD_KEY_K, OSD_KEY_J, OSD_KEY_L, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_DOWN },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN0 player 1*/
		0x00,   /* default_value */
		{ OSD_KEY_W, OSD_KEY_S, OSD_KEY_A, OSD_KEY_D, 0, 0, OSD_KEY_T, OSD_KEY_G },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE3 }
	},
	{       /* IN1 player 2*/
		0x00,   /* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0,0,0 },
		 { 0, 0, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /*  IN1 player1*/
		0x00,  /*  default_value */
		{ OSD_KEY_F, OSD_KEY_H, 0 ,0, 0, 0, 0, 0 },
		{ OSD_JOY_FIRE1, OSD_JOY_FIRE4, 0, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},

	{ -1 }  /* end of table */
};

static struct KEYSet splat_keys[] =
{
  { 1, 0, "PL1 WALK UP" },
  { 1, 1, "PL1 WALK DOWN" },
  { 1, 2, "PL1 WALK LEFT" },
  { 1, 3, "PL1 WALK RIGHT" },
  { 1, 6, "PL1 THROW UP" },
  { 1, 7, "PL1 THROW DOWN" },
  { 3, 0, "PL1 THROW LEFT" },
  { 3, 1, "PL1 THROW RIGHT" },
  { 0, 0, "PL2 WALK UP" },
  { 0, 1, "PL2 WALK DOWN" },
  { 0, 2, "PL2 WALK LEFT" },
  { 0, 3, "PL2 WALK RIGHT" },
  { 0, 6, "PL2 THROW UP" },
  { 0, 7, "PL2 THROW DOWN" },
  { 2, 0, "PL2 THROW LEFT" },
  { 2, 1, "PL2 THROW RIGHT" },
/*
  { 1, 4, "1 PLAYER" },
  { 1, 5, "2 PLAYERS" },
  { 4, 2, "COIN IN" },
*/
  { 4, 0, "AUTO UP" },
  { 4, 1, "ADVANCE" },
  { 4, 3, "HIGHSCORE RESET" },
  { -1 }
};


/*
 *   Blaster buttons
 */

static struct InputPort blaster_input_ports[] =
{
	{       /* IN0 */
		0x00,   /* default_value */
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN1 */
		0x00,   /* default_value */
		{ OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_Z, 0, OSD_KEY_1, OSD_KEY_2, 0,0 },
		{ OSD_JOY_FIRE2, OSD_JOY_FIRE1, OSD_JOY_FIRE3, 0, 0, 0, 0, 0 }      /* V.V */
	},
	{       /* IN2 */
		0x00,   /* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }      /* not affected by joystick */
	},

	{ -1 }  /* end of table */
};

static struct KEYSet blaster_keys[] =
{
  { 0, 0, "MOVE UP" },
  { 0, 1, "MOVE DOWN" },
  { 0, 2, "MOVE LEFT" },
  { 0, 3, "MOVE RIGHT" },
  { 1, 1, "BLAST" },
  { 1, 0, "THRUST PANEL" },
  { 1, 2, "THRUST JOYSTICK" },
/*
  { 1, 4, "1 PLAYER" },
  { 1, 5, "2 PLAYERS" },
  { 2, 2, "COIN IN" },
*/
  { 2, 0, "AUTO UP" },
  { 2, 1, "ADVANCE" },
  { 2, 3, "HIGHSCORE RESET" },
  { -1 }
};


/* To do... */
static struct KEYSet keys[] =
{
 { 4, 0, "MOVE UP" },
 { -1 }
};


/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the background color table. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	4,	/* 4 bits per pixel */
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
			writemem,               /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
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
			writemem,               /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};




static struct MachineDriver stargate_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */ /*Stargate do not like 1 mhz*/
			0,                      /* memory region */
			stargate_readmem,       /* MemoryReadAddress */ /* JB 970823 */
			writemem,               /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Stargate_Interrupt,     /* interrupt routine */
			2                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
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
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
	0                                       /* sh_update routine */
};




static struct MachineDriver defender_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz */
			0,                      /* memory region */
			defender_readmem,       /* MemoryReadAddress */
			defender_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Defender_Interrupt,     /* interrupt routine */
			2                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_nofastop_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
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
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
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
			readmem,                /* MemoryReadAddress */
			writemem,               /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
  304, 240,                                     /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};




static struct MachineDriver blaster_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1100000,                /* ? Mhz */
			0,                      /* memory region */
			blaster_readmem,        /* MemoryReadAddress */
			blaster_writemem,       /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			Williams_Interrupt,     /* interrupt routine */
			4                       /* interrupts per frame (should be 4ms) */
		}
	},
	60,                                     /* frames per second */
	williams_nofastop_init_machine,          /* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256,                               /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,                          /* GfxDecodeInfo * */
	1+16,                                  /* total colors */
	16,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	blaster_vh_screenrefresh,               /* vh_update routine */

	/* sound hardware */
	0,                                      /* pointer to samples */
	0,                                      /* sh_init routine */
	0,                                      /* sh_start routine */
	0,                                      /* sh_stop routine */
	williams_sh_update                      /* sh_update routine */
};


static int cmos_load(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"rb")) != 0)
	{
		fread(&RAM[0xCC00],1,0x400,f);
		fclose(f);
	}else{
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
		displaytext(dt,0);

		while (!osd_key_pressed(OSD_KEY_6));    /* wait for key press */
		while (osd_key_pressed(OSD_KEY_6));     /* wait for key release */
	}

	return 1;
}

static void cmos_save(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0xCC00],1,0x400,f);
		fclose(f);
	}
}


static int defender_cmos_load(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"rb")) != 0)
	{
		fread(&RAM[0x10400],1,0x100,f);
		fclose(f);
	}else{
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
		displaytext(dt,0);

		while (!osd_key_pressed(OSD_KEY_6));    /* wait for key press */
		while (osd_key_pressed(OSD_KEY_6));     /* wait for key release */
	}

	return 1;
}

static void defender_cmos_save(const char *name)
{
	FILE *f;

	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x10400],1,0x100,f);
		fclose(f);
	}
}


static const char *williams_sample_names[] =
{
	"sound00.sam",
	"sound01.sam",
	"sound02.sam",
	"sound03.sam",
	"sound04.sam",
	"sound05.sam",
	"sound06.sam",
	"sound07.sam",
	"sound08.sam",
	"sound09.sam",
	"sound10.sam",
	"sound11.sam",
	"sound12.sam",
	"sound13.sam",
	"sound14.sam",
	"sound15.sam",
	"sound16.sam",
	"sound17.sam",
	"sound18.sam",
	"sound19.sam",
	"sound20.sam",
	"sound21.sam",
	"sound22.sam",
	"sound23.sam",
	"sound24.sam",
	"sound25.sam",
	"sound26.sam",
	"sound27.sam",
	"sound28.sam",
	"sound29.sam",
	"sound30.sam",
	"sound31.sam",
	"sound32.sam",
	"sound33.sam",
	"sound34.sam",
	"sound35.sam",
	"sound36.sam",
	"sound37.sam",
	"sound38.sam",
	"sound39.sam",
	"sound40.sam",
	"sound41.sam",
	"sound42.sam",
	"sound43.sam",
	"sound44.sam",
	"sound45.sam",
	"sound46.sam",
	"sound47.sam",
	"sound48.sam",
	"sound49.sam",
	"sound50.sam",
	"sound51.sam",
	"sound52.sam",
	"sound53.sam",
	"sound54.sam",
	"sound55.sam",
	"sound56.sam",
	"sound57.sam",
	"sound58.sam",
	"sound59.sam",
	"sound60.sam",
	"sound61.sam",
	"sound62.sam",
	"sound63.sam",
	0       /* end of array */
};



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
ROM_END


struct GameDriver robotron_driver =
{
	"robotron",
	"robotron",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&robotron_machine_driver,       /* MachineDriver * */

	robotron_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	robotron_input_ports, 0,           /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	robotron_keys,                  /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};



ROM_START( joust_rom )
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
ROM_END

struct GameDriver joust_driver =
{
	"joust",
	"joust",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&joust_machine_driver,          /* MachineDriver * */

	joust_rom,                      /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	joust_input_ports, 0,              /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	joust_keys,                     /* KEY def    */

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
ROM_END

struct GameDriver sinistar_driver =
{
	"sinistar",
	"sinistar",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&sinistar_machine_driver,       /* MachineDriver * */

	sinistar_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	sinistar_input_ports, 0,           /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	keys,                           /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

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
ROM_END

struct GameDriver bubbles_driver =
{
	"bubbles",
	"bubbles",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&bubbles_machine_driver,        /* MachineDriver * */

	bubbles_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	bubbles_input_ports, 0,            /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	keys,                           /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};



ROM_START( stargate_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "stargate.01", 0x0000, 0x1000, 0xe5fecedc )
	ROM_LOAD( "stargate.02", 0x1000, 0x1000, 0xbd525dec )
	ROM_LOAD( "stargate.03", 0x2000, 0x1000, 0xb5fed8d8 )
	ROM_LOAD( "stargate.04", 0x3000, 0x1000, 0x41ba0bc0 )
	ROM_LOAD( "stargate.05", 0x4000, 0x1000, 0xdde98c57 )
	ROM_LOAD( "stargate.06", 0x5000, 0x1000, 0x1b795abd )
	ROM_LOAD( "stargate.07", 0x6000, 0x1000, 0xe45af9ca )
	ROM_LOAD( "stargate.08", 0x7000, 0x1000, 0xa1026964 )
	ROM_LOAD( "stargate.09", 0x8000, 0x1000, 0xad03f4f1 )
	ROM_LOAD( "stargate.10", 0xd000, 0x1000, 0x001c6dec )
	ROM_LOAD( "stargate.11", 0xe000, 0x1000, 0xd33c4d64 )
	ROM_LOAD( "stargate.12", 0xf000, 0x1000, 0x94afba0b )
ROM_END

struct GameDriver stargate_driver =
{
	"stargate",
	"stargate",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&stargate_machine_driver,       /* MachineDriver * */

	stargate_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	stargate_input_ports, 0,           /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	stargate_keys,                  /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};



ROM_START( defender_rom )
	ROM_REGION(0x15000)     /* 64k for code + 5 banks of 4K */
	ROM_LOAD( "defend.1", 0xd000, 0x0800, 0x4aa8c614 )
	ROM_LOAD( "defend.4", 0xd800, 0x0800, 0x99e9bb31 )
	ROM_LOAD( "defend.2", 0xe000, 0x1000, 0x8991dceb )
	ROM_LOAD( "defend.3", 0xf000, 0x1000, 0x3f6e9fe2 )
  /* 10000 to 10fff is the place for CMOS ram (BANK 0) */
	ROM_LOAD( "defend.9", 0x11000, 0x0800, 0x3e2646ae )
	ROM_LOAD( "defend.12",0x11800, 0x0800, 0xd13eeb4a )
	ROM_LOAD( "defend.8", 0x12000, 0x0800, 0x67afa299 )
	ROM_LOAD( "defend.11",0x12800, 0x0800, 0x287572ed )
	ROM_LOAD( "defend.7", 0x13000, 0x0800, 0x344c9bd0 )
	ROM_LOAD( "defend.10",0x13800, 0x0800, 0xee30b06e )
	ROM_LOAD( "defend.6", 0x14000, 0x0800, 0x8c7b2da3 )
	ROM_LOAD( "defend.10",0x14800, 0x0800, 0xee30b06e )
ROM_END

struct GameDriver defender_driver =
{
	"defender",
	"defender",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&defender_machine_driver,       /* MachineDriver * */

	defender_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	defender_input_ports, 0,           /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	defender_keys,                  /* KEY def    */

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
ROM_END


struct GameDriver splat_driver =
{
	"splat",
	"splat",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&splat_machine_driver,          /* MachineDriver * */

	splat_rom,                      /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	splat_input_ports, 0,              /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	splat_keys,                     /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

  cmos_load, cmos_save
};






ROM_START( blaster_rom )
	ROM_REGION(0x49000)     /* 256k for code */
	ROM_LOAD( "blaster.16", 0x0d000, 0x1000, 0xd6e04ee2 )
	ROM_LOAD( "blaster.13", 0x0e000, 0x2000, 0x376fc541 )
	ROM_LOAD( "blaster.15", 0x10000, 0x4000, 0x1c345c06 )
	ROM_LOAD( "blaster.8",  0x14000, 0x4000, 0xc297d5ab )
	ROM_LOAD( "blaster.9",  0x18000, 0x4000, 0xe88478a0 )
	ROM_LOAD( "blaster.10", 0x1c000, 0x4000, 0xc68a1386 )
	ROM_LOAD( "blaster.6",  0x20000, 0x4000, 0x3142941e )
	ROM_LOAD( "blaster.5",  0x24000, 0x4000, 0x2ebd56e5 )
	ROM_LOAD( "blaster.14", 0x28000, 0x4000, 0xe0267262 )
	ROM_LOAD( "blaster.7",  0x2c000, 0x4000, 0x17bff9a5 )
	ROM_LOAD( "blaster.1",  0x30000, 0x4000, 0x37d9abe5 )
	ROM_LOAD( "blaster.2",  0x34000, 0x4000, 0xd99ff133 )
	ROM_LOAD( "blaster.4",  0x38000, 0x4000, 0x8d86011c )
	ROM_LOAD( "blaster.3",  0x3c000, 0x4000, 0x86ddd013 )
	ROM_LOAD( "blaster.1",  0x40000, 0x4000, 0x37d9abe5 )
	ROM_LOAD( "blaster.11", 0x44000, 0x2000, 0xdc7831b2 )
	ROM_LOAD( "blaster.12", 0x46000, 0x2000, 0x68244eb6 )
	ROM_LOAD( "blaster.17", 0x48000, 0x1000, 0xecbf6a51 )
ROM_END


struct GameDriver blaster_driver =
{
	"blaster",
	"blaster",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI\nAARON GILES",
	&blaster_machine_driver,        /* MachineDriver * */

	blaster_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	williams_sample_names,          /* samplenames */

	blaster_input_ports, 0,            /* InputPort  */
        trak_ports,                     /* TrackBall  */
	williams_dsw,                   /* DSW        */
	blaster_keys,                   /* KEY def    */

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


