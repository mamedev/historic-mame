/* Data East Hardware using Custom Chip 156
   placeholder driver

  all games on this driver have encrypted code,
  they may also have additional protection but
  at this stage that is unknown.  the current
  hold-up is the encryption.

  They use Data East Custom Chip 'DE156' which is
  probably the main processor. It might be an
  encrypted ARM7 based cpu

  This driver may be split up when (if) the actual
  code is decrypted as the games are not on identical
  hardware

  see deco32.c and deco_mlc.c for more games using
  DE156

*/

#define DE156CPU ARM

#include "driver.h"
#include "decocrpt.h"

/* dummy vidhrdw */
VIDEO_START(deco156){return 0;}
VIDEO_UPDATE(deco156){}

INPUT_PORTS_START( deco156 )
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

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static ADDRESS_MAP_START( deco156_map, ADDRESS_SPACE_PROGRAM, 32 )
ADDRESS_MAP_END


static MACHINE_INIT (deco156)
{
	/* code isn't decrypted so we don't want the cpu going anywhere */
	cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
	cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
}


static struct GfxLayout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(0,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static struct GfxLayout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(0,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+8 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_2[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX2, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX3, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */
	{ REGION_GFX4, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( deco156_1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(DE156CPU, 7000000)	/* DE156 */
	MDRV_CPU_PROGRAM_MAP(deco156_map,0)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_MACHINE_INIT (deco156)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(deco156)
	MDRV_VIDEO_UPDATE(deco156)

	/* sound hardware */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( deco156_2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(DE156CPU, 7000000)	/* DE156 */
	MDRV_CPU_PROGRAM_MAP(deco156_map,0)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_MACHINE_INIT (deco156)
	MDRV_GFXDECODE(gfxdecodeinfo_2)

	MDRV_VIDEO_START(deco156)
	MDRV_VIDEO_UPDATE(deco156)

	/* sound hardware */
MACHINE_DRIVER_END

DRIVER_INIT(deco156_1)
{
	deco56_decrypt(REGION_GFX1);
	//deco156_decrypt(REGION_CPU1);
}

DRIVER_INIT(deco156_2)
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);

	//deco156_decrypt(REGION_CPU1);
}
/*
Osman
Mitchell Corp., 1996

This game runs on Data East hardware. The game is very
similar to Strider.

PCB Layout
----------

DEC-22VO
MT5601-0
------------------------------------------------------------
|                 MCF04.14H     MCF-03.14D     MCF-02.14A  |
|                                                          |
|     OKI M6295   SA01-0.13H            52     MCF-01.13A  |
|                                                          |
--|   OKI M6295                                            |
  |                                                        |
--|                               CY7C185-25   MCF-00.9A   |
|                                 CY7C185-25               |
|                                                          |
|                                                          |
|J                     GAL16V8           28.000MHz         |
|A                  CY7C185-25                             |
|M                  CY7C185-25     141                     |
|M                                                         |
|A          223                                            |
|                                             GAL16V8      |
|                                GAL16V8                   |
|                             CY7C185-25   223             |
--|                93C45.2H   CY7C185-25                   |
  |                                                  156   |
--| TESTSW                  SA00-0.1E                      |
|                                                          |
------------------------------------------------------------

*/

ROM_START( osman )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "sa00-0.1e",    0x000000, 0x080000, CRC(ec6b3257) SHA1(10a42a680ce122ab030eaa2ccd99d302cb77854e) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcf-00.9a",    0x000000, 0x080000, CRC(247712dc) SHA1(bcb765afd7e756b68131c97c30d210de115d6b50) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcf-02.14a",    0x000000, 0x200000, CRC(21251b33) SHA1(d252fe5c6eef8cbc9327e4176b4868b1cb17a738) )
	ROM_LOAD16_BYTE( "mcf-04.14h",    0x000001, 0x200000, CRC(4fa55577) SHA1(e229ba9cce46b92ce255aa33b974e19b214c4017) )
	ROM_LOAD16_BYTE( "mcf-01.13a",    0x400000, 0x200000, CRC(83881e25) SHA1(ae82cf0f704e6efea94c6c1d276d4e3e5b3ebe43) )
	ROM_LOAD16_BYTE( "mcf-03.14d",    0x400001, 0x200000, CRC(faf1d51d) SHA1(675dbbfe15b8010d54b2b3af26d42cdd753c2ce2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "sa01-0.13h",    0x00000, 0x40000,  CRC(cea8368e) SHA1(1fcc641381fdc29bd50d3a4b23e67647f79e505a))

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcf-05.12f",    0x00000, 0x200000, CRC(f007d376) SHA1(4ba20e5dabeacc3278b7f30c4462864cbe8f6984) )
ROM_END

/*

Chain Reaction
Data East 1995

DE-0409-1

156           E1
       223    2063     93C45
              2063
                              223
               141
       28MHz         5864
                     5864
            6264
            6264
  MCC-00

  U5  U6     52      MCC-03   AD-65
  U3  U4             MCC-04   AD-65


*/

ROM_START( chainrec )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "e1",    0x000000, 0x080000, CRC(8a8340ef) SHA1(4aaee56127b73453b862ff2a33dc241eeabf5658) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "u3",    0x000000, 0x080000, CRC(92659721) SHA1(b446ce98ec9c2c16375ef00639cfb463b365b8f7) )
	ROM_LOAD32_BYTE( "u4",    0x000001, 0x080000, CRC(e304eb32) SHA1(61a647ec89695a6b25ff924bdc6d29cbd7aca82b) )
	ROM_LOAD32_BYTE( "u5",    0x000002, 0x080000, CRC(1b6f01ea) SHA1(753fc670707432e317d035b09b0bad0762fea731) )
	ROM_LOAD32_BYTE( "u6",    0x000003, 0x080000, CRC(531a56f2) SHA1(89602bb873a3b110bffc216f921ba228e53380f9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )
ROM_END

ROM_START( magdrop )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "re00-2.e1",    0x000000, 0x080000,  CRC(7138f10f) SHA1(ca93c3c2dc9a7dd6901c8429a6bf6883076a9b8f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcc-01.a13",    0x000000, 0x100000, CRC(13d88745) SHA1(0ce4ec1481f31be860ee80322de6e32f9a566229) )
	ROM_LOAD16_BYTE( "mcc-02.a14",    0x000001, 0x100000, CRC(d0f97126) SHA1(3848a6f00d0e57aaf383298c4d111eb63a88b073) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "93c45.h2",    0x00, 0x80, CRC(16ce8d2d) SHA1(1a6883c75d34febbd92a16cfe204ff12550c85fd) )
ROM_END

ROM_START( magdropp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "rz00-1.e1",    0x000000, 0x080000,  CRC(28caf639) SHA1(a17e792c82e65009e21680094acf093c0c4f1021) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcc-00",    0x000000, 0x100000, CRC(646b03ec) SHA1(9a2fc11b1575032b5a784d88c3a90913068d1e69) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcc-01.a13",    0x000000, 0x100000, CRC(13d88745) SHA1(0ce4ec1481f31be860ee80322de6e32f9a566229) )
	ROM_LOAD16_BYTE( "mcc-02.a14",    0x000001, 0x100000, CRC(d0f97126) SHA1(3848a6f00d0e57aaf383298c4d111eb63a88b073) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mcc-04",    0x00000, 0x40000,  CRC(86ee6ade) SHA1(56ad3f432c7f430f19fcba7c89940c63da165906) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcc-03",    0x00000, 0x100000, CRC(da2ebba0) SHA1(96d31dea4c7226ee1d386b286919fa334388c7a1) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "eeprom.2h",    0x00, 0x80, CRC(d13d9edd) SHA1(e98ee2b696f0a7a8752dc30ef8b41bfe6598cbe4) )
ROM_END

/* Charlie Ninja

Charlie Ninja by Mitchell

rom E1  27c4002  labeled "ND 00-1"
rom H13  27c2001  labeled "ND 01-0"

maskrom b14  27c800  labeled "MBR-01"
maskrom b9  27c160  labeled "MBR-03"
maskrom h12  27c800  labeled "MBR-02"
maskrom h14  27c160  labeled "MBR-00"

28.8 mHz
oki m6295

*/

ROM_START( charlien )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "c-ninja.e1",    0x000000, 0x080000,  CRC(f18f4b23) SHA1(cb0c159b4dde3a3c5f295f270485996811e5e4d2) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c-ninja.b9",    0x000000, 0x080000, CRC(ecf2c7f0) SHA1(3c735a4eef2bc49f16ac9365a5689101f43c13e9) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "c-ninja.b14",    0x000001, 0x100000, CRC(46c90215) SHA1(152acdeea34ec1db3f761066a0c1ff6e43e47f9d) )
	/* i think we're missing a rom .. c-ninja.b9 was in here twice .. */
	ROM_LOAD16_BYTE( "c-ninja.gfx",    0x000000, 0x100000, NO_DUMP )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "c-ninja.h13",    0x00000, 0x40000,  CRC(635a100a) SHA1(f6ec70890892e7557097ccd519de37247bb8c98d) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "c-ninja.h12",    0x00000, 0x100000, CRC(4f67d333) SHA1(608f921bfa6b7020c0ce72e5229b3f1489208b23) )
ROM_END

/* Joe and Mac Returns

Joe and Mac Returns
Data East 1994

DE-0491-1

156         MW00
      223              223
             141

  MBN00

  MBN01   52   MBN03  M6295
  MBN02        MBN04  M6295

*/

ROM_START( joemacr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "mw00",    0x000000, 0x080000,  CRC(e1b78f40) SHA1(e611c317ada5a049a5e05d69c051e22a43fa2845) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) // this looks bad, could probably be fixed with other set
	ROM_LOAD( "mbn00",    0x000000, 0x100000,  BAD_DUMP CRC(383a70dd) SHA1(d8f57b1ed6ffc4483c739c3450a8144accdc84b9) ) // both halves identical (bad?)

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbn01",    0x000000, 0x080000, CRC(a3a37353) SHA1(c4509c8268afb647c20e71b42ae8ebd2bdf075e6) )
	ROM_LOAD16_BYTE( "mbn02",    0x000001, 0x080000,  CRC(aa2230c5) SHA1(43b7ac5c69cde1840a5255a8897e1c5d5f89fd7b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */  // this could be bad, could probably be fixed with other set
	ROM_LOAD( "mbn04",    0x00000, 0x80000,  CRC(a4226947) SHA1(c6f7aae41bddd474b83916003d1e16ba0f1fe52c) ) // both halves identical (bad?)

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mbn03",    0x00000, 0x200000, CRC(70b71a2a) SHA1(45851b0692de73016fc9b913316001af4690534c) )
ROM_END

/*
This is a bootleg board, dumped by Marco_A
there was a 42-pin rom which isn't dumped, i imagine its sound..

there is a rom which still looks to be encrypted, i guess its the cpu code
*/

ROM_START( joemacra )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "05.rom",    0x000000, 0x080000,  CRC(74e9a158) SHA1(eee447303ac0884e152b89f59a9694afade87336) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "01.rom",    0x000000, 0x080000,  CRC(636f25f1) SHA1(9dd09dd1fe90137a997121ba832a22e547d246d0) )
	ROM_LOAD( "02.rom",    0x080000, 0x080000,  CRC(642c08db) SHA1(9a541fd56ae34c24f803e08869702be6fafd81d1) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbn01",    0x000000, 0x080000, CRC(a3a37353) SHA1(c4509c8268afb647c20e71b42ae8ebd2bdf075e6) ) // 03.bin
	ROM_LOAD16_BYTE( "mbn02",    0x000001, 0x080000,  CRC(aa2230c5) SHA1(43b7ac5c69cde1840a5255a8897e1c5d5f89fd7b) ) // 04.bin

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "07.rom",    0x00000, 0x40000,  CRC(dcbd4771) SHA1(2a1ab6b0fc372333c7eb17aab077fe1ca5ba1dea) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) not in this set */
	ROM_LOAD( "mbn03",    0x00000, 0x200000, CRC(70b71a2a) SHA1(45851b0692de73016fc9b913316001af4690534c) )
ROM_END

/*

Ganbare! Gonta!! 2 (Lady Killer Part 2 - Party Time)
(c)1995 Mitchell
DEC-22V0 (board is manufactured by DECO)
MT5601-0

CPU  : surface-scratched 100pin PQFP DECO custom?
Sound: M6295x2
OSC  : 28.0000MHz

ROMs:
rd_00-0.1e - Main program (27c4096)

mcb-00.9a  - Graphics? (23c16000)

mcb-01.13a - Graphics (23c16000)
mcb-02.14a |
mcb-03.14d |
mcb-05.14h /

rd_01-0.13h - Samples (27c020)

mcb-04.12f - Samples (23c16000)

GALs (16V8B, not dumped):
vz-00.5c
vz-01.4e
vz-02.8f

Custom chips:
Surface-scratched 128pin PQFP (location 12d) DECO 52?
Surface-scratched 100pin PQFP (location 5j)
Surface-scratched 160pin PQFP (location 6e) DECO 56?
Surface-scratched 100pin PQFP (location 2a)
Surface-scratched 100pin PQFP (location 3d)

Other:
EEPROM 93C45

*/

ROM_START( gangonta )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD( "rd_00-0.1e",    0x000000, 0x080000, CRC(f80f43bb) SHA1(f9d26829eb90d41a6c410d4d673fe9595f814868) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcb-00.9a",    0x000000, 0x080000, CRC(c48a4f2b) SHA1(2dee5f8507b2a7e6f7e44b14f9abca36d0ebf78b) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mcb-02.14a",    0x000000, 0x200000, CRC(423cfb38) SHA1(b8c772a8ab471c365a11a88c85e1c8c7d2ad6e80) )
	ROM_LOAD16_BYTE( "mcb-05.14h",    0x000001, 0x200000, CRC(81540cfb) SHA1(6f7bc62c3c4d4a29eb1e0cfb261ace461bbca57c) )
	ROM_LOAD16_BYTE( "mcb-01.13a",    0x400000, 0x200000, CRC(06f40a57) SHA1(896f1d373e911dcff7223bf21756ad35b28b4c5d) )
	ROM_LOAD16_BYTE( "mcb-03.14d",    0x400001, 0x200000, CRC(0aef73af) SHA1(76cf13f53da5202da80820f98660edee1eef7f1a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "rd_01-0.13h",    0x00000, 0x40000,  CRC(70fd18c6) SHA1(368cd8e10c5f5a13eb3813974a7e6b46a4fa6b6c) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mcb-04.12f",    0x00000, 0x200000, CRC(e23d3590) SHA1(dc8418edc525f56e84f26e9334d5576000b14e5f) )
ROM_END

/*

Heavy Smash
Data East, 1993

PCB Layout

DE-0385-2  DEC-22VO
|----------------------------------------------|
|                      28MHz  DE52             |
|           MBG-04.13J               MBG-02.11A|
|  M6295(1) MBG-03.10K  VL-02        MBG-01.10A|
|  M6295(2)                   DE153  MBG-00.9A |
|                                              |
|                                              |
|J                                             |
|A               93C46.8K                      |
|M                                             |
|M                            DE141            |
|A                                VL-01 VL-00  |
|                  6264                        |
|      DE153       6264                        |
|                                              |
|                                              |
|TEST_SW                                 DE156 |
|           LP01-2.3J  6264   6264             |
|           LP00-2.2J  6264   6264             |
|                                              |
|----------------------------------------------|

Notes:
      - CPU is unknown. It's chip DE156. The clock input is 7.000MHz on pin 90
        It's thought to be a custom-made encrypted ARM7-based CPU.
        The package is a Quad Flat Pack and has 100 pins.
      - OKI M6295(1) clock: 1.000MHz (28 / 28), sample rate = 1000000 / 132
      - OKI M6295(2) clock: 2.000MHz (28 / 14), sample rate = 2000000 / 132
      - VSync: 58Hz
      - VL-00 (PAL16R8), VL-01 (PAL16L8), VL-02 (PAL16R6)
      - On the Data East boards of this type (using DE156) that use an EEPROM, the EEPROM contains the
        country/region code also. It has been proven by comparing the dumps of Osman and Cannon Dancer....
        they were identical and there are no region jumper pads on the PCB. Therefore the EEPROM must
        hold the region code.

      ROMs
      ----
      - MBG-00, MBG-01, MBG-02  - 16M MASK  Graphics
      - LP00, LP01              - 27C4096   Main program
      - MBG-03                  - 4M MASK   Sound (samples, linked to M6295(1)
      - MBG-04                  - 16M MASK  Sound (samples, linked to M6295(2)
      - 93C46                   - 128 bytes EEPROM (Note! this chip has identical halves and fixed
                                                    bits, but the dump is correct!)

*/

ROM_START( hvysmsh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD16_BYTE( "lp00-2.2j",    0x000000, 0x080000, CRC(3f8fd724) SHA1(8efb27b96dbdc58715eb44c7846f30d485e1ded4) )
	ROM_LOAD16_BYTE( "lp01-2.3j",    0x000001, 0x080000, CRC(a6fe282a) SHA1(10295b740ced35b3bb1f48ca3af2e985912405ec) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbg-00.9a",    0x000000, 0x080000, CRC(7d94eb16) SHA1(31cf5302eba37e935865822aebd76c700bc51eaf) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbg-01.10a",    0x000000, 0x200000, CRC(bcd7fb29) SHA1(a54a813b5adcb4df0bfdd58285b1f8e17fbbb7a2) )
	ROM_LOAD16_BYTE( "mbg-02.11a",    0x000001, 0x200000, CRC(0cc16440) SHA1(1cbf620a9d875ec87dd28a97a256584b6ef277cd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-03.10k",    0x00000, 0x80000,  CRC(4b809420) SHA1(ad0278745002320804a31af0b772f9ab5f075027) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples? (banked?) */
	ROM_LOAD( "mbg-04.11k",    0x00000, 0x200000, CRC(2281c758) SHA1(934691b4002ecd6bc9a09b8970ff18a09451d492) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "93c46.8k",    0x00, 0x80, CRC(d31fbd5b) SHA1(bf044408c637f6b39afd30ccb86af183ec0acc02) )
ROM_END

/*
World Cup Volley '95
Data East, 1995

PCB Layout

DE-0430-2
|----------------------------------------------|
|          MBX-03.13J                MBX-02.13A|
|       LC7881         28MHz   DE52            |
|             YMZ280B-F              MBX-01.12A|
|                         CY7C185              |
|                         CY7C185    MBX-00.9A |
|             WE-02                            |
|J                                             |
|A                                             |
|M                 6264                        |
|M                             DE141           |
|A     DE223       6264                        |
|                                              |
|                                       WE-00  |
|                              WE-01           |
|                                              |
|TEST_SW           PN01-0.4F   6264            |
|         93C46.3K             6264            |
|                  PN00-0.2F   6264     DE156  |
|                              6264            |
|----------------------------------------------|

Notes:
      - CPU is unknown. It's chip DE156. The clock input is 7.000MHz on pin 90
        It's thought to be a custom-made encrypted ARM7-based CPU.
        The package is a Quad Flat Pack and has 100 pins.
      - YMZ280B-F clock: 14.000MHz (28 / 2)
        SANYO LC7881 clock: 2.3333MHz (28 / 12)
      - VSync: 58Hz
      - WE-00, WE-01 and WE-02 are PALs type GAL16V8

*/

ROM_START( wcvol95 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD16_BYTE( "pn00-0.2f",    0x000000, 0x080000, CRC(c9ed2006) SHA1(cee93eafc42c4de7a1453c85e7d6bca8d62cdc7b) )
	ROM_LOAD16_BYTE( "pn01-0.4f",    0x000001, 0x080000, CRC(1c3641c3) SHA1(60dddc3585e4dedb485f7505fee03495f615c0c0) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbx-00.9a",    0x000000, 0x080000, CRC(a0b24204) SHA1(cec8089c6c635f23b3a4aeeef2c43f519568ad70) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbx-01.12a",    0x000000, 0x100000, CRC(73deb3f1) SHA1(c0cabecfd88695afe0f27c5bb115b4973907207d) )
	ROM_LOAD16_BYTE( "mbx-02.13a",    0x000001, 0x100000, CRC(3204d324) SHA1(44102f71bae44bf3a9bd2de7e5791d959a2c9bdd) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* YMZ280B-F samples */
	ROM_LOAD( "mbx-03.13j",    0x00000, 0x200000,  CRC(061632bc) SHA1(7900ac56e59f4a4e5768ce72f4a4b7c5875f5ae8) )

	ROM_REGION( 0x80, REGION_USER1, 0 ) /* eeprom */
	ROM_LOAD( "93c46.3k",    0x00, 0x80, CRC(88f8e270) SHA1(cb82203ad38e0c12ea998562b7b785979726afe5) )
ROM_END

/*

Backfire!
Data East, 1995

This game is similar to World Rally, Blomby Car, Drift Out'94 etc


PCB Layout
----------


DE-0432-2
---------------------------------------------------------------------
|              MBZ-06.19L     28.000MHz                MBZ-04.19A * |
|                                           52                      |
|                              153                     MBZ-03.18A + |
|              MBZ-05.17L                                           |
|                                                                   |
--|        LC7881  YMZ280B-F   153          52         MBZ-04.16A * |
  |                                                                 |
--|                                                    MBZ-03.15A + |
|                     CY7C185 (x2)                                  |
|J                                     141                          |
|                                                      MBZ-02.12A   |
|A                                                                  |
|                                                      MBZ-01.10A   |
|M       223                                                        |
|                                                      MBZ-00.9A    |
|M         93C45.8M   CY7C185 (x2)     141                          |
|                                                                   |
|A                                                                  |
|                                                                   |
--|                                                                 |
  |                                                                 |
--|        TSW1                                                     |
|                                          CY7C185 (x4)             |
|                                                           156     |
|                 ADC0808       RA01-0.3J                           |
|                               RA00-0.2J                           |
|CONN2      CONN1    D4701                                          |
|                                                                   |
---------------------------------------------------------------------


Notes:
CONN1 & CONN2: For connection of potentiometer or opto steering wheel.
               Joystick (via JAMMA) can also be used for controls.
TSW1: Push Button TEST switch to access options menu (coins/lives etc).
*   : These ROMs have identical contents AND identical halves.
+   : These ROMs have identical contents AND identical halves.

*/

ROM_START( backfire )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD16_BYTE( "ra00-0.2j",    0x000000, 0x080000, CRC(790da069) SHA1(84fd90fb1833b97459cb337fdb92f7b6e93b5936) )
	ROM_LOAD16_BYTE( "ra01-0.3j",    0x000001, 0x080000, CRC(447cb57b) SHA1(1d503b9cf1cadd3fdd7c9d6d59d4c40a59fa25ab))

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* Tiles 1 */
	ROM_LOAD( "mbz-00.9a",    0x000000, 0x080000, CRC(1098d504) SHA1(1fecd26b92faffce0b59a8a9646bfd457c17c87c) )
	ROM_CONTINUE( 0x200000, 0x080000)
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x300000, 0x080000)
	ROM_LOAD( "mbz-01.10a",    0x080000, 0x080000, CRC(19b81e5c) SHA1(4c8204a6a4ad30b23fbfdd79c6e39581e23de6ae) )
	ROM_CONTINUE( 0x280000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)
	ROM_CONTINUE( 0x380000, 0x080000)

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles 2 */
	ROM_LOAD( "mbz-02.12a",    0x000000, 0x100000, CRC(2bd2b0a1) SHA1(8fcb37728f3248ad55e48f2d398b014b36c9ec05) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbz-03.15a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD16_BYTE( "mbz-04.16a",    0x000001, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbz-03.18a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD16_BYTE( "mbz-04.19a",    0x000001, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* samples 1 */
	ROM_LOAD( "mbz-06.19l",    0x00000, 0x080000,  CRC(4a38c635) SHA1(7f0fb6a7a4aa6774c04fa38e53ceff8744fe1e9f) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* samples 2 */
	ROM_LOAD( "mbz-05.17l",    0x00000, 0x200000,  CRC(947c1da6) SHA1(ac36006e04dc5e3990f76539763cc76facd08376) )
ROM_END

/* these are probably all basically the same */
GAMEX(1996, osman,   0,        deco156_1,      deco156, deco156_1,      ROT0, "Mitchell", "Osman", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, chainrec,0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East","Chain Reaction", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, magdrop, chainrec, deco156_1,      deco156, deco156_1,      ROT0, "Data East","Magical Drop", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, magdropp,chainrec, deco156_1,      deco156, deco156_1,      ROT0, "Data East","Magical Drop Plus", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, charlien,0,        deco156_1,      deco156, deco156_1,      ROT0, "Mitchell", "Charlie Ninja", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1994, joemacr, 0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East", "Joe & Mac Returns", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1994, joemacra,joemacr,  deco156_1,      deco156, deco156_1,      ROT0, "Data East", "Joe & Mac Returns (set 2)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, gangonta,0,        deco156_1,      deco156, deco156_1,      ROT90,"Mitchell", "Ganbare! Gonta!! 2", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)

/* similar but different? */
GAMEX(1993, hvysmsh, 0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East", "Heavy Smash", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1993, wcvol95, 0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East", "World Cup Volley '95", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)

/* more gfx chips */
GAMEX(1995, backfire, 0,        deco156_2,      deco156, deco156_2,      ROT0, "Data East", "Backfire!", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
