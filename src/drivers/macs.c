/*

macs.c - Multi Amenity Cassette System

processor seems to be ST0016 (z80 based) from SETA

around 0x3700 of the bios (when interleaved) contains the ram test text


----- Game Notes -----

Kisekae Mahjong  (c)1995 I'MAX
Kisekae Hanafuda (c)1995 I'MAX
Seimei-Kantei-Meimei-Ki Cult Name (c)1996 I'MAX

KISEKAE -- info

* DIP SWITCH *

                      | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
-------------------------------------------------------
 P2 Level |  Normal   |off|off|                       |
          |   Weak    |on |off|                       |
          |  Strong   |off|on |                       |
          |Very strong|on |on |                       |
-------------------------------------------------------
 P2 Points|  Normal   |       |off|off|               |
          |  Easy     |       |on |off|               |
          |  Hard     |       |off|on |               |
          | Very hard |       |on |on |               |
-------------------------------------------------------
 P1       |  1000pts  |               |off|           |
 points   |  2000pts  |               |on |           |
-------------------------------------------------------
  Auto    |   Yes     |                   |off|       |
  tumo    |   No      |                   |on |       |
-------------------------------------------------------
  Not     |           |                       |off|   |
  Used    |           |                       |on |   |
-------------------------------------------------------
  Tumo    |   Long    |                           |off|
  time    |   Short   |                           |on |
-------------------------------------------------------

* at slotA -> DIP SW3
     slotB -> DIP SW4


*/

#include "driver.h"

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_BANK1)
ADDRESS_MAP_END

INPUT_PORTS_START( macs )
INPUT_PORTS_END

VIDEO_START(macs)
{
	return 0;
}

VIDEO_UPDATE(macs)
{

}

static MACHINE_DRIVER_START( macs )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? what processor ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(macs)
	MDRV_VIDEO_UPDATE(macs)
MACHINE_DRIVER_END

DRIVER_INIT ( macs )
{
	cpu_setbank( 1, memory_region( REGION_USER1 ) );
}

#define MACS_BIOS \
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) \
	ROM_REGION( 0x100000, REGION_USER1, 0 ) \
	ROM_LOAD16_BYTE( "macsos_l.u43", 0x00000, 0x80000, CRC(0b5aed5e) SHA1(042e705017ee34656e2c6af45825bb2dd3447747) ) \
	ROM_LOAD16_BYTE( "macsos_h.u44", 0x00001, 0x80000, CRC(538b68e4) SHA1(a0534147791e94e726f49451d0e95671ae0a87d5) ) \

ROM_START( macsbios )
	MACS_BIOS
	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

