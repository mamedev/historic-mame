/***************************************************************************

Pacmania - (c) 1987 Namco / Manufactured by Atari Games Co.

Preliminary driver by:
Ernesto Corvi
ernesto@imagina.com


Namco System 1 Banking explained:

This system uses a custom IC to allow accessing an address space of
4 MB, whereas the CPU it uses can only address a maximum of
64 KB. The way it does it is writing to the addresses $Ex00, where
x is the destination bank, and the data written is a17 to a21.
It then can read an 8k bank at $x000.

Examples:
When a write to $E000 happens, then the proper chip get selected
and the cpu can now access it from $0000 to $1fff.
When a write to $E200 happens, then the proper chip get selected
and the cpu can now access it from $2000 to $3fff.
Etc.

Emulating this is not trivial.


stubs for driver.c:
------------------

extern struct GameDriver blazer_driver;
extern struct GameDriver ws90_driver;
extern struct GameDriver dspirits_driver;
extern struct GameDriver splatter_driver;
extern struct GameDriver galaga88_driver;
extern struct GameDriver pacmania_driver;
extern struct GameDriver alice_driver;
extern struct GameDriver shadowld_driver;

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/M6809/M6809.h"
#include "cpu/M6800/M6800.h"

/* from vidhrdw */
extern void namcos1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void namcos1_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern int namcos1_vh_start( void );
extern void namcos1_vh_stop( void );

/* from machine */
extern void namcos1_bankswitch_w( int offset, int data );
extern int namcos1_0_banked_area0_r(int offset);
extern int namcos1_0_banked_area1_r(int offset);
extern int namcos1_0_banked_area2_r(int offset);
extern int namcos1_0_banked_area3_r(int offset);
extern int namcos1_0_banked_area4_r(int offset);
extern int namcos1_0_banked_area5_r(int offset);
extern int namcos1_0_banked_area6_r(int offset);
extern int namcos1_0_banked_area7_r(int offset);
extern int namcos1_1_banked_area0_r(int offset);
extern int namcos1_1_banked_area1_r(int offset);
extern int namcos1_1_banked_area2_r(int offset);
extern int namcos1_1_banked_area3_r(int offset);
extern int namcos1_1_banked_area4_r(int offset);
extern int namcos1_1_banked_area5_r(int offset);
extern int namcos1_1_banked_area6_r(int offset);
extern int namcos1_1_banked_area7_r(int offset);
extern void namcos1_0_banked_area0_w( int offset, int data );
extern void namcos1_0_banked_area1_w( int offset, int data );
extern void namcos1_0_banked_area2_w( int offset, int data );
extern void namcos1_0_banked_area3_w( int offset, int data );
extern void namcos1_0_banked_area4_w( int offset, int data );
extern void namcos1_0_banked_area5_w( int offset, int data );
extern void namcos1_0_banked_area6_w( int offset, int data );
extern void namcos1_1_banked_area0_w( int offset, int data );
extern void namcos1_1_banked_area1_w( int offset, int data );
extern void namcos1_1_banked_area2_w( int offset, int data );
extern void namcos1_1_banked_area3_w( int offset, int data );
extern void namcos1_1_banked_area4_w( int offset, int data );
extern void namcos1_1_banked_area5_w( int offset, int data );
extern void namcos1_1_banked_area6_w( int offset, int data );
extern void namcos1_cpu_control_w( int offset, int data );
extern void namcos1_sound_bankswitch_w( int offset, int data );
extern void namcos1_machine_init( void );
extern void pacmania_driver_init( void );
extern void galaga88_driver_init( void );
extern void splatter_driver_init( void );
extern void dspirits_driver_init( void );
extern void ws90_driver_init( void );
extern void blazer_driver_init( void );
extern void alice_driver_init( void );
extern void shadowld_driver_init( void );

/************************************************************************************/

