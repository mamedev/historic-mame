/***************************************************************************

I'm in no mood to write documentation now, so if you have any questions
about this driver, please address them to Pat Lawrence <pjl@ns.net>.  I'll
be happy to help you out any way I can.

Crystal Castles memory map.

 Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
          1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
          5 4 3 2 1 0                      W
-------------------------------------------------------------------------------
0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
-------------------------------------------------------------------------------
8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
                                      0 0 R/W D D D D D D D D  MOB Picture
                                      0 1 R/W D D D D D D D D  MOB Vertial
                                      1 0 R/W D D D D D D D D  MOB Priority
                                      1 1 R/W D D D D D D D D  MOB Horizontal
-------------------------------------------------------------------------------
9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
9400-9401 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL
9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL
9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                           R                D  COIN R
                                           R              D    COIN L
                                           R            D      COIN AUX
                                           R          D        SLAM
                                           R        D          SELF TEST
                                           R      D            SPARE
                                           R    D              JMP1
                                           R  D                JMP2
-------------------------------------------------------------------------------
9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
9A08                                                    D D D  Option SW
                                                      D        SPARE
                                                    D          SPARE
                                                  D            SPARE
9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
-------------------------------------------------------------------------------
9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
          1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
9E80                                0 0 0  W                D  Trak Ball Light
9E81                                0 0 1  W                D  
9E82                                0 1 0  W                D  Store Low
9E83                                0 1 1  W                D  Store High
9E84                                1 0 0  W                D  Spare
9E85                                1 0 1  W                D  Coin Counter R
9E86                                1 1 0  W                D  Coin Counter L
9E87                                1 1 1  W                D  BANK0-BANK1
          1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
9F00                                0 0 0  W          D        ^AX
9F01                                0 0 1  W          D        ^AY
9F02                                0 1 0  W          D        ^XINC
9F03                                0 1 1  W          D        ^YINC
9F04                                1 0 0  W          D        PLAYER2
9F05                                1 0 1  W          D        ^SIRE
9F06                                1 1 0  W          D        BOTHRAM
9F07                                1 1 1  W          D        BUF1/^BUF2
9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

Things to do:
1) Sound.  It's better!  The startup sound no longer plays.  The 
   game sounds are slightly slow.....
2) Dynamic palette.  I haven't given this the time it deserves.  I'll get
   to this one soon.
3) The game is always in cocktail mode.  If I try to take it out of
   cocktail mode, the player 1 and player 2 buttons no longer work.
   Needless to say, I have not emulated cocktail mode, nor do I plan to so
   only try 1 player games for now. :-)
4) No high score saving yet.
5) Cannot change the settings of the game.
6) The MSDOS driver needs a tweaked VGA mode for 256x232.
7) Vertical scrolling is wrong.  Is horizontal scrolling right?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *ccastles_bitmapram;
extern unsigned char *ccastles_mobram1;
extern unsigned char *ccastles_mobram2;
extern int ccastles_sh_start(void);
extern void ccastles_sh_stop(void);
extern void ccastles_sh_update(void);
extern void ccastles_vh_screenrefresh(struct osd_bitmap *bitmap);
extern int ccastles_vh_start(void);
extern void ccastles_vh_stop(void);

extern int ccastles_init_machine(const char *gamename);
extern int ccastles_interrupt(void);
extern int ccastles_trakball_x(int data);
extern int ccastles_trakball_y(int data);

extern int ccastles_trakball_r(int offset);
extern int ccastles_xy_r(int offset);
extern int ccastles_rom_r(int offset);
extern int ccastles_IN0_r(int offset);
extern int ccastles_random_r(int offset);
extern int ccastles_bitmode_r(int offset);

extern void ccastles_xy_w(int offset, int data);
extern void ccastles_axy_w(int offset, int data);
extern void ccastles_bitmode_w(int offset, int data);
extern void ccastles_pokey0_w(int offset,int data);
extern void ccastles_pokey1_w(int offset,int data);
extern void ccastles_bitmapram_w(int offset,int data);
extern void ccastles_bankswitch_w(int offset,int data);
extern void ccastles_colorram_w(int offset,int data);

static struct MemoryReadAddress readmem[] =
{
  { 0x0000, 0x0001, MRA_RAM },
  { 0x0002, 0x0002, ccastles_bitmode_r },
  { 0x0003, 0x90ff, MRA_RAM },	/* All RAM */
  { 0x9500, 0x9501, ccastles_trakball_r },
  { 0x9600, 0x9600, ccastles_IN0_r },	/* IN0 */
  { 0x980A, 0x980A, ccastles_random_r },/* Random # generator on a Pokey */
  { 0x9A08, 0x9A08, input_port_1_r },	/* OPTION SW */
  { 0x9C80, 0x9C80, MRA_RAM },    /* Horizontal Scroll */
  { 0x9D00, 0x9D00, MRA_RAM },    /* Vertical Scroll */
  { 0x9F80, 0x9FBF, MRA_RAM },    /* COLORAM */
  { 0xA000, 0xDFFF, ccastles_rom_r },
  { 0xE000, 0xFFFF, MRA_ROM },	/* ROMs/interrupt vectors */
  { -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
  { 0x0000, 0x0001, ccastles_xy_w },
  { 0x0002, 0x0002, ccastles_bitmode_w },
  { 0x0003, 0x90FF, MWA_RAM },  /* All RAM */
  { 0x9800, 0x980F, ccastles_pokey0_w },
  { 0x9A00, 0x9A0F, ccastles_pokey1_w },
  { 0x9C80, 0x9C80, MWA_RAM },    /* Horizontal Scroll */
  { 0x9D00, 0x9D00, MWA_RAM },    /* Vertical Scroll */
  { 0x9D80, 0x9D80, MWA_NOP },
  { 0x9E00, 0x9E00, MWA_NOP },
  { 0x9E80, 0x9E81, MWA_NOP },
  { 0x9E87, 0x9E87, ccastles_bankswitch_w },
  { 0x9F00, 0x9F01, ccastles_axy_w },
  { 0x9F02, 0x9F07, MWA_RAM },
  { 0x9F80, 0x9FBF, ccastles_colorram_w },
  { -1 }	/* end of table */
};

