/***************************************************************************

Asteroids Memory Map (preliminary)

Asteroids settings:

0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


8 SWITCH DIP
87654321
--------
XXXXXX11   English
XXXXXX10   German
XXXXXX01   French
XXXXXX00   Spanish
XXXXX1XX   4-ship game
XXXXX0XX   3-ship game
11XXXXXX   Free Play
10XXXXXX   1 Coin  for 2 Plays
01XXXXXX   1 Coin  for 1 Play
00XXXXXX   2 Coins for 1 Play

Asteroids Deluxe settings:

0 = OFF  1 = ON  X = Don't Care  $ = Atari suggests


8 SWITCH DIP (R5)
87654321
--------
XXXXXX11   English $
XXXXXX10   German
XXXXXX01   French
XXXXXX00   Spanish
XXXX11XX   2-4 ships
XXXX10XX   3-5 ships $
XXXX01XX   4-6 ships
XXXX00XX   5-7 ships
XXX1XXXX   1-play minimum $
XXX0XXXX   2-play minimum
11XXXXXX   Bonus ship every 10,000 points $
10XXXXXX   Bonus ship every 12,000 points
01XXXXXX   Bonus ship every 15,000 points
00XXXXXX   No bonus ships (adds one ship at game start)


8 SWITCH DIP (L8)
87654321
--------
XXXXXX11   Free Play
XXXXXX10   1 Coin = 2 Plays
XXXXXX01   1 Coin = 1 Play
XXXXXX00   2 Coins = 1 Play $
XXXX11XX   Right coin mech * 1 $
XXXX10XX   Right coin mech * 4
XXXX01XX   Right coin mech * 5
XXXX00XX   Right coin mech * 6
XXX1XXXX   Center coin mech * 1 $
XXX0XXXX   Center coin mech * 2
111XXXXX   No bonus coins
110XXXXX   For every 2 coins inserted, game logic adds 1 more coin
101XXXXX   For every 4 coins inserted, game logic adds 1 more coin
100XXXXX   For every 4 coins inserted, game logic adds 2 more coins $
011XXXXX   For every 5 coins inserted, game logic adds 1 more coin
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/atari_vg.h"
#include "sndhrdw/pokyintf.h"


int asteroid_init_machine(const char *gamename);
int asteroid_IN0_r(int offset);
int asteroid_IN1_r(int offset);
int asteroid_DSW1_r(int offset);

void asteroid_bank_switch_w(int offset, int data);
void astdelux_bank_switch_w(int offset, int data);

void asteroid_explode_w(int offset,int data);
void asteroid_thump_w(int offset,int data);
void asteroid_sounds_w(int offset,int data);
void asteroid_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6800, 0x7fff, MRA_ROM },
	{ 0x5000, 0x57ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram },
	{ 0x2000, 0x2007, asteroid_IN0_r },	/* IN0 */
	{ 0x2400, 0x2407, asteroid_IN1_r },	/* IN1 */
	{ 0x2800, 0x2803, asteroid_DSW1_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM, &vectorram },
	{ 0x3000, 0x3000, atari_vg_go },
	{ 0x3200, 0x3200, asteroid_bank_switch_w },
	{ 0x3400, 0x3400, MWA_NOP }, /* watchdog clear */
	{ 0x3600, 0x3600, asteroid_explode_w },
	{ 0x3a00, 0x3a00, asteroid_thump_w },
	{ 0x3c00, 0x3c05, asteroid_sounds_w },
	{ 0x6800, 0x7fff, MWA_ROM },
	{ 0x5000, 0x57ff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress astdelux_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0x4800, 0x57ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram },
	{ 0x2000, 0x2007, asteroid_IN0_r },	/* IN0 */
	{ 0x2400, 0x2407, asteroid_IN1_r },	/* IN1 */
	{ 0x2800, 0x2803, asteroid_DSW1_r },	/* DSW1 */
	{ 0x2c08, 0x2c08, input_port_3_r },		/* DSW2 */
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x2c40, 0x2c7f, atari_vg_earom_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress astdelux_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM, &vectorram },
	{ 0x3000, 0x3000, atari_vg_go },
	{ 0x2c00, 0x2c0f, pokey1_w },
	{ 0x3200, 0x323f, atari_vg_earom_w },
/*	{ 0x3400, 0x3400, wdclr }, */
	{ 0x3600, 0x3600, asteroid_explode_w },
	{ 0x3800, 0x3800, vg_reset },
	{ 0x3a00, 0x3a00, atari_vg_earom_ctrl },
	{ 0x3c00, 0x3c07, astdelux_bank_switch_w },
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x4800, 0x57ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0x00,
		{ 0, 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, OSD_KEY_F2, OSD_KEY_F1 },
		{ 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0, 0 }
	},
	{       /* IN1 */
		0x07,
		{ OSD_KEY_3, 0, 0, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, 0, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT }
	},
	{       /* DSW1 */
		0x80,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct InputPort astdelux_input_ports[] =
{
	{       /* IN0 */
		0x00,
		{ 0, 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, OSD_KEY_F2, OSD_KEY_F1 },
		{ 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0, 0 }
	},
	{       /* IN1 */
		0x07,
		{ OSD_KEY_3, 0, 0, OSD_KEY_1, OSD_KEY_2, OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, 0, OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_LEFT }
	},
	{       /* DSW1 */
		0x04,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW2 - Asteroids Deluxe only */
		0x9d,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 0, 3, "HYPERSPACE OR SHIELD" },
        { 0, 4, "FIRE" },
        { 1, 5, "THRUST" },
        { 1, 6, "RIGHT" },
        { 1, 7, "LEFT" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ 2, 0x04, "SHIPS", { "4", "3" } },
	{ 2, 0xc0, "COIN", { "FREE", "1 COIN 2 PLAY", "1 COIN 1 PLAY", "2 COIN 1 PLAY" } },
	{ -1 }
};

static struct DSW astdelux_dsw[] =
{
	{ 2, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ 2, 0x0c, "SHIPS", { "2 TO 4", "3 TO 5", "4 TO 6", "5 TO 7" } },
	{ 2, 0x10, "PLAY", { "MINIMUM 1", "MINIMUM 2" } },
	{ 2, 0xc0, "BONUS SHIP", { "10K", "12K", "15K", "NONE" } },
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
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x02, /* CYAN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x01,0x02, /* CYAN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01	/* WHITE */};

static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x001d],"\x00\x00",2) == 0 &&
			memcmp(&RAM[0x0050],"\x00\x00",2) == 0 &&
			memcmp(&RAM[0x0032],"\xff\xff",2) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x001d],1,2*10+3*11,f);
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
		fwrite(&RAM[0x001d],1,2*10+3*11,f);
		fclose(f);
	}
}

