/***************************************************************************

     TEMPEST
     -------
     HEX        R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
     0000-07FF  R/W   D  D  D  D  D  D  D  D   program ram (2K)
     0800-080F   W                D  D  D  D   Colour ram

     0C00        R                         D   Right coin sw
     0C00        R                      D      Center coin sw
     0C00        R                   D         Left coin sw
     0C00        R                D            Slam sw
     0C00        R             D               Self test sw
     0C00        R          D                  Diagnostic step sw
     0C00        R       D                     Halt
     0C00        R    D                        3khz ??
     0D00        R    D  D  D  D  D  D  D  D   option switches
     0E00        R    D  D  D  D  D  D  D  D   option switches

     2000-2FFF  R/W   D  D  D  D  D  D  D  D   Vector Ram (4K)
     3000-3FFF   R    D  D  D  D  D  D  D  D   Vector Rom (4K)

     4000        W                         D   Right coin counter
     4000        W                      D      left  coin counter
     4000        W                D            Video invert - x
     4000        W             D               Video invert - y
     4800        W                             Vector generator GO

     5000        W                             WD clear
     5800        W                             Vect gen reset

     6000-603F   W    D  D  D  D  D  D  D  D   EAROM write
     6040        W    D  D  D  D  D  D  D  D   EAROM control
     6040        R    D                        Mathbox status
     6050        R    D  D  D  D  D  D  D  D   EAROM read

     6060        R    D  D  D  D  D  D  D  D   Mathbox read
     6070        R    D  D  D  D  D  D  D  D   Mathbox read
     6080-609F   W    D  D  D  D  D  D  D  D   Mathbox start

     60C0-60CF  R/W   D  D  D  D  D  D  D  D   Custom audio chip 1
     60D0-60DF  R/W   D  D  D  D  D  D  D  D   Custom audio chip 2

     60E0        R                         D   one player start LED
     60E0        R                      D      two player start LED
     60E0        R                   D         FLIP

     9000-DFFF  R     D  D  D  D  D  D  D  D   Program ROM (20K)

     notes: program ram decode may be incorrect, but it appears like
     this on the schematics, and the troubleshooting guide.

     ZAP1,FIRE1,FIRE2,ZAP2 go to pokey2 , bits 3,and 4
     (depending on state of FLIP)
     player1 start, player2 start are pokey2 , bits 5 and 6

     encoder wheel goes to pokey1 bits 0-3
     pokey1, bit4 is cocktail detect


TEMPEST SWITCH SETTINGS (Atari, 1980)
-------------------------------------


GAME OPTIONS:
(8-position switch at L12 on Analog Vector-Generator PCB)

1   2   3   4   5   6   7   8   Meaning
-------------------------------------------------------------------------
Off Off                         2 lives per game
On  On                          3 lives per game
On  Off                         4 lives per game
Off On                          5 lives per game
        On  On  Off             Bonus life every 10000 pts
        On  On  On              Bonus life every 20000 pts
        On  Off On              Bonus life every 30000 pts
        On  Off Off             Bonus life every 40000 pts
        Off On  On              Bonus life every 50000 pts
        Off On  Off             Bonus life every 60000 pts
        Off Off On              Bonus life every 70000 pts
        Off Off Off             No bonus lives
                    On  On      English
                    On  Off     French
                    Off On      German
                    Off Off     Spanish
                            On  1-credit minimum
                            Off 2-credit minimum


GAME OPTIONS:
(4-position switch at D/E2 on Math Box PCB)

1   2   3   4                   Meaning
-------------------------------------------------------------------------
    Off                         Minimum rating range: 1, 3, 5, 7, 9
    On                          Minimum rating range tied to high score
        Off Off                 Medium difficulty (see notes)
        Off On                  Easy difficulty (see notes)
        On  Off                 Hard difficulty (see notes)
        On  On                  Medium difficulty (see notes)


PRICING OPTIONS:
(8-position switch at N13 on Analog Vector-Generator PCB)

1   2   3   4   5   6   7   8   Meaning
-------------------------------------------------------------------------
On  On  On                      No bonus coins
On  On  Off                     For every 2 coins, game adds 1 more coin
On  Off On                      For every 4 coins, game adds 1 more coin
On  Off Off                     For every 4 coins, game adds 2 more coins
Off On  On                      For every 5 coins, game adds 1 more coin
Off On  Off                     For every 3 coins, game adds 1 more coin
On  Off                 Off On  Demonstration Mode (see notes)
Off Off                 Off On  Demonstration-Freeze Mode (see notes)
            On                  Left coin mech * 1
            Off                 Left coin mech * 2
                On  On          Right coin mech * 1
                On  Off         Right coin mech * 4
                Off On          Right coin mech * 5
                Off Off         Right coin mech * 6
                        Off On  Free Play
                        Off Off 1 coin 2 plays
                        On  On  1 coin 1 play
                        On  Off 2 coins 1 play


GAME SETTING NOTES:
-------------------

Demonstration Mode:
- Plays a normal game of Tempest, but pressing SUPERZAP sends you
  directly to the next level.

Demonstration-Freeze Mode:
- Just like Demonstration Mode, but with frozen screen action.

Both Demonstration Modes:
- Pressing RESET in either mode will cause the game to lock up.
  To recover, set switch 1 to On.
- You can start at any level from 1..81, so it's an easy way of
  seeing what the game can throw at you
- The score is zeroed at the end of the game, so you also don't
  have to worry about artificially high scores disrupting your
  scoring records as stored in the game's EAROM.

Easy Difficulty:
- Enemies move more slowly
- One less enemy shot on the screen at any given time

Hard Difficulty:
- Enemies move more quickly
- 1-4 more enemy shots on the screen at any given time
- One more enemy may be on the screen at any given time

High Scores:
- Changing toggles 1-5 at L12 (more/fewer lives, bonus ship levels)
  will erase the high score table.
- You should also wait 8-10 seconds after a game has been played
  before entering self-test mode or powering down; otherwise, you
  might erase or corrupt the high score table.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/mathbox.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/atari_vg.h"
#include "sndhrdw/pokyintf.h"


int tempest_freakout(int data);
int tempest_IN0_r(int offset);
int tempest_IN1_r(int offset);
int tempest_spinner(int offset);

static struct MemoryReadAddress readmem[] =
{
//	{ 0x011f, 0x011f, tempest_freakout },	/* hack to avoid lockup at 150,000 points */
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x9000, 0xdfff, MRA_ROM },
	{ 0x3000, 0x3fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x2000, 0x2fff, MRA_RAM, &vectorram },
	{ 0x0c00, 0x0c00, tempest_IN0_r },	/* IN0 */
	{ 0x60c8, 0x60c8, tempest_IN1_r },	/* IN1/DSW0 */
	{ 0x60d8, 0x60d8, input_port_2_r },	/* IN2 */
	{ 0x0d00, 0x0d00, input_port_3_r },	/* DSW1 */
	{ 0x0e00, 0x0e00, input_port_4_r },	/* DSW2 */
	{ 0x6040, 0x6040, mb_status_r },
	{ 0x6050, 0x6050, atari_vg_earom_r },
	{ 0x6060, 0x6060, mb_lo_r },
	{ 0x6070, 0x6070, mb_hi_r },
	{ 0x60c0, 0x60cf, pokey1_r },
	{ 0x60d0, 0x60df, pokey2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x080f, atari_vg_colorram_w },
	{ 0x2000, 0x2fff, MWA_RAM, &vectorram },
	{ 0x6000, 0x603f, atari_vg_earom_w },
	{ 0x6040, 0x6040, atari_vg_earom_ctrl },
	{ 0x60c0, 0x60cf, pokey1_w },
	{ 0x60d0, 0x60df, pokey2_w },
	{ 0x6080, 0x609f, mb_go },
	{ 0x4800, 0x4800, atari_vg_go },
	{ 0x5000, 0x5000, MWA_RAM },
	{ 0x5800, 0x5800, vg_reset },
	{ 0x9000, 0xdfff, MWA_ROM },
	{ 0x3000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x60e0, 0x60e0, MWA_NOP },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT)
	PORT_DIPNAME ( 0x10, 0x10, "Test", OSD_KEY_F2 )
	PORT_DIPSETTING(     0x10, "Off" )
	PORT_DIPSETTING(     0x00, "On" )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Diagnostic Step", OSD_KEY_F1, IP_JOY_NONE, 0 )
	/* bit 6 is the VG HALT bit */
	/* bit 7 is tied to a 3khz (?) clock */
	/* they are both handled by tempest_IN0_r() */

	PORT_START	/* IN1/DSW0 */
	/* The four lower bits are for the spinner. We cheat here
	 * by asking for joystick/keyboard input. It might be a good
	 * idea to have generic IPT-definitions for trackball support
	 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* spinner bit 2 */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* spinner bit 3 */
	/* The next one is reponsible for cocktail mode.
	 * According to the documentation, this is not a switch, although
	 * it may have been planned to put it on the Math Box PCB, D/E2 )
	 */
	PORT_DIPNAME( 0x10, 0x10, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_DIPNAME ( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(     0x01, "Easy" )
	PORT_DIPSETTING(     0x00, "Medium1" )
	PORT_DIPSETTING(     0x03, "Medium2" )
	PORT_DIPSETTING(     0x02, "Hard" )
	PORT_DIPNAME ( 0x04, 0x00, "Rating", IP_KEY_NONE )
	PORT_DIPSETTING(     0x00, "1, 3, 5, 7, 9" )
	PORT_DIPSETTING(     0x04, "tied to high score" )
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 - (N13 on analog vector generator PCB */
	PORT_DIPNAME (0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING (   0x03, "1 Coin/2 Credits" )
	PORT_DIPSETTING (   0x02, "Free Play" )
	PORT_DIPNAME (0x0c, 0x00, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x04, "*4" )
	PORT_DIPSETTING (   0x08, "*5" )
	PORT_DIPSETTING (   0x0c, "*6" )
	PORT_DIPNAME (0x10, 0x00, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x10, "*2" )
	PORT_DIPNAME (0xe0, 0x00, "Bonus Coins", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "None" )
	PORT_DIPSETTING (   0x80, "1 each 5" )
	PORT_DIPSETTING (   0x40, "1 each 4 (+Demo)" )
	PORT_DIPSETTING (   0xa0, "1 each 3" )
	PORT_DIPSETTING (   0x60, "2 each 4 (+Demo)" )
	PORT_DIPSETTING (   0x20, "1 each 2" )
	PORT_DIPSETTING (   0xc0, "Freeze Mode" )
	PORT_DIPSETTING (   0xe0, "Freeze Mode" )

	PORT_START	/* DSW2 - (L12 on analog vector generator PCB */
	PORT_DIPNAME (0x01, 0x00, "Minimum", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "1 Credit" )
	PORT_DIPSETTING (   0x01, "2 Credit" )
	PORT_DIPNAME (0x06, 0x00, "Language", IP_KEY_NONE)
	PORT_DIPSETTING (   0x00, "English" )
	PORT_DIPSETTING (   0x02, "French" )
	PORT_DIPSETTING (   0x04, "German" )
	PORT_DIPSETTING (   0x06, "Spanish" )
	PORT_DIPNAME (0x38, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING (   0x08, "10000" )
	PORT_DIPSETTING (   0x00, "20000" )
	PORT_DIPSETTING (   0x10, "30000" )
	PORT_DIPSETTING (   0x18, "40000" )
	PORT_DIPSETTING (   0x20, "50000" )
	PORT_DIPSETTING (   0x28, "60000" )
	PORT_DIPSETTING (   0x30, "70000" )
	PORT_DIPSETTING (   0x38, "None" )
	PORT_DIPNAME (0xc0, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING (   0xc0, "2" )
	PORT_DIPSETTING (   0x00, "3" )
	PORT_DIPSETTING (   0x40, "4" )
	PORT_DIPSETTING (   0x80, "5" )
INPUT_PORTS_END

static struct TrakPort trak_ports[] =
{
	{
		X_AXIS,
		1,
		1.0,
		tempest_spinner
	},
        { -1 }
};

/*
 * Tempest does not really have a colorprom, nor any graphics to
 * decode. It has a 16 byte long colorram, but only the lower nibble
 * of each byte is important.
 * The (inverted) meaning of the bits is:
 * 3-green 2-blue 1-red 0-intensity.
 * To dynamically alter the colors, we need access to the colortable.
 * This is what this fakelayout is for.
 */
static struct GfxLayout fakelayout =
{
        1,1,
        0,
        1,
        { 0 },
        { 0 },
        { 0 },
        0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0,      &fakelayout,     0, 256 },
        { -1 } /* end of array */
};

static unsigned char color_prom[] =
{
	0x02,0x02,0x02,	/* WHITE */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x02,0x02, /* CYAN */
	0x00,0x01,0x01, /* CYAN */
	0x02,0x02,0x00, /* YELLOW */
	0x01,0x01,0x00, /* YELLOW */
	0x00,0x02,0x00, /* GREEN */
	0x00,0x01,0x00, /* GREEN */
	0x02,0x00,0x02, /* MAGENTA */
	0x01,0x00,0x01, /* MAGENTA */
	0x00,0x00,0x02,	/* BLUE */
	0x00,0x00,0x01,	/* BLUE */
	0x02,0x00,0x00, /* RED */
	0x01,0x00,0x00, /* RED */
	0x00,0x00,0x00, /* BLACK */
	0x00,0x00,0x00  /* BLACK */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,4 /* 4 interrupts per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	224, 288, { 0, 580, 0, 540 },
	gfxdecodeinfo,
	256,256,
	atari_vg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	atari_vg_avg_start,
	atari_vg_stop,
	atari_vg_screenrefresh,

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

static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	/* I think the whole block of 0x200 needs saving but I'm not positive - LBO */
	if (memcmp(&RAM[0x0606],"\x07\x04\x01",3))
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0600],1,0x200,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


/* This is no longer valid. We save the earom instead */
static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x0600],1,0x200,f);
		fclose(f);
	}
}

ROM_START( tempest_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "136002.113", 0x9000, 0x0800, 0xc4180c0e )
	ROM_LOAD( "136002.114", 0x9800, 0x0800, 0x3bf4999a )
	ROM_LOAD( "136002.115", 0xa000, 0x0800, 0x22bb1713 )
	ROM_LOAD( "136002.116", 0xa800, 0x0800, 0xee2a0306 )
	ROM_LOAD( "136002.117", 0xb000, 0x0800, 0x85788680 )
	ROM_LOAD( "136002.118", 0xb800, 0x0800, 0x461ca3a4 )
	ROM_LOAD( "136002.119", 0xc000, 0x0800, 0x02e4a6ae )
	ROM_LOAD( "136002.120", 0xc800, 0x0800, 0x82d1e4ed )
	ROM_LOAD( "136002.121", 0xd000, 0x0800, 0xe663151f )
	ROM_LOAD( "136002.122", 0xd800, 0x0800, 0x292ebfb4 )
	ROM_RELOAD(             0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "136002.123", 0x3000, 0x0800, 0xca906060 )
	ROM_LOAD( "136002.124", 0x3800, 0x0800, 0xb6c4f9f8 )
ROM_END

#if 0
ROM_START( tempest_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tempest.x", 0x9000, 0x1000, -1 )
	ROM_LOAD( "tempest.1", 0xa000, 0x1000, -1 )
	ROM_LOAD( "tempest.3", 0xb000, 0x1000, -1 )
	ROM_LOAD( "tempest.5", 0xc000, 0x1000, -1 )
	ROM_LOAD( "tempest.7", 0xd000, 0x1000, -1 )
	ROM_RELOAD(            0xf000, 0x1000 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "tempest.np3", 0x3000, 0x1000, -1 )
ROM_END
#endif

struct GameDriver tempest_driver =
{
	"Tempest",
	"tempest",
	"Al Kossow\n"
	"Hedley Rainnie\n"
	"Eric Smith\n"
	"  (original VECSIM code)\n"
	"Brad Oliver\n"
	"  (MAME driver)\n"
	"Bernd Wiebelt\n"
	"Allard van der Bas\n"
	"  (additional work)\n"
	"Neil Bradley\n"
	"  (critical bug fix)\n",
	&machine_driver,

	tempest_rom,
	0, 0,
	0,

	0/*TBR*/,input_ports, trak_ports,0/*TBR*/,0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	atari_vg_earom_load, atari_vg_earom_save
};