static struct InputPort ccastles_input_ports[] = {
  {	/* IN0 */
    0xFF,
    { OSD_KEY_5, OSD_KEY_3, OSD_KEY_4, 0, 0, 0, OSD_KEY_CONTROL, 0 },
    { 0, 0, 0, 0, 0, 0, OSD_JOY_FIRE, 0 }
  },
  {	/* IN1 */
    0x3F,
    { 0, 0, 0, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  },
  { -1 }	/* end of table */
};

/* Added 300697 PJL */
static struct TrakPort ccastles_trak_ports[] = {
  {
    Y_AXIS,
    0,
    1.5,
    ccastles_trakball_y
  },
  {
    X_AXIS,
    0,
    1.0,
    ccastles_trakball_x
  },
  { -1 }
};
/* End 300697 PJL */

static struct KEYSet ccastles_keys[] = {
  { -1 }
};

static struct DSW ccastles_dsw[] = {
  { 0, 0x10, "SELF TEST", { "ON", "OFF" } },
  { -1 }
};

static struct GfxLayout ccastles_charlayout = {
  5,5,	/* 8*8 characters */
  39,	/* 38 characters */
  1,	/* 1 bits per pixel */
  { 0 },	/* the two bitplanes are separated */
  { 0*8, 1*8, 2*8, 3*8, 4*8 },
  { 4, 3, 2, 1, 0 },
  5*8	/* every char takes 5 consecutive bytes */
};

static struct GfxLayout ccastles_spritelayout = {
  8,16,				/* 16*16 sprites */
  256,                             /* 128 sprites */
  3,				/* 4 bits per pixel */
  { 0, 4, 0x2000*8+4 },	/* the four bitplanes are separated */
  { 0, 1, 2, 3, 1*8+0, 1*8+1, 1*8+2, 1*8+3 },
  { 0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,8*16,9*16,10*16,11*16,12*16,
    13*16,14*16,15*16 },
  16*8*2                  /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ccastles_gfxdecodeinfo[] = {
  { 0, 0xE20C, &ccastles_charlayout, 0, 1 },
  { 1, 0x0000, &ccastles_spritelayout, 2, 1 },
  { -1 } /* end of array */
};

static unsigned char ccastles_palette[] = {
  0x00,0x00,0x00,   /* Color 0 */
  0xC0,0x00,0x00,   /* Color 1 */
  0xFF,0x00,0x00,   /* Color 2 - Sprite color 0 */
  0xF3,0xB3,0x00,   /* Color 3 - Sprite color 1 */
  0xAB,0x8F,0x1B,   /* Color 4 - Sprite color 2 */
  0x73,0x00,0x37,   /* Color 5 - Sprite color 3 */
  0x77,0x53,0x07,   /* Color 6 - Sprite color 4 */
  0x00,0x7F,0x00,   /* Color 7 - Sprite color 5 */
  0x6B,0xC3,0x53,   /* Color 8 - Sprite color 6 */
  0x00,0x00,0x00,   /* Color 9 - Sprite color 7 */
  0x00,0x00,0x00,   /* Color 10 - Background */
  0xC0,0xC0,0xC0,   /* Color 11 - Hidden 0 - walkways */
  0x70,0x92,0xA6,   /* Color 12 - Score border: cyan */
  0x40,0x5F,0x71,   /* Color 13 - Hidden 1 - dark walls: dark grey */
  0x00,0x00,0x00,   /* Color 14 - Hidden 2 - outline : black */
  0xC0,0x00,0x00,   /* Color 15 - Score border: blue */
  0xFF,0xFF,0xB0,   /* Color 16 - Pathways between castles */
  0xE0,0xE0,0xE0,   /* Color 17 - Text */
  0x00,0x00,0x00,   /* Color 18 - Interior Corridor */
  0xC0,0xC0,0xC0,   /* Color 19 - Main walkways */
  0x70,0x92,0xA6,   /* Color 20 - Bright walls */
  0x40,0x5F,0x71,   /* Color 21 - Dark walls */
  0x00,0x00,0x00,   /* Color 22 - Main castle outline */
  0xC0,0x00,0x00,   /* Color 23 - Gems */
  0x00,0xFF,0x00,   /* Color 24 - ??? */
  0xC0,0xC0,0xC0,   /* Color 25 - Gem highlight */
  0x00,0xFF,0x00,   /* Color 26 - ??? */
};

static unsigned char ccastles_colortable[] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B,
  0x1C, 0x1D, 0x1E, 0x1F
};

static struct MachineDriver ccastles_machine = {
  /* basic machine hardware */
  {
    {
      CPU_M6502,
      1500000,	/* 1.5 Mhz */
      0,
      readmem,writemem,0,0,
      ccastles_interrupt,4
    }
  },
  60,
  ccastles_init_machine,
  
