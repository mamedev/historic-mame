/*

Vamp Half + other hyperstone based games
wip, to be used for testing Hyperstone CPU core
these will be split up later

*/

#include "driver.h"

static MEMORY_READ32_START( readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM },
	{ 0xfff80000, 0xffffffff, MRA32_BANK1 },
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM },
	{ 0xfff80000, 0xffffffff, MWA32_ROM },
MEMORY_END

static MEMORY_READ32_START( xfiles_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM },
	{ 0xffc00000, 0xffffffff, MRA32_BANK1 },
MEMORY_END

static MEMORY_WRITE32_START( xfiles_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM },
	{ 0xffc00000, 0xffffffff, MWA32_ROM },
MEMORY_END

INPUT_PORTS_START( vamphalf )
INPUT_PORTS_END


VIDEO_START( vamphalf )
{
	return 0;
}

VIDEO_UPDATE( vamphalf )
{

}

static struct GfxLayout vamphalf_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24, 32,40,48,56, 64,72,80,88 ,96,104,112,120 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*128,
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &vamphalf_layout,   0x0, 1  }, /* bg tiles */
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( vamphalf )
	MDRV_CPU_ADD(E132XS,10000000)		 /* ?? */
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xfiles )
	MDRV_CPU_ADD(E132XS,10000000)		 /* ?? */
	MDRV_CPU_MEMORY(xfiles_readmem,xfiles_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END

/* f2 systems hardware */

ROM_START( vamphalf )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("prom1", 0x00000000,    0x080000,   CRC(f05e8e96) SHA1(c860e65c811cbda2dc70300437430fb4239d3e2d))

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 16x16x8 Sprites? */
	ROM_LOAD32_WORD( "roml00",       0x000000, 0x200000, CRC(cc075484) SHA1(6496d94740457cbfdac3d918dce2e52957341616) )
	ROM_LOAD32_WORD( "roml01",       0x400000, 0x200000, CRC(626c9925) SHA1(c90c72372d145165a8d3588def12e15544c6223b) )
	ROM_LOAD32_WORD( "romu00",       0x000002, 0x200000, CRC(711c8e20) SHA1(1ef7f500d6f5790f5ae4a8b58f96ee9343ef8d92) )
	ROM_LOAD32_WORD( "romu01",       0x400002, 0x200000, CRC(d5be3363) SHA1(dbdd0586909064e015f190087f338f37bbf205d2) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "vrom1",        0x000000, 0x040000, CRC(ee9e371e) SHA1(3ead5333121a77d76e4e40a0e0bf0dbc75f261eb) )
ROM_END

/* eolith hardware */

ROM_START( hidnctch )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("hc_u43.bin", 0x00000000,    0x080000,  CRC(635b4478) SHA1(31ea4a9725e0c329447c7d221c22494c905f6940) )

	/* there are more roms but we don't load them yet .. not needed for testing cpu core */

	ROM_REGION( 0x800000, REGION_GFX1, 0 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Oki Samples */
ROM_END

ROM_START( landbrk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("lb_1.u43", 0x00000000,    0x080000,   CRC(f8bbcf44) SHA1(ad85a890ae2f921cd08c1897b4d9a230ccf9e072) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("lb2-000.u39", 0x0000001,    0x0400000, CRC(b37faf7a) SHA1(30e9af3957ada7c72d85f55add221c2e9b3ea823) )
	ROM_LOAD16_BYTE("lb2-001.u34", 0x0000000,    0x0400000, CRC(07e620c9) SHA1(19f95316208fb4e52cef78f18c5d93460a644566) )
	ROM_LOAD16_BYTE("lb2-002.u40", 0x0800001,    0x0400000, CRC(3bb4bca6) SHA1(115029be4a4e322549a35f3ae5093ec161e9a421) )
	ROM_LOAD16_BYTE("lb2-003.u35", 0x0800000,    0x0400000, CRC(28ce863a) SHA1(1ba7d8be0ed4459dbdf99df18a2ad817904b9f04) )
	ROM_LOAD16_BYTE("lb2-004.u41", 0x1000001,    0x0400000, CRC(cbe84b06) SHA1(52505939fb88cd24f409c795fe5ceed5b41a52c2))
	ROM_LOAD16_BYTE("lb2-005.u36", 0x1000000,    0x0400000, CRC(350c77a3) SHA1(231e65ea7db19019615a8aa4444922bcd5cf9e5c) )
	ROM_LOAD16_BYTE("lb2-006.u42", 0x1800001,    0x0400000, CRC(22c57cd8) SHA1(c9eb745523005876395ff7f0b3e996994b3f1220))
	ROM_LOAD16_BYTE("lb2-007.u37", 0x1800000,    0x0400000, CRC(31f957b3) SHA1(ab1c4c50c2d5361ba8db047feb714423d84e6df4) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("lb_2.108", 0x000000,    0x080000,  CRC(a99182d7) SHA1(628c8d09efb3917a4e97d9e02b6b0ca1f339825d) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("lb.107", 0x000000,    0x08000,    CRC(afd5263d) SHA1(71ace1b749d8a6b84d08b97185e7e512d04e4b8d) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("lb_3.u97", 0x000000,    0x080000,  CRC(5b34dff0) SHA1(1668763e977e272781ddcc74beba97b53477cc9d) )
ROM_END

ROM_START( racoon )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("racoon-u.43", 0x00000000,    0x080000,  CRC(711ee026) SHA1(c55dfaa24cbaa7a613657cfb25e7f0085f1e4cbf) )

	/* there are more roms but we don't load them yet .. not needed for testing cpu core */

	ROM_REGION( 0x800000, REGION_GFX1, 0 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
ROM_END

/* ?? dfpix hardware */

ROM_START( xfiles )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP("u9.bin", 0x00000000,    0x400000,   CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )

	/* there are more roms but we don't load them yet .. not needed for testing cpu core */

	ROM_REGION( 0x800000, REGION_GFX1, 0 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
ROM_END

DRIVER_INIT( vamphalf )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
}

GAMEX( 19??, vamphalf, 0, vamphalf, vamphalf, vamphalf, ROT0, "Danbi", "Vamp 1/2", GAME_NO_SOUND )
GAMEX( 19??, hidnctch, 0, vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Hidden Catch", GAME_NO_SOUND )
GAMEX( 19??, landbrk, 0, vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Land Breaker", GAME_NO_SOUND )
GAMEX( 19??, racoon, 0, vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Racoon World", GAME_NO_SOUND )
GAMEX( 19??, xfiles, 0, xfiles, vamphalf, vamphalf, ROT0, "dfPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND )
