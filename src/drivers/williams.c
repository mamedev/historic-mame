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

extern void williams_sh_w(int offset,int data);
extern void williams_sh_update(void);
extern void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);

int williams_init_machine(const char *gamename);

extern unsigned char *robotron_videoram;
extern void williams_videoram_w(int offset,int data);
extern void blaster_videoram_w(int offset,int data);
extern int williams_videoram_r(int offset);
extern int blaster_videoram_r(int offset);
extern void williams_StartBlitter(int offset,int data);
extern void williams_StartBlitter2(int offset,int data);
extern void williams_BlasterStartBlitter(int offset,int data);

extern int williams_vh_start(void);
extern void williams_vh_stop(void);
extern void williams_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void defender_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void blaster_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void Williams_Palette_w(int offset,int data);

extern int defender_bank_r(int offset);
extern void defender_bank_w(int offset,int data);
extern void defender_bank_select_w(int offset,int data);
extern void defender_videoram_w(int offset,int data);

extern void blaster_bank_select_w(int offset,int data);
extern int Williams_Interrupt(void);
extern int Stargate_Interrupt(void);
extern int Defender_Interrupt(void);

extern int video_counter_r(int offset);
extern int test_r(int offset);
extern void test_w(int offset,int data);
extern int input_port_0_1(int offset);
extern int blaster_input_port_0(int offset);




/*
 *   Read mem for Robotron Stargate Bubbles Splat
 */

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },	/* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },	/* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },     /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },            /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
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
	{ -1 }	/* end of table */
};


/*
 *   Read mem for Joust
 */

static struct MemoryReadAddress joust_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_1 },	     /* IN0-1 */
	{ 0xc806, 0xc806, input_port_2_r },	     /* IN2 */
	{ 0xc80c, 0xc80c, input_port_3_r },          /* IN3 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
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
	{ -1 }	/* end of table */
};


/*
 *   Read mem for Sinistar
 */

static struct MemoryReadAddress sinistar_readmem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_r },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, input_port_0_r },	     /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },	     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xdfff, MRA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
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
	{ -1 }	/* end of table */
};


/*
 *   Read mem for Blaster
 */

static struct MemoryReadAddress blaster_readmem[] =
{
	{ 0x0000, 0x96ff, blaster_videoram_r },
	{ 0x9700, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc804, blaster_input_port_0 },    /* IN0 */
	{ 0xc806, 0xc806, input_port_1_r },	     /* IN1 */
	{ 0xc80c, 0xc80c, input_port_2_r },          /* IN2 */
	{ 0xc80e, 0xc80e, MRA_RAM },                 /* not used? */
	{ 0xcb00, 0xcb00, video_counter_r },
	{ 0xCC00, 0xCFFF, MRA_RAM },                 /* CMOS      */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
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
	{ -1 }	/* end of table */
};


/*
 *   Read mem for Defender
 */

static struct MemoryReadAddress defender_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, defender_bank_r },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
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
	{ -1 }	/* end of table */
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
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_DOWN },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{ -1 }	/* end of table */
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
	{	/* IN0 Player 2 */
		0x00,	/* default_value */
		{ OSD_KEY_F, OSD_KEY_G, OSD_KEY_S, 0, OSD_KEY_2, OSD_KEY_1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN0 Player 1 */
		0x00,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN3 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
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
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_L, OSD_KEY_K, OSD_KEY_M, OSD_KEY_N, OSD_KEY_2, OSD_KEY_1, OSD_KEY_ALT, OSD_KEY_S },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_Q, OSD_KEY_SPACE, 0,0,0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
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
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_L, OSD_KEY_K, OSD_KEY_SPACE, OSD_KEY_N, OSD_KEY_2, OSD_KEY_1, OSD_KEY_ALT, OSD_KEY_S },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_Q, 0,0,0,0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
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
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
};



/*
 *   Bubbles buttons
 */

static struct InputPort bubbles_input_ports[] =
{
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4, OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_Q, OSD_KEY_W, OSD_KEY_E, OSD_KEY_R, OSD_KEY_T, OSD_KEY_Y, OSD_KEY_U, OSD_KEY_I },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_A, OSD_KEY_S, OSD_KEY_D, OSD_KEY_F, OSD_KEY_G, OSD_KEY_H, OSD_KEY_J, OSD_KEY_K },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{ -1 }	/* end of table */
};



