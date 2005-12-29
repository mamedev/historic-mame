/*
************************************************
Shuttle Invader? - Omori

8080 CPU

1x  SN76477
2x  SN75452
4x  8216 RAM
2x  TMS4045 RAM
16x MCM4027 RAM
1x  empty small socket. maybe (missing) PROM?
1x  8 position dipsw
1x  556
1x  458
1x  lm380 (amp chip)

xtal 18MHz
xtal 5.545MHz


************************************************
*/
#include "driver.h"
#include "vidhrdw/generic.h"


static ADDRESS_MAP_START( memory_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_ROM
	AM_RANGE(0x4400, 0x47ff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( shuttlei )

INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_decode shuttlei_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,   0, 1 },
	{ -1 }	/* end of array */
};


static MACHINE_DRIVER_START( shuttlei )
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_PROGRAM_MAP(memory_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)

	MDRV_GFXDECODE(shuttlei_gfxdecodeinfo)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

ROM_START( shuttlei )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.13c",   0x0000, 0x0400, BAD_DUMP CRC(253300dc) SHA1(7b51973ed0914b245787c5fdaaec1decfa3e2df5) )
	ROM_LOAD( "2.11c",   0x0400, 0x0400, CRC(68661147) SHA1(19feab0f9c0e03ce43b28b4f7e160d2f42c1037e) )
	ROM_LOAD( "3.13d",   0x0800, 0x0400, CRC(804bd7fb) SHA1(f019bcc2894f9b819a14c069de8f1a7d228b79eb) )
	ROM_LOAD( "4.11d",   0x0c00, 0x0400, CRC(8205b369) SHA1(685dd244881f5762d0f53cbfa935da2b857e3fba) )
	ROM_LOAD( "5.13e",   0x1000, 0x0400, CRC(3537799d) SHA1(0f1d1eff8994cca5e0484a89a18d427c3b634929) )

	ROM_REGION( 0x0400, REGION_GFX1, 0 )
	ROM_LOAD( "8.11f",   0x0000, 0x0400, CRC(f8c9a303) SHA1(ef72e93c9a8d035f39408870d215b36d99fd6ca4) )
ROM_END

GAME( 197?, shuttlei,  0,		shuttlei, shuttlei, 0, ROT270, "Omori", "Shuttle Invader", GAME_NOT_WORKING | GAME_NO_SOUND )
