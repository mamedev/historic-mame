/***************************************************************************

Lunar Lander Memory Map (preliminary)

Lunar Lander settings:

0 = OFF  1 = ON  x = Don't Care  $ = Atari suggests


8 SWITCH DIP (P8) with -01 ROMs on PCB
87654321
--------
11xxxxxx   450 fuel units per coin
10xxxxxx   600 fuel units per coin
01xxxxxx   750 fuel units per coin  $
00xxxxxx   900 fuel units per coin
xxx0xxxx   Free play
xxx1xxxx   Coined play as determined by toggles 7 & 8  $
xxxx00xx   German instructions
xxxx01xx   Spanish instructions
xxxx10xx   French instructions
xxxx11xx   English instructions  $
xxxxxx11   Right coin == 1 credit/coin  $
xxxxxx10   Right coin == 4 credit/coin
xxxxxx01   Right coin == 5 credit/coin
xxxxxx00   Right coin == 6 credit/coin
           (Left coin always registers 1 credit/coin)


8 SWITCH DIP (P8) with -02 ROMs on PCB
87654321
--------
11x1xxxx   450 fuel units per coin
10x1xxxx   600 fuel units per coin
01x1xxxx   750 fuel units per coin  $
00x1xxxx   900 fuel units per coin
11x0xxxx   1100 fuel units per coin
10x0xxxx   1300 fuel units per coin
01x0xxxx   1550 fuel units per coin
00x0xxxx   1800 fuel units per coin
xx0xxxxx   Free play
xx1xxxxx   Coined play as determined by toggles 5, 7, & 8  $
xxxx00xx   German instructions
xxxx01xx   Spanish instructions
xxxx10xx   French instructions
xxxx11xx   English instructions  $
xxxxxx11   Right coin == 1 credit/coin  $
xxxxxx10   Right coin == 4 credit/coin
xxxxxx01   Right coin == 5 credit/coin
xxxxxx00   Right coin == 6 credit/coin
           (Left coin always registers 1 credit/coin)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/atari_vg.h"

int llander_IN0_r(int offset);
int llander_IN1_r(int offset);
int llander_DSW1_r(int offset);
int llander_IN3_r(int offset);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0x4800, 0x5fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x4000, 0x47ff, MRA_RAM, &vectorram },
	{ 0x2000, 0x2000, llander_IN0_r },	/* IN0 */
	{ 0x2400, 0x2407, llander_IN1_r },	/* IN1 */
	{ 0x2800, 0x2803, llander_DSW1_r },	/* DSW1 */
	{ 0x2c00, 0x2c00, llander_IN3_r },	/* IN3 - joystick pot */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x4000, 0x47ff, MWA_RAM, &vectorram },
	{ 0x3000, 0x3000, atari_vg_go },
/*	{ 0x3400, 0x3400, wdclr }, */
/*	{ 0x3c00, 0x3c00, llander_snd }, */
/*	{ 0x3e00, 0x3e00, llander_snd_reset }, */
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x4800, 0x5fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{       /* IN0 */
		0xff,
		{ 0, OSD_KEY_F2, OSD_KEY_F1, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 */
		0x00,
		{ OSD_KEY_1, OSD_KEY_3, 0, 0, OSD_KEY_2, OSD_KEY_ALT, OSD_KEY_RIGHT, OSD_KEY_LEFT },
		{ 0, 0, 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_RIGHT, OSD_JOY_LEFT }
	},
	{       /* DSW1 */
		0x83,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 - joystick pots */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_CONTROL, 0, 0, 0, 0, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_FIRE1, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 1, 5, "ABORT" },
        { 1, 6, "RIGHT" },
        { 1, 7, "LEFT" },
        { 3, 0, "THRUST" },
        { 3, 1, "MAX THRUST" },
        { -1 }
};

/* These are the rev 2 dipswitch settings. MAME has trouble with the FUEL one. */
static struct DSW dsw[] =
{
	{ 2, 0x0c, "LANGUAGE", { "ENGLISH", "FRENCH", "SPANISH", "GERMAN" } },
	{ 2, 0x20, "PLAY", { "USE COIN", "FREE PLAY" } },
	{ 2, 0xc0, "FUEL", { "450", "600", "750", "900" } },
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
	0x01,0x00,0x00, /* RED */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x00,0x01, /* BLUE */
	0x01,0x01,0x00, /* YELLOW */
	0x00,0x01,0x01, /* CYAN */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00,	/* BLACK */
	0x01,0x00,0x00, /* RED */
	0x00,0x01,0x00, /* GREEN */
	0x00,0x00,0x01, /* BLUE */
	0x01,0x01,0x00, /* YELLOW */
	0x00,0x01,0x01, /* CYAN */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x01	/* WHITE */};

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
	288, 224, { -30, 1050, 0, 900 },
	gfxdecodeinfo,
	256, 256,
	atari_vg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	atari_vg_dvg_start,
	atari_vg_stop,
	atari_vg_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( llander_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "034572.02", 0x6000, 0x0800, 0xee299da1 )
	ROM_LOAD( "034571.02", 0x6800, 0x0800, 0xed92222e )
	ROM_LOAD( "034570.02", 0x7000, 0x0800, 0xfe8b233f )
	ROM_LOAD( "034569.02", 0x7800, 0x0800, 0x5cdbe9e5 )
	ROM_RELOAD(            0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Vector ROM */
	ROM_LOAD( "034599.01", 0x4800, 0x0800, 0x9e7084de )
	ROM_LOAD( "034598.01", 0x5000, 0x0800, 0xd006607c )
	ROM_LOAD( "034597.01", 0x5800, 0x0800, 0xfc000000 )
ROM_END

struct GameDriver llander_driver =
{
	"Lunar Lander",
	"llander",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT",
	&machine_driver,

	llander_rom,
	0, 0,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
