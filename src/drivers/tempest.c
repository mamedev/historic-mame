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


extern int tempest_IN0_r(int offset);
extern int tempest_IN3_r(int offset);
extern int tempest_interrupt(void);
extern int tempest_spinner(int);

extern int bzone_vh_start(void);
extern void bzone_vh_stop(void);
extern void bzone_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void bzone_vh_init_colors(unsigned char *palette, unsigned char *colortable, const unsigned char *color_prom);
extern int bzone_rand_r(int offset);

extern void milliped_pokey1_w(int offset,int data);
extern void milliped_pokey2_w(int offset,int data);
extern int milliped_sh_start(void);
extern void milliped_sh_stop(void);
extern void milliped_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x9000, 0xdfff, MRA_ROM },
	{ 0x3000, 0x3fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x2000, 0x2fff, MRA_RAM, &vectorram },
	{ 0x0c00, 0x0c00, tempest_IN0_r },	/* IN0 */
	{ 0x0d00, 0x0d00, input_port_1_r },	/* DSW1 */
	{ 0x0e00, 0x0e00, input_port_2_r },	/* DSW2 */
	{ 0x60c8, 0x60c8, tempest_IN3_r },	/* IN3 */
	{ 0x60d8, 0x60d8, input_port_4_r },	/* IN4 */
	{ 0x6040, 0x6040, mb_status_r },
	{ 0x6060, 0x6060, mb_lo_r },
	{ 0x6070, 0x6070, mb_hi_r },
	{ 0x60ca, 0x60ca, bzone_rand_r },
	{ 0x60da, 0x60da, bzone_rand_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x080f, MWA_RAM, &colorram },
	{ 0x2000, 0x2fff, MWA_RAM, &vectorram },
	{ 0x60c0, 0x60cf, milliped_pokey1_w },
	{ 0x60d0, 0x60df, milliped_pokey2_w },
	{ 0x6080, 0x609f, mb_go },
	{ 0x4800, 0x4800, vg_go },
	{ 0x5000, 0x5000, MWA_RAM },
	{ 0x5800, 0x5800, vg_reset },
	{ 0x9000, 0xdfff, MWA_ROM },
	{ 0x3000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_4, 0, OSD_KEY_F1, OSD_KEY_F2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW2 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2/DSW3 */
		0x00,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN4 */
		0x03,
		{ 0, 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0, 0 }
	},
	{ -1 }
};

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


static struct KEYSet keys[] =
{
	{ 3, 0, "TURN CLOCKWISE" },
	{ 3, 1, "TURN ANTICLOCKWISE" },
        { 4, 4, "FIRE" },
	{ 4, 3, "SUPER ZAPPER" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x38, "BONUS EVERY", { "20000", "10000", "30000", "40000",
				    "50000", "60000", "70000", "NONE" } },
	{ 2, 0x06, "LANGUAGE", { "ENGLISH", "FRENCH", "GERMAN", "SPANISH" } },
	{ 4, 0x01, "MINIUM RATING", { "1/3/5/7/9", "TIED TO HIGHSCORE" } },
	{ 4, 0x06, "DIFFICULTY", { "MEDIUM", "EASY", "HARD", "MEDIUM" } },
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static unsigned char color_prom[] =
{
	0x02,0x02,0x02,	/* WHITE */
	0x02,0x02,0x00, /* YELLOW */
	0x02,0x00,0x02, /* MAGENTA */
	0x02,0x00,0x00, /* RED */
	0x00,0x02,0x02, /* CYAN */
	0x00,0x02,0x00, /* GREEN */
	0x00,0x00,0x02, /* BLUE */
	0x00,0x00,0x00,	/* BLACK */
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
			tempest_interrupt,4 /* 4 interrupts per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	404, 600, { 0, 404, 0, 600 },
	gfxdecodeinfo,
	128,128,
	bzone_vh_init_colors,

	0,
	bzone_vh_start,
	bzone_vh_stop,
	bzone_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	milliped_sh_start,
	milliped_sh_stop,
	milliped_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tempest_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "136002.113", 0x9000, 0x0800 )
	ROM_LOAD( "136002.114", 0x9800, 0x0800 )
	ROM_LOAD( "136002.115", 0xa000, 0x0800 )
	ROM_LOAD( "136002.116", 0xa800, 0x0800 )
	ROM_LOAD( "136002.117", 0xb000, 0x0800 )
	ROM_LOAD( "136002.118", 0xb800, 0x0800 )
	ROM_LOAD( "136002.119", 0xc000, 0x0800 )
	ROM_LOAD( "136002.120", 0xc800, 0x0800 )
	ROM_LOAD( "136002.121", 0xd000, 0x0800 )
	ROM_LOAD( "136002.122", 0xd800, 0x0800 )
	ROM_LOAD( "136002.122", 0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "136002.123", 0x3000, 0x0800 )
	ROM_LOAD( "136002.124", 0x3800, 0x0800 )
ROM_END



struct GameDriver tempest_driver =
{
	"Tempest",
	"tempest",
	"BRAD OLIVER\nBERND WIEBELT\nALLARD VAN DER BAS\nNEIL BRADLEY\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH",
	&machine_driver,

	tempest_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	0, 0
};