  /* video hardware -- game resolution is 256x232x4
     Note: SCROLLING IS SCREWEY!  The vertical gap is
     due to the 256x256 bitmap.  However, scrolling is
     still wrong, but until I see an actual machine,
     I won't know how to fix it... <sigh> */
  256, 232, { 0, 255, 0, 231 },
  ccastles_gfxdecodeinfo,
  sizeof(ccastles_palette)/3,sizeof(ccastles_colortable),
  0,
  
  0,
  ccastles_vh_start,
  ccastles_vh_stop,
  ccastles_vh_screenrefresh,
  
  /* sound hardware */
  0,
  0,
  ccastles_sh_start,
  ccastles_sh_stop,
  ccastles_sh_update
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(ccastles_rom)
     ROM_REGION(0x14000)	/* 64k for code */
     ROM_LOAD("ccastles.303", 0xA000, 0x2000)
     ROM_LOAD("ccastles.304", 0xC000, 0x2000)
     ROM_LOAD("ccastles.305", 0xE000, 0x2000)
     ROM_LOAD("ccastles.102", 0x10000, 0x2000)	/* Bank switched ROMs con- */
     ROM_LOAD("ccastles.101", 0x12000, 0x2000)	/* aining level data. */

     ROM_REGION(0x4000)	/* temporary space for graphics */
     ROM_LOAD("ccastles.107", 0x0000, 0x2000)
     ROM_LOAD("ccastles.106", 0x2000, 0x2000)
ROM_END

struct GameDriver ccastles_driver = {
  "Crystal Castles",
  "ccastles",
  "PAT LAWRENCE\nCHRIS HARDY\nSTEVE CLYNES\n",
  &ccastles_machine,

  ccastles_rom,
  0, 0,
  0,

  ccastles_input_ports, ccastles_trak_ports,
  ccastles_dsw, ccastles_keys,

  0, ccastles_palette, ccastles_colortable,

  8*13, 8*16,

  /* Not sure where the high scores are saved in RAM... */      
  0x00, 0x00
};
