/***************************************************************************

Black Widow memory map (preliminary)

0000-04ff RAM
0800      COIN_IN
0a00      IN1
0c00      IN2

2000-27ff Vector generator RAM
5000-7fff ROM

BLACK WIDOW SWITCH SETTINGS (Atari, 1983)
-----------------------------------------

-------------------------------------------------------------------------------
Settings of 8-Toggle Switch on Black Widow CPU PCB (at D4)
 8   7   6   5   4   3   2   1   Option
-------------------------------------------------------------------------------
Off Off                          1 coin/1 credit <
On  On                           1 coin/2 credits
On  Off                          2 coins/1 credit
Off On                           Free play

        Off Off                  Right coin mechanism x 1 <
        On  Off                  Right coin mechanism x 4
        Off On                   Right coin mechanism x 5
        On  On                   Right coin mechanism x 6

                Off              Left coin mechanism x 1 <
                On               Left coin mechanism x 2

                    Off Off Off  No bonus coins (0)* <
                    Off On  On   No bonus coins (6)
                    On  On  On   No bonus coins (7)

                    On  Off Off  For every 2 coins inserted,
                                 logic adds 1 more coin (1)
                    Off On  Off  For every 4 coins inserted,
                                 logic adds 1 more coin (2)
                    On  On  Off  For every 4 coins inserted,
                                 logic adds 2 more coins (3)
                    Off Off On   For every 5 coins inserted,
                                 logic adds 1 more coin (4)
                    On  Off On   For every 3 coins inserted,
                                 logic adds 1 more coin (5)

-------------------------------------------------------------------------------

* The numbers in parentheses will appear on the BONUS ADDER line in the
  Operator Information Display (Figure 2-1) for these settings.
< Manufacturer's recommended setting

-------------------------------------------------------------------------------
Settings of 8-Toggle Switch on Black Widow CPU PCB (at B4)
 8   7   6   5   4   3   2   1   Option
 
Note: The bits are the exact opposite of the switch numbers - switch 8 is bit 0.
-------------------------------------------------------------------------------
Off Off                          Maximum start at level 13
On  Off                          Maximum start at level 21 <
Off On                           Maximum start at level 37
On  On                           Maximum start at level 53

        Off Off                  3 spiders per game <
        On  Off                  4 spiders per game
        Off On                   5 spiders per game
        On  On                   6 spiders per game

                Off Off          Easy game play
                On  Off          Medium game play <
                Off On           Hard game play
                On  On           Demonstration mode

                        Off Off  Bonus spider every 20,000 points <
                        On  Off  Bonus spider every 30,000 points
                        Off On   Bonus spider every 40,000 points
                        On  On   No bonus

-------------------------------------------------------------------------------

< Manufacturer's recommended setting


GRAVITAR SWITCH SETTINGS (Atari, 1982)
--------------------------------------
 
-------------------------------------------------------------------------------
Settings of 8-Toggle Switch on Gravitar PCB (at B4)
 8   7   6   5   4   3   2   1   Option
-------------------------------------------------------------------------------
Off On                           Free play
On  On                           1 coin for 2 credits
Off Off                          1 coin for 1 credit <
On  Off                          2 coins for 1 credit

        Off Off                  Right coin mechanism x 1 <
        On  Off                  Right coin mechanism x 4
        Off On                   Right coin mechanism x 5
        On  On                   Right coin mechanism x 6

                Off              Left coin mechanism x 1 <
                On               Left coin mechanism x 2

                    Off Off Off  No bonus coins <

                    Off On  Off  For every 4 coins inserted,
                                 logic adds 1 more coin
                    On  On  Off  For every 4 coins inserted,
                                 logic adds 2 more coins
                    Off Off On   For every 5 coins inserted,
                                 logic adds 1 more coin
                    On  Off On   For every 3 coins inserted,
                                 logic adds 1 more coin

                    Off On  On   No bonus coins
                    On  Off Off  ??? (not in manual!)
                    On  On  On   No bonus coins

-------------------------------------------------------------------------------

< Manufacturer's recommended setting

-------------------------------------------------------------------------------
Settings of 8-Toggle Switch on Gravitar PCB (at D4)
 8   7   6   5   4   3   2   1   Option
-------------------------------------------------------------------------------
                        On  On   No bonus
                        Off Off  Bonus ship every 10,000 points <
 d   d               d  On  Off  Bonus ship every 20,000 points
 e   e               e  Off On   Bonus ship every 30,000 points
 s   s               s
 U   U          On   U           Easy game play <
                Off              Hard game play
 t   t               t
 o   o  Off Off      o           3 ships per game
 N   N  On  Off      N           4 ships per game <
        Off On                   5 ships per game
        On  On                   6 ships per game

-------------------------------------------------------------------------------

< Manufacturer's recommended setting

Space Duel Settings
-------------------

(Settings of 8-Toggle Switch on Space Duel game PCB at D4)
Note: The bits are the exact opposite of the switch numbers - switch 8 is bit 0.

 8   7   6   5   4   3   2   1       Option
On  Off                         3 ships per game
Off Off                         4 ships per game $
On  On                          5 ships per game
Off On                          6 ships per game
        On  Off                *Easy game difficulty
        Off Off                 Normal game difficulty $
        On  On                  Medium game difficulty
        Off On                  Hard game difficulty
                Off Off         English $
                On  Off         German
                On  On          Spanish
                Off On          French
                                Bonus life granted every:
                        Off On  8,000 points
                        Off Off 10,000 points
                        On  Off 15,000 points
                        On  On  No bonus life

$Manufacturer's suggested settings
*Easy-In the beginning of the first wave, 3 targets appear on the
screen.  Targets increase by one in each new wave.
Normal-Space station action is the same as 'Easy'.  Fighter action has
4 targets in the beginning of the first wave.  Targets increase by 2
in each new wave.  Targets move faster and more targets enter.
Medium and Hard-In the beginning of the first wave, 4 targets appear
on the screen.  Targets increase by 2 in each new wave.  As difficulty
increases, targets move faster, and more targets enter.


(Settings of 8-Toggle Switch on Space Duel game PCB at B4)
 8   7   6   5   4   3   2   1       Option
Off On                          Free play
Off Off                        *1 coin for 1 game (or 1 player) $
On  On                          1 coin for 2 game (or 2 players)
On  Off                         2 coins for 1 game (or 1 player)
        Off Off                 Right coin mech x 1 $
        On  Off                 Right coin mech x 4
        Off On                  Right coin mech x 5
        On  On                  Right coin mech x 6
                Off             Left coin mech x 1 $
                On              Left coin mech x 2
                    Off Off Off No bonus coins $
                    Off On  Off For every 4 coins, game logic adds 1 more coin
                    On  On  Off For every 4 coins, game logic adds 2 more coin
                    Off On  On  For every 5 coins, game logic adds 1 more coin
                    On  Off On**For every 3 coins, game logic adds 1 more coin

$Manufacturer's suggested settings

**In operator Information Display, this option displays same as no bonus.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"


extern int spacduel_IN3_r(int offset);

extern void bzone_vh_init_colors(unsigned char *palette, unsigned char *colortable, const unsigned char *color_prom);
extern int bzone_vh_start(void);
extern void bzone_vh_stop(void);
extern void bzone_vh_screenrefresh(struct osd_bitmap *bitmap);
extern int bzone_IN0_r(int offset);
extern int bzone_rand_r(int offset);

extern void milliped_pokey1_w(int offset,int data);
extern void milliped_pokey2_w(int offset,int data);
extern int milliped_sh_start(void);
extern void milliped_sh_stop(void);
extern void milliped_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x9000, 0xffff, MRA_ROM },
	{ 0x2800, 0x5fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM, &vectorram },
	{ 0x7800, 0x7800, bzone_IN0_r },	/* IN0 */
	{ 0x6008, 0x6008, input_port_1_r },	/* DSW1 */
	{ 0x6808, 0x6808, input_port_2_r },	/* DSW2 */
	{ 0x8000, 0x8000, input_port_3_r },	/* IN1 */
	{ 0x8800, 0x8800, input_port_4_r },	/* IN1 */
	{ 0x600a, 0x600a, bzone_rand_r },
	{ 0x680a, 0x680a, bzone_rand_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x27ff, MWA_RAM, &vectorram },
	{ 0x6000, 0x67ff, milliped_pokey1_w },
	{ 0x6800, 0x6fff, milliped_pokey2_w },
	{ 0x8840, 0x8840, vg_go },
	{ 0x8880, 0x8880, vg_reset },
	{ 0x9000, 0xffff, MWA_ROM },
	{ 0x2800, 0x5fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress spacduel_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x4000, 0x8fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
	{ 0x2800, 0x3fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM, &vectorram },
	{ 0x0800, 0x0800, bzone_IN0_r },	/* IN0 */
	{ 0x1008, 0x1008, input_port_1_r },	/* DSW1 */
	{ 0x1408, 0x1408, input_port_2_r },	/* DSW2 */
	{ 0x0900, 0x0907, spacduel_IN3_r },	/* IN1 */
	{ 0x0900, 0x0907, input_port_3_r },	/* IN1 */
	{ 0x100a, 0x100a, bzone_rand_r },
	{ 0x140a, 0x140a, bzone_rand_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress spacduel_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x27ff, MWA_RAM, &vectorram },
	{ 0x1000, 0x13ff, milliped_pokey1_w },
	{ 0x1400, 0x17ff, milliped_pokey2_w },
	{ 0x0c80, 0x0c80, vg_go },
	{ 0x0d80, 0x0d80, vg_reset },
	{ 0x4000, 0x8fff, MWA_ROM },
	{ 0x2800, 0x3fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort bwidow_input_ports[] =
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
		0x11,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0xff,
		{ OSD_KEY_D, OSD_KEY_A, OSD_KEY_S, OSD_KEY_W,
			0, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
			0, 0, 0, 0 }
	},
	{       /* IN2 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
			0, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_FIRE4, OSD_JOY_FIRE1, OSD_JOY_FIRE3, OSD_JOY_FIRE2,
			0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet bwidow_keys[] =
{
        { 3, 0, "MOVE RIGHT" },
        { 3, 1, "MOVE LEFT" },
        { 3, 2, "MOVE DOWN" },
        { 3, 3, "MOVE UP" },
        { 4, 0, "FIRE RIGHT" },
        { 4, 1, "FIRE LEFT" },
        { 4, 2, "FIRE DOWN" },
        { 4, 3, "FIRE UP" },
        { -1 }
};


static struct DSW bwidow_dsw[] =
{
	{ 1, 0xc0, "COINS", { "1 COIN 1 CREDIT", "1 COIN 2 CREDITS", "2 COINS 1 CREDIT", "FREE PLAY" } },
	{ 2, 0xc0, "BONUS", { "20000", "30000", "40000", "NONE" } },
	{ 2, 0x30, "DIFFICULTY", { "EASY", "MEDIUM", "HARD", "DEMO" } },
	{ 2, 0x0c, "LIVES", { "3", "4", "5", "6" } },
	{ 2, 0x03, "MAX START", { "LEVEL 13", "LEVEL 21", "LEVEL 37", "LEVEL 53" } },
	{ -1 }
};

static struct InputPort gravitar_input_ports[] =
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
		0x14,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0xff,
		{ OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT,
			OSD_KEY_UP, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_FIRE2, OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT,
			OSD_JOY_UP, 0, 0, 0 }
	},
	{       /* IN2 */
		0xff,
		{ OSD_KEY_ALT, OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT,
			OSD_KEY_UP, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_FIRE2, OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT,
			OSD_JOY_UP, 0, 0, 0 }
	},
	{ -1 }
};