static struct MemoryReadAddress main_readmem[] =
{
	{ 0x0000, 0x1fff, namcos1_0_banked_area0_r },
	{ 0x2000, 0x3fff, namcos1_0_banked_area1_r },
	{ 0x4000, 0x5fff, namcos1_0_banked_area2_r },
	{ 0x6000, 0x7fff, namcos1_0_banked_area3_r },
	{ 0x8000, 0x9fff, namcos1_0_banked_area4_r },
	{ 0xa000, 0xbfff, namcos1_0_banked_area5_r },
	{ 0xc000, 0xdfff, namcos1_0_banked_area6_r },
	{ 0xe000, 0xffff, namcos1_0_banked_area7_r },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress main_writemem[] =
{
	{ 0x0000, 0x1fff, namcos1_0_banked_area0_w },
	{ 0x2000, 0x3fff, namcos1_0_banked_area1_w },
	{ 0x4000, 0x5fff, namcos1_0_banked_area2_w },
	{ 0x6000, 0x7fff, namcos1_0_banked_area3_w },
	{ 0x8000, 0x9fff, namcos1_0_banked_area4_w },
	{ 0xa000, 0xbfff, namcos1_0_banked_area5_w },
	{ 0xc000, 0xdfff, namcos1_0_banked_area6_w },
	{ 0xe000, 0xefff, namcos1_bankswitch_w },
	{ 0xf000, 0xf000, namcos1_cpu_control_w },
	{ 0xf200, 0xf200, MWA_NOP }, /* watchdog? */
    { -1 }  /* end of table */
};

static struct MemoryReadAddress sub_readmem[] =
{
	{ 0x0000, 0x1fff, namcos1_1_banked_area0_r },
	{ 0x2000, 0x3fff, namcos1_1_banked_area1_r },
	{ 0x4000, 0x5fff, namcos1_1_banked_area2_r },
	{ 0x6000, 0x7fff, namcos1_1_banked_area3_r },
	{ 0x8000, 0x9fff, namcos1_1_banked_area4_r },
	{ 0xa000, 0xbfff, namcos1_1_banked_area5_r },
	{ 0xc000, 0xdfff, namcos1_1_banked_area6_r },
	{ 0xe000, 0xffff, namcos1_1_banked_area7_r },
   { -1 }  /* end of table */
};

static struct MemoryWriteAddress sub_writemem[] =
{
	{ 0xe000, 0xefff, namcos1_bankswitch_w },
	{ 0x0000, 0x1fff, namcos1_1_banked_area0_w },
	{ 0x2000, 0x3fff, namcos1_1_banked_area1_w },
	{ 0x4000, 0x5fff, namcos1_1_banked_area2_w },
	{ 0x6000, 0x7fff, namcos1_1_banked_area3_w },
	{ 0x8000, 0x9fff, namcos1_1_banked_area4_w },
	{ 0xa000, 0xbfff, namcos1_1_banked_area5_w },
	{ 0xc000, 0xdfff, namcos1_1_banked_area6_w },
	{ 0xf000, 0xf000, MWA_NOP }, /* IO Chip */
	{ 0xf200, 0xf200, MWA_NOP }, /* watchdog? */
    { -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1 },	/* Banked ROMs */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5100, 0x513f, MRA_RAM, &namco_soundregs }, /* PSG device */
	{ 0x5000, 0x54ff, MRA_BANK2 },	/* Sound RAM 1 - ( Shared ) */
	{ 0x7000, 0x77ff, MRA_BANK4 },	/* Sound RAM 2 - ( Shared ) */
	{ 0x8000, 0x9fff, MRA_RAM },	/* Sound RAM 3 */
	{ 0xc000, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },	/* Banked ROMs */
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5100, 0x513f, namcos1_sound_w }, /* PSG device */
	{ 0x5000, 0x54ff, MWA_BANK2 },	/* Sound RAM 1 - ( Shared ) */
	{ 0x7000, 0x77ff, MWA_BANK4 },	/* Sound RAM 2 - ( Shared ) */
	{ 0x8000, 0x9fff, MWA_RAM },	/* Sound RAM 3 */
	{ 0xc000, 0xc001, namcos1_sound_bankswitch_w }, /* bank selector */
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xc000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

static int dsw_r( int offs ) {
	int ret = readinputport( 2 );

	if ( offs == 0 )
		ret >>= 4;

	return 0xf0 | ( ret & 0x0f );
}

/* This is very obscure, but i havent found any better way yet. */
/* Works with all games so far.									*/
static void mcu_patch_w( int offs, int data ) {
void mwh_bank3(int _address,int _data);

	if ( cpu_get_pc() == 0xba8e )
		return;

	mwh_bank3( offs, data );
}

