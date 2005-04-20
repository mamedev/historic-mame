/*

Dream World
(c)2000 SemiCom

can't make any more progress until the protection is figured out
but I suspect it will be similar to all the protection semicom
used for everything else (0x200 bytes of code/data placed in RAM
at startup by the MCU)

*/

#include "driver.h"
#include "machine/random.h"

VIDEO_START(dreamwld)
{
	return 0;
}

VIDEO_UPDATE(dreamwld)
{

}

READ32_HANDLER( dreamwld_random_read)
{
	return mame_rand();
}

static ADDRESS_MAP_START( dreamwld_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x400000, 0x40100f) AM_RAM
	AM_RANGE(0x600000, 0x601fff) AM_RAM
	AM_RANGE(0x800000, 0x805fff) AM_RAM
	AM_RANGE(0xc00000, 0xc0ffff) AM_READ(dreamwld_random_read)
	AM_RANGE(0xfe0000, 0xfeffff) AM_RAM
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END



INPUT_PORTS_START(dreamwld)
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*
static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};
*/
static struct GfxLayout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX4, 0, &tiles16x16_layout, 0, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( dreamwld )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)
	MDRV_CPU_PROGRAM_MAP(dreamwld_map, 0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024,1024)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_GFXDECODE(gfxdecodeinfo)


	MDRV_VIDEO_START(dreamwld)
	MDRV_VIDEO_UPDATE(dreamwld)
MACHINE_DRIVER_END

ROM_START( dreamwld )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "1.bin", 0x000002, 0x040000, CRC(35c94ee5) SHA1(3440a65a807622b619c97bc2a88fd7d875c26f66) )
	ROM_LOAD32_BYTE( "2.bin", 0x000003, 0x040000, CRC(5409e7fc) SHA1(2f94a6a8e4c94b36b43f0b94d58525f594339a9d) )
	ROM_LOAD32_BYTE( "3.bin", 0x000000, 0x040000, CRC(e8f7ae78) SHA1(cfd393cec6dec967c82e1131547b7e7fdc5d814f) )
	ROM_LOAD32_BYTE( "4.bin", 0x000001, 0x040000, CRC(3ef5d51b) SHA1(82a00b4ff7155f6d5553870dfd510fed9469d9b5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP ) /* can't be dumped */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "6.bin", 0x000000, 0x80000, CRC(c8b91f30) SHA1(706004ca56d0a74bc7a3dfd73a21cdc09eb90f05) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* OKI Samples */
	ROM_LOAD( "5.bin", 0x000000, 0x80000, CRC(9689570a) SHA1(4414233da8f46214ca7e9022df70953922a63aa4) )


	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprite Tiles? - decoded */
	ROM_LOAD( "9.bin", 0x000000, 0x200000, CRC(fa84e3af) SHA1(5978737d348fd382f4ec004d29870656c864d137) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* Sprite Tiles? - decoded */
	ROM_LOAD( "10.bin", 0x000000, 0x200000, CRC(3553e4f5) SHA1(c335494f4a12a01a88e7cd578cae922954303cfd) )

	ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Other GFX? - not decoded - they go together but are at opposite ends of the board??*/
	ROM_LOAD16_BYTE( "7.bin", 0x000000, 0x020000, CRC(a68bf35f) SHA1(f48540a5415a7d9723ca6e7e03cab039751dce17) )
	ROM_LOAD16_BYTE( "8.bin", 0x000001, 0x020000, CRC(8d570df6) SHA1(e53e4b099c64eca11d027e0083caa101fcd99959) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded */
	ROM_LOAD( "11.bin", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END


GAMEX(2000, dreamwld, 0,        dreamwld, dreamwld, 0, ROT0,  "SemiCom", "Dream World", GAME_NOT_WORKING|GAME_NO_SOUND )