static struct KEYSet gravitar_keys[] =
{
        { 3, 0, "P1 SHIELD" },
        { 3, 1, "P1 FIRE" },
        { 3, 2, "P1 RIGHT" },
        { 3, 3, "P1 LEFT" },
        { 3, 4, "P1 THRUST" },
        { 4, 0, "P2 SHIELD" },
        { 4, 1, "P2 FIRE" },
        { 4, 2, "P2 RIGHT" },
        { 4, 3, "P2 LEFT" },
        { 4, 4, "P2 THRUST" },
        { -1 }
};

static struct DSW gravitar_dsw[] =
{
	{ 1, 0x03, "COINS", { "1 COIN 1 CREDIT", "1 COIN 2 CREDITS", "2 COINS 1 CREDIT", "FREE PLAY" } },
	{ 2, 0xc0, "BONUS", { "20000", "30000", "40000", "NONE" } },
	{ 2, 0x10, "DIFFICULTY", { "HARD", "EASY" } },
	{ 2, 0x0c, "LIVES", { "3", "4", "5", "6" } },
	{ -1 }
};

static struct InputPort spacduel_input_ports[] =
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
	/* These next 2 are shared between 7 memory locations. See machine/spacduel.c for more */
	{       /* IN1 - player 1 controls */
		0x00,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, OSD_KEY_ALT,
			OSD_KEY_UP, OSD_KEY_1, OSD_KEY_2, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE1, OSD_JOY_FIRE2,
			OSD_JOY_UP, 0, 0, 0 }
	},
	{       /* IN2  - player 2 controls */
		0x00,
		{ OSD_KEY_A, OSD_KEY_D, OSD_KEY_SPACE, OSD_KEY_S,
			OSD_KEY_W, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE1, OSD_JOY_FIRE2,
			OSD_JOY_UP, 0, 0, 0 }
	},
	{ -1 }
};

