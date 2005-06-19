/* IGS - The Great Wall aka 68k base system? */

/* hardware is probably very similar to China Dragon / Dragon World */

#include "driver.h"

VIDEO_START(grtwall)
{
	return 0;
}

VIDEO_UPDATE(grtwall)
{

}
static ADDRESS_MAP_START( grtwall_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)

ADDRESS_MAP_END

static ADDRESS_MAP_START( grtwall_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)

ADDRESS_MAP_END


static struct GfxLayout grtwall_charlayout =

{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0,
	  12,  8,
	  20,16,
	  28,24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static struct GfxLayout grtwall2_charlayout =

{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,
	  16,24,
	  32,40,
	  48,56 },
	{ 0*32*2, 1*32*2, 2*32*2, 3*32*2, 4*32*2, 5*32*2, 6*32*2, 7*32*2 },
	8*32*2
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &grtwall_charlayout,   0, 1  },
	{ REGION_GFX1, 0, &grtwall2_charlayout,   0, 1  },
	{ -1 } /* end of array */
};


INPUT_PORTS_START( grtwall )
INPUT_PORTS_END

static MACHINE_DRIVER_START( grtwall )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(grtwall_readmem,grtwall_writemem)
//  MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x300)

	MDRV_VIDEO_START(grtwall)
	MDRV_VIDEO_UPDATE(grtwall)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
MACHINE_DRIVER_END


void gw_decrypt(void)
{
	int i;
	data16_t *src = (data16_t *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for(i=0; i<rom_size/2; i++) {
		data16_t x = src[i];

    	if((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
    		x ^= 0x0004;
    	if((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
    		x ^= 0x0020;
    	if((i & 0x2400) == 0x0000 || (i & 0x4100) == 0x4100 || ((i & 0x2000) == 0x2000 && (i & 0x0c00) != 0x0000))
    		x ^= 0x0200;
    	if((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
    	   x ^= 0x0024;
		src[i] = x;

	}
}

void lhb_decrypt(void)
{
	int i;
	data16_t *src = (data16_t *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for(i=0; i<rom_size/2; i++) {
		unsigned short x = src[i];

		if((i & 0x1100) != 0x0100)
			x ^= 0x0002;

		if((i & 0x0150) != 0x0000 && (i & 0x0152) != 0x0010)
			x ^= 0x0400;

		if((i & 0x2084) != 0x2084 && (i & 0x2094) != 0x2014)
			x ^= 0x2000;

		src[i] = (x >> 8) | (x <<8);
	}

}

static DRIVER_INIT( grtwall )
{
	gw_decrypt();
}

static DRIVER_INIT( lhb )
{
	lhb_decrypt();
}

ROM_START( grtwall )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "wlcc4096.rom",         0x00000, 0x100000, CRC(3b16729f) SHA1(4ef4e5cbd6ccc65775e36c2c8b459bc1767d6574) ) // 1ST+2ND IDENTICAL

	/* below are 'bios?' '68kbase' roms? */
	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "m0201-ig.rom",         0x00000, 0x200000, CRC(ec54452c) SHA1(0ee7ffa3d4845af083944e64faf5a1c78247aaa2) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples? */
	ROM_LOAD( "040-c3c2.snd",         0x00000, 0x100000, CRC(220949aa) SHA1(1e0dba168a0687d32aaaed42714ae24358f4a3e7) )  // 1ST+2ND IDENTICAL
ROM_END

ROM_START( lhb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD( "v305j-409",         0x00000, 0x80000,  CRC(701de8ef) SHA1(4a77160f642f4de02fa6fbacf595b75c0d4a505d) )

	/* below are 'bios?' '68kbase' roms? */
	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "m0201-ig.rom",         0x00000, 0x200000, CRC(ec54452c) SHA1(0ee7ffa3d4845af083944e64faf5a1c78247aaa2) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples? */
	ROM_LOAD( "040-c3c2.snd",         0x00000, 0x100000, CRC(220949aa) SHA1(1e0dba168a0687d32aaaed42714ae24358f4a3e7) )  // 1ST+2ND IDENTICAL
ROM_END

ROM_START( xymg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD( "u30-ebac.rom",         0x00000, 0x80000, CRC(7d272b6f) SHA1(15fd1be23cabdc77b747541f5cd9fed6b08be4ad) )
	ROM_LOAD( "ygxy-u8.rom",         0x80000, 0x80000, CRC(56a2706f) SHA1(98bf4b3153eef53dd449e2538b4b7ff2cc2fe6fa) ) // gfx?

	/* below are 'bios?' '68kbase' roms? */
	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "m0201-ig.rom",         0x00000, 0x200000, CRC(ec54452c) SHA1(0ee7ffa3d4845af083944e64faf5a1c78247aaa2) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples? */
	ROM_LOAD( "040-c3c2.snd",         0x00000, 0x100000, CRC(220949aa) SHA1(1e0dba168a0687d32aaaed42714ae24358f4a3e7) )  // 1ST+2ND IDENTICAL
ROM_END


GAMEX( 1994, grtwall, 0, grtwall, grtwall, grtwall, ROT0, "IGS", "The Great Wall", GAME_NO_SOUND | GAME_NOT_WORKING )

GAMEX( 199?, lhb, 0, grtwall, grtwall, lhb, ROT0, "IGS", "Long Hu Bang", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 199?, xymg, 0, grtwall, grtwall, lhb, ROT0, "IGS", "Xing Yen Man Guan", GAME_NO_SOUND | GAME_NOT_WORKING )
