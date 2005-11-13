/*
    Wink    -   (c) 1985 Midcoin

    TODO:
    - finish decryption before anything can be emulated

*/

#include "driver.h"
#include "vidhrdw/generic.h"


static ADDRESS_MAP_START( wink_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( wink )
INPUT_PORTS_END


VIDEO_START(wink)
{
	return 0;
}

VIDEO_UPDATE(wink)
{
}


static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 4 },
	{ -1 } /* end of array */
};


static DRIVER_INIT( wink )
{
	unsigned char c;
	unsigned int i;

	UINT8 *ROM = memory_region(REGION_CPU1);

	for (i=0; i<memory_region_length(REGION_CPU1); i++)
	{
		c = ROM[i];
		// Swap found by HIGHWAYMAN under the Z80 inside the big module
		c = BITSWAP8(c, 5,3,2,0,1,4,7,6); /* swapped inside of the bigger module */
		ROM[i] = c;
	}
}


static MACHINE_DRIVER_START( wink )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 12288000 / 4)	/* 3.072 MHz ? */
	MDRV_CPU_PROGRAM_MAP(wink_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(wink)
	MDRV_VIDEO_UPDATE(wink)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 12288000 / 8) // AY8912 actually
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wink )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	//right order?
	ROM_LOAD( "midcoin-wink01.rom", 0x0000, 0x4000, CRC(acb0a392) SHA1(428c24845a27b8021823a4a930071b3b47108f01) )
	ROM_LOAD( "midcoin-wink00.rom", 0x4000, 0x4000, CRC(044f82d6) SHA1(4269333578c4fb14891b937c683aa5b105a193e7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU */
	ROM_LOAD( "midcoin-wink05.rom", 0x0000, 0x2000, CRC(c6c9d9cf) SHA1(99984905282c2310058d1ce93aec68d8a920b2c0) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "midcoin-wink02.rom", 0x0000, 0x2000, CRC(d1cd9d06) SHA1(3b3ce61a0516cc94663f6d3aff3fea46aceb771f) )
	ROM_LOAD( "midcoin-wink03.rom", 0x2000, 0x2000, CRC(2346f50c) SHA1(a8535fcde0e9782ea61ad18443186fd5a6ebdc7d) )
	ROM_LOAD( "midcoin-wink04.rom", 0x4000, 0x2000, CRC(06dd229b) SHA1(9057cf10e9ec4119297c2d40b26f0ce0c1d7b86a) )
ROM_END


GAME( 1985, wink, 0, wink, wink, wink, ROT0, "Midcoin", "Wink", GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING)
