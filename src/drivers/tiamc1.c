/*****************************************************************************

  TIA-MC1 driver

  driver by Eugene Sandulenko
  special thanks to Shiru for his standalone emulator and documentation

  Games supported:
      * Konek-Gorbunok

 ***************************************************************

  To enter test mode hold F2 on emulator startup
  Also use F2 to skip screens during the gameplay (factory-made cheat)

 ***************************************************************

  This is one of last Soviet-made arcades. Several games are known on this
  platform. It was created by government company Terminal (Vinnitsa, Ukraine),
  which later was turned into EXTREMA-Ukraine company which manufactures
  fruit mashines these days (http://www.extrema-ua.com).

 ***************************************************************

  TIA-MC1 arcade internals

  This arcade consists of four boards:

    BEIA-100
      Main CPU board. Also contains address PROM, color DAC and input ports

    BEIA-101
      Video board 1. Background generator, video tiles RAM and video sync schematics

    BEIA-102
      Video board 2. Sprite generator, video buffer RAM (hardware triple buffering)

    BEIA-103
      ROM banks, RAM. Contains ROMs and multiplexors. Interchangeable between games

  TODO
  o Sound

*/

#include "driver.h"

extern PALETTE_INIT( tiamc1 );
extern VIDEO_START( tiamc1 );
extern VIDEO_UPDATE( tiamc1 );

extern WRITE8_HANDLER( tiamc1_palette_w );
extern WRITE8_HANDLER( tiamc1_videoram_w );
extern WRITE8_HANDLER( tiamc1_bankswitch_w );
extern WRITE8_HANDLER( tiamc1_sprite_x_w );
extern WRITE8_HANDLER( tiamc1_sprite_y_w );
extern WRITE8_HANDLER( tiamc1_sprite_a_w );
extern WRITE8_HANDLER( tiamc1_sprite_n_w );

extern UINT8 *tiamc1_charram, *tiamc1_tileram;
extern UINT8 *tiamc1_spriteram_x, *tiamc1_spriteram_y, *tiamc1_spriteram_n,
	*tiamc1_spriteram_a;

MACHINE_RESET( tiamc1 )
{
        UINT8 *RAM = memory_region(REGION_CPU1) + 0x10000;

        tiamc1_charram = RAM + 0x0800;     // Ram is banked
        tiamc1_tileram = RAM + 0x0000;

	tiamc1_spriteram_y = RAM + 0x3000;
	tiamc1_spriteram_x = RAM + 0x3010;
	tiamc1_spriteram_n = RAM + 0x3020;
	tiamc1_spriteram_a = RAM + 0x3030;

	tiamc1_bankswitch_w(0, 0);
}

static ADDRESS_MAP_START( tiamc1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xb000, 0xb7ff) AM_WRITE(tiamc1_videoram_w)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( tiamc1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x4f) AM_WRITE(tiamc1_sprite_y_w) /* sprites Y */
	AM_RANGE(0x50, 0x5f) AM_WRITE(tiamc1_sprite_x_w) /* sprites X */
	AM_RANGE(0x60, 0x6f) AM_WRITE(tiamc1_sprite_n_w) /* sprites # */
	AM_RANGE(0x70, 0x7f) AM_WRITE(tiamc1_sprite_a_w) /* sprites attributes */
	AM_RANGE(0xa0, 0xaf) AM_WRITE(tiamc1_palette_w) /* color ram */
	AM_RANGE(0xbc, 0xbc) AM_WRITENOP /* background H scroll */
	AM_RANGE(0xbd, 0xbd) AM_WRITENOP /* background V scroll */
	AM_RANGE(0xbe, 0xbe) AM_WRITE(tiamc1_bankswitch_w) /* VRAM selector */
	AM_RANGE(0xbf, 0xbf) AM_WRITENOP /* charset control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xd0, 0xd0) AM_READ(input_port_0_r)
	AM_RANGE(0xd1, 0xd1) AM_READ(input_port_1_r)
	AM_RANGE(0xd2, 0xd2) AM_READ(input_port_2_r)
	AM_RANGE(0xb3, 0xb3) AM_READNOP /* i/o port 8355 */
	AM_RANGE(0xb4, 0xb6) AM_READNOP /* timer 1 */
	AM_RANGE(0xb7, 0xb7) AM_READNOP /* timer 1 control word */
	AM_RANGE(0xbb, 0xbb) AM_READNOP /* sound */
	AM_RANGE(0xc0, 0xc2) AM_READNOP /* timer 2 */
	AM_RANGE(0xc3, 0xc3) AM_READNOP /* timer 2 control word */
