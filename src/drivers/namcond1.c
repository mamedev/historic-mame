/*************************************************************

    Namco ND-1 Driver - Mark McDougall

        With contributions from:
            James Jenkins
            Walter Fath

    Currently Supported Games:
        Namco Classics Vol #1
        Namco Classics Vol #2

    T.B.D.
        Sound

 *************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/ygv608.h"
#include "namcond1.h"

// thought this would be in b16???
#define NODUMPKNOWN  0

/*************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_r },  // shared ram
	{ 0x800000, 0x80000f, ygv608_r },
	{ 0xA00000, 0xA03FFF, MRA16_RAM },              // EEPROM
#ifdef MAME_DEBUG
	{ 0xB00000, 0xB00001, debug_trigger },
#endif
	{ 0xc3ff00, 0xc3ffff, namcond1_cuskey_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x0fffff, MWA16_NOP },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_w, &namcond1_shared_ram },        // shared ram?
	{ 0x800000, 0x80000f, ygv608_w },
	{ 0xA00000, 0xA03FFF, MWA16_RAM, &namcond1_eeprom },
	{ 0xc3ff00, 0xc3ff0f, namcond1_cuskey_w },
MEMORY_END

/*************************************************************/

INPUT_PORTS_START( namcond1 )
	PORT_START      /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START  	/* dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPF_TOGGLE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )
	PORT_BIT( 0xE8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START    /* coin mech inputs - a hack */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END


/* text-layer characters */

static struct GfxLayout pts_8x8_4bits_layout =
{
	8,8,	      /* 8*8 pixels */
	65536,        /* 65536 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ) },
    { STEP8( 0*256, 8*4 ) },
	8*8*4
};

static struct GfxLayout pts_16x16_4bits_layout =
{
	16,16,        /* 16*16 pixels */
	16384,        /* 16384 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ) },
	16*16*4
};

static struct GfxLayout pts_32x32_4bits_layout =
{
	32,32,        /* 32*32 pixels */
	4096,         /* 4096 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ), STEP8( 4*256, 4 ), STEP8( 5*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ), STEP8( 8*256, 8*4 ), STEP8( 10*256, 8*4 ) },
	32*32*4
};

static struct GfxLayout pts_64x64_4bits_layout =
{
	64,64,        /* 32*32 pixels */
	1024,         /* 1024 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ), STEP8( 4*256, 4 ), STEP8( 5*256, 4 ),
      STEP8( 16*256, 4 ), STEP8( 17*256, 4 ), STEP8( 20*256, 4 ), STEP8( 21*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ), STEP8( 8*256, 8*4 ), STEP8( 10*256, 8*4 ),
      STEP8( 32*256, 8*4 ), STEP8( 34*256, 8*4 ), STEP8( 40*256, 8*4 ), STEP8( 42*256, 8*4 ) },
	64*64*4
};

static struct GfxLayout pts_8x8_8bits_layout =
{
	8,8,	      /* 8*8 pixels */
	32768,        /* 32768 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { STEP8( 0*512, 8 ) },
    { STEP8( 0*512, 8*8 ) },
	8*8*8
};

static struct GfxLayout pts_16x16_8bits_layout =
{
	16,16,        /* 16*16 pixels */
	8192,         /* 8192 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { STEP8( 0*512, 8 ), STEP8( 1*512, 8 ) },
    { STEP8( 0*512, 8*8 ), STEP8( 2*512, 8*8 ) },
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000000, &pts_8x8_4bits_layout,    0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_32x32_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_64x64_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_8x8_8bits_layout,    0, 256 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_8bits_layout,  0, 256 },
	{ -1 }
};

/******************************************
  ND-1 Master clock = 49.152MHz
  - 680000  = 12288000 (CLK/4)
  - H8/3002 = 16666667 (CLK/3) ??? huh?
  - H8/3002 = 16384000 (CLK/3)
  - The level 1 interrupt to the 68k has been measured at 60Hz.
*******************************************/

static MACHINE_DRIVER_START( namcond1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)
	MDRV_CPU_PERIODIC_INT(ygv608_timed_interrupt, 1000)

	MDRV_FRAMES_PER_SECOND(60.0)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(namcond1)
	MDRV_NVRAM_HANDLER(namcond1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(288, 224)   // maximum display resolution (512x512 in theory)
	MDRV_VISIBLE_AREA(0, 287, 0, 223)   // default visible area
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(ygv608)
	MDRV_VIDEO_UPDATE(ygv608)
	MDRV_VIDEO_STOP(ygv608)
MACHINE_DRIVER_END


ROM_START( ncv1 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-eng.14d", 0x00000, 0x80000, 0x4ffc530b )
	ROM_LOAD16_WORD( "nc1-eng.13d", 0x80000, 0x80000, 0x26499a4e )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1-voic.7b",     0x000000, 0x200000, NODUMPKNOWN )
ROM_END

ROM_START( ncv1j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-jpn.14d",  0x00000, 0x80000, 0x48ce0b2b )
	ROM_LOAD16_WORD( "nc1-jpn.13d",  0x80000, 0x80000, 0x49f99235 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1-voic.7b",     0x000000, 0x200000, NODUMPKNOWN )
ROM_END

ROM_START( ncv1j2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-jpn2.14d", 0x00000, 0x80000, 0x7207469d )
	ROM_LOAD16_WORD( "nc1-jpn2.13d", 0x80000, 0x80000, 0x52401b17 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1-voic.7b",     0x000000, 0x200000, NODUMPKNOWN )
ROM_END

ROM_START( ncv2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc2-eng.14e", 0x00000, 0x80000, 0xfb8a4123 )
	ROM_LOAD16_WORD( "nc2-eng.13e", 0x80000, 0x80000, 0x7a5ef23b )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc2.1d",          0x00000, 0x80000, 0x365cadbf )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "cg0.10e",         0x000000, 0x200000, 0xfdd24dbe )
	ROM_LOAD( "cg1.10e",         0x200000, 0x200000, 0x007b19de )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc2-voic.7c",     0x000000, 0x200000, 0xed05fd88 )
ROM_END

ROM_START( ncv2j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc2-jpn.14e", 0x00000, 0x80000, 0x99991192 )
	ROM_LOAD16_WORD( "nc2-jpn.13e", 0x80000, 0x80000, 0xaf4ba4f6 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc2.1d",          0x00000, 0x80000, 0x365cadbf )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "cg0.10e",         0x000000, 0x200000, 0xfdd24dbe )
	ROM_LOAD( "cg1.10e",         0x200000, 0x200000, 0x007b19de )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc2-voic.7c",     0x000000, 0x200000, 0xed05fd88 )
ROM_END

#if 0

static void namcond1_patch( int *addr )
{
    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; addr[i]; i++ )
      // insert a NOP instruction
      WRITE_WORD( &ROM[addr[i]], 0x4e71 );
}

/*
 *  These are the patch locations to skip coldboot check
 *  - they were required before the MCU simulation was
 *    sufficiently advanced.
 *  - please do not delete these comments *just in case*!
 *
 *  ncv1    0x15038
 *  ncv1j   0x151c0
 *  ncv1j2  0x152e4
 *  ncv2    0x17974
 *  ncv2j   0x17afc
 */

#endif


GAMEX( 1995, ncv1,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Vol.1", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j,  ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Vol.1 (Japan set 1)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j2, ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Vol.1 (Japan set 2)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, ncv2,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Vol.2", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, ncv2j,  ncv2, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Vol.2 (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
