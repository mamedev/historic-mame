

/*
Taito Gladiator (1986)
(at least one bootleg exists, called "Golden Castle" )

Credits:
- Victor Trucco: original emulation and MAME driver
- Steve Ellenoff: high score save, input port patches, panning fix, sprite banking
- Phil Stroffolino: palette, sprites, misc video driver fixes

special thanks to:
- Camilty for precious hardware information and screenshots
- Jason Richmond for hardware information and misc. notes
- Joe Rounceville for schematics
- and everyone else who'se offered support along the way!

Issues:
- communication to/from sound CPU(s) not yet hooked up
- input ports/dip switches are working RAM hacks (see above)
- CPU speed may not be accurate
- some sprites linger on later stages (probably a sprite enable bit)

Preliminary Gladiator Memory Map

Main CPU (Z80)

$0000-$3FFF	QB0-5
$4000-$5FFF	QB0-4

$6000-$7FFF	QB0-1 Paged
$8000-$BFFF	QC0-3 Paged

    if port 02 = 00     QB0-1 offset (0000-1fff) at location 6000-7fff
                        QC0-3 offset (0000-3fff) at location 8000-bfff

    if port 02 = 01     QB0-1 offset (2000-3fff) at location 6000-7fff
                        QC0-3 offset (4000-7fff) at location 8000-bfff

$C000-$C3FF	sprite RAM
$C400-$C7FF	sprite attributes
$C800-$CBFF	more sprite attributes

$CC00-$D7FF	video registers

(scrolling, 2 screens wide)
$D800-DFFF	background layer VRAM (tiles)
$E000-E7FF	background layer VRAM (attributes)
$E800-EFFF	foreground text layer VRAM

$F000-$F3FF	Battery Backed RAM
$F400-$F7FF	Work RAM

Audio CPU (Z80)
$0000-$3FFF	QB0-17
$8000-$83FF	Work RAM 2.CPU


Preliminary Descriptions of I/O Ports.

Main z80
8 pins of LS259:
  00 - OBJACS ? (I can't read the name in schematics)
  01 - OBJCGBK (Sprite banking)
  02 - PR.BK (ROM banking)
  03 - NMIFG (connects to NMI of main Z80, but there's no code in 0066)
  04 - SRST (probably some type of reset)
  05 - CBK0 (unknown)
  06 - LOBJ (connected near graphic ROMs)
  07 - REVERS

9E, 9F - Comunication ports to 2nd Z80 (dip switch 0 is read here too)

2nd z80

00 - YM2203 Control Reg.
01 - YM2203 Data Read / Write Reg.
		Port B of the YM2203 is connected to dip switch 1

20 - Send data to NEC 8741 (comunicate with Main z80)
		(dip switch 2 is read in these ports too)

21 - Send commands to NEC 8741 (comunicate with Main z80)

80, 81 - Read buttons (Fire 1, 2 and 3 (both players), service button)
60, 61 - Read Joystick and Coin Slot (both players)
E0     - Comunication port to 6809
*/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *gladiator_text;
extern void gladiatr_spritebank_w( int offset, int data );
extern void gladiatr_video_registers_w( int offset, int data );
extern int gladiatr_video_registers_r( int offset );
extern void gladiatr_paletteram_rg_w( int offset, int data );
extern void gladiatr_paletteram_b_w( int offset, int data );
extern int gladiatr_vh_start(void);
extern void gladiatr_vh_stop(void);
extern void gladiatr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void gladiatr_bankswitch_w(int offset,int data);
extern int gladiatr_bankswitch_r(int offset);
extern void gladiatr_port9e_c1w(int offset,int data);
extern void gladiatr_port9f_c1w(int offset,int data);
extern int gladiatr_port9e_c1r(int offset);
extern int gladiatr_port9f_c1r(int offset);
extern  int gladiatr_patch(int offset);

static int hack_timer = 0;
static unsigned char *dipsw_ram;
static int dipsw_patch_r( int offset );
static int dipsw_patch_r( int offset ){
	if( hack_timer==221 ){ /* wait until after RAM test */
		switch( offset ){
			case 0x0: return readinputport( 0 ); /* difficulty, bonus, lives */
			case 0x1: return readinputport( 1 ); /* monitor type, coins per play */
			case 0x2: return readinputport( 2 ); /* test mode */
		}
	}
	else {
		hack_timer++;
	}

	return dipsw_ram[offset];
}

