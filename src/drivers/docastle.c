/***************************************************************************

Mr. Do's Castle memory map (preliminary)

FIRST CPU:
0000-7fff ROM
8000-97ff RAM
9800-99ff Sprites
b000-b3ff Video RAM
b400-b7ff Color RAM

read:
a000-a008 data from second CPU

write:
a000-a008 data for second CPU
a800      Watchdog reset?
e000      Trigger NMI on second CPU

SECOND CPU:
0000-3fff ROM
8000-87ff RAM

read:
a000-a008 data from first CPU
all the following ports can be read both from c00x and from c08x. I don't know
what's the difference.
c001      DSWB
c081      coins per play
c002      DSWA
          bit 6-7 = lives
		  bit 5 = upright/cocktail (0 = upright)
          bit 4 = difficulty of EXTRA (1 = easy)
          bit 3 = unused?
          bit 2 = RACK TEST
		  bit 0-1 = difficulty
c003      IN0
          bit 4-7 = joystick player 2
          bit 0-3 = joystick player 1
c004      ?
c005	  IN1
          bit 7 = START 2
		  bit 6 = unused
		  bit 5 = jump player 2
		  bit 4 = fire player 2
		  bit 3 = START 1
		  bit 2 = unused
          bit 1 = jump player 1(same effect as fire)
          bit 0 = fire player 1
c085      during the boot sequence, clearing any of bits 0, 1, 3, 4, 5, 7 enters the
          test mode, while clearing bit 2 or 6 seems to lock the machine.
c007      IN2
          bit 7 = unused
          bit 6 = unused
          bit 5 = COIN 2
          bit 4 = COIN 1
          bit 3 = PAUSE
          bit 2 = SERVICE (keep pressed)
          bit 1 = TEST (doesn't work?)
          bit 0 = TILT

write:
a000-a008 data for first CPU
e000      sound port 1
e400      sound port 2
e800      sound port 3
ec00      sound port 4

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int docastle_shared0_r(int offset);
int docastle_shared1_r(int offset);
void docastle_shared0_w(int offset,int data);
void docastle_shared1_w(int offset,int data);
void docastle_nmitrigger(int offset,int data);

void docastle_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int docastle_vh_start(void);
void docastle_vh_stop(void);
void docastle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x97ff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xb800, 0xbbff, videoram_r }, /* mirror of video ram */
	{ 0xbc00, 0xbfff, colorram_r }, /* mirror of color ram */
	{ 0xa000, 0xa008, docastle_shared0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x97ff, MWA_RAM },
	{ 0xb000, 0xb3ff, videoram_w, &videoram, &videoram_size },
	{ 0xb400, 0xb7ff, colorram_w, &colorram },
	{ 0x9800, 0x99ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa000, 0xa008, docastle_shared1_w },
	{ 0xe000, 0xe000, docastle_nmitrigger },
	{ 0xa800, 0xa800, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress readmem2[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc003, 0xc003, input_port_0_r },
	{ 0xc083, 0xc083, input_port_0_r },
	{ 0xc005, 0xc005, input_port_1_r },
	{ 0xc085, 0xc085, input_port_1_r },
	{ 0xc007, 0xc007, input_port_2_r },
	{ 0xc087, 0xc087, input_port_2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc082, 0xc082, input_port_3_r },
	{ 0xc001, 0xc001, input_port_4_r },
	{ 0xc081, 0xc081, input_port_4_r },
	{ 0xa000, 0xa008, docastle_shared1_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa008, docastle_shared0_w },
	{ 0xe000, 0xe000, SN76496_0_w },
	{ 0xe400, 0xe400, SN76496_1_w },
	{ 0xe800, 0xe800, SN76496_2_w },
	{ 0xec00, 0xec00, SN76496_3_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Fire */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Jump */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as test */
/* coin input must be active for 32 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 32)
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "DSW5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Extra", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x01, 0x02, 0x03, 0x04, 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x10, 0x20, 0x30, 0x40, 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8   /* every char takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0, 4, 8, 12, 16, 20, 24, 28,
			32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 64 },
	{ 1, 0x4000, &spritelayout, 64*16, 32 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	4,	/* 4 chips */
	4000000,	/* 4 Mhz? */
	{ 255, 255, 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz */
			3,	/* memory region #3 */
			readmem2,writemem2,0,0,
			interrupt,8
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when communication takes place */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	258, 96*16,
	docastle_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	docastle_vh_start,
	docastle_vh_stop,
	docastle_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( docastle_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "01P_A1.BIN", 0x0000, 0x2000, 0xd80a3ed2 )
	ROM_LOAD( "01N_A2.BIN", 0x2000, 0x2000, 0x5c022c7a )
	ROM_LOAD( "01L_A3.BIN", 0x4000, 0x2000, 0x8e6aea18 )
	ROM_LOAD( "01K_A4.BIN", 0x6000, 0x2000, 0x38d8dc40 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "03A_A5.BIN", 0x0000, 0x4000, 0x85e90c0d )
	ROM_LOAD( "04M_A6.BIN", 0x4000, 0x2000, 0xc53b3bc7 )
	ROM_LOAD( "04L_A7.BIN", 0x6000, 0x2000, 0x3ed9763d )
	ROM_LOAD( "04J_A8.BIN", 0x8000, 0x2000, 0xc159accb )
	ROM_LOAD( "04H_A9.BIN", 0xa000, 0x2000, 0x4d6a5692 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "09C.BIN", 0x0000, 0x0200, 0xcef54dad )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "07N_A0.BIN", 0x0000, 0x4000, 0xe955939f )
ROM_END

ROM_START( docastl2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "A1",  0x0000, 0x2000, 0x3da4962a )
	ROM_LOAD( "A2",  0x2000, 0x2000, 0x95c22212 )
	ROM_LOAD( "A3",  0x4000, 0x2000, 0xbb0a5c16 )
	ROM_LOAD( "A4",  0x6000, 0x2000, 0x3006fcde )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "03A_A5.BIN", 0x0000, 0x4000, 0x85e90c0d )
	ROM_LOAD( "04M_A6.BIN", 0x4000, 0x2000, 0xc53b3bc7 )
	ROM_LOAD( "04L_A7.BIN", 0x6000, 0x2000, 0x3ed9763d )
	ROM_LOAD( "04J_A8.BIN", 0x8000, 0x2000, 0xc159accb )
	ROM_LOAD( "04H_A9.BIN", 0xa000, 0x2000, 0x4d6a5692 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "09C.BIN", 0x0000, 0x0200, 0xcef54dad )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "A10", 0x0000, 0x4000, 0xda659397 )
