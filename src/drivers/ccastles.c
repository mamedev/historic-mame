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
9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                           R                D  COIN R
                                           R              D    COIN L
                                           R            D      COIN AUX
                                           R          D        SLAM
                                           R        D          SELF TEST
                                           R      D            VBLANK
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
9E81                                0 0 1  W                D  Flip screen
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
1) Sound.  It's better! The game sounds are slightly slow.....
3) The game is always in cocktail mode.  If I try to take it out of
   cocktail mode, the player 1 and player 2 buttons no longer work.
   Needless to say, I have not emulated cocktail mode, nor do I plan to so
   only try 1 player games for now. :-)
7) Vertical scrolling is wrong.  Is horizontal scrolling right?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokyintf.h"

extern unsigned char *ccastles_bitmapram;
extern unsigned char *ccastles_mobram1;
extern unsigned char *ccastles_mobram2;

void ccastles_paletteram_w(int offset,int data);
int ccastles_vh_start(void);
void ccastles_vh_stop(void);
void ccastles_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void ccastles_vh_screenrefresh(struct osd_bitmap *bitmap);

int ccastles_trakball_x(int data);
int ccastles_trakball_y(int data);

int ccastles_trakball_r(int offset);
int ccastles_xy_r(int offset);
int ccastles_rom_r(int offset);
int ccastles_bitmode_r(int offset);

void ccastles_xy_w(int offset, int data);
void ccastles_axy_w(int offset, int data);
void ccastles_bitmode_w(int offset, int data);
void ccastles_pokey0_w(int offset,int data);
void ccastles_pokey1_w(int offset,int data);
void ccastles_bitmapram_w(int offset,int data);
void ccastles_bankswitch_w(int offset,int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0001, MRA_RAM },
	{ 0x0002, 0x0002, ccastles_bitmode_r },
	{ 0x0003, 0x90ff, MRA_RAM },	/* All RAM */
	{ 0x9400, 0x9401, ccastles_trakball_r },
	{ 0x9500, 0x9501, ccastles_trakball_r },	/* mirror address for the above */
	{ 0x9600, 0x9600, input_port_0_r },	/* IN0 */
	{ 0x9a08, 0x9a08, input_port_1_r },	/* OPTION SW */
	{ 0x9800, 0x980f, pokey1_r }, /* Random # generator on a Pokey */
	{ 0x9a00, 0x9a0f, pokey2_r }, /* Random # generator on a Pokey */
	{ 0xA000, 0xDFFF, ccastles_rom_r },
	{ 0xE000, 0xFFFF, MRA_ROM },	/* ROMs/interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0001, ccastles_xy_w },
	{ 0x0002, 0x0002, ccastles_bitmode_w },
	{ 0x0003, 0x90ff, MWA_RAM },  /* All RAM */
	{ 0x9800, 0x980F, pokey1_w },
	{ 0x9A00, 0x9A0F, pokey2_w },
	{ 0x9C80, 0x9C80, MWA_RAM },    /* Horizontal Scroll */
	{ 0x9D00, 0x9D00, MWA_RAM },    /* Vertical Scroll */
	{ 0x9D80, 0x9D80, MWA_NOP },
	{ 0x9E00, 0x9E00, MWA_NOP },
	{ 0x9E80, 0x9E81, MWA_NOP },
	{ 0x9E85, 0x9E86, MWA_NOP },
	{ 0x9E87, 0x9E87, ccastles_bankswitch_w },
	{ 0x9F00, 0x9F01, ccastles_axy_w },
	{ 0x9F02, 0x9F07, MWA_RAM },
	{ 0x9F80, 0x9FBF, ccastles_paletteram_w },
	{ -1 }	/* end of table */
};

static struct InputPort ccastles_input_ports[] = {
  {	/* IN0 */
    0xdf,
    { OSD_KEY_4, OSD_KEY_3, OSD_KEY_5, OSD_KEY_T, 0, IPB_VBLANK, OSD_KEY_CONTROL, 0 },
    { 0, 0, 0, 0, 0, 0, OSD_JOY_FIRE, 0 }
  },
  {	/* IN1 */
    0x3f,
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
  { 0, 0x10, "SELF TEST", { "ON", "OFF" }, 1 },
  { -1 }
};

static struct GfxLayout ccastles_spritelayout =
{
	8,16,	/* 8*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel (the most significant bit is always 0) */
	{ 0x2000*8+0, 0x2000*8+4, 0, 4 },	/* the three bitplanes are separated */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
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
	{ 1, 0x0000, &ccastles_spritelayout,  0, 1 },
	{ 0, 0,      &fakelayout,            16, 1 },
	{ -1 } /* end of array */
};




static struct MachineDriver ccastles_machine = {
  /* basic machine hardware */
  {
    {
      CPU_M6502,
      1500000,	/* 1.5 Mhz */
      0,
      readmem,writemem,0,0,
      interrupt,4
    }
  },
  60,
  0,
  256, 232, { 0, 255, 0, 231 },
  gfxdecodeinfo,
  32, 16+16,
  ccastles_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
  0,
  ccastles_vh_start,
  ccastles_vh_stop,
  ccastles_vh_screenrefresh,

  /* sound hardware */
  0,
  0,
  pokey2_sh_start,
  pokey_sh_stop,
  pokey_sh_update
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(ccastles_rom)
     ROM_REGION(0x14000)	/* 64k for code */
     ROM_LOAD( "ccastles.303",  0xA000, 0x2000, 0xe3d3d32d )
     ROM_LOAD( "ccastles.304",  0xC000, 0x2000, 0x31eab944 )
     ROM_LOAD( "ccastles.305",  0xE000, 0x2000, 0xd765a559 )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, 0x5bbb3ac1 )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, 0xe2aa8e74 )	/* containing level data. */

     ROM_REGION(0x4000)	/* temporary space for graphics */
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, 0x399cc984 )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, 0x8b4c0208 )
ROM_END



static int hiload(const char *name)
{
	/* Read the NVRAM contents from disk */
	/* No check necessary */
	FILE *f;


	if ((f = fopen(name,"rb")) != 0)
	{
		fread(&RAM[0x9000],1,0x100,f);
		fclose(f);
	}
	return 1;
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x9000],1,0x100,f);
		fclose(f);
	}
}



struct GameDriver ccastles_driver =
{
	"Crystal Castles",
	"ccastles",
	"PAT LAWRENCE\nCHRIS HARDY\nSTEVE CLYNES\nNICOLA SALMORIA",
	&ccastles_machine,

	ccastles_rom,
	0, 0,
	0,

	ccastles_input_ports, 0, ccastles_trak_ports,
	ccastles_dsw, ccastles_keys,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