static struct MemoryReadAddress mcu_readmem[] =
{
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM }, /* built in RAM */
	{ 0x1400, 0x1400, input_port_0_r },
	{ 0x1401, 0x1401, input_port_1_r },
	{ 0x1000, 0x1002, dsw_r },
	{ 0xb800, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_BANK3 },
	{ 0xc800, 0xcfff, MRA_RAM }, /* EEPROM */
	{ 0xf000, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress mcu_writemem[] =
{
	{ 0x0000, 0x001f, hd63701_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM }, /* built in RAM */
	{ 0xb800, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, mcu_patch_w },
	{ 0xc000, 0xc7ff, MWA_BANK3 },
    { 0xc800, 0xcfff, MWA_RAM }, /* EEPROM */
    { 0xf000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

static struct IOReadPort mcu_readport[] =
{
	{ HD63701_PORT1, HD63701_PORT1, input_port_3_r },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
    PORT_START /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

    PORT_START /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x00, "Auto Data Sampling" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Freeze Display" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Self Test Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Service Button", OSD_KEY_F1, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE, "Service Switch", OSD_KEY_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
    8,8,    /* 8*8 characters */
    16384,   /* 32*256 characters */
    1,      /* bits per pixel */
    { 0 },     /* bitplanes offset */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
    8,8,    /* 8*8 characters */
	16384,   /* 16384 characters max */
    8,      /* 8 bits per pixel */
    { 0, 1, 2, 3, 4, 5, 6, 7 },     /* bitplanes offset */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    { 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
    64*8     /* every char takes 64 consecutive bytes */
};

static struct GfxLayout spritelayout0 =
{
	8,8,  /* 8*8 sprites */
	2*8192,	/* 2*8192 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	8,8,  /* 8*8 sprites */
	2*8192,	/* 2*8192 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout16 =
{
	16,16,  /* 32*32 sprites */
	8192,	/* 8192 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	  8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*4*16, 1*4*16, 2*4*16, 3*4*16, 4*4*16, 5*4*16, 6*4*16, 7*4*16,
	  8*4*16, 9*4*16, 10*4*16, 11*4*16, 12*4*16, 13*4*16, 14*4*16, 15*4*16 },
	32*4*8    /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, 	&charlayout,   0, 1 }, /* character's mask */
	{ 1, 0x20000, 	&tilelayout,   256*8, 16 }, /* characters */
	{ 1, 0x120000,	&spritelayout16, 0, 128 }, /* sprites */
	{ 1, 0x120000,	&spritelayout0, 0, 128 }, /* sprites */
	{ 1, 0x120000,	&spritelayout1, 0, 128 }, /* sprites */
	{ -1 } /* end of array */
};

static void namcos1_sound_interrupt( int irq ) {
	cpu_set_irq_line( 2, 1 , irq ? ASSERT_LINE : CLEAR_LINE);
	//cpu_cause_interrupt( 2, M6809_INT_FIRQ );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ */
	{ 50 },
	{ namcos1_sound_interrupt },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	23920/2,/* sample rate (approximate value) */
	8,		/* number of voices */
	50,		/* playback volume */
	-1,		/* memory region */
	1		/* stereo */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
		    CPU_M6809,
		    49152000/32,        /* Not sure if divided by 32 or 24 */
		    0,
		    main_readmem,main_writemem,0,0,
		    interrupt,1,
		},
		{
		    CPU_M6809,
		    49152000/32,        /* Not sure if divided by 32 or 24 */
		    2,
		    sub_readmem,sub_writemem,0,0,
		    interrupt,1,
		},
		{
		    CPU_M6809,
			49152000/32,        /* Not sure if divided by 32 or 24 */
		    3,
		    sound_readmem,sound_writemem,0,0,
		    interrupt,1
		},
		{
		    CPU_HD63701,	/* or compatible 6808 with extra instructions */
			49152000/8/4,
		    7,
		    mcu_readmem,mcu_writemem,mcu_readport,0,
		    interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	25, /* 25 CPU slice per frame - enough for the cpu's to communicate */
	namcos1_machine_init,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	3*256*8,3*256*8,
	namcos1_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	namcos1_vh_start,
	namcos1_vh_stop,
	namcos1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

/* Alice in Wonderland */
ROM_START( alice_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "MM.CH8",			0x00000, 0x20000, 0xa3784dfe )	/* missing! */
	ROM_LOAD( "MM.CH0",			0x20000, 0x20000, 0x43ff2dfc )	/* characters */
	ROM_LOAD( "MM.CH1",			0x40000, 0x20000, 0xb9b4b72d )	/* characters */
	ROM_LOAD( "MM.CH2",			0x60000, 0x20000, 0xbee28425 )	/* characters */
	ROM_LOAD( "MM.CH3",			0x80000, 0x20000, 0xd9f41e5c )	/* characters */
	ROM_LOAD( "MM.CH4",			0xa0000, 0x20000, 0x3484f4ae )	/* characters */
	ROM_LOAD( "MM.CH5",			0xc0000, 0x20000, 0xc863deba )	/* characters */

	ROM_LOAD( "MM.OB0",			0x120000, 0x20000, 0xd4b7e698 )	/* sprites */
	ROM_LOAD( "MM.OB1",			0x140000, 0x20000, 0x1ce49e04 )	/* sprites */
	ROM_LOAD( "MM.OB2",			0x160000, 0x20000, 0x3d3d5de3 )	/* sprites */
	ROM_LOAD( "MM.OB3",			0x180000, 0x20000, 0xdac57358 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x40000)     /* 192k for the sound cpu */
	ROM_LOAD( "MM.SD0",			0x0c000, 0x04000, 0x25d25e07 )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "MM1.SD1",		0x20000, 0x10000, 0x4a3cc89e )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "MM1.P7",			0x000000, 0x10000, 0x085e58cc )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "MM1.P6",			0x020000, 0x10000, 0xeaf530d8 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x030000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "MM.P2",   		0x0a0000, 0x20000, 0x91bde09f )	/* 8 * 8k banks */
    ROM_LOAD( "MM.P1",   		0x0c0000, 0x20000, 0x6ba14e41 )	/* 8 * 8k banks */
	ROM_LOAD( "MM.P0",   		0x0e0000, 0x20000, 0xe169a911 )	/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x80000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "MM.V0",			0x10000, 0x0f800, 0xee974cff )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_CONTINUE(				0x20000, 0x10000 ) /* 16 * 4k banks */
	ROM_LOAD( "MM.V1",			0x30000, 0x20000, 0xd09b5830 )	/* 32 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Blazer */
ROM_START( blazer_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
//	ROM_LOAD( "chr8.rom",		0x00000, 0x20000, 1 )	/* missing! */
	ROM_LOAD( "chr0.rom",		0x20000, 0x20000, 0xd346ba61 )	/* characters */
	ROM_LOAD( "chr1.rom",		0x40000, 0x20000, 0xe45eb2ea )	/* characters */
	ROM_LOAD( "chr2.rom",		0x60000, 0x20000, 0x599079ee )	/* characters */
	ROM_LOAD( "chr3.rom",		0x80000, 0x20000, 0xd5182e36 )	/* characters */
	ROM_LOAD( "chr4.rom",		0xa0000, 0x20000, 0xe788259e )	/* characters */
	ROM_LOAD( "chr5.rom",		0xc0000, 0x20000, 0x107e6814 )	/* characters */
	ROM_LOAD( "chr6.rom",		0xe0000, 0x20000, 0x0312e2ba )	/* characters */
	ROM_LOAD( "chr7.rom",		0x100000, 0x20000, 0x8cc1827b )	/* characters */

	ROM_LOAD( "obj0.rom",		0x120000, 0x20000, 0xa1e5fb3f )	/* sprites */
	ROM_LOAD( "obj1.rom",		0x140000, 0x20000, 0x4fc4acca )	/* sprites */
	ROM_LOAD( "obj2.rom",		0x160000, 0x20000, 0x114cbc09 )	/* sprites */
	ROM_LOAD( "obj3.rom",		0x180000, 0x20000, 0x7117d08a )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x40000)     /* 192k for the sound cpu */
	ROM_LOAD( "sound0.rom",		0x0c000, 0x04000, 0x6c3a580b )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "sound1.rom",		0x20000, 0x20000, 0xd206b1bd )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "prg7.rom",		0x000000, 0x10000, 0x2d4cbb95 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg6.rom",		0x020000, 0x20000, 0x81c48fc0 )	/* 8 * 8k banks */
    ROM_LOAD( "prg5.rom",		0x040000, 0x20000, 0x900da191 )	/* 8 * 8k banks */
    ROM_LOAD( "prg4.rom",		0x060000, 0x20000, 0xb866e9b0 )	/* 8 * 8k banks */
    ROM_LOAD( "prg3.rom",		0x080000, 0x10000, 0x81b32a1a )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x090000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg2.rom",   	0x0a0000, 0x10000, 0x5d700aed )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0b0000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg1.rom",   	0x0c0000, 0x10000, 0xc54bbbf4 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0d0000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "prg0.rom",   	0x0e0000, 0x10000, 0xa7dd195b )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0f0000, 0x10000 )		/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x80000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "voice0.rom",		0x10000, 0x0f800, 0x3d09d32e )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "voice1.rom",		0x20000, 0x20000, 0x2005c9c0 )	/* 32 * 4k banks */
	ROM_LOAD( "voice2.rom",		0x40000, 0x20000, 0xc876fba6 )	/* 32 * 4k banks */
	ROM_LOAD( "voice3.rom",		0x60000, 0x20000, 0x7d22ac3f )	/* 32 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Dragon Spirits */