ROM_START( kisekaem )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "am-mj.u8", 0x000000, 0x100000, CRC(3cf85151) SHA1(e05400065c384730f04ef565db5ba27eb3973d15) )
	ROM_LOAD16_BYTE( "am-mj.u7", 0x000001, 0x100000, CRC(4b645354) SHA1(1dbf9141c3724e5dff2cd8066117fb1b94671a80) )
	ROM_LOAD16_BYTE( "am-mj.u6", 0x200000, 0x100000, CRC(23b3aa24) SHA1(bfabdb16f9b1b60230bb636a944ab46fdfda49d7) )
	ROM_LOAD16_BYTE( "am-mj.u5", 0x200001, 0x100000, CRC(b4d53e29) SHA1(d7683fdd5531bf1aa0ef1e4e6f517b31e2d5829e) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

ROM_START( kisekaeh )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "kh-u8.bin", 0x000000, 0x100000, CRC(601b9e6a) SHA1(54508a6db3928f78897df64ce400791e4789d0f6) )
	ROM_LOAD16_BYTE( "kh-u7.bin", 0x000001, 0x100000, CRC(8f6e4bb3) SHA1(361545189feeda0887f930727d25655309b84629) )
	ROM_LOAD16_BYTE( "kh-u6.bin", 0x200000, 0x100000, CRC(8e700204) SHA1(876e5530d749828de077293cb109a71b67cef140) )
	ROM_LOAD16_BYTE( "kh-u5.bin", 0x200001, 0x100000, CRC(709bf7c8) SHA1(0a93e0c4f9be22a3302a1c5d2a6ec4739b202ea8) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

ROM_START( cultname ) // uses printer
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "cult-d0.u8", 0x000000, 0x100000, CRC(394bc1a6) SHA1(98df5406862234815b46c7b0ac0b19e4b597d1b6) )
	ROM_LOAD16_BYTE( "cult-d1.u7", 0x000001, 0x100000, CRC(f628133b) SHA1(f06e20212074e5d95cc7d419ac8ce98fb9be3b62) )
	ROM_LOAD16_BYTE( "cult-d2.u6", 0x200000, 0x100000, CRC(c5521bc6) SHA1(7554b56b0201b7d81754defa2244fb7ff7452bf6) )
	ROM_LOAD16_BYTE( "cult-d3.u5", 0x200001, 0x100000, CRC(4325b09b) SHA1(45699a0444a221f893724754c917d33041cabcb9) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
	ROM_LOAD16_BYTE( "cult-g0.u8", 0x000000, 0x100000, CRC(f5ab977b) SHA1(e7ee758cc2864500b339e236b944f98df9a1c10e) )
	ROM_LOAD16_BYTE( "cult-g1.u7", 0x000001, 0x100000, CRC(32ae15a4) SHA1(061992efec1ed5527f200bf4c111344b156e759d) )
	ROM_LOAD16_BYTE( "cult-g2.u6", 0x200000, 0x100000, CRC(30ed056d) SHA1(71735339bb501b94402ef403b5a2a60effa39c36) )
	ROM_LOAD16_BYTE( "cult-g3.u5", 0x200001, 0x100000, CRC(fe58b418) SHA1(512f5c544cfafaa98bd2b3791ff1cf67adecec8d) )
ROM_END

/* these are listed as MACS2 sub-boards, is it the same? */

ROM_START( yuka )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "yu-ka_1.u8", 0x000000, 0x100000, CRC(bccd1b15) SHA1(02511f3be60c53b5f5d90f12f0648f6e184ca667) )
	ROM_LOAD16_BYTE( "yu-ka_3.u7", 0x000001, 0x100000, CRC(45b8263e) SHA1(59e1846c91dc39a086e8306260506673eb91de0b) )
	ROM_LOAD16_BYTE( "yu-ka_2.u6", 0x200000, 0x100000, CRC(c3c5728b) SHA1(e53cdcae556f34bab45d9342fd78ec29b6543c46) )
	ROM_LOAD16_BYTE( "yu-ka_4.u5", 0x200001, 0x100000, CRC(7e391ee6) SHA1(3a0c122c9d0e2a91df6d8039fb958b6d00997747) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

ROM_START( yujan )
	MACS_BIOS

	ROM_REGION( 0x400000, REGION_USER2, 0 ) // Slot A
	ROM_LOAD16_BYTE( "yu-jan_1.u8", 0x000000, 0x100000, CRC(feeeee6a) SHA1(e9613f50d6d2e62fac6b529f81486250cfe83819) )
	ROM_LOAD16_BYTE( "yu-jan_3.u7", 0x000001, 0x100000, CRC(1c1d6997) SHA1(9b07ae6b9ef1c0b57fbaa5fd0bcf1d2d7f17351f) )
	ROM_LOAD16_BYTE( "yu-jan_2.u6", 0x200000, 0x100000, CRC(2f4a8d4b) SHA1(4b328a253b1980a76f46a9a98a7f486813894a33) )
	ROM_LOAD16_BYTE( "yu-jan_4.u5", 0x200001, 0x100000, CRC(226df87b) SHA1(a887728f1ea2ef5f6b4dcd6b5b61586f5e8f267d) )

	ROM_REGION( 0x400000, REGION_USER3, 0 ) // Slot B
ROM_END

GAMEX( 1995, macsbios, 0,        macs, macs, macs, ROT0, "I'Max", "Multi Amenity Cassette System BIOS", NOT_A_DRIVER | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, kisekaem, macsbios, macs, macs, macs, ROT0, "I'Max", "Kisekae Mahjong", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1995, kisekaeh, macsbios, macs, macs, macs, ROT0, "I'Max", "Kisekae Hanafuda", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, cultname, macsbios, macs, macs, macs, ROT0, "I'Max", "Seimei-Kantei-Meimei-Ki Cult Name", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, yuka,     macsbios, macs, macs, macs, ROT0, "Yubis / T.System", "Yu-Ka", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, yujan,    macsbios, macs, macs, macs, ROT0, "Yubis / T.System", "Yu-Jan", GAME_NO_SOUND | GAME_NOT_WORKING )
