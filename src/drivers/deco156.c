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
extern void decrypt156(void);

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
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
ADDRESS_MAP_END


static MACHINE_INIT (deco156)
{
	/* code isn't decrypted so we don't want the cpu going anywhere */
//  cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
//  cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
}


static gfx_layout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static gfx_layout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static gfx_layout spritelayout =
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

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,     0, 16 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout, 0, 1},	/* Sprites (16x16) */

	{ -1 } /* end of array */
};

static gfx_decode gfxdecodeinfo_2[] =
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

	decrypt156();
}

DRIVER_INIT(deco156_2)
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);

	decrypt156();
}

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
	ROM_LOAD32_WORD( "lp00-2.2j",    0x000002, 0x080000, CRC(3f8fd724) SHA1(8efb27b96dbdc58715eb44c7846f30d485e1ded4) )
	ROM_LOAD32_WORD( "lp01-2.3j",    0x000000, 0x080000, CRC(a6fe282a) SHA1(10295b740ced35b3bb1f48ca3af2e985912405ec) )

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
	ROM_LOAD32_WORD( "pn00-0.2f",    0x000002, 0x080000, CRC(c9ed2006) SHA1(cee93eafc42c4de7a1453c85e7d6bca8d62cdc7b) )
	ROM_LOAD32_WORD( "pn01-0.4f",    0x000000, 0x080000, CRC(1c3641c3) SHA1(60dddc3585e4dedb485f7505fee03495f615c0c0) )

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
	ROM_LOAD32_WORD( "ra00-0.2j",    0x000002, 0x080000, CRC(790da069) SHA1(84fd90fb1833b97459cb337fdb92f7b6e93b5936) )
	ROM_LOAD32_WORD( "ra01-0.3j",    0x000000, 0x080000, CRC(447cb57b) SHA1(1d503b9cf1cadd3fdd7c9d6d59d4c40a59fa25ab))

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

ROM_START( backfira )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "rb-00h.h2",    0x000002, 0x080000, CRC(60973046) SHA1(e70d9be9cb172920da2a2ac9d317768b1438c59d) )
	ROM_LOAD32_WORD( "rb-01l.h3",    0x000000, 0x080000, CRC(27472f60) SHA1(d73b1e68dc51e28b1148db39ce22bd2e93f6fd0a) )

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

/* similar but different? */
GAMEX(1993, hvysmsh, 0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East", "Heavy Smash", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1993, wcvol95, 0,        deco156_1,      deco156, deco156_1,      ROT0, "Data East", "World Cup Volley '95", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)

/* more gfx chips */
GAMEX(1995, backfire, 0,        deco156_2,      deco156, deco156_2,      ROT0, "Data East", "Backfire!", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
GAMEX(1995, backfira, backfire, deco156_2,      deco156, deco156_2,      ROT0, "Data East", "Backfire! (set 2)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING)
