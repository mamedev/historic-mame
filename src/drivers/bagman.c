/***************************************************************************

Bagman memory map (preliminary)

0000-5fff ROM
6000-67ff RAM
9000-93ff Video RAM
9800-9bff Color RAM
9800-981f Sprites (hidden portion of color RAM)
9c00-9fff ? (filled with 3f, not used otherwise)

memory mapped ports:

read:
a000      random number generator?
a800      ? (read only in one place, not used)
b000      DSW
b800      watchdog reset?

*
 * DSW (all bits are inverted)
 * bit 7 : COCKTAIL or UPRIGHT cabinet (1 = UPRIGHT)
 * bit 6 : Bonus  1 = 30000  0 = 40000
 * bit 5 : Language  1 = English  0 = French
 * bit 4 : \ Difficulty
 * bit 3 : / 11 = Easy  10 = Medium  01 = Hard  00 = Hardest
 * bit 2 : Coins per play  1 = 1  0 = 2
 * bit 1 : \ Number of lives
 * bit 0 : / 11 = 2  10 = 3  01 = 4  00 = 5
 *

write:
a000      interrupt enable
a001      ?
a002      ?
a003      video enable?
a004      ?
a007      ?
a800-a805 ?
b000      ?
b800      ?

I/O ports:

I/O 8  ;AY-3-8910 Control Reg.
I/O 9  ;AY-3-8910 Data Write Reg.
I/O C  ;AY-3-8910 Data Read Reg.
        Port A of the 8910 is connected to IN0
        Port B of the 8910 is connected to IN1
*
 * IN0 (all bits are inverted)
 * bit 7 : FIRE player 1
 * bit 6 : DOWN player 1
 * bit 5 : UP player 1
 * bit 4 : RIGHT player 1
 * bit 3 : LEFT player 1
 * bit 2 : START 1
 * bit 1 : COIN 2
 * bit 0 : COIN 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : FIRE player 2
 * bit 6 : DOWN player 2
 * bit 5 : UP player 2
 * bit 4 : RIGHT player 2
 * bit 3 : LEFT player 2
 * bit 2 : START 2
 * bit 1 : COIN 4
 * bit 0 : COIN 6
 *

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int bagman_rand_r(int offset);

extern unsigned char *bagman_video_enable;
extern void bagman_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int bagman_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW1 */
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x9bff, MRA_RAM },	/* color RAM + sprites */
	{ 0xa000, 0xa000, bagman_rand_r },
	{ 0xa800, 0xa800, MRA_NOP },
	{ 0xb800, 0xb800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x9bff, colorram_w, &colorram },
	{ 0x9800, 0x981f, MWA_RAM, &spriteram, &spriteram_size },	/* hidden portion of color RAM */
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa003, 0xa003, MWA_RAM, &bagman_video_enable },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x9c00, 0x9fff, MWA_NOP },	/* ???? */
	{ 0xa001, 0xa002, MWA_NOP },	/* ???? */
	{ 0xa004, 0xa004, MWA_NOP },	/* ???? */
	{ 0xa007, 0xa007, MWA_NOP },	/* ???? */
	{ 0xa800, 0xa805, MWA_NOP },	/* ???? */
	{ 0xb000, 0xb000, MWA_NOP },	/* ???? */
	{ 0xb800, 0xb800, MWA_NOP },	/* ???? */
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0c, 0x0c, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x08, 0x08, AY8910_control_port_0_w },
	{ 0x09, 0x09, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, 0, OSD_KEY_1, OSD_KEY_LEFT,
				OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_CONTROL },
		{ 0, 0, 0, OSD_JOY_LEFT,
				OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_FIRE },
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, OSD_KEY_2, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xb6,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 5, "MOVE UP" },
        { 0, 3, "MOVE LEFT"  },
        { 0, 4, "MOVE RIGHT" },
        { 0, 6, "MOVE DOWN" },
        { 0, 7, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x03, "LIVES", { "5", "4", "3", "2" }, 1 },
	{ 2, 0x18, "DIFFICULTY", { "HARDEST", "HARD", "MEDIUM", "EASY" }, 1 },
	{ 2, 0x40, "BONUS", { "40000", "30000" }, 1 },
	{ 2, 0x20, "LANGUAGE", { "FRENCH", "ENGLISH" }, 1 },
	{ -1 }
};


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 16 },	/* char set #1 */
	{ 1, 0x2000, &charlayout,      0, 16 },	/* char set #2 */
	{ 1, 0x0000, &spritelayout,    0, 16 },	/* sprites */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0x49,0x00,0x00,	/* DKRED1 */
	0x92,0x00,0x00,	/* DKRED2 */
	0xff,0x00,0x00,	/* RED */
	0x00,0x24,0x00,	/* DKGRN1 */
	0x92,0x24,0x00,	/* DKBRN1 */
	0xb6,0x24,0x00,	/* DKBRN2 */
	0xff,0x24,0x00,	/* LTRED1 */
	0xdb,0x49,0x00,	/* BROWN */
	0x00,0x6c,0x00,	/* DKGRN2 */
	0xff,0x6c,0x00,	/* LTORG1 */
	0x00,0x92,0x00,	/* DKGRN3 */
	0x92,0x92,0x00,	/* DKYEL */
	0xdb,0x92,0x00,	/* DKORG */
	0xff,0x92,0x00,	/* ORANGE */
	0x00,0xdb,0x00,	/* GREEN1 */
	0x6d,0xdb,0x00,	/* LTGRN1 */
	0x00,0xff,0x00,	/* GREEN2 */
	0x49,0xff,0x00,	/* LTGRN2 */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0x55,	/* DKBLU1 */
	0xff,0x00,0x55,	/* DKPNK1 */
	0xff,0x24,0x55,	/* DKPNK2 */
	0xff,0x6d,0x55,	/* LTRED2 */
	0xdb,0x92,0x55,	/* LTBRN */
	0xff,0x92,0x55,	/* LTORG2 */
	0x24,0xff,0x55,	/* LTGRN3 */
	0x49,0xff,0x55,	/* LTGRN4 */
	0xff,0xff,0x55,	/* LTYEL */
	0x00,0x00,0xaa,	/* DKBLU2 */
	0xff,0x00,0xaa,	/* PINK1 */
	0x00,0x24,0xaa,	/* DKBLU3 */
	0xff,0x24,0xaa,	/* PINK2 */
	0xdb,0xdb,0xaa,	/* CREAM */
	0xff,0xdb,0xaa,	/* LTORG3 */
	0x00,0x00,0xff,	/* BLUE */
	0xdb,0x00,0xff,	/* PURPLE */
	0x00,0xb6,0xff,	/* LTBLU1 */
	0x92,0xdb,0xff,	/* LTBLU2 */
	0xdb,0xdb,0xff,	/* WHITE1 */
	0xff,0xff,0xff	/* WHITE2 */
};