static unsigned char *input_ram;
static int input_patch_r( int offset );
static int input_patch_r( int offset ){
	if( hack_timer==221 ){
		switch( offset ){
			case 0: {
				int result = readinputport(3); /* Player 1 Controls */

				int special = readinputport(5); /* coins, start buttons */
				if( special&0x08 ) result |= 0x01; /* P1 start */
				if( special&0x10 ) result |= 0x02; /* P2 start */
				if( special&0x07 ) result |= 0x80; /* coins */

				return result;
			}
			case 1: return readinputport(4); /* Player 2 Controls */
		}
	}
	return input_ram[offset];
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1},
	{ 0x8000, 0xbfff, MRA_BANK2},
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_r },
	{ 0xd000, 0xf69f, MRA_RAM },
	{ 0xf6a0, 0xf6ac, dipsw_patch_r, &dipsw_ram },
	{ 0xf6ad, 0xf782, MRA_RAM },
	{ 0xf783, 0xf784, input_patch_r, &input_ram },
	{ 0xf785, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcbff, MWA_RAM, &spriteram },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_w },
	{ 0xd000, 0xd1ff, gladiatr_paletteram_rg_w, &paletteram },
	{ 0xd400, 0xd5ff, gladiatr_paletteram_b_w, &paletteram_2 },
	{ 0xd800, 0xdfff, videoram_w, &videoram },
	{ 0xe000, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xefff, MWA_RAM, &gladiator_text },
	{ 0xf000, 0xf3ff, MWA_RAM }, /* battery backed RAM */
	{ 0xf400, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x02, 0x02, gladiatr_bankswitch_r },
//	{ 0x9e, 0x9e, gladiatr_port9e_c1r },
//	{ 0x9f, 0x9F, gladiatr_port9f_c1r },
	{ 0x9f, 0x9f, gladiatr_patch },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, gladiatr_spritebank_w},
	{ 0x02, 0x02, gladiatr_bankswitch_w},
//	{ 0x9e, 0x9e, gladiatr_port9e_c1w },
//	{ 0x9f, 0x9f, gladiatr_port9f_c1w },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport_cpu2[] =
{
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x21, 0x21, IORP_NOP },	/* NEC8741 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport_cpu2[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "After 4 Stages", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Continues" )
	PORT_DIPSETTING(    0x00, "Ends" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Every 100000" )
	PORT_DIPSETTING(    0x08, "Only at 100000" )
	PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW2 */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Memory Backup", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Clear" )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE, "Coin C", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************************/

static struct GfxLayout gladiator_text_layout  =   /* gfxset 0 */
{
	8,8,	/* 8*8 tiles */
	1024,	/* number of tiles */
	1,		/* bits per pixel */
	{ 0 },	/* plane offsets */
	{ 0,1,2,3,4,5,6,7 }, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, /* y offsets */
	64 /* offset to next tile */
};

/*******************************************************************/

#define DEFINE_LAYOUT( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	8,8,512,3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3 }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, \
	128 \
};

