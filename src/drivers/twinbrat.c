/*

Twin Brats
Elettronica Video-Games S.R.L, 1995

PCB Layout
----------

|----------------------------------------------|
|  1.BIN                                       |
|      M6295  PAL                  6116   6116 |
|                                  6116   6116 |
|   62256                PAL                   |
|   62256                                      |
|                        6116                  |
|J  6116   |---------|   6116                  |
|A  6116   |ACTEL    |                         |
|M         |A1020A   |                         |
|M         |PL84C    |                         |
|A    PAL  |         |       30MHz       11.BIN|
|          |---------|6264   PAL         10.BIN|
|     62256    62256  6264   |---------| 9.BIN |
|     2.BIN    3.BIN         |ACTEL    | 8.BIN |
|   |-----------------|      |A1020A   | 7.BIN |
|   |   MC68000P12    |      |PL84C    | 6.BIN |
| * |                 | PAL  |         | 5.BIN |
|   |-----------------| PAL  |---------| 4.BIN |
| 93C46  14.7456MHz                            |
|----------------------------------------------|
Notes:
      68000 clock : 14.7456MHz
      M6295 clock : 0.9375MHz (30/32). Sample Rate = 937500 / 132
      VSync       : 58Hz
      *           : Push button test switch



--

E2 ROM ERROR is put in ram (eeprom error?)
clocks not corrected
gfx emulation not started
game is probably crap.

*/

#include "driver.h"
#include "machine/random.h"

READ16_HANDLER( twinbrat_random )
{
	return mame_rand();
}

static ADDRESS_MAP_START( twinbrat_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_READ(MRA16_RAM) // includes tilemap ram, main ram etc.
	AM_RANGE(0x400002, 0x400003) AM_READ(twinbrat_random)
	AM_RANGE(0x410000, 0x410001) AM_READ(twinbrat_random)
ADDRESS_MAP_END

static ADDRESS_MAP_START( twinbrat_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x400010, 0x400011) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x40001e, 0x40001f) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x410000, 0x410001) AM_WRITE(MWA16_NOP)
ADDRESS_MAP_END

INPUT_PORTS_START( twinbrat )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	// "Rotate" - also IPT_START1
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	// "Help"
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	// "Rotate" - also IPT_START2
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	// "Help"
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Helps" )			// "Power Count" in test mode
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0400, "1" )
	PORT_DIPNAME( 0x0800, 0x0800, "Bonus Bar Level" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( High ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Picture View" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

static struct GfxLayout twinbrat_tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3},
	{ 4,0,12,8,20,16,28,24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout twinbrat_sprite_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{  RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 135,134,133,132,131,130,129,128,7,6,5,4,3,2,1,0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	16*16
};




static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &twinbrat_tile_layout,   0x0, 2  },
	{ REGION_GFX2, 0, &twinbrat_sprite_layout,  0x0, 2  },

	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};


VIDEO_START(twinbrat)
{
	return 0;
}

VIDEO_UPDATE(twinbrat)
{

}

static MACHINE_DRIVER_START( twinbrat )
	MDRV_CPU_ADD_TAG("main", M68000, 14318180 /2)	 // or 10mhz? ?
	MDRV_CPU_PROGRAM_MAP(twinbrat_readmem,twinbrat_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(twinbrat)
	MDRV_VIDEO_UPDATE(twinbrat)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


ROM_START( twinbrat )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "3.bin", 0x00001, 0x20000, CRC(0e3fa9b0) SHA1(0148cc616eac84dc16415e1557ec6040d14392d4) )
	ROM_LOAD16_BYTE( "2.bin", 0x00000, 0x20000, CRC(5e75f568) SHA1(f42d2a73d737e6b01dd049eea2a10fc8c8096d8f) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "1.bin", 0x00000, 0x80000, CRC(76296578) SHA1(04eca78abe60b283269464c0d12815579126ac08) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "5.bin", 0x000000, 0x80000, CRC(cf235eeb) SHA1(d067e2dd4f28a8986dd76ec0eba90e1adbf5787c) )
	ROM_LOAD16_BYTE( "4.bin", 0x000001, 0x80000, CRC(1ae8a751) SHA1(5f30306580c6ab4af0ddbdc4519eb4e0ab9bd23a) )
	ROM_LOAD16_BYTE( "7.bin", 0x100000, 0x80000, CRC(3696345a) SHA1(ea38be3586757527b2a1aad2e22b83937f8602da) )
	ROM_LOAD16_BYTE( "6.bin", 0x100001, 0x80000, CRC(af10ddfd) SHA1(e5e83044f20d6cbbc1b4ef1812ac57b6dc958a8a) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD( "9.bin",  0x000000, 0x40000, CRC(13194d89) SHA1(95c35b6012f98a64630abb40fd55b24ff8a5e031) )
	ROM_LOAD( "8.bin",  0x040000, 0x40000, CRC(79f14528) SHA1(9c07d9a9e59f69a525bbaec05d74eb8d21bb9563) )
	ROM_LOAD( "11.bin", 0x080000, 0x40000, CRC(00eecb03) SHA1(5913da4d2ad97c1ce5e8e601a22b499cd93af744) )
	ROM_LOAD( "10.bin", 0x0c0000, 0x40000, CRC(7556bee9) SHA1(3fe99c7e9378791b79c43b04f5d0a36404448beb) )
ROM_END

GAMEX( 1995, twinbrat, 0, twinbrat, twinbrat, 0, ROT0,  "Elettronica Video-Games S.R.L,", "Twin Brats", GAME_NOT_WORKING )
