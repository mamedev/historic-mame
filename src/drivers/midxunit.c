/*************************************************************************

	Midway X-unit system

	driver by Aaron Giles
	based on older drivers by Ernesto Corvi, Alex Pasadyn, Zsolt Vasvari

	Games supported:
		* Revolution X

	Known bugs:
		* none at this time

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "sndhrdw/dcs.h"
#include "midwunit.h"



/*************************************
 *
 *	Memory maps
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), midtunit_vram_data_r },
	{ TOBYTE(0x00800000), TOBYTE(0x00bfffff), midtunit_vram_color_r },
	{ TOBYTE(0x20000000), TOBYTE(0x20ffffff), MRA16_RAM },
	{ TOBYTE(0x60400000), TOBYTE(0x6040001f), midxunit_status_r },
	{ TOBYTE(0x60c00000), TOBYTE(0x60c0007f), midxunit_io_r },
	{ TOBYTE(0x60c000e0), TOBYTE(0x60c000ff), midwunit_security_r },
	{ TOBYTE(0x80800000), TOBYTE(0x8080001f), midxunit_analog_r },
	{ TOBYTE(0x80c00000), TOBYTE(0x80c000ff), midxunit_uart_r },
	{ TOBYTE(0xa0440000), TOBYTE(0xa047ffff), midwunit_cmos_r },
	{ TOBYTE(0xa0800000), TOBYTE(0xa08fffff), midxunit_paletteram_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_r },
	{ TOBYTE(0xc0c00000), TOBYTE(0xc0c000ff), midtunit_dma_r },
	{ TOBYTE(0xf8000000), TOBYTE(0xfeffffff), midwunit_gfxrom_r },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( writemem )
	{ TOBYTE(0x00000000), TOBYTE(0x003fffff), midtunit_vram_data_w },
	{ TOBYTE(0x00800000), TOBYTE(0x00bfffff), midtunit_vram_color_w },
	{ TOBYTE(0x20000000), TOBYTE(0x20ffffff), MWA16_RAM, &midyunit_scratch_ram },
	{ TOBYTE(0x40800000), TOBYTE(0x4fffffff), midxunit_unknown_w },
	{ TOBYTE(0x60400000), TOBYTE(0x6040001f), midxunit_security_clock_w },
	{ TOBYTE(0x60c00080), TOBYTE(0x60c000df), midxunit_io_w },
	{ TOBYTE(0x60c000e0), TOBYTE(0x60c000ff), midxunit_security_w },
	{ TOBYTE(0x80800000), TOBYTE(0x8080001f), midxunit_analog_select_w },
	{ TOBYTE(0x80c00000), TOBYTE(0x80c000ff), midxunit_uart_w },
	{ TOBYTE(0xa0440000), TOBYTE(0xa047ffff), midxunit_cmos_w, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ TOBYTE(0xa0800000), TOBYTE(0xa08fffff), midxunit_paletteram_w, &paletteram16 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00003ff), tms34020_io_register_w },
	{ TOBYTE(0xc0800000), TOBYTE(0xc08000ff), midtunit_dma_w },
	{ TOBYTE(0xc0c00000), TOBYTE(0xc0c000ff), midtunit_dma_w },
	{ TOBYTE(0xf8000000), TOBYTE(0xfbffffff), MWA16_ROM, (data16_t **)&midwunit_decode_memory },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MWA16_ROM, &midyunit_code_rom },
MEMORY_END



/*************************************
 *
 *	Input ports
 *
 *************************************/