/*
 *   Splat buttons
 */

static struct InputPort splat_input_ports[] =
{
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_W, OSD_KEY_S, OSD_KEY_A, OSD_KEY_D, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_DOWN },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
};

static struct KEYSet splat_keys[] =
{
  { 0, 0, "WALK UP" },
  { 0, 1, "WALK DOWN" },
  { 0, 2, "WALK LEFT" },
  { 0, 3, "WALK RIGHT" },
  { 0, 6, "THROW UP" },
  { 0, 7, "THROW DOWN" },
  { 1, 0, "THROW LEFT" },
  { 1, 1, "THROW RIGHT" },
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
 *   Blaster buttons
 */

static struct InputPort blaster_input_ports[] =
{
	{	/* IN0 */
		0x00,	/* default_value */
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN1 */
		0x00,	/* default_value */
		{ OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_Z, 0, OSD_KEY_1, OSD_KEY_2, 0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* IN2 */
		0x00,	/* default_value */
		{ OSD_KEY_5, OSD_KEY_6, OSD_KEY_3, OSD_KEY_7, 0,0,0,0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},

	{ -1 }	/* end of table */
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







/* Not really used. See williams_vh_start() in VIDHRDW/WILLIAMS.C */
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	40,	/* 40 characters */
	1,	/* 1 bits per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

/*  Just to allocate the memory  */
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 4 },
	{ -1 } /* end of array */
};






static struct MachineDriver robotron_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */
			0,			/* memory region */
			readmem,		/* MemoryReadAddress */
			writemem,		/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver joust_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */
			0,			/* memory region */
			joust_readmem,		/* MemoryReadAddress */
			writemem,		/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver stargate_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */ /*Stargate do not like 1 mhz*/
			0,			/* memory region */
			readmem,		/* MemoryReadAddress */
			writemem,		/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Stargate_Interrupt,	/* interrupt routine */
			2			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver sinistar_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */ /*Sinistar do not like 1 mhz*/
			0,			/* memory region */
			sinistar_readmem,	/* MemoryReadAddress */
			sinistar_writemem,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6, 					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	0					/* sh_update routine */
};




static struct MachineDriver defender_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,		/* ? Mhz */ /*Defender do not like 1 mhz */
			0,			/* memory region */
			defender_readmem,	/* MemoryReadAddress */
			defender_writemem,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Defender_Interrupt,	/* interrupt routine */
			2			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6, 					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	defender_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver splat_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */
			0,			/* memory region */
			readmem,		/* MemoryReadAddress */
			splat_writemem,		/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver bubbles_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,		/* ? Mhz */
			0,			/* memory region */
			readmem,		/* MemoryReadAddress */
			writemem,		/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
  304, 240, 			                /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 }, 	        /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	williams_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
};




static struct MachineDriver blaster_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1100000,		/* ? Mhz */
			0,			/* memory region */
			blaster_readmem,	/* MemoryReadAddress */
			blaster_writemem,	/* MemoryWriteAddress */
			0,			/* IOReadPort */
			0,			/* IOWritePort */
			Williams_Interrupt,	/* interrupt routine */
			4			/* interrupts per frame (should be 4ms) */
		}
	},
	60,					/* frames per second */
	williams_init_machine,		/* init machine routine */

	/* video hardware */
#ifdef HEIGHT_240
	304, 240, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#else
	304, 256, 			        /* screen_width, screen_height */
	{ 0, 304-1, 0, 256-1 },                 /* struct rectangle visible_area */
#endif
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,					/* total colors */
	6,					/* color table length */
	williams_vh_convert_color_prom,		/* convert color prom routine */

	0,					/* vh_init routine */
	williams_vh_start,			/* vh_start routine */
	williams_vh_stop,			/* vh_stop routine */
	blaster_vh_screenrefresh,	        /* vh_update routine */

	/* sound hardware */
	0,					/* pointer to samples */
	0,					/* sh_init routine */
	0,					/* sh_start routine */
	0,					/* sh_stop routine */
	williams_sh_update		        /* sh_update routine */
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

		dt[0].color = Machine->gamedrv->paused_color;
		dt[0].x = 10;
		dt[0].y = 20;
		dt[1].text = 0;
		displaytext(dt,0);

		while (!osd_key_pressed(OSD_KEY_6));	/* wait for key press */
		while (osd_key_pressed(OSD_KEY_6));	/* wait for key release */
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

		dt[0].color = Machine->gamedrv->paused_color;
		dt[0].x = 10;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,0);

		while (!osd_key_pressed(OSD_KEY_6));	/* wait for key press */
		while (osd_key_pressed(OSD_KEY_6));	/* wait for key release */
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
	0	/* end of array */
};