ADDRESS_MAP_END

INPUT_PORTS_START( tiamc1 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static const gfx_layout sprites16x16_layout =
{
	16,16,
	256,
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 256*16*8+0, 256*16*8+1, 256*16*8+2, 256*16*8+3, 256*16*8+4, 256*16*8+5, 256*16*8+6, 256*16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static const gfx_layout char_layout =
{
	8,8,
	256,
	4,
	{ 256*8*8*3, 256*8*8*2, 256*8*8*1, 256*8*8*0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x10800, &char_layout, 0, 16 },
	{ REGION_GFX1, 0x0000, &sprites16x16_layout, 0, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( tiamc1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080,15750000/9)		 /* 1.575 MHz */
	MDRV_CPU_PROGRAM_MAP(tiamc1_readmem,tiamc1_writemem)
	MDRV_CPU_IO_MAP(tiamc1_readport,tiamc1_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(128) // (??)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_MACHINE_RESET(tiamc1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_PALETTE_INIT(tiamc1)
	MDRV_VIDEO_START(tiamc1)
	MDRV_VIDEO_UPDATE(tiamc1)
MACHINE_DRIVER_END


ROM_START( konek )
	ROM_REGION( 0x13040, REGION_CPU1, 0 )
	ROM_LOAD( "g1.d17", 0x00000, 0x2000, CRC(f41d82c9) SHA1(63ac1be2ad58af0e5ef2d33e5c8d790769d80af9) )
	ROM_LOAD( "g2.d17", 0x02000, 0x2000, CRC(b44e7491) SHA1(ff4cb1d76a36f504d670a207ee25556c5faad435) )
	ROM_LOAD( "g3.d17", 0x04000, 0x2000, CRC(91301282) SHA1(cb448a1bb7a9c1768f870a8c062e37807431c9c7) )
	ROM_LOAD( "g4.d17", 0x06000, 0x2000, CRC(3ff0c20b) SHA1(3d999c05b3986149e569630779ed5581fc202842) )
	ROM_LOAD( "g5.d17", 0x08000, 0x2000, CRC(e3196d30) SHA1(a03d9f75926be9fcf5ee05df8b00fbf87361ea5b) )
	ROM_FILL( 0xa000, 0x2000, 0x00 )
	ROM_LOAD( "g7.d17", 0x0c000, 0x2000, CRC(fe4e9fdd) SHA1(2033585a6c53455d1dafee85cbb807d424ed231d) )
	ROM_FILL( 0x10000, 0x2800, 0x00 ) // Banked tilemap and charmap
	ROM_FILL( 0x13000, 0x0040, 0x00 ) // Sprites data

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "a2.b07", 0x00000, 0x2000, CRC(9eed06ee) SHA1(1b64a3f8fe3df4b4870315dbdf69bf60b1c272d0) )
	ROM_LOAD( "a3.g07", 0x02000, 0x2000, CRC(eeff9b77) SHA1(5dc66292a59f24277a8c2f38158a2e1d58f81338) )
	ROM_LOAD( "a5.l07", 0x04000, 0x2000, CRC(fff9e089) SHA1(f0d64dceaf72da785d55316bf8a7433faa09fabb) )
	ROM_LOAD( "a6.r07", 0x06000, 0x2000, CRC(092e8ee2) SHA1(6c4842e992c592b9f0663e039668f61a7b56700f) )

ROM_END

GAME( 1988, konek, 0, tiamc1, tiamc1, 0, ROT0, "Terminal", "Konek-Gorbunok", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
