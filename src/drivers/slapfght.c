/***************************************************************************

Slap Fight driver by K.Wilkins Jan 1998

Slap Fight - Taito

The three drivers provided are identical, only the 1st CPU EPROM is different
which shows up in the boot message, one if Japanese domestic and the other
is English. The proms which MAY be the original slapfight ones currently
give a hardware error and fail to boot.

slapfigh - Arcade ROMs from Japan http://home.onestop.net/j_rom/
slapboot - Unknown source
slpboota - ROMS Dumped by KW 29/12/97 from unmarked Slap Fight board (bootleg?)

PCB Details from slpboota boardset:

Upper PCB (Sound board)
---------
Z80A CPU
Toshiba TMM2016BP-10 (2KB SRAM)
sf_s05 (Fujitsu MBM2764-25 8KB EPROM) - Sound CPU Code

Yamaha YM2149F (Qty 2 - Pin compatible with AY-3-8190)
Hitachi SRAM - HM6464 (8KB - Qty 4)

sf_s01 (OKI M27256-N 32KB PROM)              Sprite Data (16x16 4bpp)
sf_s02 (OKI M27256-N 32KB PROM)              Sprite Data
sf_s03 (OKI M27256-N 32KB PROM)              Sprite Data
sf_s04 (OKI M27256-N 32KB PROM)              Sprite Data


Lower PCB
---------
Z80B CPU
12MHz Xtal
Toshiba TMM2016BP-10 (2KB SRAM - Total Qty 6 = 2+2+1+1)

sf_s10 (Fujitsu MBM2764-25 8KB EPROM)        Font/Character Data (8x8 2bpp)
sf_s11 (Fujitsu MBM2764-25 8KB EPROM)

sf_s06 (OKI M27256-N 32KB PROM)              Tile Data (8x8 4bpp)
sf_s07 (OKI M27256-N 32KB PROM)              Tile Data
sf_s08 (OKI M27256-N 32KB PROM)              Tile Data
sf_s09 (OKI M27256-N 32KB PROM)              Tile Data

sf_s16 (Fujitsu MBM2764-25 8KB EPROM)        Colour Tables (512B used?)

sf_sH  (OKI M27256-N 32KB PROM)              Level Maps ???

sf_s19 (NEC S27128 16KB EPROM)               CPU Code $0000-$3fff
sf_s20 (Mitsubishi M5L27128K 16KB EPROM)     CPU Code $4000-$7fff


Main CPU Memory Map
-------------------

$0000-$3fff    ROM (SF_S19)
$4000-$7fff    ROM (SF_S20)
$8000-$bfff    ROM (SF_SH) - This is a 32K ROM - Paged ????? How ????

$c000-$c7ff    2K RAM
$c800-$cfff    READ:Unknown H/W  WRITE:Unknown H/W (Upper PCB)
$d000-$d7ff    Background RAM1
$d800-$dfff    Background RAM2
$e000-$e7ff    Sprite RAM
$e800-$efff    READ:Unknown H/W  WRITE:Unknown H/W
$f000-$f7ff    READ:SF_S16       WRITE:Character RAM
$f800-$ffff    READ:Unknown H/W  WRITE:Attribute RAM

$c800-$cfff    Appears to be RAM BUT 1st 0x10 bytes are swapped with
               the sound CPU and visversa for READ OPERATIONS


Write I/O MAP
-------------
Addr    Address based write                     Data based write

$00     Reset sound CPU
$01     Clear sound CPU reset
$02
$03
$04
$05
$06     Clear/Disable Hardware interrupt
$07     Enable Hardware interrupt
$08     LOW Bank select for SF_SH               X axis character scroll reg
$09     HIGH Bank select for SF_SH              X axis pixel scroll reg
$0a
$0b
$0c
$0e
$0f

Read I/O Map
------------

$00     Status regsiter - cycle 0xc7, 0x55, 0x00  (Thanks to Dave Spicer for the info)


Known Info
----------

2K Character RAM at write only address $f000-$f7fff looks to be organised
64x32 chars with the screen rotated thru 90 degrees clockwise. There
appears to be some kind of attribute(?) RAM above at $f800-$ffff organised
in the same manner.

From the look of data in the buffer it is arranged thus: 37x32 (HxW) which
would make the overall frame buffer 296x256.

Print function maybe around $09a2 based on info from log file.

$e000 looks like sprite ram, setup routines at $0008.


Sound System CPU Details
------------------------

Memory Map
$0000-$1fff  ROM(SF_S5)
$a080        AY-3-8910(PSG1) Register address
$a081        AY-3-8910(PSG1) Read register
$a082        AY-3-8910(PSG1) Write register
$a090        AY-3-8910(PSG2) Register address
$a091        AY-3-8910(PSG2) Read register
$a092        AY-3-8910(PSG2) Write register
$c800-$cfff  RAM(2K)

Strangely the RAM hardware registers seem to be overlaid at $c800
$00a6 routine here reads I/O ports and stores in, its not a straight
copy, the data is mangled before storage:
PSG1-E -> $c808
PSG1-F -> $c80b
PSG2-E -> $c809
PSG2-F -> $c80a - DIP Switch Bank 2 (Test mode is here)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* VIDHRDW */