ROM_START( dspirits_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "DSCHR-8.BIN",	0x00000, 0x20000, 0x946eb242 )	/* characters */
	ROM_LOAD( "DSCHR-0.BIN",	0x20000, 0x20000, 0x7bf28ac3 )	/* characters */
	ROM_LOAD( "DSCHR-1.BIN",	0x40000, 0x20000, 0x03582fea )	/* characters */
	ROM_LOAD( "DSCHR-2.BIN",	0x60000, 0x20000, 0x5e05f4f9 )	/* characters */
	ROM_LOAD( "DSCHR-3.BIN",	0x80000, 0x20000, 0xdc540791 )	/* characters */
	ROM_LOAD( "DSCHR-4.BIN",	0xa0000, 0x20000, 0xffd1f35c )	/* characters */
	ROM_LOAD( "DSCHR-5.BIN",	0xc0000, 0x20000, 0x8472e0a3 )	/* characters */
	ROM_LOAD( "DSCHR-6.BIN",	0xe0000, 0x20000, 0xa799665a )	/* characters */
	ROM_LOAD( "DSCHR-7.BIN",	0x100000, 0x20000, 0xa51724af )	/* characters */

	ROM_LOAD( "DSOBJ-0.BIN",	0x120000, 0x20000, 0x03ec3076 )	/* sprites */
	ROM_LOAD( "DSOBJ-1.BIN",	0x140000, 0x20000, 0xe67a8fa4 )	/* sprites */
	ROM_LOAD( "DSOBJ-2.BIN",	0x160000, 0x20000, 0x061cd763 )	/* sprites */
	ROM_LOAD( "DSOBJ-3.BIN",	0x180000, 0x20000, 0x63225a09 )	/* sprites */
	ROM_LOAD( "DSOBJ-4.BIN",	0x1a0000, 0x10000, 0xa6246fcb )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "DSSND-0.BIN",	0x0c000, 0x04000, 0x27100065 )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "DSSND-1.BIN",	0x20000, 0x10000, 0xb398645f )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "DSPRG-7.BIN",	0x000000, 0x10000, 0xf4c0d75e )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "DSPRG-6.BIN",	0x020000, 0x10000, 0xa82737b4 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x030000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "DSPRG-5.BIN", 	0x040000, 0x10000, 0x9a3a1028 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x050000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "DSPRG-4.BIN",   	0x060000, 0x10000, 0xf3307870 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x070000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "DSPRG-3.BIN",   	0x080000, 0x10000, 0xc6e5954b )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x090000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "DSPRG-2.BIN",   	0x0a0000, 0x10000, 0x3c9b0100 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0b0000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "DSPRG-1.BIN",   	0x0c0000, 0x10000, 0xf7e3298a )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0d0000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "DSPRG-0.BIN",   	0x0e0000, 0x10000, 0xb22a2856 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0f0000, 0x10000 )		/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0xa0000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "DSVOI-0.BIN",	0x10000, 0x0f800, 0x313b3508 )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "DSVOI-1.BIN",	0x20000, 0x20000, 0x54790d4e )	/* 32 * 4k banks */
	ROM_LOAD( "DSVOI-2.BIN",	0x40000, 0x20000, 0x05298534 )	/* 32 * 4k banks */
	ROM_LOAD( "DSVOI-3.BIN",	0x60000, 0x20000, 0x13e84c7e )	/* 32 * 4k banks */
	ROM_LOAD( "DSVOI-4.BIN",	0x80000, 0x20000, 0x34fbb8cd )	/* 32 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Galaga 88 */
