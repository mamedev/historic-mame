/* Taito Midnight Landing
  Dual 68k + Z80

  no other hardware info..but it doesn't seem related to taitoair.c at all
*/

#include "driver.h"

READ16_HANDLER( mlanding_unk_r )
{
	return mame_rand();
}

WRITE16_HANDLER( mlanding_unk_w )
{

}

static UINT16 *mlanding_video;


/* Main CPU Map */

static ADDRESS_MAP_START( mlanding_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_READ(MRA16_RAM)

/* are these readable? */
	AM_RANGE(0x100000, 0x1cffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x280000, 0x2807ff) AM_READ(MRA16_RAM)

	AM_RANGE(0x240004, 0x240005) AM_READ(mlanding_unk_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mlanding_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_WRITE(MWA16_RAM)

	// 1c0000 - 1c3fff seems to have some special meaning, maybe cpu comms / shared? */
	AM_RANGE(0x100000, 0x1cffff) AM_WRITE(MWA16_RAM) // data gets transfered from 0x20000 in rom here (tiles if we load them there..)
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM) AM_BASE(&mlanding_video) // something gets put here
	AM_RANGE(0x280000, 0x2807ff) AM_WRITE(MWA16_RAM) // probably palette but nothing uploaded..
ADDRESS_MAP_END

/* Sub CPU Map */

static ADDRESS_MAP_START( mlanding_sub_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x040000, 0x043fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x1c4010, 0x1c4011) AM_READ(mlanding_unk_r) // command from other cpu?

	AM_RANGE(0x050000, 0x0503ff) AM_READ(MRA16_RAM) // is this readable?


	AM_RANGE(0x200000, 0x203fff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mlanding_sub_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x040000, 0x043fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x050000, 0x0503ff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x203fff) AM_WRITE(MWA16_RAM) // looks like some kind of list .. sprite list?

	AM_RANGE(0x1c0000, 0x1c3fff) AM_WRITE(MWA16_RAM) // maybe it shares?
	AM_RANGE(0x1c4010, 0x1c4011) AM_WRITE(mlanding_unk_w) // response to other cpu??

ADDRESS_MAP_END

VIDEO_START(mlanding)
{
	return 0;
}

VIDEO_UPDATE(mlanding)
{
#if 0
	int x,y;
	int count = 0;

	for (y=0;y<32;y++)
	{
		for (x=0;x<64;x++)
		{
			int tile = mlanding_video[count]&0x0fff;
			drawgfx(bitmap,Machine->gfx[0],tile+0x4000,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			count++;
		}
	}
#endif
	return 0;
}


INPUT_PORTS_START( mlanding )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 8, 16, 24 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( mlanding )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000 )		/* 12 MHz ??? (guess) */
	MDRV_CPU_PROGRAM_MAP(mlanding_readmem,mlanding_writemem)
//  MDRV_CPU_VBLANK_INT(irq1_line_hold,1) // all irqs point the same place apart from 6
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1) // all irqs point the same place apart from 6

	MDRV_CPU_ADD(M68000, 12000000 )		/* 12 MHz ??? (guess) */
	MDRV_CPU_PROGRAM_MAP(mlanding_sub_readmem,mlanding_sub_writemem)
	/* No Valid IRQs? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 511, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(mlanding)
	MDRV_VIDEO_UPDATE(mlanding)
MACHINE_DRIVER_END

ROM_START( mlanding )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "ml_b0929.epr", 0x00000, 0x10000, CRC(ab3f38f3) SHA1(4357112ca11a8e7bfe08ba99ac3bddac046c230a))
	ROM_LOAD16_BYTE( "ml_b0928.epr", 0x00001, 0x10000, CRC(21e7a8f6) SHA1(860d3861d4375866cd27d426d546ddb2894a6629) )
	/* but it copies from 20000 up to 5ffff - maybe ram based tiles, these are gfx */
	ROM_LOAD16_BYTE( "ml_b0927.epr", 0x20000, 0x10000, CRC(b02f1805) SHA1(b8050f955c7070dc9b962db329b5b0ee8b2acb70) )
	ROM_LOAD16_BYTE( "ml_b0926.epr", 0x20001, 0x10000, CRC(d57ff428) SHA1(8ff1ab666b06fb873f1ba9b25edf4cd49b9861a1) )
	ROM_LOAD16_BYTE( "ml_b0925.epr", 0x40000, 0x10000, CRC(ff59f049) SHA1(aba490a28aba03728415f34d321fd599c31a5fde) )
	ROM_LOAD16_BYTE( "ml_b0924.epr", 0x40001, 0x10000, CRC(9bc3e1b0) SHA1(6d86804327df11a513a0f06dceb57b83b34ac007) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_ERASE00 )
//  ROM_LOAD16_BYTE( "ml_b0926.epr", 0x00001, 0x10000, CRC(d57ff428) SHA1(8ff1ab666b06fb873f1ba9b25edf4cd49b9861a1) )
//  ROM_LOAD16_BYTE( "ml_b0927.epr", 0x00000, 0x10000, CRC(b02f1805) SHA1(b8050f955c7070dc9b962db329b5b0ee8b2acb70) )
//  ROM_LOAD16_BYTE( "ml_b0924.epr", 0x20001, 0x10000, CRC(9bc3e1b0) SHA1(6d86804327df11a513a0f06dceb57b83b34ac007) )
//  ROM_LOAD16_BYTE( "ml_b0925.epr", 0x20000, 0x10000, CRC(ff59f049) SHA1(aba490a28aba03728415f34d321fd599c31a5fde) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "ml_b0923.epr", 0x00000, 0x10000, CRC(81b2c871) SHA1(a085bc528c63834079469db6ae263a5b9b984a7c) )
	ROM_LOAD16_BYTE( "ml_b0922.epr", 0x00001, 0x10000, CRC(36923b42) SHA1(c31d7c45a563cfc4533379f69f32889c79562534) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* z80 */
	ROM_LOAD( "ml_b0935.epr", 0x00000, 0x8000, CRC(b85915c5) SHA1(656e97035ae304f84e90758d0dd6f0616c40f1db) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )	/* these look like sample roms */
	ROM_LOAD( "ml_b0930.epr", 0x00000, 0x10000, CRC(214a30e2) SHA1(3dcc3a89ed52e4dbf232d2a92a3e64975b46c2dd) )
	ROM_LOAD( "ml_b0931.epr", 0x00000, 0x10000, CRC(9c4a82bf) SHA1(daeac620c636013a36595ce9f37e84e807f88977) )
	ROM_LOAD( "ml_b0932.epr", 0x00000, 0x10000, CRC(4721dc59) SHA1(faad75d577344e9ba495059040a2cf0647567426) )
	ROM_LOAD( "ml_b0933.epr", 0x00000, 0x10000, CRC(f5cac954) SHA1(71abdc545e0196ad4d357af22dd6312d10a1323f) )
	ROM_LOAD( "ml_b0934.epr", 0x00000, 0x10000, CRC(0899666f) SHA1(032e3ddd4caa48f82592570616e16c084de91f3e) )
	ROM_LOAD( "ml_b0936.epr", 0x00000, 0x02000, CRC(51fd3a77) SHA1(1fcbadf1877e25848a1d1017322751560a4823c0) )
	ROM_LOAD( "ml_b0937.epr", 0x00000, 0x08000, CRC(4bdf15ed) SHA1(b960208e63cede116925e064279a6cf107aef81c) )
ROM_END

GAME( 1990, mlanding, 0,        mlanding,   mlanding, 0,        ROT0,    "Taito Corporation", "Midnight Landing", GAME_NOT_WORKING|GAME_NO_SOUND )
