/***************************************************************************

Red Baron memory map (preliminary)

0000-04ff RAM
0800      COIN_IN
0a00      IN1
0c00      IN2

1200      Vector generator start (write)
1400
1600      Vector generator reset (write)

1800      Mathbox Status register
1802      Button inputs
1804      Mathbox value (lo-byte)
1806      Mathbox value (hi-byte)
1808      Red Baron Sound (bit 1 selects joystick pot to read also)
1810-181f POKEY I/O
1818      Joystick inputs
1860-187f Mathbox RAM

2000-2fff Vector generator RAM
3000-37ff Mathbox ROM
5000-7fff ROM

RED BARON DIP SWITCH SETTINGS
Donated by Dana Colbert


$=Default
"K" = 1,000

Switch at position P10
                                  8    7    6    5    4    3    2    1
                                _________________________________________
English                        $|    |    |    |    |    |    |Off |Off |
Spanish                         |    |    |    |    |    |    |Off | On |
French                          |    |    |    |    |    |    | On |Off |
German                          |    |    |    |    |    |    | On | On |
                                |    |    |    |    |    |    |    |    |
 Bonus airplane granted at:     |    |    |    |    |    |    |    |    |
Bonus at 2K, 10K and 30K        |    |    |    |    |Off |Off |    |    |
Bonus at 4K, 15K and 40K       $|    |    |    |    |Off | On |    |    |
Bonus at 6K, 20K and 50K        |    |    |    |    | On |Off |    |    |
No bonus airplanes              |    |    |    |    | On | On |    |    |
                                |    |    |    |    |    |    |    |    |
2 aiplanes per game             |    |    |Off |Off |    |    |    |    |
3 airplanes per game           $|    |    |Off | On |    |    |    |    |
4 airplanes per game            |    |    | On |Off |    |    |    |    |
5 airplanes per game            |    |    | On | On |    |    |    |    |
                                |    |    |    |    |    |    |    |    |
1-play minimum                 $|    |Off |    |    |    |    |    |    |
2-play minimum                  |    | On |    |    |    |    |    |    |
                                |    |    |    |    |    |    |    |    |
Self-adj. game difficulty: on  $|Off |    |    |    |    |    |    |    |
Self-adj. game difficulty: off  | On |    |    |    |    |    |    |    |
                                -----------------------------------------

  If self-adjusting game difficulty feature is
turned on, the program strives to maintain the
following average game lengths (in seconds):

                                        Airplanes per game:
     Bonus airplane granted at:          2   3     4     5
2,000, 10,000 and 30,000 points         90  105$  120   135
4,000, 15,000 and 40,000 points         75   90   105   120
6,000, 20,000 and 50,000 points         60   75    90   105
             No bonus airplanes         45   60    75    90



Switch at position M10
                                  8    7    6    5    4    3    2    1
                                _________________________________________
    50  PER PLAY                |    |    |    |    |    |    |    |    |
 Straight 25  Door:             |    |    |    |    |    |    |    |    |
No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On | On |
Bonus $1= 3 plays               |Off | On | On |Off |Off |Off | On | On |
Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off |Off | On | On |
                                |    |    |    |    |    |    |    |    |
 25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
No Bonus Coins                  |Off |Off |Off |Off |Off | On | On | On |
Bonus $1= 3 plays               |Off | On | On |Off |Off | On | On | On |
Bonus $1= 3 plays, 75 = 2 plays |Off |Off | On |Off |Off | On | On | On |
                                |    |    |    |    |    |    |    |    |
    25  PER PLAY                |    |    |    |    |    |    |    |    |
 Straight 25  Door:             |    |    |    |    |    |    |    |    |
No Bonus Coins                  |Off |Off |Off |Off |Off |Off | On |Off |
Bonus 50 = 3 plays              |Off |Off | On |Off |Off |Off | On |Off |
Bonus $1= 5 plays               |Off | On |Off |Off |Off |Off | On |Off |
                                |    |    |    |    |    |    |    |    |
 25 /$1 Door or 25 /25 /$1 Door |    |    |    |    |    |    |    |    |
No Bonus Coins                  |Off |Off |Off |Off |Off | On | On |Off |
Bonus 50 = 3 plays              |Off |Off | On |Off |Off | On | On |Off |
Bonus $1= 5 plays               |Off | On |Off |Off |Off | On | On |Off |
                                -----------------------------------------

Switch at position L11
                                                      1    2    3    4
                                                    _____________________
All 3 mechs same denomination                       | On | On |    |    |
Left and Center same, right different denomination  | On |Off |    |    |
Right and Center same, left differnnt denomination  |Off | On |    |    |
All different denominations                         |Off |Off |    |    |
                                                    ---------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/mathbox.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/atari_vg.h"
#include "machine/atari_vg.h"
#include "sndhrdw/pokyintf.h"

int bzone_IN0_r(int offset);

void redbaron_joyselect (int offset, int data);
int redbaron_joy_r (int offset);


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
	{ 0x1818, 0x1818, redbaron_joy_r },	/* IN3 */
	{ 0x1802, 0x1802, input_port_4_r },	/* IN4 */
	{ 0x1800, 0x1800, mb_status_r },
	{ 0x1804, 0x1804, mb_lo_r },
	{ 0x1806, 0x1806, mb_hi_r },
	{ 0x1810, 0x181f, pokey1_r },
	{ 0x1820, 0x185f, atari_vg_earom_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_RAM, &vectorram },
	{ 0x1808, 0x1808, redbaron_joyselect }, /* used for sound, too */
	{ 0x1000, 0x1000, MWA_NOP }, /* coin out */
	{ 0x180a, 0x180a, MWA_NOP }, /* sound reset, yet todo */
	{ 0x180c, 0x180c, atari_vg_earom_ctrl },
	{ 0x1810, 0x181f, pokey1_w },
	{ 0x1820, 0x185f, atari_vg_earom_w },
	{ 0x1860, 0x187f, mb_go },
	{ 0x1200, 0x1200, atari_vg_go },
	{ 0x1400, 0x1400, MWA_NOP }, /* watchdog clear */
	{ 0x1600, 0x1600, vg_reset },
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
	{       /* DSW1  - coin values */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* DSW2 - gameplay settings */
		0xeb,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 - RedBaron uses an analog joystick, so this is
                 *       partially a fake. */
		0x00,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN, 0, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN, 0, 0, 0, 0 }
	},
	{       /* IN4 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_1, OSD_KEY_CONTROL },
		{ 0, 0, 0, 0, 0, 0, 0, OSD_JOY_FIRE }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        {
		X_AXIS,
	  	1,
	  	1.0,
		0
	},
	{
		Y_AXIS,
		1,
		1.0,
		0
	},
	{ -1 }
};


static struct KEYSet keys[] =
{
        { 3, 0, "BANK LEFT" },
	{ 3, 1, "BANK RIGHT" },
        { 3, 2, "CLIMB" },
	{ 3, 3, "DIVE" },
	{ 4, 7, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x03, "LANGUAGE", { "GERMAN", "FRENCH", "SPANISH", "ENGLISH" } },
	{ 2, 0x0c, "BONUS PLANE", { "NONE", "6K 20K 50K", "4K 15K 40K", "2K 10K 30K" } },
	{ 2, 0x30, "LIVES", { "5", "4", "3", "2" } },
	{ 2, 0x80, "SELF ADJUST DIFF", { "ON", "OFF" } },
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
	0x00,0x01,0x01, /* CYAN */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00,	/* BLACK */
	0x00,0x01,0x01, /* CYAN */
	0x00,0x00,0x01, /* BLUE */
	0x00,0x01,0x00, /* GREEN */
	0x01,0x00,0x00, /* RED */
	0x01,0x00,0x01, /* MAGENTA */
	0x01,0x01,0x00, /* YELLOW */
	0x01,0x01,0x01,	/* WHITE */
	0x00,0x00,0x00	/* BLACK */
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
			nmi_interrupt,4 /* 1 interrupt per frame? */
		}
	},
	60, /* frames per second */
	0,

	/* video hardware */
	288, 224, { 0, 520, 0, 400 },
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