/* Asteroids Deluxe now uses the earom routines
 * However, we keep the highscore location, just in case
 *		fwrite(&RAM[0x0023],1,3*10+3*11,f);
 */

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
	asteroid_init_machine,

	/* video hardware */
	288, 224, { -30, 1030, 50, 900 },
	gfxdecodeinfo,
	256, 256,
	atari_vg_init_colors,

	0,
	atari_vg_dvg_start,
	atari_vg_stop,
	atari_vg_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	asteroid_sh_update
};

static struct MachineDriver astdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			astdelux_readmem,astdelux_writemem,0,0,
			nmi_interrupt,4 /* 4 interrupts per frame? */
		}
	},
	60, /* frames per second */
	asteroid_init_machine,

	/* video hardware */
	288, 224, { -30, 1030, 50,900 },
	gfxdecodeinfo,
	256, 256,
	atari_vg_init_colors,

	0,
	atari_vg_dvg_start,
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

static const char *asteroid_sample_names[] =
{
	"thumphi.sam",
	"thumplo.sam",
	"fire.sam",
	"lsaucer.sam",
	"ssaucer.sam",
	"thrust.sam",
	"sfire.sam",
	"life.sam",
	"explode1.sam",
	"explode2.sam",
	"explode3.sam",
    0	/* end of array */
};

ROM_START( asteroid_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "035145.02", 0x6800, 0x0800, 0xd84a7878 )
	ROM_LOAD( "035144.02", 0x7000, 0x0800, 0x15f39999 )
	ROM_LOAD( "035143.02", 0x7800, 0x0800, 0x93d25050 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "035127.02", 0x5000, 0x0800, 0xa144e0e0 )
ROM_END

ROM_START( asteroi2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "035145.02", 0x6800, 0x0800, 0x8de50d0d )
	ROM_LOAD( "035144.02", 0x7000, 0x0800, 0xa0dbd7d7 )
	ROM_LOAD( "035143.02", 0x7800, 0x0800, 0x0454f0f0 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "035127.02", 0x5000, 0x0800, 0xed041c1c )
ROM_END



struct GameDriver asteroid_driver =
{
	"Asteroids",
	"asteroid",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&machine_driver,

	asteroid_rom,
	0, 0,
	asteroid_sample_names,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	hiload, hisave
};

struct GameDriver asteroi2_driver =
{
	"Asteroids (alternate version)",
	"asteroi2",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&machine_driver,

	asteroi2_rom,
	0, 0,
	asteroid_sample_names,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	hiload, hisave
};


ROM_START( astdelux_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036430.02", 0x6000, 0x0800, 0x4eb73d37 )
	ROM_LOAD( "036431.02", 0x6800, 0x0800, 0x859ee29c )
	ROM_LOAD( "036432.02", 0x7000, 0x0800, 0x49c7b38d )
	ROM_LOAD( "036433.03", 0x7800, 0x0800, 0x39b00100 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "036800.02", 0x4800, 0x0800, 0x6ea4e6c6 )
	ROM_LOAD( "036799.01", 0x5000, 0x0800, 0x5bcf256d )
ROM_END

ROM_START( astdelu1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "036430.02", 0x6000, 0x0800, 0x4bca4e5e )
	ROM_LOAD( "036431.02", 0x6800, 0x0800, 0x2d01a733 )
	ROM_LOAD( "036432.02", 0x7000, 0x0800, 0xf432f48a )
	ROM_LOAD( "036433.03", 0x7800, 0x0800, 0x8c188eea )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "036800.02", 0x4800, 0x0800, 0xc05aa9d8 )
	ROM_LOAD( "036799.01", 0x5000, 0x0800, 0x5bcf256d )
ROM_END



struct GameDriver astdelux_driver =
{
	"Asteroids Deluxe",
	"astdelux",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&astdelux_machine_driver,

	astdelux_rom,
	0, 0,
	asteroid_sample_names,

	astdelux_input_ports, trak_ports, astdelux_dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	atari_vg_earom_load, atari_vg_earom_save
};

struct GameDriver astdelu1_driver =
{
	"Asteroids Deluxe (alternate version)",
	"astdelu1",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&astdelux_machine_driver,

	astdelu1_rom,
	0, 0,
	asteroid_sample_names,

	astdelux_input_ports, trak_ports, astdelux_dsw, keys,

	color_prom, 0, 0,
	140, 110,      /* paused_x, paused_y */

	atari_vg_earom_load, atari_vg_earom_save
};
