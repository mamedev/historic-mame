/***************************************************************************

Sega Vector memory map (preliminary)

Most of the info here comes from the wiretap archive at:
http://www.spies.com/arcade/simulation/gameHardware/

 * Sega G80 Vector Simulation

ROM Address Map
---------------
       Eliminator Elim4Player Space Fury  Zektor  TAC/SCAN  Star Trk
-----+-----------+-----------+-----------+-------+---------+---------+
0000 | 969       | 1390      | 969       | 1611  | 1711    | 1873    | CPU u25
-----+-----------+-----------+-----------+-------+---------+---------+
0800 | 1333      | 1347      | 960       | 1586  | 1670    | 1848    | ROM u1
-----+-----------+-----------+-----------+-------+---------+---------+
1000 | 1334      | 1348      | 961       | 1587  | 1671    | 1849    | ROM u2
-----+-----------+-----------+-----------+-------+---------+---------+
1800 | 1335      | 1349      | 962       | 1588  | 1672    | 1850    | ROM u3
-----+-----------+-----------+-----------+-------+---------+---------+
2000 | 1336      | 1350      | 963       | 1589  | 1673    | 1851    | ROM u4
-----+-----------+-----------+-----------+-------+---------+---------+
2800 | 1337      | 1351      | 964       | 1590  | 1674    | 1852    | ROM u5
-----+-----------+-----------+-----------+-------+---------+---------+
3000 | 1338      | 1352      | 965       | 1591  | 1675    | 1853    | ROM u6
-----+-----------+-----------+-----------+-------+---------+---------+
3800 | 1339      | 1353      | 966       | 1592  | 1676    | 1854    | ROM u7
-----+-----------+-----------+-----------+-------+---------+---------+
4000 | 1340      | 1354      | 967       | 1593  | 1677    | 1855    | ROM u8
-----+-----------+-----------+-----------+-------+---------+---------+
4800 | 1341      | 1355      | 968       | 1594  | 1678    | 1856    | ROM u9
-----+-----------+-----------+-----------+-------+---------+---------+
5000 | 1342      | 1356      |           | 1595  | 1679    | 1857    | ROM u10
-----+-----------+-----------+-----------+-------+---------+---------+
5800 | 1343      | 1357      |           | 1596  | 1680    | 1858    | ROM u11
-----+-----------+-----------+-----------+-------+---------+---------+
6000 | 1344      | 1358      |           | 1597  | 1681    | 1859    | ROM u12
-----+-----------+-----------+-----------+-------+---------+---------+
6800 | 1345      | 1359      |           | 1598  | 1682    | 1860    | ROM u13
-----+-----------+-----------+-----------+-------+---------+---------+
7000 |           | 1360      |           | 1599  | 1683    | 1861    | ROM u14
-----+-----------+-----------+-----------+-------+---------+---------+
7800 |                                   | 1600  | 1684    | 1862    | ROM u15
-----+-----------+-----------+-----------+-------+---------+---------+
8000 |                                   | 1601  | 1685    | 1863    | ROM u16
-----+-----------+-----------+-----------+-------+---------+---------+
8800 |                                   | 1602  | 1686    | 1864    | ROM u17
-----+-----------+-----------+-----------+-------+---------+---------+
9000 |                                   | 1603  | 1687    | 1865    | ROM u18
-----+-----------+-----------+-----------+-------+---------+---------+
9800 |                                   | 1604  | 1688    | 1866    | ROM u19
-----+-----------+-----------+-----------+-------+---------+---------+
A000 |                                   | 1605  | 1709    | 1867    | ROM u20
-----+-----------+-----------+-----------+-------+---------+---------+
A800 |                                   | 1606  | 1710    | 1868    | ROM u21
-----+-----------+-----------+-----------+-------+---------+---------+
B000 |                                                     | 1869    | ROM u22
-----+-----------+-----------+-----------+-------+---------+---------+
B800 |                                                     | 1870    | ROM u23
-----+-----------+-----------+-----------+-------+---------+---------+

I/O ports:
read:

write:

These games all have dipswitches, but they are mapped in such a way as to make using
them with MAME extremely difficult. I might try to implement them in the future.

SWITCH MAPPINGS
---------------

+------+------+------+------+------+------+------+------+
|SW1-8 |SW1-7 |SW1-6 |SW1-5 |SW1-4 |SW1-3 |SW1-2 |SW1-1 |
+------+------+------+------+------+------+------+------+
 F8:08 |F9:08 |FA:08 |FB:08 |F8:04 |F9:04  FA:04  FB:04    Zektor &
       |      |      |      |      |      |                Space Fury
       |      |      |      |      |      |
   1  -|------|------|------|------|------|--------------- upright
   0  -|------|------|------|------|------|--------------- cocktail
       |      |      |      |      |      |
       |  1  -|------|------|------|------|--------------- voice
       |  0  -|------|------|------|------|--------------- no voice
              |      |      |      |      |
              |  1   |  1  -|------|------|--------------- 5 ships
              |  0   |  1  -|------|------|--------------- 4 ships
              |  1   |  0  -|------|------|--------------- 3 ships
              |  0   |  0  -|------|------|--------------- 2 ships
                            |      |      |
                            |  1   |  1  -|--------------- hardest
                            |  0   |  1  -|--------------- hard
1 = Open                    |  1   |  0  -|--------------- medium
0 = Closed                  |  0   |  0  -|--------------- easy

+------+------+------+------+------+------+------+------+
|SW2-8 |SW2-7 |SW2-6 |SW2-5 |SW2-4 |SW2-3 |SW2-2 |SW2-1 |
+------+------+------+------+------+------+------+------+
|F8:02 |F9:02 |FA:02 |FB:02 |F8:01 |F9:01 |FA:01 |FB:01 |
|      |      |      |      |      |      |      |      |
|  1   |  1   |  0   |  0   |  1   | 1    | 0    |  0   | 1 coin/ 1 play
+------+------+------+------+------+------+------+------+

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"


int zektor_IN4_r (int offset);
int startrek_IN4_r (int offset);
int sega_spinner(int offset);

int sega_interrupt(void);
int sega_mult_r (int offset);
void sega_mult1_w (int offset, int data);
void sega_mult2_w (int offset, int data);
void sega_switch_w (int offset, int data);

int sega_sh_start (void);
int sega_sh_r (int offset);
void sega_sh_speech_w (int offset, int data);
void sega_sh_update(void);

void sega_init_colors (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int sega_vh_start (void);
int tacscan_vh_start (void);
void sega_vh_stop (void);
void sega_vh_screenrefresh(struct osd_bitmap *bitmap);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM, &vectorram, &vectorram_size },
	{ 0xd000, 0xdfff, MRA_RAM }, /* sound ram */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xc800, 0xcfff, MWA_RAM },
	{ 0xe000, 0xefff, MWA_RAM, &vectorram },
	{ 0xd000, 0xdfff, MWA_RAM }, /* sound ram */
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xf8, input_port_0_r },
	{ 0xf9, 0xf9, input_port_1_r },
	{ 0xfa, 0xfa, input_port_2_r },
	{ 0xfb, 0xfb, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x38, 0x38, sega_sh_speech_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort zektor_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xf8, input_port_0_r },
	{ 0xf9, 0xf9, input_port_1_r },
	{ 0xfa, 0xfa, input_port_2_r },
	{ 0xfb, 0xfb, input_port_3_r },
	{ 0xfc, 0xfc, zektor_IN4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort zektor_writeport[] =
{
	{ 0x38, 0x38, sega_sh_speech_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
 	{ 0xf8, 0xf8, sega_switch_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort elim2_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xf8, input_port_0_r },
	{ 0xf9, 0xf9, input_port_1_r },
	{ 0xfa, 0xfa, input_port_2_r },
	{ 0xfb, 0xfb, input_port_3_r },
	{ 0xfc, 0xfc, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct IOReadPort startrek_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xf8, input_port_0_r },
	{ 0xf9, 0xf9, input_port_1_r },
	{ 0xfa, 0xfa, input_port_2_r },
	{ 0xfb, 0xfb, input_port_3_r },
	{ 0xfc, 0xfc, startrek_IN4_r },
	{ -1 }	/* end of table */
};


static struct InputPort spacfury_input_ports[] =
{
	{       /* IN0 - port 0xf8 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 - port 0xf9 */
		0xfb,
		{ 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2 - port 0xfa */
		0xfc,
		{ 0, 0, 0, 0, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{       /* IN3 - port 0xfb */
		0xfc,
		{ 0, 0, 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, 0, 0 }
	},
	{ -1 }
};

static struct InputPort zektor_input_ports[] =
{
	{       /* IN0 - port 0xf8 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 - port 0xf9 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2 - port 0xfa */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 - port 0xfb */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN4 - port 0xfc */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT },
		{ 0, 0, OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT }
	},
	{ -1 }
};

static struct InputPort startrek_input_ports[] =
{
	{       /* IN0 - port 0xf8 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 - port 0xf9 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2 - port 0xfa */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN3 - port 0xfb */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN4 - port 0xfc */
		0x00,
		{ OSD_KEY_1, OSD_KEY_2, OSD_KEY_A, OSD_KEY_D, OSD_KEY_F, OSD_KEY_S, OSD_KEY_LEFT, OSD_KEY_RIGHT },
		{ 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1, OSD_JOY_FIRE3, OSD_JOY_FIRE4, OSD_JOY_LEFT, OSD_JOY_RIGHT },
	},
	{ -1 }
};

static struct InputPort elim2_input_ports[] =
{
	{       /* IN0 - port 0xf8 */
		0xfb,
		{ 0, 0, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN1 - port 0xf9 */
		0xfb,
		{ 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{       /* IN2 - port 0xfa */
		0xfc,
		{ 0, 0, 0, 0, OSD_KEY_ALT, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0 },
		{ 0, 0, 0, 0, OSD_JOY_FIRE2, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0 }
	},
	{       /* IN3 - port 0xfb */
		0xfc,
		{ 0, 0, 0, 0, OSD_KEY_A, OSD_KEY_CONTROL, 0, 0 },
		{ 0, 0, 0, 0, 0, OSD_JOY_FIRE1, 0, 0 }
	},
	{       /* IN4 - port 0xfc */
		0x00,
		{ OSD_KEY_F, OSD_KEY_D, OSD_KEY_S, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct TrakPort spinner_trak_ports[] =
{
	{
		X_AXIS,
		1,
		0.5,
		sega_spinner
	},
        { -1 }
};

static struct TrakPort startrek_spinner_ports[] =
{
	{
		X_AXIS,
		1,
		2,
		sega_spinner
	},
	{ -1 }
};

/* Space Fury */
static struct KEYSet keys[] =
{
        { 2, 4, "ROTATE RIGHT"  },
        { 2, 5, "ROTATE LEFT" },
        { 3, 4, "THRUST" },
        { 3, 5, "FIRE" },
        { -1 }
};

static struct KEYSet elim2_keys[] =
{
        { 2, 4, "BLUE THRUST" },
        { 2, 5, "BLUE ROTATE RIGHT"  },
        { 2, 6, "BLUE ROTATE LEFT" },
        { 3, 5, "BLUE FIRE" },
        { 3, 4, "RED ROTATE LEFT" },
        { 4, 2, "RED ROTATE RIGHT"  },
        { 4, 0, "RED FIRE" },
        { 4, 1, "RED THRUST" },
        { -1 }
};

/* Zektor */
static struct KEYSet zektor_keys[] =
{
	{ 4, 6, "ROTATE LEFT" },
	{ 4, 7, "ROTATE RIGHT" },
	{ 4, 2, "FIRE" },
	{ 4, 3, "THRUST" },
	{ -1 }
};

/* Tac/Scan */
static struct KEYSet tacscan_keys[] =
{
	{ 4, 6, "ROTATE LEFT" },
	{ 4, 7, "ROTATE RIGHT" },
	{ 4, 2, "FIRE" },
	{ 4, 3, "ADD SHIP" },
	{ -1 }
};


static struct KEYSet startrek_keys[] =
{
	{ 4, 6, "ROTATE LEFT" },
	{ 4, 7, "ROTATE RIGHT" },
	{ 4, 2, "IMPULSE" },
	{ 4, 3, "PHASER" },
	{ 4, 4, "PHOTON TORPEDO" },
	{ 4, 5, "WARP 9, MR. SULU!" },
	{ -1 }
};

static struct DSW dsw[] =
{
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



/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *spacfury_sample_names[] =
{
	"sf01.sam",
	"sf02.sam",
	"sf03.sam",
	"sf04.sam",
	"sf05.sam",
	"sf06.sam",
	"sf07.sam",
	"sf08.sam",
	"sf09.sam",
	"sf0a.sam",
	"sf0b.sam",
	"sf0c.sam",
	"sf0d.sam",
	"sf0e.sam",
	"sf0f.sam",
	"sf10.sam",
	"sf11.sam",
	"sf12.sam",
	"sf13.sam",
	"sf14.sam",
	"sf15.sam",
    0	/* end of array */
};

ROM_START( spacfury_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "969.BIN", 0x0000, 0x0800, 0xcc7c8410 )
	ROM_LOAD( "960M.BIN", 0x0800, 0x0800, 0x1ab91d25 )
	ROM_LOAD( "961M.BIN", 0x1000, 0x0800, 0x87019ccb )
	ROM_LOAD( "962M.BIN", 0x1800, 0x0800, 0x5e104c2a )
	ROM_LOAD( "963M.BIN", 0x2000, 0x0800, 0x30361c0e )
	ROM_LOAD( "964.BIN", 0x2800, 0x0800, 0x059d031f )
	ROM_LOAD( "965M.BIN", 0x3000, 0x0800, 0x25db0b81 )
	ROM_LOAD( "966M.BIN", 0x3800, 0x0800, 0x7bcfbd0b )
	ROM_LOAD( "967M.BIN", 0x4000, 0x0800, 0xd4eef096 )
	ROM_LOAD( "968.BIN", 0x4800, 0x0800, 0xd8987bc8 )
ROM_END

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,readport,writeport,
			sega_interrupt,1
		}
	},
	40,
	0,

	/* video hardware */
	256, 232, { 512, 1536, 512, 1536 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	sega_sh_start,
	0,
	sega_sh_update
};



struct GameDriver spacfury_driver =
{
	"Space Fury",
	"spacfury",
	"AL KOSSOW\nBRAD OLIVER",
	&machine_driver,

	spacfury_rom,
	0, 0,
	spacfury_sample_names,

	spacfury_input_ports, trak_ports, dsw, keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *zektor_sample_names[] =
{
	"zk01.sam",
	"zk02.sam",
	"zk03.sam",
	"zk04.sam",
	"zk05.sam",
	"zk06.sam",
	"zk07.sam",
	"zk08.sam",
	"zk09.sam",
	"zk0a.sam",
	"zk0b.sam",
	"zk0c.sam",
	"zk0d.sam",
	"zk0e.sam",
	"zk0f.sam",
	"zk10.sam",
	"zk11.sam",
	"zk12.sam",
	"zk13.sam",
    0	/* end of array */
};

ROM_START( zektor_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1611.cpu", 0x0000, 0x0800, 0x983937af )
	ROM_LOAD( "1586M.rom", 0x0800, 0x0800, 0x3cc05a7c )
	ROM_LOAD( "1587.rom", 0x1000, 0x0800, 0x7b9683a8 )
	ROM_LOAD( "1588.rom", 0x1800, 0x0800, 0xda1f5c49 )
	ROM_LOAD( "1589.rom", 0x2000, 0x0800, 0x8111734f )
	ROM_LOAD( "1590.rom", 0x2800, 0x0800, 0xf8e0690e )
	ROM_LOAD( "1591.rom", 0x3000, 0x0800, 0x75fc709a )
	ROM_LOAD( "1592.rom", 0x3800, 0x0800, 0x649c649c )
	ROM_LOAD( "1593.rom", 0x4000, 0x0800, 0xe89e17f4 )
	ROM_LOAD( "1594.rom", 0x4800, 0x0800, 0x7039e1c3 )
	ROM_LOAD( "1595.rom", 0x5000, 0x0800, 0x22aa812c )
	ROM_LOAD( "1596.rom", 0x5800, 0x0800, 0x4c2b25fd )
	ROM_LOAD( "1597.rom", 0x6000, 0x0800, 0xe6dd26d3 )
	ROM_LOAD( "1598M.rom", 0x6800, 0x0800, 0xf252b210 )
	ROM_LOAD( "1599M.rom", 0x7000, 0x0800, 0x8418a92e )
	ROM_LOAD( "1600M.rom", 0x7800, 0x0800, 0xb8da50fe )
	ROM_LOAD( "1601.rom", 0x8000, 0x0800, 0xfff0224c )
	ROM_LOAD( "1602.rom", 0x8800, 0x0800, 0xb76d9577 )
	ROM_LOAD( "1603M.rom", 0x9000, 0x0800, 0x288f4c1b )
	ROM_LOAD( "1604M.rom", 0x9800, 0x0800, 0xabe3ad97 )
	ROM_LOAD( "1605.rom", 0xa000, 0x0800, 0xde90c85c )
	ROM_LOAD( "1606.rom", 0xa800, 0x0800, 0x68a20d5a )
ROM_END

static struct MachineDriver zektor_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,zektor_readport,zektor_writeport,
			sega_interrupt,1
		}
	},
	40,
	0,

	/* video hardware */
	256, 232, { 512, 1536, 512, 1536 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	sega_sh_start,
	0,
	sega_sh_update
};



struct GameDriver zektor_driver =
{
	"Zektor",
	"zektor",
	"AL KOSSOW\nBRAD OLIVER",
	&zektor_machine_driver,

	zektor_rom,
	0, 0,
	zektor_sample_names,

	zektor_input_ports, spinner_trak_ports, dsw, zektor_keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tacscan_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1711.BIN", 0x0000, 0x0800, 0xc6225a26 )
	ROM_LOAD( "1670M.BIN", 0x0800, 0x0800, 0x591dbf59 )
	ROM_LOAD( "1671M.BIN", 0x1000, 0x0800, 0xde77bc5f )
	ROM_LOAD( "1672.BIN", 0x1800, 0x0800, 0x4f46f708 )
	ROM_LOAD( "1673M.BIN", 0x2000, 0x0800, 0x7f36d826 )
	ROM_LOAD( "1674M.BIN", 0x2800, 0x0800, 0x7b630a35 )
	ROM_LOAD( "1675M.BIN", 0x3000, 0x0800, 0x2a51d9a5 )
	ROM_LOAD( "1676M.BIN", 0x3800, 0x0800, 0xe295160b )
	ROM_LOAD( "1677M.BIN", 0x4000, 0x0800, 0xdf368e34 )
	ROM_LOAD( "1678M.BIN", 0x4800, 0x0800, 0x304616ba )
	ROM_LOAD( "1679M.BIN", 0x5000, 0x0800, 0x10d9d9a5 )
	ROM_LOAD( "1680M.BIN", 0x5800, 0x0800, 0x9240cbcc )
	ROM_LOAD( "1681M.BIN", 0x6000, 0x0800, 0xf9b759fb )
	ROM_LOAD( "1682.BIN", 0x6800, 0x0800, 0x9e75d4d1 )
	ROM_LOAD( "1683.BIN", 0x7000, 0x0800, 0xdbd24e9c )
	ROM_LOAD( "1684M.BIN", 0x7800, 0x0800, 0xd822fda4 )
	ROM_LOAD( "1685M.BIN", 0x8000, 0x0800, 0xfce18e49 )
	ROM_LOAD( "1686M.BIN", 0x8800, 0x0800, 0x0b2c14d2 )
	ROM_LOAD( "1687M.BIN", 0x9000, 0x0800, 0x3b705294 )
	ROM_LOAD( "1688.BIN", 0x9800, 0x0800, 0xf51ace36 )
	ROM_LOAD( "1709.BIN", 0xa000, 0x0800, 0x709e505c )
	ROM_LOAD( "1710.BIN", 0xa800, 0x0800, 0x63a010cc )
ROM_END

static struct MachineDriver tacscan_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,zektor_readport,zektor_writeport,
			sega_interrupt,1
		}
	},
	40,
	0,

	/* video hardware */
	256, 232, { 512, 1536, 512, 1536 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	0,
	tacscan_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver tacscan_driver =
{
	"Tac-Scan",
	"tacscan",
	"AL KOSSOW\nBRAD OLIVER",
	&tacscan_machine_driver,

	tacscan_rom,
	0, 0,
	0,

	zektor_input_ports, spinner_trak_ports, dsw, tacscan_keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( elim2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "969.BIN", 0x0000, 0x0800, 0x63ef8173 )
	ROM_LOAD( "1333M.BIN", 0x0800, 0x0800, 0xfa48fe38 )
	ROM_LOAD( "1334M.BIN", 0x1000, 0x0800, 0xcfa0b080 )
	ROM_LOAD( "1335.BIN", 0x1800, 0x0800, 0x05f7f015 )
	ROM_LOAD( "1336.BIN", 0x2000, 0x0800, 0xfce1ed93 )
	ROM_LOAD( "1337M.BIN", 0x2800, 0x0800, 0x244cce4e )
	ROM_LOAD( "1338M.BIN", 0x3000, 0x0800, 0x7f778a0b )
	ROM_LOAD( "1339M.BIN", 0x3800, 0x0800, 0x9bac7ee6 )
	ROM_LOAD( "1340M.BIN", 0x4000, 0x0800, 0x14b0106c )
	ROM_LOAD( "1341M.BIN", 0x4800, 0x0800, 0xece057b8 )
	ROM_LOAD( "1342M.BIN", 0x5000, 0x0800, 0x86a14fb7 )
	ROM_LOAD( "1343M.BIN", 0x5800, 0x0800, 0x67fd5141 )
	ROM_LOAD( "1344M.BIN", 0x6000, 0x0800, 0xb11f62af )
	ROM_LOAD( "1345M.BIN", 0x6800, 0x0800, 0xb7b054d6 )
ROM_END

static struct MachineDriver elim2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,elim2_readport,zektor_writeport,
			sega_interrupt,1
		}
	},
	40,
	0,

	/* video hardware */
	256, 232, { 512, 1536, 512, 1536 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

struct GameDriver elim2_driver =
{
	"Eliminator (2 Player)",
	"elim2",
	"AL KOSSOW\nBRAD OLIVER",
	&elim2_machine_driver,

	elim2_rom,
	0, 0,
	0,

	elim2_input_ports, trak_ports, dsw, elim2_keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *startrek_sample_names[] =
{
	"st01.sam",
	"st02.sam",
	"st03.sam",
	"st04.sam",
	"st05.sam",
	"st06.sam",
	"st07.sam",
	"st08.sam",
	"st09.sam",
	"st0a.sam",
	"st0b.sam",
	"st0c.sam",
	"st0d.sam",
	"st0e.sam",
	"st0f.sam",
	"st10.sam",
	"st11.sam",
	"st12.sam",
	"st13.sam",
	"st14.sam",
	"st15.sam",
	"st16.sam",
	"st17.sam",
    0	/* end of array */
};

ROM_START( startrek_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu1873.bin", 0x0000, 0x0800, 0xa70b114f )
	ROM_LOAD( "1848m.BIN", 0x0800, 0x0800, 0xd8257d95 )
	ROM_LOAD( "1849m.bin", 0x1000, 0x0800, 0x3e625adc )
	ROM_LOAD( "1850.bin", 0x1800, 0x0800, 0xb57a12d2 )
	ROM_LOAD( "1851.bin", 0x2000, 0x0800, 0xd03a429e )
	ROM_LOAD( "1852m.bin", 0x2800, 0x0800, 0x9886ec6c )
	ROM_LOAD( "1853m.bin", 0x3000, 0x0800, 0x4b56709a )
	ROM_LOAD( "1854m.bin", 0x3800, 0x0800, 0x50b14759 )
	ROM_LOAD( "1855m.bin", 0x4000, 0x0800, 0x0dac0782 )
	ROM_LOAD( "1856m.bin", 0x4800, 0x0800, 0x63693ae1 )
	ROM_LOAD( "1857.bin", 0x5000, 0x0800, 0x1ec61d2e )
	ROM_LOAD( "1858.bin", 0x5800, 0x0800, 0x57331e55 )
	ROM_LOAD( "1859.bin", 0x6000, 0x0800, 0x3bb7c0d7 )
	ROM_LOAD( "1860m.bin", 0x6800, 0x0800, 0x399a736c )
	ROM_LOAD( "1861.bin", 0x7000, 0x0800, 0x8ec0e258 )
	ROM_LOAD( "1862.bin", 0x7800, 0x0800, 0xd08f2543 )
	ROM_LOAD( "1863.bin", 0x8000, 0x0800, 0x21b5dc1f )
	ROM_LOAD( "1864.bin", 0x8800, 0x0800, 0x2adbfa49 )
	ROM_LOAD( "1865.bin", 0x9000, 0x0800, 0x952a28ce )
	ROM_LOAD( "1866.bin", 0x9800, 0x0800, 0xf46e7f98 )
	ROM_LOAD( "1867.bin", 0xa000, 0x0800, 0x78bf2f2b )
	ROM_LOAD( "1868.bin", 0xa800, 0x0800, 0x81f02c94 )
	ROM_LOAD( "1869.bin", 0xb000, 0x0800, 0xafaa3d74 )
	ROM_LOAD( "1870.bin", 0xb800, 0x0800, 0x5328d652 )
ROM_END

static struct MachineDriver startrek_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz (?) */
			0,
			readmem,writemem,startrek_readport,zektor_writeport,
			sega_interrupt,1
		}
	},
	40,
	0,

	/* video hardware */
	256, 232, { 512, 1536, 512, 1536 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	sega_sh_start,
	0,
	sega_sh_update
};

struct GameDriver startrek_driver =
{
	"Star Trek",
	"startrek",
	"AL KOSSOW\nBRAD OLIVER",
	&startrek_machine_driver,

	startrek_rom,
	0, 0,
	startrek_sample_names,

	startrek_input_ports, startrek_spinner_ports, dsw, startrek_keys,

	0, 0, 0,
	8*13, 8*16,

	0, 0
};