ROM_START( redbaron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "037587.01",  0x4800, 0x0800, 0x8e42eeee )
	ROM_CONTINUE(           0x5800, 0x0800 )
	ROM_LOAD( "037000.01E", 0x5000, 0x0800, 0x6dea3bc4 )
	ROM_LOAD( "036998.01E", 0x6000, 0x0800, 0x0c59f20d )
	ROM_LOAD( "036997.01E", 0x6800, 0x0800, 0x31e948b7 )
	ROM_LOAD( "036996.01E", 0x7000, 0x0800, 0xd2c126d9 )
	ROM_LOAD( "036995.01E", 0x7800, 0x0800, 0x2d259e61 )
	ROM_RELOAD(             0xf800, 0x0800 )	/* for reset/interrupt vectors */
	/* Mathbox ROMs */
	ROM_LOAD( "037006.01E", 0x3000, 0x0800, 0xbd8f807f )
	ROM_LOAD( "037007.01E", 0x3800, 0x0800, 0xd59dec13 )
ROM_END

struct GameDriver redbaron_driver =
{
	"Red Baron",
	"redbaron",
	"BRAD OLIVER\nAL KOSSOW\nHEDLEY RAINNIE\nERIC SMITH\n"
	"ALLARD VAN DER BAS\nBERND WIEBELT\nBALOO",
	&machine_driver,

	redbaron_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	140, 110,     /* paused_x, paused_y */

	atari_vg_earom_load, atari_vg_earom_save
};