ROM_START( robotron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ROBOTRON.SB1", 0x0000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB2", 0x1000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB3", 0x2000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB4", 0x3000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB5", 0x4000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB6", 0x5000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB7", 0x6000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB8", 0x7000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SB9", 0x8000, 0x1000 )

	ROM_LOAD( "ROBOTRON.SBa", 0xd000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SBb", 0xe000, 0x1000 )
	ROM_LOAD( "ROBOTRON.SBc", 0xf000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END


struct GameDriver robotron_driver =
{
	"robotron",
	"robotron",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&robotron_machine_driver,	/* MachineDriver * */

	robotron_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	robotron_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	robotron_keys,                  /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

        cmos_load, cmos_save
};



ROM_START( joust_rom )
	ROM_REGION(0x10000)	/* 64k for code */

	ROM_LOAD( "JOUST.SR1", 0x0000, 0x1000 )
	ROM_LOAD( "JOUST.SR2", 0x1000, 0x1000 )
	ROM_LOAD( "JOUST.SR3", 0x2000, 0x1000 )
	ROM_LOAD( "JOUST.SR4", 0x3000, 0x1000 )
	ROM_LOAD( "JOUST.SR5", 0x4000, 0x1000 )
	ROM_LOAD( "JOUST.SR6", 0x5000, 0x1000 )
	ROM_LOAD( "JOUST.SR7", 0x6000, 0x1000 )
	ROM_LOAD( "JOUST.SR8", 0x7000, 0x1000 )
	ROM_LOAD( "JOUST.SR9", 0x8000, 0x1000 )

	ROM_LOAD( "JOUST.SRa", 0xd000, 0x1000 )
	ROM_LOAD( "JOUST.SRb", 0xe000, 0x1000 )
	ROM_LOAD( "JOUST.SRc", 0xf000, 0x1000 )

/*
	ROM_LOAD( "JOUST.SG1", 0x0000, 0x1000 )
	ROM_LOAD( "JOUST.SG2", 0x1000, 0x1000 )
	ROM_LOAD( "JOUST.SG3", 0x2000, 0x1000 )
	ROM_LOAD( "JOUST.SG4", 0x3000, 0x1000 )
	ROM_LOAD( "JOUST.SG5", 0x4000, 0x1000 )
	ROM_LOAD( "JOUST.SG6", 0x5000, 0x1000 )
	ROM_LOAD( "JOUST.SG7", 0x6000, 0x1000 )
	ROM_LOAD( "JOUST.SG8", 0x7000, 0x1000 )
	ROM_LOAD( "JOUST.SG9", 0x8000, 0x1000 )

	ROM_LOAD( "JOUST.SGa", 0xd000, 0x1000 )
	ROM_LOAD( "JOUST.SGb", 0xe000, 0x1000 )
	ROM_LOAD( "JOUST.SGc", 0xf000, 0x1000 )
*/

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END

struct GameDriver joust_driver =
{
	"joust",
	"joust",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&joust_machine_driver,		/* MachineDriver * */

	joust_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	joust_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	joust_keys,                     /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	cmos_load, cmos_save
};



ROM_START( sinistar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
/*
	ROM_LOAD( "SIN1", 0x0000, 0x1000 )
	ROM_LOAD( "SIN2", 0x1000, 0x1000 )
	ROM_LOAD( "SIN3", 0x2000, 0x1000 )
	ROM_LOAD( "SIN4", 0x3000, 0x1000 )
	ROM_LOAD( "SIN5", 0x4000, 0x1000 )
	ROM_LOAD( "SIN6", 0x5000, 0x1000 )
	ROM_LOAD( "SIN7", 0x6000, 0x1000 )
	ROM_LOAD( "SIN8", 0x7000, 0x1000 )
	ROM_LOAD( "SIN9", 0x8000, 0x1000 )

	ROM_LOAD( "SIN10", 0xe000, 0x1000 )
	ROM_LOAD( "SIN11", 0xf000, 0x1000 )
*/
	ROM_LOAD( "SINISTAR.01", 0x0000, 0x1000 )
	ROM_LOAD( "SINISTAR.02", 0x1000, 0x1000 )
	ROM_LOAD( "SINISTAR.03", 0x2000, 0x1000 )
	ROM_LOAD( "SINISTAR.04", 0x3000, 0x1000 )
	ROM_LOAD( "SINISTAR.05", 0x4000, 0x1000 )
	ROM_LOAD( "SINISTAR.06", 0x5000, 0x1000 )
	ROM_LOAD( "SINISTAR.07", 0x6000, 0x1000 )
	ROM_LOAD( "SINISTAR.08", 0x7000, 0x1000 )
	ROM_LOAD( "SINISTAR.09", 0x8000, 0x1000 )

	ROM_LOAD( "SINISTAR.10", 0xe000, 0x1000 )
	ROM_LOAD( "SINISTAR.11", 0xf000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END

struct GameDriver sinistar_driver =
{
	"sinistar",
	"sinistar",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&sinistar_machine_driver,	/* MachineDriver * */

	sinistar_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	sinistar_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	cmos_load, cmos_save
};



ROM_START( bubbles_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "BUBBLES.1B", 0x0000, 0x1000 )
	ROM_LOAD( "BUBBLES.2B", 0x1000, 0x1000 )
	ROM_LOAD( "BUBBLES.3B", 0x2000, 0x1000 )
	ROM_LOAD( "BUBBLES.4B", 0x3000, 0x1000 )
	ROM_LOAD( "BUBBLES.5B", 0x4000, 0x1000 )
	ROM_LOAD( "BUBBLES.6B", 0x5000, 0x1000 )
	ROM_LOAD( "BUBBLES.7B", 0x6000, 0x1000 )
	ROM_LOAD( "BUBBLES.8B", 0x7000, 0x1000 )
	ROM_LOAD( "BUBBLES.9B", 0x8000, 0x1000 )

	ROM_LOAD( "BUBBLES.10B", 0xd000, 0x1000 )
	ROM_LOAD( "BUBBLES.11B", 0xe000, 0x1000 )
	ROM_LOAD( "BUBBLES.12B", 0xf000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END

struct GameDriver bubbles_driver =
{
	"bubbles",
	"bubbles",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&bubbles_machine_driver,	/* MachineDriver * */

	bubbles_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	bubbles_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	keys,                           /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	cmos_load, cmos_save
};



ROM_START( stargate_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "01", 0x0000, 0x1000 )
	ROM_LOAD( "02", 0x1000, 0x1000 )
	ROM_LOAD( "03", 0x2000, 0x1000 )
	ROM_LOAD( "04", 0x3000, 0x1000 )
	ROM_LOAD( "05", 0x4000, 0x1000 )
	ROM_LOAD( "06", 0x5000, 0x1000 )
	ROM_LOAD( "07", 0x6000, 0x1000 )
	ROM_LOAD( "08", 0x7000, 0x1000 )
	ROM_LOAD( "09", 0x8000, 0x1000 )

	ROM_LOAD( "10", 0xd000, 0x1000 )
	ROM_LOAD( "11", 0xe000, 0x1000 )
	ROM_LOAD( "12", 0xf000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END

struct GameDriver stargate_driver =
{
	"stargate",
	"stargate",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&stargate_machine_driver,	/* MachineDriver * */

	stargate_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	stargate_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	stargate_keys,                  /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	cmos_load, cmos_save
};



ROM_START( defender_rom )
	ROM_REGION(0x15000)	/* 64k for code + 5 banks of 4K */
	ROM_LOAD( "defend.1", 0xd000, 0x0800 )
	ROM_LOAD( "defend.4", 0xd800, 0x0800 )
	ROM_LOAD( "defend.2", 0xe000, 0x1000 )
	ROM_LOAD( "defend.3", 0xf000, 0x1000 )

  /* 10000 to 10fff is the place for CMOS ram (BANK 0) */
	ROM_LOAD( "defend.9", 0x11000, 0x0800 )
	ROM_LOAD( "defend.12",0x11800, 0x0800 )
	ROM_LOAD( "defend.8", 0x12000, 0x0800 )
	ROM_LOAD( "defend.11",0x12800, 0x0800 )
	ROM_LOAD( "defend.7", 0x13000, 0x0800 )
	ROM_LOAD( "defend.10",0x13800, 0x0800 )
	ROM_LOAD( "defend.6", 0x14000, 0x0800 )
	ROM_LOAD( "defend.10",0x14800, 0x0800 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END

struct GameDriver defender_driver =
{
	"defender",
	"defender",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&defender_machine_driver,	/* MachineDriver * */

	defender_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	defender_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	defender_keys,                  /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	defender_cmos_load, defender_cmos_save
};





ROM_START( splat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "SPLAT.01", 0x0000, 0x1000 )
	ROM_LOAD( "SPLAT.02", 0x1000, 0x1000 )
	ROM_LOAD( "SPLAT.03", 0x2000, 0x1000 )
	ROM_LOAD( "SPLAT.04", 0x3000, 0x1000 )
	ROM_LOAD( "SPLAT.05", 0x4000, 0x1000 )
	ROM_LOAD( "SPLAT.06", 0x5000, 0x1000 )
	ROM_LOAD( "SPLAT.07", 0x6000, 0x1000 )
	ROM_LOAD( "SPLAT.08", 0x7000, 0x1000 )
	ROM_LOAD( "SPLAT.09", 0x8000, 0x1000 )

	ROM_LOAD( "SPLAT.10", 0xd000, 0x1000 )
	ROM_LOAD( "SPLAT.11", 0xe000, 0x1000 )
	ROM_LOAD( "SPLAT.12", 0xf000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END


struct GameDriver splat_driver =
{
	"splat",
	"splat",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&splat_machine_driver,		/* MachineDriver * */

	splat_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	splat_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	splat_keys,                     /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

  cmos_load, cmos_save
};






ROM_START( blaster_rom )
	ROM_REGION(0x49000)	/* 256k for code */
/*
	ROM_LOAD( "blaster.1",  0x0000, 0x4000 )
	ROM_LOAD( "blaster.11", 0x4000, 0x2000 )
	ROM_LOAD( "blaster.12", 0x6000, 0x2000 )
	ROM_LOAD( "blaster.17", 0x8000, 0x1000 )
*/
	ROM_LOAD( "blaster.16", 0xd000, 0x1000 )
	ROM_LOAD( "blaster.13", 0xe000, 0x2000 )

	ROM_LOAD( "blaster.15",0x10000, 0x4000 )
	ROM_LOAD( "blaster.8", 0x14000, 0x4000 )
	ROM_LOAD( "blaster.9", 0x18000, 0x4000 )
	ROM_LOAD( "blaster.10",0x1c000, 0x4000 )
	ROM_LOAD( "blaster.6", 0x20000, 0x4000 )
	ROM_LOAD( "blaster.5", 0x24000, 0x4000 )
	ROM_LOAD( "blaster.14",0x28000, 0x4000 )
	ROM_LOAD( "blaster.7", 0x2c000, 0x4000 )
	ROM_LOAD( "blaster.1", 0x30000, 0x4000 )
	ROM_LOAD( "blaster.2", 0x34000, 0x4000 )
	ROM_LOAD( "blaster.4", 0x38000, 0x4000 )
	ROM_LOAD( "blaster.3", 0x3c000, 0x4000 )

	ROM_LOAD( "blaster.1", 0x40000, 0x4000 )
	ROM_LOAD( "blaster.11", 0x44000, 0x2000 )
	ROM_LOAD( "blaster.12", 0x46000, 0x2000 )
	ROM_LOAD( "blaster.17", 0x48000, 0x1000 )

	ROM_REGION(0x200)	/* 512 byte for fonts */
	ROM_LOAD( "williams.fnt",  0x0000, 0x0120 )

ROM_END


struct GameDriver blaster_driver =
{
	"blaster",
	"blaster",
	"MARC LAFONTAINE\nSTEVEN HUGG\nMIRKO BUFFONI",
	&blaster_machine_driver,	/* MachineDriver * */

	blaster_rom,			/* RomModule * */
	0, 0,				/* ROM decrypt routines */
	williams_sample_names,		/* samplenames */

	blaster_input_ports,	        /* InputPort  */
	williams_dsw,		        /* DSW        */
	blaster_keys,                   /* KEY def    */

	0,				/* color prom */
	0,		                /* palette */
	0,	                        /* color table */
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23 },
	0x00, 0x01,			/* white_text, yellow_text for DIP switch menu */
	140, 110, 0x00,		        /* paused_x, paused_y, paused_color for PAUSED */

	cmos_load, cmos_save
};