int  slapfight_vh_start(void);
void slapfight_vh_stop(void);
void slapfight_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void slapfight_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

/* MACHINE */

void slapfight_init_machine(void);

int slapfight_cpu_interrupt(void);
int slapfight_sound_interrupt(void);

extern unsigned char *slapfight_bg_ram1;
extern unsigned char *slapfight_bg_ram2;
extern int slapfight_bg_ram_size;

extern unsigned char *slapfight_dpram;
extern int slapfight_dpram_size;
void slapfight_dpram_w(int offset, int data);
int slapfight_dpram_r(int offset);

int slapfight_bankrom_r(int offset);

int  slapfight_port_00_r(int offset);
int  slapfight_port_01_r(int offset);
int  slapfight_port_02_r(int offset);
int  slapfight_port_03_r(int offset);
int  slapfight_port_04_r(int offset);
int  slapfight_port_05_r(int offset);
int  slapfight_port_06_r(int offset);
int  slapfight_port_07_r(int offset);
int  slapfight_port_08_r(int offset);
int  slapfight_port_09_r(int offset);
int  slapfight_port_0a_r(int offset);
int  slapfight_port_0b_r(int offset);
int  slapfight_port_0c_r(int offset);
int  slapfight_port_0d_r(int offset);
int  slapfight_port_0e_r(int offset);
int  slapfight_port_0f_r(int offset);

void slapfight_port_00_w(int offset, int data);
void slapfight_port_01_w(int offset, int data);
void slapfight_port_02_w(int offset, int data);
void slapfight_port_03_w(int offset, int data);
void slapfight_port_04_w(int offset, int data);
void slapfight_port_05_w(int offset, int data);
void slapfight_port_06_w(int offset, int data);
void slapfight_port_07_w(int offset, int data);
void slapfight_port_08_w(int offset, int data);
void slapfight_port_09_w(int offset, int data);
void slapfight_port_0a_w(int offset, int data);
void slapfight_port_0b_w(int offset, int data);
void slapfight_port_0c_w(int offset, int data);
void slapfight_port_0d_w(int offset, int data);
void slapfight_port_0e_w(int offset, int data);
void slapfight_port_0f_w(int offset, int data);



/* Driver structure definition */

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, slapfight_bankrom_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc80f, slapfight_dpram_r , &slapfight_dpram },
	{ 0xc810, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd7ff, MRA_RAM, &slapfight_bg_ram1 },
	{ 0xd800, 0xdfff, MRA_RAM, &slapfight_bg_ram2 },
	{ 0xe000, 0xe7ff, MRA_RAM, &spriteram },
	{ 0xf000, 0xf7ff, videoram_r, &videoram },
	{ 0xf800, 0xffff, colorram_r, &colorram },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc80f, slapfight_dpram_w, &slapfight_dpram, &slapfight_dpram_size },
	{ 0xc810, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, MWA_RAM, &slapfight_bg_ram1, &slapfight_bg_ram_size },
	{ 0xd800, 0xdfff, MWA_RAM, &slapfight_bg_ram2 },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, videoram_w, &videoram, &videoram_size },
	{ 0xf800, 0xffff, colorram_w, &colorram },
	{ -1 } /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0000, 0x0000, slapfight_port_00_r },
//	{ 0x0001, 0x0001, slapfight_port_01_r },
//	{ 0x0002, 0x0002, slapfight_port_02_r },
//	{ 0x0003, 0x0003, slapfight_port_03_r },
//	{ 0x0004, 0x0004, slapfight_port_04_r },
//	{ 0x0005, 0x0005, slapfight_port_05_r },
//	{ 0x0006, 0x0006, slapfight_port_06_r },
//	{ 0x0007, 0x0007, slapfight_port_07_r },
//	{ 0x0008, 0x0008, slapfight_port_08_r },
//	{ 0x0009, 0x0009, slapfight_port_09_r },
//	{ 0x000a, 0x000a, slapfight_port_0a_r },
//	{ 0x000b, 0x000b, slapfight_port_0b_r },
//	{ 0x000c, 0x000c, slapfight_port_0c_r },
//	{ 0x000d, 0x000d, slapfight_port_0d_r },
//	{ 0x000e, 0x000e, slapfight_port_0e_r },
//	{ 0x000f, 0x000f, slapfight_port_0f_r },
	{ -1 } /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0000, 0x0000, slapfight_port_00_w },
	{ 0x0001, 0x0001, slapfight_port_01_w },
