/***************************************************************************

Battlezone memory map (preliminary)

0000-04ff RAM
0800      IN0
0a00      IN1
0c00      IN2

1200      Vector generator start (write)
1400
1600      Vector generator reset (write)

1800      Mathbox Status register
1810      Mathbox value (lo-byte)
1818      Mathbox value (hi-byte)
1820-182f POKEY I/O
1828      Control inputs
1860-187f Mathbox RAM

2000-2fff Vector generator RAM
3000-37ff Mathbox ROM
5000-7fff ROM

Battlezone settings:

0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests

** IMPORTANT - BITS are INVERTED in the game itself **

TOP 8 SWITCH DIP
87654321
--------
XXXXXX11   Free Play
XXXXXX10   1 coin for 2 plays
XXXXXX01   1 coin for 1 play
XXXXXX00   2 coins for 1 play
XXXX11XX   Right coin mech x 1
XXXX10XX   Right coin mech x 4
XXXX01XX   Right coin mech x 5
XXXX00XX   Right coin mech x 6
XXX1XXXX   Center (or Left) coin mech x 1
XXX0XXXX   Center (or Left) coin mech x 2
111XXXXX   No bonus coin
110XXXXX   For every 2 coins inserted, game logic adds 1 more
101XXXXX   For every 4 coins inserted, game logic adds 1 more
100XXXXX   For every 4 coins inserted, game logic adds 2 more
011XXXXX   For every 5 coins inserted, game logic adds 1 more

BOTTOM 8 SWITCH DIP
87654321
--------
XXXXXX11   Game starts with 2 tanks
XXXXXX10   Game starts with 3 tanks  $
XXXXXX01   Game starts with 4 tanks
XXXXXX00   Game starts with 5 tanks
XXXX11XX   Missile appears after 5,000 points
XXXX10XX   Missile appears after 10,000 points  $
XXXX01XX   Missile appears after 20,000 points
XXXX00XX   Missile appears after 30,000 points
XX11XXXX   No bonus tank
XX10XXXX   Bonus taks at 15,000 and 100,000 points  $
XX01XXXX   Bonus taks at 20,000 and 100,000 points
XX00XXXX   Bonus taks at 50,000 and 100,000 points
11XXXXXX   English language
10XXXXXX   French language
01XXXXXX   German language
00XXXXXX   Spanish language

4 SWITCH DIP

XX11   All coin mechanisms register on one coin counter
XX01   Left and center coin mechanisms on one coin counter, right on second
XX10   Center and right coin mechanisms on one coin counter, left on second
XX00   Each coin mechanism has it's own counter

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/mathbox.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/atari_vg.h"
#include "sndhrdw/pokyintf.h"

int bzone_IN0_r(int offset);
int bzone_IN3_r(int offset);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0x3000, 0x3fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x2000, 0x2fff, MRA_RAM, &vectorram },
	{ 0x0800, 0x0800, bzone_IN0_r },	/* IN0 */
	{ 0x0a00, 0x0a00, input_port_1_r },	/* DSW1 */
	{ 0x0c00, 0x0c00, input_port_2_r },	/* DSW2 */
	{ 0x1828, 0x1828, bzone_IN3_r },	/* IN3 */
	{ 0x1800, 0x1800, mb_status_r },
	{ 0x1810, 0x1810, mb_lo_r },
	{ 0x1818, 0x1818, mb_hi_r },
	{ 0x1820, 0x182f, pokey1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_RAM, &vectorram },
	{ 0x1820, 0x182f, pokey1_w },
	{ 0x1860, 0x187f, mb_go },
	{ 0x1200, 0x1200, atari_vg_go },
	{ 0x1000, 0x1000, MWA_NOP }, /* coin out */
	{ 0x1400, 0x1400, MWA_NOP }, /* watchdog clear */
	{ 0x1600, 0x1600, vg_reset },
	{ 0x1840, 0x1840, MWA_NOP }, /* bzone sound, yet todo */
	{ 0x5000, 0x7fff, MWA_ROM },
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
		0x11,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW2 */
		0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 */
		0x00,
		{ OSD_KEY_K, OSD_KEY_I, OSD_KEY_S, OSD_KEY_W, OSD_KEY_SPACE, OSD_KEY_1, OSD_KEY_2, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* This is just a fake to play BZONE with only one joystick */
		0x00,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_FIRE, 0, 0, 0 },
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 3, 0, "RIGHT BACKWARD" },
        { 3, 1, "RIGHT FORWARD" },
        { 3, 2, "LEFT BACKWARD" },
        { 3, 3, "LEFT FORWARD" },
	{ 3, 4, "FIRE" },
        { 4, 0, "UP" },
	{ 4, 1, "DOWN" },
	{ 4, 2, "TURN LEFT" },
	{ 4, 3, "TURN RIGHT" },
	{ 4, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "2", "3", "4", "5" } },
	{ 1, 0x0c, "MISSILE APPEARS AT", { "5000", "10000", "20000", "30000" } },
	{ 1, 0x30, "BONUS TANK", { "NONE", "15K AND 100K", "20K AND 100K", "50K AND 100K" } },
	{ 1, 0xc0, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ -1 }
};


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
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x01, /* CYAN */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN again */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x01, /* CYAN */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN again */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00	/* BLACK */
};



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0300],"\x05\x00\x00",3) == 0 &&
			memcmp(&RAM[0x0339],"\x22\x28\x38",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0300],1,6*10,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x0300],1,6*10,f);
		fclose(f);
	}
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,4 /* 4 interrupts per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	288, 224, { 0, 480, 0, 400 },
	gfxdecodeinfo,
	256, 256,
	atari_vg_init_colors,

	0,
	atari_vg_avg_start,
	atari_vg_stop,
	atari_vg_screenrefresh,

	/* sound hardware */
	0,
	0,
	pokey1_sh_start,
	pokey_sh_stop,
	pokey_sh_update
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bzone_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036414.01", 0x5000, 0x0800, 0xc40b04fb )
	ROM_LOAD( "036413.01", 0x5800, 0x0800, 0x9f3aa956 )
	ROM_LOAD( "036412.01", 0x6000, 0x0800, 0x5c3bda25 )
	ROM_LOAD( "036411.01", 0x6800, 0x0800, 0xa40bfa05 )
	ROM_LOAD( "036410.01", 0x7000, 0x0800, 0x364b14eb )
	ROM_LOAD( "036409.01", 0x7800, 0x0800, 0x7a21b649 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "036422.01", 0x3000, 0x0800, 0x5c8342bd )
	ROM_LOAD( "036421.01", 0x3800, 0x0800, 0x16a742bd )
ROM_END

ROM_START( bzone2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036414.01", 0x5000, 0x0800, 0x1fe91ce3 )
	ROM_LOAD( "036413.01", 0x5800, 0x0800, 0x9f3aa956 )
	ROM_LOAD( "036412.01", 0x6000, 0x0800, 0x5c3bda25 )
	ROM_LOAD( "036411.01", 0x6800, 0x0800, 0xa40bfa05 )
	ROM_LOAD( "036410.01", 0x7000, 0x0800, 0x364b14eb )
	ROM_LOAD( "036409.01", 0x7800, 0x0800, 0x7a21b649 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "036422.01", 0x3000, 0x0800, 0x5c8342bd )
	ROM_LOAD( "036421.01", 0x3800, 0x0800, 0x16a742bd )
ROM_END



struct GameDriver bzone_driver =
{
	"Battle Zone",
	"bzone",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT\nMAURO MINENNA\n",
	&machine_driver,

	bzone_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	hiload, hisave
};

struct GameDriver bzone2_driver =
{
	"Battle Zone (alternate version)",
	"bzone2",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&machine_driver,

	bzone2_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	hiload, hisave
};