static struct KEYSet spacduel_keys[] =
{
        { 3, 0, "P1 LEFT" },
        { 3, 1, "P1 RIGHT" },
        { 3, 2, "P1 FIRE" },
        { 3, 3, "P1 SHIELD" },
        { 3, 4, "P1 THRUST" },
        { 4, 0, "P2 LEFT" },
        { 4, 1, "P2 RIGHT" },
        { 4, 2, "P2 FIRE" },
        { 4, 3, "P2 SHIELD" },
        { 4, 4, "P2 THRUST" },
        { -1 }
};

static struct DSW spacduel_dsw[] =
{
	{ 1, 0x03, "COINS", { "1 COIN 1 CREDIT", "2 COINS 1 CREDIT", "FREE PLAY", "1 COIN 2 CREDIT" } },
	{ 2, 0xc0, "BONUS", { "10000", "15000", "8000", "NONE" } },
	{ 2, 0x30, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ 2, 0x0c, "DIFFICULTY", { "NORMAL", "EASY", "HARD", "MEDIUM" } },
	{ 2, 0x03, "LIVES", { "4", "3", "6", "5" } },
	{ -1 }
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};


static unsigned char color_prom[] =
{
	0x00,0x00,0x00, /* BLACK */
	0x00,0x00,0xff, /* BLUE */
	0x00,0xff,0x00, /* GREEN */
	0x00,0xff,0xff, /* CYAN */
	0xff,0x00,0x00, /* RED */
	0xff,0x00,0xff, /* MAGENTA */
	0xff,0xff,0x00, /* YELLOW */
	0xff,0xff,0xff	/* WHITE */
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
			interrupt,4 /* 1 interrupt per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	540, 480, { 0, 540, 0, 480 },
	gfxdecodeinfo,
	128, 128,
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

static struct MachineDriver spacduel_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			spacduel_readmem,spacduel_writemem,0,0,
			interrupt,4 /* 1 interrupt per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	540, 480, { 0, 540, 0, 480 },
	gfxdecodeinfo,
	128, 128,
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

ROM_START( bwidow_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	/* Vector ROM */
	ROM_LOAD( "136017.107",  0x2800, 0x0800 )
	ROM_LOAD( "136017.108",  0x3000, 0x1000 )
	ROM_LOAD( "136017.109",  0x4000, 0x1000 )
	ROM_LOAD( "136017.110",  0x5000, 0x1000 )
	/* Program ROM */
	ROM_LOAD( "136017.101",  0x9000, 0x1000 )
	ROM_LOAD( "136017.102",  0xa000, 0x1000 )
	ROM_LOAD( "136017.103",  0xb000, 0x1000 )
	ROM_LOAD( "136017.104",  0xc000, 0x1000 )
	ROM_LOAD( "136017.105",  0xd000, 0x1000 )
	ROM_LOAD( "136017.106",  0xe000, 0x1000 )
	ROM_LOAD( "136017.106",  0xf000, 0x1000 )	/* for reset/interrupt vectors */
ROM_END



struct GameDriver bwidow_driver =
{
	"Black Widow",
	"bwidow",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\nBERND WIEBELT",
	&machine_driver,

	bwidow_rom,
	0, 0,
	0,

	bwidow_input_ports, trak_ports, bwidow_dsw, bwidow_keys,

	color_prom, 0,0,
	140, 110,     /* paused_x, paused_y */

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gravitar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	/* Vector ROM */
	ROM_LOAD( "136010.210",  0x2800, 0x0800 )
	ROM_LOAD( "136010.207",  0x3000, 0x1000 )
	ROM_LOAD( "136010.208",  0x4000, 0x1000 )
	ROM_LOAD( "136010.209",  0x5000, 0x1000 )
	/* Program ROM */
	ROM_LOAD( "136010.201",  0x9000, 0x1000 )
	ROM_LOAD( "136010.202",  0xa000, 0x1000 )
	ROM_LOAD( "136010.203",  0xb000, 0x1000 )
	ROM_LOAD( "136010.204",  0xc000, 0x1000 )
	ROM_LOAD( "136010.205",  0xd000, 0x1000 )
	ROM_LOAD( "136010.206",  0xe000, 0x1000 )
	ROM_LOAD( "136010.206",  0xf000, 0x1000 )	/* for reset/interrupt vectors */
ROM_END



struct GameDriver gravitar_driver =
{
	"Gravitar",
	"gravitar",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH",
	&machine_driver,

	gravitar_rom,
	0, 0,
	0,

	gravitar_input_ports, trak_ports, gravitar_dsw, gravitar_keys,

	color_prom, 0, 0,
	140, 110,     /* paused_x, paused_y */

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( spacduel_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	/* Vector ROM */
	ROM_LOAD( "136006.106",  0x2800, 0x0800 )
	ROM_LOAD( "136006.107",  0x3000, 0x1000 )
	/* Program ROM */
	ROM_LOAD( "136006.201",  0x4000, 0x1000 )
	ROM_LOAD( "136006.102",  0x5000, 0x1000 )
	ROM_LOAD( "136006.103",  0x6000, 0x1000 )
	ROM_LOAD( "136006.104",  0x7000, 0x1000 )
	ROM_LOAD( "136006.105",  0x8000, 0x1000 )
	ROM_LOAD( "136006.105",  0xf000, 0x1000 )	/* for reset/interrupt vectors */
ROM_END



struct GameDriver spacduel_driver =
{
	"Space Duel",
	"spacduel",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH",
	&spacduel_machine_driver,

	spacduel_rom,
	0, 0,
	0,

	spacduel_input_ports, trak_ports, spacduel_dsw, spacduel_keys,

	color_prom, 0, 0,
	140, 110,     /* paused_x, paused_y */

	0, 0
};

