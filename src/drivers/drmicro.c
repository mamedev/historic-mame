/*****************************************************************************

Dr. Micro (c) 1983 Sanritsu

		driver by Uki

Quite similar to Appoooh

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define MCLK 18432000

PALETTE_INIT( drmicro );
VIDEO_START( drmicro );
VIDEO_UPDATE( drmicro );

WRITE_HANDLER( drmicro_flipscreen_w );
WRITE_HANDLER( drmicro_priority_w );

READ_HANDLER( drmicro_videoram_r );
WRITE_HANDLER( drmicro_videoram_w );

extern void drmicro_flip_w( int flip );

/****************************************************************************/

static int drmicro_nmi_enable;

INTERRUPT_GEN( drmicro_interrupt )
{
	if (drmicro_nmi_enable)
		 cpu_set_nmi_line(0, PULSE_LINE);
}

static WRITE_HANDLER( nmi_enable_w )
{	// bit2,3 unknown
	drmicro_nmi_enable = data & 1;
	drmicro_flip_w(data & 2);
}

/****************************************************************************/

static int pcm_adr;

static void pcm_w(int irq)
{
	data8_t *PCM = memory_region(REGION_SOUND1);

	int data = PCM[pcm_adr / 2];

	if (data != 0x70) // ??
	{
		if (~pcm_adr & 1)
			data >>= 4;

		MSM5205_data_w(0, data & 0x0f);
		MSM5205_reset_w(0, 0);

		pcm_adr = (pcm_adr + 1) & 0x7fff;
	}
	else
		MSM5205_reset_w(0, 1);
}

static WRITE_HANDLER( pcm_set_w )
{
	pcm_adr = ((data & 0x3f) << 9);
	pcm_w(0);
}

/****************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, drmicro_videoram_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, drmicro_videoram_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START ( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x03, 0x03, input_port_2_r },
	{ 0x04, 0x04, input_port_3_r },
	{ 0x05, 0x05, IORP_NOP }, // unused?
PORT_END

static PORT_WRITE_START ( writeport )
	{ 0x00, 0x00, SN76496_0_w },
	{ 0x01, 0x01, SN76496_1_w },
	{ 0x02, 0x02, SN76496_2_w },
	{ 0x03, 0x03, pcm_set_w },
	{ 0x04, 0x04, nmi_enable_w },
	{ 0x05, 0x05, IOWP_NOP }, // watchdog?
PORT_END

/****************************************************************************/

INPUT_PORTS_START( drmicro )
	PORT_START // 1P (0)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START // 2P (1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )

	PORT_START // DSW1 (2)
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000 100000" )
	PORT_DIPSETTING(    0x08, "50000 150000" )
	PORT_DIPSETTING(    0x10, "70000 200000" )
	PORT_DIPSETTING(    0x18, "100000 300000" )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START // DSW2 (3)
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // 4-8

INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout spritelayout4 =
{
	16,16,
	0x100,
	2,
	{0,0x2000*8},
	{STEP8(7,-1),STEP8(71,-1)},
	{STEP8(0,8),STEP8(128,8)},
	8*8*4
};

static struct GfxLayout spritelayout8 =
{
	16,16,
	0x100,
	3,
	{0x2000*16,0x2000*8,0},
	{STEP8(7,-1),STEP8(71,-1)},
	{STEP8(0,8),STEP8(128,8)},
	8*8*4
};

static struct GfxLayout charlayout4 =
{
	8,8,
	0x400,
	2,
	{0,0x2000*8},
	{STEP8(7,-1)},
	{STEP8(0,8)},
	8*8*1
};

static struct GfxLayout charlayout8 =
{
	8,8,
	0x400,
	3,
	{0x2000*16,0x2000*8,0},
	{STEP8(7,-1)},
	{STEP8(0,8)},
	8*8*1
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout4,     0, 64 }, /* tiles */
	{ REGION_GFX2, 0x0000, &charlayout8,   256, 32 }, /* tiles */
	{ REGION_GFX1, 0x0000, &spritelayout4,   0, 64 }, /* sprites */
	{ REGION_GFX2, 0x0000, &spritelayout8, 256, 32 }, /* sprites */
	{ -1 } /* end of array */
};

static struct SN76496interface sn76496_interface =
{
	3,							/* 3 chips */
	{ MCLK/4, MCLK/4, MCLK/4 }, /* 4.608MHz? */
	{ 50, 50, 50 }				/* volume */
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip */
	384000,				/* 384 KHz */
	{ pcm_w },			/* IRQ handler */
	{ MSM5205_S64_4B },	/* 6 KHz */
	{ 75 }				/* volume */
};

/****************************************************************************/

static MACHINE_DRIVER_START( drmicro )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,MCLK/6)	/* 3.072MHz? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(drmicro_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(drmicro)
	MDRV_VIDEO_START(drmicro)
	MDRV_VIDEO_UPDATE(drmicro)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

/****************************************************************************/

ROM_START( drmicro )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) // CPU
	ROM_LOAD( "dm-00.13b", 0x0000,  0x2000, 0x270f2145 )
	ROM_LOAD( "dm-01.14b", 0x2000,  0x2000, 0xbba30c80 )
	ROM_LOAD( "dm-02.15b", 0x4000,  0x2000, 0xd9e4ca6b )
	ROM_LOAD( "dm-03.13d", 0x6000,  0x2000, 0xb7bcb45b )
	ROM_LOAD( "dm-04.14d", 0x8000,  0x2000, 0x071db054 )
	ROM_LOAD( "dm-05.15d", 0xa000,  0x2000, 0xf41b8d8a )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE ) // gfx 1
	ROM_LOAD( "dm-23.5l",  0x0000,  0x2000, 0x279a76b8 )
	ROM_LOAD( "dm-24.5n",  0x2000,  0x2000, 0xee8ed1ec )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE ) // gfx 2
	ROM_LOAD( "dm-20.4a",  0x0000,  0x2000, 0x6f5dbf22)
	ROM_LOAD( "dm-21.4c",  0x2000,  0x2000, 0x8b17ff47 )
	ROM_LOAD( "dm-22.4d",  0x4000,  0x2000, 0x84daf771 )

	ROM_REGION( 0x04000, REGION_SOUND1, 0 ) // samples
	ROM_LOAD( "dm-40.12m",  0x0000,  0x2000, 0x3d080af9 )
	ROM_LOAD( "dm-41.13m",  0x2000,  0x2000, 0xddd7bda2 )

	ROM_REGION( 0x00220, REGION_PROMS, 0 ) // PROMs
	ROM_LOAD( "dm-62.9h", 0x0000,  0x0020, 0xe3e36eaf )
	ROM_LOAD( "dm-61.4m", 0x0020,  0x0100, 0x0dd8e365 )
	ROM_LOAD( "dm-60.6e", 0x0120,  0x0100, 0x540a3953 )
ROM_END

GAME( 1983, drmicro, 0, drmicro, drmicro, 0, ROT270, "Sanritsu", "Dr. Micro" )