DEFINE_LAYOUT( gladiator_tile0, 4,			0x08000*8, 0x08000*8+4 )
DEFINE_LAYOUT( gladiator_tile1, 0,			0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tile2, 4+0x2000*8, 0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tile3, 0+0x2000*8, 0x12000*8, 0x12000*8+4 )
DEFINE_LAYOUT( gladiator_tile4, 4+0x4000*8, 0x0C000*8, 0x0C000*8+4 )
DEFINE_LAYOUT( gladiator_tile5, 0+0x4000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tile6, 4+0x6000*8, 0x14000*8, 0x14000*8+4 )
DEFINE_LAYOUT( gladiator_tile7, 0+0x6000*8, 0x16000*8, 0x16000*8+4 )
DEFINE_LAYOUT( gladiator_tileA, 4+0x2000*8, 0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tileB, 0,			0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tileC, 4+0x6000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tileD, 0+0x4000*8, 0x14000*8, 0x14000*8+4 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* monochrome text layer */
	{ 1, 0x34000, &gladiator_text_layout, 512, 1 },

	/* background tiles */
	{ 1, 0x18000, &gladiator_tile0, 0, 64 },
	{ 1, 0x18000, &gladiator_tile1, 0, 64 },
	{ 1, 0x18000, &gladiator_tile2, 0, 64 },
	{ 1, 0x18000, &gladiator_tile3, 0, 64 },
	{ 1, 0x18000, &gladiator_tile4, 0, 64 },
	{ 1, 0x18000, &gladiator_tile5, 0, 64 },
	{ 1, 0x18000, &gladiator_tile6, 0, 64 },
	{ 1, 0x18000, &gladiator_tile7, 0, 64 },

	/* sprites */
	{ 1, 0x30000, &gladiator_tile0, 0, 64 },
	{ 1, 0x30000, &gladiator_tileB, 0, 64 },

	{ 1, 0x30000, &gladiator_tileA, 0, 64 },
	{ 1, 0x30000, &gladiator_tile3, 0, 64 }, /* "GLAD..." */

	{ 1, 0x00000, &gladiator_tile0, 0, 64 },
	{ 1, 0x00000, &gladiator_tileB, 0, 64 },

	{ 1, 0x00000, &gladiator_tileA, 0, 64 },
	{ 1, 0x00000, &gladiator_tile3, 0, 64 }, /* ...DIATOR */

	{ 1, 0x00000, &gladiator_tile4, 0, 64 },
	{ 1, 0x00000, &gladiator_tileD, 0, 64 },

	{ 1, 0x00000, &gladiator_tileC, 0, 64 },
	{ 1, 0x00000, &gladiator_tile7, 0, 64 },

	{ -1 } /* end of array */
};

#undef DEFINE_LAYOUT