INPUT_PORTS_START( revx )
	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0f00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, 0, "Volume Down", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, 0, "Volume Up", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* coin door */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* bill validator */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0001, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0000, "Dipswitch Coinage" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0002, DEF_STR( On ))
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x001c, "1" )
	PORT_DIPSETTING(      0x0018, "2" )
	PORT_DIPSETTING(      0x0014, "3" )
	PORT_DIPSETTING(      0x000c, "USA ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x00e0, 0x0060, "Credits" )
	PORT_DIPSETTING(      0x0020, "3 Start/1 Continue" )
	PORT_DIPSETTING(      0x00e0, "2 Start/2 Continue" )
	PORT_DIPSETTING(      0x00a0, "2 Start/1 Continue" )
	PORT_DIPSETTING(      0x0000, "1 Start/4 Continue" )
	PORT_DIPSETTING(      0x0040, "1 Start/3 Continue" )
	PORT_DIPSETTING(      0x0060, "1 Start/1 Continue" )
	PORT_DIPNAME( 0x0300, 0x0300, "Country" )
	PORT_DIPSETTING(      0x0300, "USA" )
	PORT_DIPSETTING(      0x0100, "French" )
	PORT_DIPSETTING(      0x0200, "German" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Unused ))
	PORT_DIPNAME( 0x0400, 0x0400, "Bill Validator" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0000, "Two Counters" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Players" )
	PORT_DIPSETTING(      0x1000, "3 Players" )
	PORT_DIPSETTING(      0x0000, "2 Players" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Cabinet ))
	PORT_DIPSETTING(      0x2000, "Rev X" )
	PORT_DIPSETTING(      0x0000, "Terminator 2" )
	PORT_DIPNAME( 0x4000, 0x4000, "Video Freeze" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER1, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER1, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER2, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER2, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_X | IPF_REVERSE | IPF_PLAYER3, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x00ff, 0x0080, IPT_LIGHTGUN_Y | IPF_PLAYER3, 20, 10, 0, 0xff)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	34010 configuration
 *
 *************************************/

static struct tms34010_config cpu_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	midtunit_to_shiftreg,			/* write to shiftreg function */
	midtunit_from_shiftreg,			/* read from shiftreg function */
	0,								/* display address changed */
	0								/* display interrupt callback */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

/*
	visible areas and VBLANK timing based on these video params:

	          VERTICAL                   HORIZONTAL
	revx:     0014-0112 / 0120 (254)     0065-001F5 / 01F9 (400)
*/