ROM_START( galaga88_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "G88_CHR8.ROM",	0x00000, 0x20000, 0x3862ed0a )	/* characters */
	ROM_LOAD( "G88_CHR0.ROM",	0x20000, 0x20000, 0x68559c78 )	/* characters */
	ROM_LOAD( "G88_CHR1.ROM",	0x40000, 0x20000, 0x3dc0f93f )	/* characters */
	ROM_LOAD( "G88_CHR2.ROM",	0x60000, 0x20000, 0xdbf26f1f )	/* characters */
	ROM_LOAD( "G88_CHR3.ROM",	0x80000, 0x20000, 0xf5d6cac5 )	/* characters */

	ROM_LOAD( "G88_OBJ0.ROM",	0x120000, 0x20000, 0xd7112e3f )	/* sprites */
	ROM_LOAD( "G88_OBJ1.ROM",	0x140000, 0x20000, 0x680db8e7 )	/* sprites */
	ROM_LOAD( "G88_OBJ2.ROM",	0x160000, 0x20000, 0x13c97512 )	/* sprites */
	ROM_LOAD( "G88_OBJ3.ROM",	0x180000, 0x20000, 0x3ed3941b )	/* sprites */
	ROM_LOAD( "G88_OBJ4.ROM",	0x1a0000, 0x20000, 0x370ff4ad )	/* sprites */
	ROM_LOAD( "G88_OBJ5.ROM",	0x1c0000, 0x20000, 0xb0645169 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "G88_SND0.ROM",	0x0c000, 0x04000, 0x164a3fdc )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "G88_SND1.ROM",	0x20000, 0x10000, 0x16a4b784 )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "G88_PRG7.ROM",    0x000000, 0x10000, 0x7c10965d )	/* 8 * 8k banks */
    ROM_RELOAD( 				 0x010000, 0x10000 )				/* 8 * 8k banks */
    ROM_LOAD( "G88_PRG6.ROM",    0x020000, 0x10000, 0xe7203707 )	/* 8 * 8k banks */
    ROM_RELOAD( 				 0x030000, 0x10000 )				/* 8 * 8k banks */
    ROM_LOAD( "G88_PRG5.ROM",    0x040000, 0x10000, 0x4fbd3f6c )	/* 8 * 8k banks */
    ROM_RELOAD( 				 0x050000, 0x10000 )				/* 8 * 8k banks */
    ROM_LOAD( "G88_PRG1.ROM",    0x0c0000, 0x10000, 0xe68cb351 )	/* 8 * 8k banks */
    ROM_RELOAD( 				 0x0d0000, 0x10000 )				/* 8 * 8k banks */
	ROM_LOAD( "G88_PRG0.ROM",    0x0e0000, 0x10000, 0x0f0778ca )	/* 8 * 8k banks */
    ROM_RELOAD( 				 0x0f0000, 0x10000 )				/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x70000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "G88_VCE0.ROM",   0x10000, 0x0f800, 0x86921dd4 )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "G88_VCE1.ROM",   0x20000, 0x10000, 0x9c300e16 )	/* 16 * 4k banks */
	ROM_LOAD( "G88_VCE2.ROM",   0x30000, 0x10000, 0x5316b4b0 )	/* 16 * 4k banks */
	ROM_LOAD( "G88_VCE3.ROM",   0x40000, 0x10000, 0xdc077af4 )	/* 16 * 4k banks */
	ROM_LOAD( "G88_VCE4.ROM",   0x50000, 0x10000, 0xac0279a7 )	/* 16 * 4k banks */
	ROM_LOAD( "G88_VCE5.ROM",   0x60000, 0x10000, 0x014ddba1 )	/* 16 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Pacmania */