static struct AY8910interface ym2203_interface =
{
	1,	/* 1 chip */
	3000000,	/* 3 MHz? */
	{ YM2203_VOL(255,255) },
	{ 0 },
 	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	{
		{
			CPU_Z80,
			6000000, /* 6 MHz? */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000, /* 3 MHz? */
			2,	/* memory region #2 */
			readmem_cpu2,writemem_cpu2,readport_cpu2,writeport_cpu2,
			interrupt,1

		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			3000000, /* 3 MHz? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION, /* fps, vblank duration */
	10,	/* interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0, 255, 0+16, 255-16 },

	gfxdecodeinfo,
	512+2, 512+2,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gladiatr_vh_start,
	gladiatr_vh_stop,
	gladiatr_vh_screenrefresh,

	/* sound hardware */
	0, 0,	0, 0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gladiatr_rom )
	ROM_REGION(0x1c000)
	ROM_LOAD( "QB0-5", 0x00000,	0x4000, 0x4f9d05a1 )
	ROM_LOAD( "QB0-4", 0x04000,	0x2000, 0x4bc45fda )
	ROM_LOAD( "QB0-1", 0x10000,	0x4000, 0x11836769 )
	ROM_LOAD( "QC0-3", 0x14000,	0x8000, 0xabd3f7d7 )

	ROM_REGION(0x44000)	/* temporary space for graphics (disposed after conversion) */
	/* sprites */
	ROM_LOAD( "QC2-7",	0x00000, 0x8000, 0x4831f275 ) /* plane 3 */
	ROM_LOAD( "QC1-10",	0x08000, 0x8000, 0x39a63d4a ) /* planes 1,2 */
	ROM_LOAD( "QC2-11",	0x10000, 0x8000, 0x3a74528e ) /* planes 1,2 */

	ROM_LOAD( "QC1-6",	0x30000, 0x4000, 0x43a8bd82 ) /* plane 3 */
	ROM_LOAD( "QC0-8",	0x38000, 0x4000, 0x70b2dc78 ) /* planes 1,2 */
	ROM_LOAD( "QC1-9",	0x40000, 0x4000, 0x5dbe5056 ) /* planes 1,2 */

	/* tiles */
	ROM_LOAD( "QB0-12",	0x18000, 0x8000, 0x021c6258 ) /* plane 3 */
	ROM_LOAD( "QB0-13",	0x20000, 0x8000, 0x796ff62b ) /* planes 1,2 */
	ROM_LOAD( "QB0-14",	0x28000, 0x8000, 0x2f23bdff ) /* planes 1,2 */

	ROM_LOAD( "QC0-15",	0x34000, 0x2000, 0xe3f02c06 ) /* (monochrome) */

	ROM_REGION( 0x10000 ) /* Code for de 2nd. CPU */
	ROM_LOAD( "QB0-17",	0x0000, 0x4000, 0xf68e3364 )

	ROM_REGION( 0x20000 )  /* QB0-18, QB0-19, QB0-20 contain 4-bit adpcm sounds */
	ROM_LOAD( "QB0-18", 0x08000, 0x8000, 0x9437974d )
	ROM_LOAD( "QB0-19", 0x10000, 0x8000, 0xa31d0a5f )
	ROM_LOAD( "QB0-20", 0x18000, 0x8000, 0x8af2da92 )
ROM_END

ROM_START( ogonsiro_rom )
	ROM_REGION(0x1c000)
	ROM_LOAD( "QB0-5", 0x00000,	0x4000, 0x4f9d05a1 )
	ROM_LOAD( "QB0-4", 0x04000,	0x2000, 0x4bc45fda )
	ROM_LOAD( "QB0-1", 0x10000,	0x4000, 0x11836769 )
	ROM_LOAD( "QB0_3", 0x14000,	0x8000, 0xacd3f737 )

	ROM_REGION(0x44000)	/* temporary space for graphics (disposed after conversion) */
	/* sprites */
	ROM_LOAD( "QB0_7",	0x00000, 0x8000, 0xa86b93a9 ) /* plane 3 */
	ROM_LOAD( "QB0_10",	0x08000, 0x8000, 0x7c6f2813 ) /* planes 1,2 */
	ROM_LOAD( "QB0_11",	0x10000, 0x8000, 0x8a96c5ec ) /* planes 1,2 */

	ROM_LOAD( "QB0_6",	0x30000, 0x4000, 0xeaa88da2 ) /* plane 3 */
	ROM_LOAD( "QC0-8",	0x38000, 0x4000, 0x70b2dc78 ) /* planes 1,2 */
	ROM_LOAD( "QB0_9",	0x40000, 0x4000, 0x17ce0786 ) /* planes 1,2 */

	/* tiles */
	ROM_LOAD( "QB0-12",	0x18000, 0x8000, 0x021c6258 ) /* plane 3 */
	ROM_LOAD( "QB0-13",	0x20000, 0x8000, 0x796ff62b ) /* planes 1,2 */
	ROM_LOAD( "QB0-14",	0x28000, 0x8000, 0x2f23bdff ) /* planes 1,2 */

	ROM_LOAD( "QB0_15",	0x34000, 0x2000, 0x598891f2 ) /* (monochrome) */

	ROM_REGION( 0x10000 ) /* Code for de 2nd. CPU */
	ROM_LOAD( "QB0-17",	0x0000, 0x4000, 0xf68e3364 )

	ROM_REGION( 0x20000 )  /* QB0-18, QB0-19, QB0-20 contain 4-bit adpcm sounds */
	ROM_LOAD( "QB0-18", 0x08000, 0x8000, 0x9437974d )
	ROM_LOAD( "QB0-19", 0x10000, 0x8000, 0xa31d0a5f )
	ROM_LOAD( "QB0-20", 0x18000, 0x8000, 0x8af2da92 )
ROM_END



static int gladiatr_nvram_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xf000],0x400);
		osd_fclose(f);
	}
	return 1;
}

static void gladiatr_nvram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xf000],0x400);
		osd_fclose(f);
	}
}



struct GameDriver gladiatr_driver =
{
	__FILE__,
	0,
	"gladiatr",
	"Gladiator",
	"1986",
	"Taito America",
	"Victor Trucco\nSteve Ellenoff\nPhil Stroffolino",
	0,
	&machine_driver,

	gladiatr_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gladiatr_nvram_load, gladiatr_nvram_save
};

struct GameDriver ogonsiro_driver =
{
	__FILE__,
	&gladiatr_driver,
	"ogonsiro",
	"Ohgon no Siro",
	"1986",
	"Taito",
	"Victor Trucco\nSteve Ellenoff\nPhil Stroffolino",
	0,
	&machine_driver,

	ogonsiro_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gladiatr_nvram_load, gladiatr_nvram_save
};
