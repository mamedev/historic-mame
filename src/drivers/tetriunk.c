/* Tetris
  Unknown manufacturer, seems to be a port of a PC game, accesses common vga ports, contains a pc bios etc.
*/

#include "driver.h"

UINT8* tetriunk_videoram;

READ8_HANDLER( tetriunk_0x061_r )
{
	return 0xff;
}

READ8_HANDLER( tetriunk_0x040_r )
{
	return 0x00;
}

READ8_HANDLER( tetriunk_0x042_r )
{
	return 0x00;
}

WRITE8_HANDLER( tetriunk_0x061_w )
{

}

WRITE8_HANDLER( tetriunk_0x042_w )
{

}

WRITE8_HANDLER( tetriunk_0x043_w )
{

}

static ADDRESS_MAP_START( tetriunk_readio, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x040, 0x040) AM_READ( tetriunk_0x040_r )
	AM_RANGE(0x042, 0x042) AM_READ( tetriunk_0x042_r )
	AM_RANGE(0x061, 0x061) AM_READ( tetriunk_0x061_r )
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetriunk_writeio, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x042, 0x042) AM_WRITE( tetriunk_0x042_w )
	AM_RANGE(0x043, 0x043) AM_WRITE( tetriunk_0x043_w )
	AM_RANGE(0x061, 0x061) AM_WRITE( tetriunk_0x061_w )
	ADDRESS_MAP_END

static ADDRESS_MAP_START( tetriunk_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_READ(MRA8_RAM)
	AM_RANGE(0xb0000, 0xbffff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf0000, 0xfffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetriunk_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xb0000, 0xbffff) AM_WRITE(MWA8_RAM)  AM_BASE(&tetriunk_videoram)
ADDRESS_MAP_END

static INTERRUPT_GEN( tetriunk_irq )
{
// no idea
//  cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

INPUT_PORTS_START( tetriunk )
INPUT_PORTS_END

VIDEO_START(tetriunk)
{
	return 0;
}


VIDEO_UPDATE(tetriunk)
{
	int x,y;
	int count = 0;

	for (y=0;y<64;y++)
	{
		for (x=0;x<64;x++)
		{
			int tile = tetriunk_videoram[count*2+0x8000];
			drawgfx(bitmap,Machine->gfx[0],tile&0xff,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);
			count++;
		}
	}

	return 0;
}

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0  },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( tetriunk )
	/* basic machine hardware */
	MDRV_CPU_ADD(I86, 8000000)        // ? might be 8080
	MDRV_CPU_PROGRAM_MAP(tetriunk_readmem, tetriunk_writemem)
	MDRV_CPU_IO_MAP(tetriunk_readio,tetriunk_writeio)

	MDRV_CPU_VBLANK_INT(tetriunk_irq,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512,512)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 512-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(tetriunk)
	MDRV_VIDEO_UPDATE(tetriunk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END




ROM_START( tetriunk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* code */
	ROM_LOAD( "b-10.u10", 0xf0000, 0x10000, CRC(efc2a0f6) SHA1(5f0f1e90237bee9b78184035a32055b059a91eb3) )

	ROM_REGION( 0x10000, REGION_GFX1,0 ) /* gfx - 1bpp font*/
	ROM_LOAD( "b-3.u36", 0x00000, 0x2000, CRC(1a636f9a) SHA1(a356cc57914d0c9b9127670b55d1f340e64b1ac9) )

	ROM_REGION( 0x80000, REGION_GFX2,0 ) /* gfx (backgrounds?) - not decoded, bitmaps? */
	ROM_LOAD( "b-1.u59", 0x00000, 0x10000, CRC(4719d986) SHA1(6e0499944b968d96fbbfa3ead6237d69c769d634) )
	ROM_LOAD( "b-2.u58", 0x10000, 0x10000, CRC(599e1154) SHA1(14d99f90b4fedeab0ac24ffa9b1fd9ad0f0ba699) )

	ROM_LOAD( "b-4.u54", 0x20000, 0x10000, CRC(e112c450) SHA1(dfdecfc6bd617ec520b7563b7caf44b79d498bd3) )
	ROM_LOAD( "b-5.u53", 0x30000, 0x10000, CRC(050b7650) SHA1(5981dda4ed43b6e81fbe48bfba90a8775d5ecddf) )

	ROM_LOAD( "b-6.u49", 0x40000, 0x10000, CRC(d596ceb0) SHA1(8c82fb638688971ef11159a6b240253e63f0949d) )
	ROM_LOAD( "b-7.u48", 0x50000, 0x10000, CRC(79336b6c) SHA1(7a95875f3071bdc3ee25c0e6a5a3c00ef02dc977) )

	ROM_LOAD( "b-8.u44", 0x60000, 0x10000, CRC(1f82121a) SHA1(106da0f39f1260d0761217ed0a24c1611bfd7f05) )
	ROM_LOAD( "b-9.u43", 0x70000, 0x10000, CRC(4ea22349) SHA1(14dfd3dbd51f8bd6f3290293b8ea1c165e8cf7fd) )
ROM_END

GAME( 1989, tetriunk, 0,        tetriunk, tetriunk, 0, ROT0,  "Unknown",      "Tetris (Unknown Manufacturer)"       , GAME_NO_SOUND | GAME_NOT_WORKING )