ROM_START( pacmania_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "PM_CHR8.BIN",	0x00000, 0x10000, 0xf3afd65d )	/* characters */
	ROM_LOAD( "PM_CHR0.BIN",	0x20000, 0x20000, 0x7c57644c )	/* characters */
	ROM_LOAD( "PM_CHR1.BIN",	0x40000, 0x20000, 0x7eaa67ed )	/* characters */
	ROM_LOAD( "PM_CHR2.BIN",	0x60000, 0x20000, 0x27e739ac )	/* characters */
	ROM_LOAD( "PM_CHR3.BIN",	0x80000, 0x20000, 0x1dfda293 )	/* characters */

	ROM_LOAD( "PM_OBJ0.BIN",	0x120000, 0x20000, 0xfda57e8b )	/* sprites */
	ROM_LOAD( "PM_OBJ1.BIN",	0x140000, 0x20000, 0x4c08affe )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "PM_SND0.BIN",	0x0c000, 0x04000, 0xc10370fa )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "PM_SND1.BIN",	0x20000, 0x10000, 0xf761ed5a )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, load it at the bottom 64k */
    ROM_LOAD( "PM_PRG7.BIN",    0x000000, 0x10000, 0x462fa4fd )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )				/* 8 * 8k banks */
    ROM_LOAD( "PM_PRG6.BIN",    0x020000, 0x20000, 0xfe94900c )	/* 16 * 8k banks */

	ROM_REGION( 0x14000 ) /* 64k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x20000 ) /* 128k for the MCU & voice */
	ROM_LOAD( "PM_VOICE.BIN",   0x10000, 0x0f800, 0x1ad5788f )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Shadowland */
