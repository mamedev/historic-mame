/* Sang Ho Soft 'Puzzle Star' PCB */

/*

 This driver is being worked on by the original author,
 it would be best suited to somebody with an understanding
 of the MSX system, and a possibly a working PCB to analyse

 It is important that somebody looks at this before all the
 PCBs are dead to establish if we need anything additional
 from the boards which can only be retrived while they are
 in working condition.

 */

/*

Each board contains a custom FGPA on a sub-board with
a warning   "WARNING ! NO TOUCH..." printed on the PCB

A battery is connected to the underside of the sub-board
and if the battery dies the PCB is no-longer functional.

It is possible that important game code is stored within
the battery.

The ROMs for "Puzzle Star" don't appear to have code at 0
and all boards found so far have been dead.

The Sexy Boom board was working, but it may only be a
matter of time before that board dies too.

It is thought that these games are based on MSX hardware
as some of the Puzzle Star roms appear to be a hacked
MSX Bios.  If we're lucky then the FGPA may only contain
Sang Ho's MSX simulation, rather than any specific game code.

The FGPA is labeled 'Custom 3'

There is another covered chip on the PCB labeled 'Custom 2'
at U17.  It is unknown what this chip is.

Custom 1 is underneath the sub-board and is a UM3567 which
is a YM2413 compatible chip.

*/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

INPUT_PORTS_START( sangho )
    PORT_START
    PORT_DIPNAME(   0x01, 0x01, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME(   0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
INPUT_PORTS_END


VIDEO_START( sangho )
{
	return 0;
}

VIDEO_UPDATE( sangho )
{

}

static MACHINE_DRIVER_START( sangho )

	MDRV_CPU_ADD(Z80,8000000) // ?
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
//  MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	//MDRV_GFXDECODE(gfxdecodeinfo) // GFX don't seem to be tile based
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(sangho)
	MDRV_VIDEO_UPDATE(sangho)
MACHINE_DRIVER_END

ROM_START( pzlestar )
	ROM_REGION( 0x20000*15, REGION_CPU1, 0 ) // 15 sockets, 13 used
	ROM_LOAD( "rom01.bin", 0x000000, 0x20000, CRC(0b327a3b) SHA1(450fd27f9844b9f0b710c1886985bd67cac2716f) ) // seems to be some code at 0x10000
	ROM_LOAD( "rom02.bin", 0x020000, 0x20000, CRC(dc859437) SHA1(e9fe5aab48d80e8857fc679ff7e35298ce4d47c8) )
	ROM_LOAD( "rom03.bin", 0x040000, 0x20000, CRC(f92b5624) SHA1(80be9000fc4326d790801d02959550aada111548) )
	ROM_LOAD( "rom04.bin", 0x060000, 0x20000, CRC(929e7491) SHA1(fb700d3e1d50fefa9b85ccd3702a9854df53a210) )
	ROM_LOAD( "rom05.bin", 0x080000, 0x20000, CRC(8c6f71e5) SHA1(3597b03fe61216256437c56c583d55c7d59b5525) )
	ROM_LOAD( "rom06.bin", 0x0a0000, 0x20000, CRC(84599227) SHA1(d47c6cdbf3b64f83627c768059148e31f8de1f36) )
	ROM_LOAD( "rom07.bin", 0x0c0000, 0x20000, CRC(6f64cc35) SHA1(3e3270834ad31e8240748c2b61f9b8f138d22f68) )
	ROM_LOAD( "rom08.bin", 0x0e0000, 0x20000, CRC(18d2bfe2) SHA1(cb92ee51d061bc053e296fcba10708f69ba12a61) )
	ROM_LOAD( "rom09.bin", 0x100000, 0x20000, CRC(19a31115) SHA1(fa6ead5c8bf6be21d07797f74fcba13f0d041937) )
	ROM_LOAD( "rom10.bin", 0x120000, 0x20000, CRC(c003328b) SHA1(5172e2c48e118ac9f9b9dd4f4df8804245047b33) )
	ROM_LOAD( "rom11.bin", 0x140000, 0x20000, CRC(d36c1f92) SHA1(42b412c1ab99cb14f2e15bd80fede34c0df414b9) )
	ROM_LOAD( "rom12.bin", 0x160000, 0x20000, CRC(baa82727) SHA1(ed3dd1befa615003204f903472ef1af1eb702c38) )
	ROM_LOAD( "rom13.bin", 0x180000, 0x20000, CRC(8b4b6a2c) SHA1(4b9c188260617ccce94cbf6cccb45aab455af09b) )
	/* 14 empty */
	/* 15 empty */
ROM_END

ROM_START( sexyboom )
	ROM_REGION( 0x20000*15, REGION_CPU1, 0 ) // 15 sockets, 12 used
	ROM_LOAD( "rom1.bin",  0x000000, 0x20000, CRC(7827a079) SHA1(a48e7c7d16e50de24c8bf77883230075c1fad858) )
	ROM_LOAD( "rom2.bin",  0x020000, 0x20000, CRC(7028aa61) SHA1(77d5e5945b90d3e15ba2c1364b76f6643247592d) )
	ROM_LOAD( "rom3.bin",  0x040000, 0x20000, CRC(340edac9) SHA1(47ffc4553cb34ff932d3d54d5cefe82571c6ddbf) )
	ROM_LOAD( "rom4.bin",  0x060000, 0x20000, CRC(25f76d7f) SHA1(caff03ba4ca9ad44e488618141c4e1f43a0cdc48) )
	ROM_LOAD( "rom5.bin",  0x080000, 0x20000, CRC(3a3dda85) SHA1(b174cf87be5dd7a7430ff29c70c8093c577f4033) )
	ROM_LOAD( "rom6.bin",  0x0a0000, 0x20000, CRC(d0428a82) SHA1(4409c2ebd2f70828286769c9367cbccac483cdaf) )
	ROM_LOAD( "rom7.bin",  0x0c0000, 0x20000, CRC(2d2e4df2) SHA1(8ec36c8c021c2b9d9be7b61e09e31a7a18a06dad) )
	ROM_LOAD( "rom8.bin",  0x0e0000, 0x20000, CRC(283ba870) SHA1(98f35d95caf58595f002d57a4bafc39b6d67ed1a) )
	ROM_LOAD( "rom9.bin",  0x100000, 0x20000, CRC(a78310f4) SHA1(7a14cabd371d6ba4e335f0e00135de3dd8a4e642) )
	ROM_LOAD( "rom10.bin", 0x120000, 0x20000, CRC(b20fabd2) SHA1(a6a3bac1ac19e1ecd2fd0aeb17fbf16ffa07df13) )
	ROM_LOAD( "rom11.bin", 0x140000, 0x20000, CRC(e4aa16bc) SHA1(c5889b813ceb7a1c0deb8a9d4d006932b266a482) )
	ROM_LOAD( "rom12.bin", 0x160000, 0x20000, CRC(cd8b6b5d) SHA1(ffddc7781e13146e198ad12a9c89504f270857d8) )
	/* 13 empty */
	/* 14 empty */
	/* 15 empty */
ROM_END

GAME( 199?, pzlestar,  0,    sangho, sangho, 0, ROT90, "Sang Ho Soft", "Puzzle Star? (Sang Ho)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1992, sexyboom,  0,    sangho, sangho, 0, ROT90, "Sang Ho Soft", "Sexy Boom", GAME_NOT_WORKING | GAME_NO_SOUND )