enum {BLACK,DKRED1,DKRED2,RED,DKGRN1,DKBRN1,DKBRN2,LTRED1,BROWN,DKGRN2,
	LTORG1,DKGRN3,DKYEL,DKORG,ORANGE,GREEN1,LTGRN1,GREEN2,LTGRN2,YELLOW,
	DKBLU1,DKPNK1,DKPNK2,LTRED2,LTBRN,LTORG2,LTGRN3,LTGRN4,LTYEL,DKBLU2,
	PINK1,DKBLU3,PINK2,CREAM,LTORG3,BLUE,PURPLE,LTBLU1,LTBLU2,WHITE1,
	WHITE2};

static unsigned char colortable[] =
{
	/* characters and sprites */
	BLACK,PINK1,RED,DKGRN3,               /* 1st level */
	BLACK,LTORG1,WHITE1,LTRED1,             /* pauline with kong */
	BLACK,RED,CREAM,BLUE,		/* Mario */
	BLACK,PINK1,RED,DKGRN3,                  /* 3rd level */
	BLACK,BLUE,LTBLU1,LTYEL,                /* 4th lvl */
	BLACK,BLUE,LTYEL,LTBLU1,               /* 2nd level */
	BLACK,RED,CREAM,BLUE,                  /* blue text */
	BLACK,LTYEL,BROWN,WHITE1,	/* hammers */
	BLACK,LTBRN,BROWN,CREAM,               /* kong */
	BLACK,RED,LTRED1,YELLOW,             /* oil flame */
	BLACK,LTBRN,CREAM,LTRED1,              /* pauline */
	BLACK,LTYEL,BLUE,BROWN,		/* barrels */
	BLACK,CREAM,LTBLU2,BLUE,	/* "oil" barrel */
	BLACK,YELLOW,BLUE,RED,               /* small mario, spring */
	BLACK,DKGRN3,LTBLU1,BROWN,            /* scared flame */
	BLACK,LTRED1,YELLOW,BLUE,            /* flame */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	bagman_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	bagman_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bagman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4_9e.bin", 0x0000, 0x1000 )
	ROM_LOAD( "a4_9f.bin", 0x1000, 0x1000 )
	ROM_LOAD( "a4_9j.bin", 0x2000, 0x1000 )
	ROM_LOAD( "a4_9k.bin", 0x3000, 0x1000 )
	ROM_LOAD( "a4_9m.bin", 0x4000, 0x1000 )
	ROM_LOAD( "a4_9n.bin", 0x5000, 0x1000 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a2_1e.bin", 0x0000, 0x1000 )
	ROM_LOAD( "a2_1j.bin", 0x1000, 0x1000 )
	ROM_LOAD( "a2_1c.bin", 0x2000, 0x1000 )
	ROM_LOAD( "a2_1f.bin", 0x3000, 0x1000 )

	ROM_REGION(0x2000)	/* ??? */
	ROM_LOAD( "a1_9r.bin", 0x0000, 0x1000 )
	ROM_LOAD( "a1_9t.bin", 0x1000, 0x1000 )
ROM_END



struct GameDriver bagman_driver =
{
	"Bagman",
	"bagman",
	"ROBERT ANSCHUETZ\nNICOLA SALMORIA",
	&machine_driver,

	bagman_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	0, 0
};