ROM_START( shadowld_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "yd.CH8",			0x00000, 0x20000, 0x0c8e69d0 )	/* missing! */
	ROM_LOAD( "yd.CH0",			0x20000, 0x20000, 0x717441dd )	/* characters */
	ROM_LOAD( "yd.CH1",			0x40000, 0x20000, 0xc1be6e35 )	/* characters */
	ROM_LOAD( "yd.CH2",			0x60000, 0x20000, 0x2df8d8cc )	/* characters */
	ROM_LOAD( "yd.CH3",			0x80000, 0x20000, 0xd4e15c9e )	/* characters */
	ROM_LOAD( "yd.CH4",			0xa0000, 0x20000, 0xc0041e0d )	/* characters */
	ROM_LOAD( "yd.CH5",			0xc0000, 0x20000, 0x7b368461 )	/* characters */
	ROM_LOAD( "yd.CH6",			0xe0000, 0x20000, 0x3ac6a90e )	/* characters */
	ROM_LOAD( "yd.CH7",			0x100000, 0x20000, 0x8d2cffa5 )	/* characters */

	ROM_LOAD( "yd.OB0",			0x120000, 0x20000, 0xefb8efe3 )	/* sprites */
	ROM_LOAD( "yd.OB1",			0x140000, 0x20000, 0xbf4ee682 )	/* sprites */
	ROM_LOAD( "yd.OB2",			0x160000, 0x20000, 0xcb721682 )	/* sprites */
	ROM_LOAD( "yd.OB3",			0x180000, 0x20000, 0x8a6c3d1c )	/* sprites */
	ROM_LOAD( "yd.OB4",			0x1a0000, 0x20000, 0xef97bffb )	/* sprites */
	ROM_LOAD( "YD3.OB5",		0x1c0000, 0x10000, 0x1e4aa460 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "YD1.SD0",		0x0c000, 0x04000, 0xa9cb51fb )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "YD1.SD1",		0x20000, 0x10000, 0x65d1dc0d )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "YD3.P7",			0x000000, 0x10000, 0xf1c271a0 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "YD3.P6",			0x020000, 0x10000, 0x93d6811c )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x030000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "YD1.P5",			0x040000, 0x10000, 0x29a78bd6 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x050000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "YD1.P3",			0x080000, 0x10000, 0xa4f27c24 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x090000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "YD1.P2",   		0x0a0000, 0x10000, 0x62e5bbec )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0b0000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "YD1.P1",   		0x0c0000, 0x10000, 0xa8ea6bd3 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0d0000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "YD1.P0",   		0x0e0000, 0x10000, 0x07e49883 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0f0000, 0x10000 )		/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x70000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "YD1.V0",			0x10000, 0x0f800, 0xcde1ee23 )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "YD1.V1",			0x20000, 0x10000, 0xc61f462b )	/* 16 * 4k banks */
	ROM_LOAD( "YD1.V2",			0x30000, 0x10000, 0x821ad462 )	/* 16 * 4k banks */
	ROM_LOAD( "YD1.V3",			0x40000, 0x10000, 0x1e003489 )	/* 16 * 4k banks */
	ROM_LOAD( "YD1.V4",			0x50000, 0x10000, 0xa106e6f6 )	/* 16 * 4k banks */
	ROM_LOAD( "YD1.V5",			0x60000, 0x10000, 0xde72f38f )	/* 16 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

/* Splatter House */
ROM_START( splatter_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chr8",			0x00000, 0x20000, 0x321f483b )	/* characters */
	ROM_LOAD( "chr0",			0x20000, 0x20000, 0x4dd2ef05 )	/* characters */
	ROM_LOAD( "chr1",			0x40000, 0x20000, 0x7a764999 )	/* characters */
	ROM_LOAD( "chr2",			0x60000, 0x20000, 0x6e6526ee )	/* characters */
	ROM_LOAD( "chr3",			0x80000, 0x20000, 0x8d05abdb )	/* characters */
	ROM_LOAD( "chr4",			0xa0000, 0x20000, 0x1e1f8488 )	/* characters */
	ROM_LOAD( "chr5",			0xc0000, 0x20000, 0x684cf554 )	/* characters */

	ROM_LOAD( "obj0",			0x120000, 0x20000, 0x1cedbbae )	/* sprites */
	ROM_LOAD( "obj1",			0x140000, 0x20000, 0xe56e91ee )	/* sprites */
	ROM_LOAD( "obj2",			0x160000, 0x20000, 0x3dfb0230 )	/* sprites */
	ROM_LOAD( "obj3",			0x180000, 0x20000, 0xe4e5a581 )	/* sprites */
	ROM_LOAD( "obj4",			0x1a0000, 0x20000, 0xb2422182 )	/* sprites */
	ROM_LOAD( "obj5",			0x1c0000, 0x20000, 0x24d0266f )	/* sprites */
	ROM_LOAD( "obj6",			0x1e0000, 0x20000, 0x80830b0e )	/* sprites */
	ROM_LOAD( "obj7",			0x200000, 0x20000, 0x08b1953a )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "sound0",			0x0c000, 0x04000, 0x90abd4ad )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "sound1",			0x20000, 0x10000, 0x8ece9e0a )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "prg7",			0x000000, 0x10000, 0x24c8cbd7 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg6",   		0x020000, 0x10000, 0x97a3e664 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x030000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg5",   		0x040000, 0x10000, 0x0187de9a )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x050000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg4",   		0x060000, 0x10000, 0x350dee5b )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x070000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "prg3",   		0x080000, 0x10000, 0x955ce93f )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x090000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "prg2",   		0x0a0000, 0x10000, 0x434dbe7d )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0b0000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "prg1",   		0x0c0000, 0x10000, 0x7a3efe09 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0d0000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "prg0",   		0x0e0000, 0x10000, 0x4e07e6d9 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0f0000, 0x10000 )		/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x90000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "voice0",   		0x10000, 0x0f800, 0x2199cb66 )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_CONTINUE(				0x20000, 0x10000 ) /* 16 * 4k banks */
	ROM_LOAD( "voice1",   		0x30000, 0x20000, 0x9b6472af )	/* 32 * 4k banks */
	ROM_LOAD( "voice2",			0x50000, 0x20000, 0x25ea75b6 )	/* 32 * 4k banks */
	ROM_LOAD( "voice3",			0x70000, 0x20000, 0x5eebcdb4 )	/* 32 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END


/* World Stadium 90 */
ROM_START( ws90_rom )
    ROM_REGION(0x10000)     /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

    ROM_REGION_DISPOSE(0x220000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WSCHR-8.BIN",	0x00000, 0x20000, 0xd1897b9b )	/* characters */
	ROM_LOAD( "WSCHR-0.BIN",	0x20000, 0x20000, 0x3e3e96b4 )	/* characters */
	ROM_LOAD( "WSCHR-1.BIN",	0x40000, 0x20000, 0x897dfbc1 )	/* characters */
	ROM_LOAD( "WSCHR-2.BIN",	0x60000, 0x20000, 0xe142527c )	/* characters */
	ROM_LOAD( "WSCHR-3.BIN",	0x80000, 0x20000, 0x907d4dc8 )	/* characters */
	ROM_LOAD( "WSCHR-4.BIN",	0xa0000, 0x20000, 0xafb11e17 )	/* characters */
	ROM_LOAD( "WSCHR-6.BIN",	0xe0000, 0x20000, 0xa16a17c2 )	/* characters */

	ROM_LOAD( "WSOBJ-0.BIN",	0x120000, 0x20000, 0x12dc83a6 )	/* sprites */
	ROM_LOAD( "WSOBJ-1.BIN",	0x140000, 0x20000, 0x68290a46 )	/* sprites */
	ROM_LOAD( "WSOBJ-2.BIN",	0x160000, 0x20000, 0xcd5ba55d )	/* sprites */
	ROM_LOAD( "WSOBJ-3.BIN",	0x180000, 0x10000, 0x7d0b8961 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION(0x30000)     /* 192k for the sound cpu */
	ROM_LOAD( "WSSND-0.BIN",	0x0c000, 0x04000, 0x52b84d5a )
	ROM_CONTINUE(				0x10000, 0x0c000 )
	ROM_RELOAD(					0x10000, 0x10000 )
	ROM_LOAD( "WSSND-1.BIN",	0x20000, 0x10000, 0x31bf74c1 )

	ROM_REGION( 0x100000 ) /* 1m for ROMs */
	/* PRGx ROMs go here - they can be 128k max. */
	/* if you got a 64k one, reload it */
    ROM_LOAD( "WSPRG-7.BIN",	0x000000, 0x10000, 0x37ae1b25 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x010000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "WSPRG-2.BIN",   	0x0a0000, 0x10000, 0xb9e98e2f )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0b0000, 0x10000 )		/* 8 * 8k banks */
    ROM_LOAD( "WSPRG-1.BIN",   	0x0c0000, 0x10000, 0x7ad8768f )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0d0000, 0x10000 )		/* 8 * 8k banks */
	ROM_LOAD( "WSPRG-0.BIN",   	0x0e0000, 0x10000, 0xb0234298 )	/* 8 * 8k banks */
    ROM_RELOAD( 				0x0f0000, 0x10000 )		/* 8 * 8k banks */

	ROM_REGION( 0x14000 ) /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 3 * 8k ) */

	ROM_REGION( 0x8000 ) /* 32k for Video RAM - (RAM5) */

	ROM_REGION( 0x40000 ) /* 448k for the MCU & voice */
	ROM_LOAD( "WSVOI-0.BIN",	0x10000, 0x0f800, 0xf6949199 )	/* 15 * 4k banks */
	ROM_CONTINUE(				0x0b800, 0x00800 ) /* This bank has the 63701 code */
	ROM_LOAD( "WSVOI-1.BIN",	0x20000, 0x20000, 0x210e2af9 )	/* 32 * 4k banks */
	ROM_LOAD( "ns1-mcu.bin",    0x0f000, 0x01000, 0xffb5c0bd )	/* mcu 'kernel' code */