ROM_END

ROM_START( dounicorn_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "DOREV1.BIN", 0x0000, 0x2000, 0x37a4cc78 )
	ROM_LOAD( "DOREV2.BIN", 0x2000, 0x2000, 0xadbc98e4 )
	ROM_LOAD( "DOREV3.BIN", 0x4000, 0x2000, 0x3d89c3d9 )
	ROM_LOAD( "DOREV4.BIN", 0x6000, 0x2000, 0x4010e2d6 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "DOREV5.BIN", 0x0000, 0x4000, 0x85e90c0d )
	ROM_LOAD( "DOREV6.BIN", 0x4000, 0x2000, 0x31cdcc51 )
	ROM_LOAD( "DOREV7.BIN", 0x6000, 0x2000, 0x4dcfe391 )
	ROM_LOAD( "DOREV8.BIN", 0x8000, 0x2000, 0x56488818 )
	ROM_LOAD( "DOREV9.BIN", 0xa000, 0x2000, 0x20de2a92 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "DOREVC9.BIN", 0x0000, 0x0200, 0xbf6884ee )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "DOREV10.BIN", 0x0000, 0x4000, 0x92ad5143 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8020],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0x8068],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8020],10*8);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8020],10*8);
		osd_fclose(f);
	}
}



struct GameDriver docastle_driver =
{
	__FILE__,
	0,
	"docastle",
	"Mr. Do's Castle (set 1)",
	"1983",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls",
	0,
	&machine_driver,

	docastle_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver docastl2_driver =
{
	__FILE__,
	&docastle_driver,
	"docastl2",
	"Mr. Do's Castle (set 2)",
	"1983",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls",
	0,
	&machine_driver,

	docastl2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver dounicorn_driver =
{
	__FILE__,
	&docastle_driver,
	"douni",
	"Mr. Do vs. Unicorns",
	"1983",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls",
	0,
	&machine_driver,

	dounicorn_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