static MACHINE_DRIVER_START( midxunit )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34020, 40000000/TMS34020_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(cpu_config)
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(MKLA5_FPS)
	MDRV_VBLANK_DURATION((1000000 * (288 - 254)) / (MKLA5_FPS * 288))
	MDRV_MACHINE_INIT(midxunit)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(400, 256)
	MDRV_VISIBLE_AREA(0, 399, 0, 253)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(midxunit)
	MDRV_VIDEO_UPDATE(midtunit)

	/* sound hardware */
	MDRV_IMPORT_FROM(dcs_audio_uart)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( revx )
	ROM_REGION( 0x10, REGION_CPU1, 0 )		/* 34020 dummy region */

	ROM_REGION( ADSP2100_SIZE + 0x800000, REGION_CPU2, 0 )	/* ADSP-2105 data */
	ROM_LOAD( "revx_snd.2", ADSP2100_SIZE + 0x000000, 0x80000, 0x4ed9e803 )
	ROM_LOAD( "revx_snd.3", ADSP2100_SIZE + 0x100000, 0x80000, 0xaf8f253b )
	ROM_LOAD( "revx_snd.4", ADSP2100_SIZE + 0x200000, 0x80000, 0x3ccce59c )
	ROM_LOAD( "revx_snd.5", ADSP2100_SIZE + 0x300000, 0x80000, 0xa0438006 )
	ROM_LOAD( "revx_snd.6", ADSP2100_SIZE + 0x400000, 0x80000, 0xb7b34f60 )
	ROM_LOAD( "revx_snd.7", ADSP2100_SIZE + 0x500000, 0x80000, 0x6795fd88 )
	ROM_LOAD( "revx_snd.8", ADSP2100_SIZE + 0x600000, 0x80000, 0x793a7eb5 )
	ROM_LOAD( "revx_snd.9", ADSP2100_SIZE + 0x700000, 0x80000, 0x14ddbea1 )

	ROM_REGION16_LE( 0x200000, REGION_USER1, ROMREGION_DISPOSE )	/* 34020 code */
	ROM_LOAD32_BYTE( "revx.51",  0x00000, 0x80000, 0x9960ac7c )
	ROM_LOAD32_BYTE( "revx.52",  0x00001, 0x80000, 0xfbf55510 )
	ROM_LOAD32_BYTE( "revx.53",  0x00002, 0x80000, 0xa045b265 )
	ROM_LOAD32_BYTE( "revx.54",  0x00003, 0x80000, 0x24471269 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD( "revx.120", 0x0000000, 0x80000, 0x523af1f0 )
	ROM_LOAD( "revx.121", 0x0080000, 0x80000, 0x78201d93 )
	ROM_LOAD( "revx.122", 0x0100000, 0x80000, 0x2cf36144 )
	ROM_LOAD( "revx.123", 0x0180000, 0x80000, 0x6912e1fb )

	ROM_LOAD( "revx.110", 0x0200000, 0x80000, 0xe3f7f0af )
	ROM_LOAD( "revx.111", 0x0280000, 0x80000, 0x49fe1a69 )
	ROM_LOAD( "revx.112", 0x0300000, 0x80000, 0x7e3ba175 )
	ROM_LOAD( "revx.113", 0x0380000, 0x80000, 0xc0817583 )

	ROM_LOAD( "revx.101", 0x0400000, 0x80000, 0x5a08272a )
	ROM_LOAD( "revx.102", 0x0480000, 0x80000, 0x11d567d2 )
	ROM_LOAD( "revx.103", 0x0500000, 0x80000, 0xd338e63b )
	ROM_LOAD( "revx.104", 0x0580000, 0x80000, 0xf7b701ee )

	ROM_LOAD( "revx.91",  0x0600000, 0x80000, 0x52a63713 )
	ROM_LOAD( "revx.92",  0x0680000, 0x80000, 0xfae3621b )
	ROM_LOAD( "revx.93",  0x0700000, 0x80000, 0x7065cf95 )
	ROM_LOAD( "revx.94",  0x0780000, 0x80000, 0x600d5b98 )

	ROM_LOAD( "revx.81",  0x0800000, 0x80000, 0x729eacb1 )
	ROM_LOAD( "revx.82",  0x0880000, 0x80000, 0x19acb904 )
	ROM_LOAD( "revx.83",  0x0900000, 0x80000, 0x0e223456 )
	ROM_LOAD( "revx.84",  0x0980000, 0x80000, 0xd3de0192 )

	ROM_LOAD( "revx.71",  0x0a00000, 0x80000, 0x2b29fddb )
	ROM_LOAD( "revx.72",  0x0a80000, 0x80000, 0x2680281b )
	ROM_LOAD( "revx.73",  0x0b00000, 0x80000, 0x420bde4d )
	ROM_LOAD( "revx.74",  0x0b80000, 0x80000, 0x26627410 )

	ROM_LOAD( "revx.63",  0x0c00000, 0x80000, 0x3066e3f3 )
	ROM_LOAD( "revx.64",  0x0c80000, 0x80000, 0xc33f5309 )
	ROM_LOAD( "revx.65",  0x0d00000, 0x80000, 0x6eee3e71 )
	ROM_LOAD( "revx.66",  0x0d80000, 0x80000, 0xb43d6fff )

	ROM_LOAD( "revx.51",  0x0e00000, 0x80000, 0x9960ac7c )
	ROM_LOAD( "revx.52",  0x0e80000, 0x80000, 0xfbf55510 )
	ROM_LOAD( "revx.53",  0x0f00000, 0x80000, 0xa045b265 )
	ROM_LOAD( "revx.54",  0x0f80000, 0x80000, 0x24471269 )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1994, revx,   0,         midxunit, revx, revx, ROT0, "Midway",   "Revolution X (Rev. 1.0 6/16/94)" )