ROM_END

struct GameDriver alice_driver =
{
	__FILE__,
	0,
    "alice",
    "Alice In Wonderland",
	"????",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	GAME_NOT_WORKING,
    &machine_driver,
	alice_driver_init,
    alice_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    0, 0
};

struct GameDriver blazer_driver =
{
	__FILE__,
	0,
    "blazer",
    "Blazer",
	"????",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	GAME_NOT_WORKING,
    &machine_driver,
	blazer_driver_init,
    blazer_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_ROTATE_90,

    0, 0
};

struct GameDriver dspirits_driver =
{
	__FILE__,
	0,
    "dspirits",
    "Dragon Spirits",
	"1987",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	GAME_NOT_WORKING,
    &machine_driver,
	dspirits_driver_init,
    dspirits_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_ROTATE_90,

    0, 0
};

struct GameDriver galaga88_driver =
{
	__FILE__,
	0,
    "galaga88",
    "Galaga 88",
	"1987",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	GAME_NOT_WORKING,
    &machine_driver,
	galaga88_driver_init,
    galaga88_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_ROTATE_90,

    0, 0
};

struct GameDriver pacmania_driver =
{
	__FILE__,
	0,
    "pacmania",
    "Pacmania",
	"1987",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	0,
    &machine_driver,
	pacmania_driver_init,
    pacmania_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_ROTATE_90,

    0, 0
};

struct GameDriver shadowld_driver =
{
	__FILE__,
	0,
    "shadowld",
    "Shadowland",
	"????",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	0,
    &machine_driver,
	shadowld_driver_init,
    shadowld_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    0, 0
};

struct GameDriver splatter_driver =
{
	__FILE__,
	0,
    "splatter",
    "Splatter House",
	"1987",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	0,
    &machine_driver,
	splatter_driver_init,
    splatter_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    0, 0
};

struct GameDriver ws90_driver =
{
	__FILE__,
	0,
    "ws90",
    "World Stadium 90",
	"1990",
	"Namco",
    "Ernesto Corvi\nJROK\nTatsuyuki Satoh(optimize)",
	GAME_NOT_WORKING,
    &machine_driver,
	ws90_driver_init,
    ws90_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    0, 0
};