//	{ 0x0002, 0x0002, slapfight_port_02_w },
//	{ 0x0003, 0x0003, slapfight_port_03_w },
//	{ 0x0004, 0x0004, slapfight_port_04_w },
//	{ 0x0005, 0x0005, slapfight_port_05_w },
	{ 0x0006, 0x0006, slapfight_port_06_w },
	{ 0x0007, 0x0007, slapfight_port_07_w },
	{ 0x0008, 0x0008, slapfight_port_08_w },
	{ 0x0009, 0x0009, slapfight_port_09_w },
//	{ 0x000a, 0x000a, slapfight_port_0a_w },
//	{ 0x000b, 0x000b, slapfight_port_0b_w },
//	{ 0x000c, 0x000c, slapfight_port_0c_w },
//	{ 0x000d, 0x000d, slapfight_port_0d_w },
//	{ 0x000e, 0x000e, slapfight_port_0e_w },
//	{ 0x000f, 0x000f, slapfight_port_0f_w },
	{ -1 } /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0xa081, 0xa081, AY8910_read_port_0_r },
	{ 0xa091, 0xa091, AY8910_read_port_1_r },
	{ 0xc800, 0xc80f, MRA_RAM, &slapfight_dpram },
	{ 0xc810, 0xcfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xa080, 0xa080, AY8910_control_port_0_w },
	{ 0xa082, 0xa082, AY8910_write_port_0_w },
	{ 0xa090, 0xa090, AY8910_control_port_1_w },
	{ 0xa092, 0xa092, AY8910_write_port_1_w },
	{ 0xc800, 0xc80f, MWA_RAM, &slapfight_dpram },
	{ 0xc810, 0xcfff, MWA_RAM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( slapfight_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x80, 0x00, "Game Style", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Inversion", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Inverted" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Normal Game" )
	PORT_DIPSETTING(    0x00, "Test Mode" )
	PORT_DIPNAME( 0x10, 0x10, "Attract Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 COIN 1 PLAY" )
	PORT_DIPSETTING(    0x04, "1 COIN 2 PLAYS" )
	PORT_DIPSETTING(    0x08, "2 COINS 1 PLAY" )
	PORT_DIPSETTING(    0x00, "2 COINS 3 PLAYS" )
	PORT_BIT(0x03, IP_ACTIVE_LOW,IPT_UNUSED)

/*  PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 COIN 1 PLAY" )
	PORT_DIPSETTING(    0x01, "1 COIN 2 PLAYS" )
	PORT_DIPSETTING(    0x02, "2 COINS 1 PLAY" )
	PORT_DIPSETTING(    0x00, "2 COINS 3 PLAYS" )
*/

	PORT_START  /* DSW2 */
	PORT_DIPNAME( 0xc0, 0xc0, "Game Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "B" )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x80, "C" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Points", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30K/100K Points" )
	PORT_DIPSETTING(    0x10, "50K/200K Points" )
	PORT_DIPSETTING(    0x20, "50K Points only" )
	PORT_DIPSETTING(    0x00, "100K Points only" )
	PORT_DIPNAME( 0x0c, 0x0c, "Number of Ships", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Dipsw Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x02, "Normal Game" )
	PORT_DIPSETTING(    0x00, "Test" )
	PORT_BIT(0x01, IP_ACTIVE_LOW,IPT_UNUSED)

/*	PORT_DIPNAME( 0x01, 0x01, "Not used", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
*/

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,   /* 8*8 characters */
	1024,  /* 1024 characters */
	2,     /* 2 bits per pixel */
	{ 0, 1*1024*8*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	8,8,    /* 8*8 tiles */
	4096,   /* 4096 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 4096*8*8, 2*4096*8*8, 3*4096*8*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8    /* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,   /* 16*16 sprites */
	1024,    /* 512 sprites */
	4,       /* 4 bits per pixel */
	{ 0, 1*1024*32*8, 2*1024*32*8, 3*1024*32*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ,11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8    /* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   0,  64 },
	{ 1, 0x04000, &tilelayout,   0,  16 },
	{ 1, 0x24000, &spritelayout, 0,  16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 255, 255 },
	{ input_port_0_r, input_port_2_r },
	{ input_port_1_r, input_port_3_r },
	{ 0, 0 },
	{ 0, 0 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,
			0,
			readmem,writemem,readport,writeport,
			slapfight_cpu_interrupt,1,			/* 2 gives approx correct sound but game is too fast */
			0,0
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			6000000,
			3,
			sound_readmem,sound_writemem,0,0,
			interrupt,0,
			slapfight_sound_interrupt,0
		}
	},
	60,
	DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	slapfight_init_machine,

	/* video hardware */
	64*8, 32*8, { 0*8, 37*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	256, 256,
	slapfight_vh_convert_color_prom, /* Convert colour prom     */


	VIDEO_TYPE_RASTER,       /* Video attributes        */
	0,                       /* Video initialisation    */
	slapfight_vh_start,      /* Video start             */
	slapfight_vh_stop,       /* Video stop              */
	slapfight_vh_screenrefresh,     /* Video update            */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

ROM_START( slapfigh_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "sf_r19.bin",  0x00000, 0x8000, 0x418aa5d8 )
	ROM_LOAD( "sf_rh.bin" ,  0x10000, 0x8000, 0xa976a578 )

	ROM_REGION(0x44000)
	ROM_LOAD( "sf_r11.bin",  0x00000, 0x2000, 0x61fa52ca )  /* Chars */
	ROM_LOAD( "sf_r10.bin",  0x02000, 0x2000, 0xe471aa15 )  /* Chars */

	ROM_LOAD( "sf_r06.bin",  0x04000, 0x8000, 0x7553c3df )  /* Tiles */
	ROM_LOAD( "sf_r09.bin",  0x0c000, 0x8000, 0x252d3deb )
	ROM_LOAD( "sf_r08.bin",  0x14000, 0x8000, 0x07db6739 )
	ROM_LOAD( "sf_r07.bin",  0x1c000, 0x8000, 0xff758785 )

	ROM_LOAD( "sf_r03.bin",  0x24000, 0x8000, 0xcbac2464 )  /* Sprites */
	ROM_LOAD( "sf_r01.bin",  0x2c000, 0x8000, 0x03f9d2e3 )
	ROM_LOAD( "sf_r04.bin",  0x34000, 0x8000, 0x227b9fad )
	ROM_LOAD( "sf_r02.bin",  0x3c000, 0x8000, 0x4c273307 )

	ROM_REGION(0x300)
	ROM_LOAD( "sf_col21.bin",0x00000, 0x0100, 0x3d370c0b )
	ROM_LOAD( "sf_col20.bin",0x00100, 0x0100, 0x8baa0604 )
	ROM_LOAD( "sf_col19.bin",0x00200, 0x0100, 0xc187010b )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "sf_r05.bin",  0x0000, 0x2000, 0xa330146a )
ROM_END


ROM_START( slapbtjp_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "sf_r19jb.bin",0x00000, 0x8000, 0x4075880b )
	ROM_LOAD( "sf_rh.bin" ,  0x10000, 0x8000, 0xa976a578 )

	ROM_REGION(0x44000)
	ROM_LOAD( "sf_r11.bin",  0x00000, 0x2000, 0x61fa52ca )  /* Chars */
	ROM_LOAD( "sf_r10.bin",  0x02000, 0x2000, 0xe471aa15 )  /* Chars */

	ROM_LOAD( "sf_r06.bin",  0x04000, 0x8000, 0x7553c3df )  /* Tiles */
	ROM_LOAD( "sf_r09.bin",  0x0c000, 0x8000, 0x252d3deb )
	ROM_LOAD( "sf_r08.bin",  0x14000, 0x8000, 0x07db6739 )
	ROM_LOAD( "sf_r07.bin",  0x1c000, 0x8000, 0xff758785 )

	ROM_LOAD( "sf_r03.bin",  0x24000, 0x8000, 0xcbac2464 )  /* Sprites */
	ROM_LOAD( "sf_r01.bin",  0x2c000, 0x8000, 0x03f9d2e3 )
	ROM_LOAD( "sf_r04.bin",  0x34000, 0x8000, 0x227b9fad )
	ROM_LOAD( "sf_r02.bin",  0x3c000, 0x8000, 0x4c273307 )

	ROM_REGION(0x300)
	ROM_LOAD( "sf_col21.bin",0x00000, 0x0100, 0x3d370c0b )
	ROM_LOAD( "sf_col20.bin",0x00100, 0x0100, 0x8baa0604 )
	ROM_LOAD( "sf_col19.bin",0x00200, 0x0100, 0xc187010b )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "sf_r05.bin",  0x0000, 0x2000, 0xa330146a )
ROM_END


ROM_START( slapbtuk_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "sf_r19eb.bin",0x00000, 0x4000, 0xf533ed37 )
	ROM_LOAD( "sf_r20eb.bin",0x04000, 0x4000, 0xc37e72b8 )
	ROM_LOAD( "sf_rh.bin" ,  0x10000, 0x8000, 0xa976a578 )

	ROM_REGION(0x44000)
	ROM_LOAD( "sf_r11.bin",  0x00000, 0x2000, 0x61fa52ca )  /* Chars */
	ROM_LOAD( "sf_r10.bin",  0x02000, 0x2000, 0xe471aa15 )  /* Chars */

	ROM_LOAD( "sf_r06.bin",  0x04000, 0x8000, 0x7553c3df )  /* Tiles */
	ROM_LOAD( "sf_r09.bin",  0x0c000, 0x8000, 0x252d3deb )
	ROM_LOAD( "sf_r08.bin",  0x14000, 0x8000, 0x07db6739 )
	ROM_LOAD( "sf_r07.bin",  0x1c000, 0x8000, 0xff758785 )

	ROM_LOAD( "sf_r03.bin",  0x24000, 0x8000, 0xcbac2464 )  /* Sprites */
	ROM_LOAD( "sf_r01.bin",  0x2c000, 0x8000, 0x03f9d2e3 )
	ROM_LOAD( "sf_r04.bin",  0x34000, 0x8000, 0x227b9fad )
	ROM_LOAD( "sf_r02.bin",  0x3c000, 0x8000, 0x4c273307 )

	ROM_REGION(0x300)
	ROM_LOAD( "sf_col21.bin",0x00000, 0x0100, 0x3d370c0b )
	ROM_LOAD( "sf_col20.bin",0x00100, 0x0100, 0x8baa0604 )
	ROM_LOAD( "sf_col19.bin",0x00200, 0x0100, 0xc187010b )

	ROM_REGION(0x10000)     /* 64k for the audio CPU */
	ROM_LOAD( "sf_r05.bin",  0x0000, 0x2000, 0xa330146a )
ROM_END


static void slapfigh_decode(void)
{
#ifdef FASTSLAPBOOT

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Remove delay after self test */
	RAM[0x575d]=0;
	RAM[0x575e]=0;
	RAM[0x575f]=0;

	/* Remove delay after copyright message */
	RAM[0x578a]=0;
	RAM[0x578b]=0;
	RAM[0x578c]=0;
	RAM[0x578d]=0;
	RAM[0x578e]=0;
	RAM[0x578f]=0;

#endif
}

static void slapbtjp_decode(void)
{
#ifdef FASTSLAPBOOT

	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Remove delay after self test */
	RAM[0x575d]=0;
	RAM[0x575e]=0;
	RAM[0x575f]=0;

	/* Remove delay after copyright message */
	RAM[0x578a]=0;
	RAM[0x578b]=0;
	RAM[0x578c]=0;
	RAM[0x578d]=0;
	RAM[0x578e]=0;
	RAM[0x578f]=0;

#endif
}

static void slapbtuk_decode(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* Remove delay after self test */
	RAM[0x575d]=0;
	RAM[0x575e]=0;
	RAM[0x575f]=0;

	/* Remove delay after "England" */
	RAM[0x578a]=0;
	RAM[0x578b]=0;
	RAM[0x578c]=0;
	RAM[0x578d]=0;
	RAM[0x578e]=0;
	RAM[0x578f]=0;

}

struct GameDriver slapfigh_driver =
{
	__FILE__,
	0,
	"slapfigh",
	"Slap Fight",
	"1986",
	"Taito",
	"Keith Wilkins\nCarlos Baides(Sprites)\n",
	GAME_NOT_WORKING,
	&machine_driver,

	slapfigh_rom,
	slapfigh_decode, 0,
	0,
	0,

	slapfight_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver slapbtjp_driver =
{
	__FILE__,
	&slapfigh_driver,
	"slapbtjp",
	"Slap Fight (Japanese Bootleg)",
	"1986",
	"bootleg",
	"Keith Wilkins\nCarlos Baides(Sprites)\n",
	0,
	&machine_driver,

	slapbtjp_rom,
	slapbtjp_decode, 0,
	0,
	0,

	slapfight_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver slapbtuk_driver =
{
	__FILE__,
	&slapfigh_driver,
	"slapbtuk",
	"Slap Fight (English Bootleg)",
	"1986",
	"bootleg",
	"Keith Wilkins\nCarlos Baides(Sprites)\n",
	0,
	&machine_driver,

	slapbtuk_rom,
	slapbtuk_decode, 0,
	0,
	0,

	slapfight_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};
