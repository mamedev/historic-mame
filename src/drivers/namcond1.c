/*************************************************************

    Namco ND-1 Driver - Mark McDougall

        With contributions from:
            James Jenkins
            Walter Fath

    Currently Supported Games:
        Namco Classics Vol #1
    Other known ND-1 games:
        Namco Classics Vol #2

 *************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/ygv608.h"
#include "namcond1.h"

/*************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_r },  // shared ram?
	{ 0x800000, 0x80000f, ygv608_r },
	{ 0xA00000, 0xA03FFF, MRA16_RAM },        // EEPROM???
#ifdef MAME_DEBUG
	{ 0xB00000, 0xB00001, debug_trigger },
#endif
	{ 0xc3ff00, 0xc3ffff, namcond1_cuskey_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_NOP },
	{ 0x080000, 0x0fffff, MWA16_NOP },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_w, &namcond1_shared_ram },        // shared ram?
	{ 0x800000, 0x80000f, ygv608_w },
	{ 0xA00000, 0xA03FFF, MWA16_RAM },        // EEPROM???
	{ 0xc3ff00, 0xc3ffff, MWA16_NOP },
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
	8,8,	        /* 8*8 pixels */
	65536,        /* 65536 patterns */
	4,	        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },
	8*8*4
};

static struct GfxLayout pts_16x16_4bits_layout =
{
	16,16,        /* 16*16 pixels */
	16384,        /* 16384 patterns */
	4,	        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	256+0*4, 256+1*4, 256+2*4, 256+3*4, 256+4*4, 256+5*4, 256+6*4, 256+7*4 },
	{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4,
	512+0*8*4, 512+1*8*4, 512+2*8*4, 512+3*8*4, 512+4*8*4, 512+5*8*4, 512+6*8*4, 512+7*8*4 },
	16*16*4
};

static struct GfxLayout pts_32x32_4bits_layout =
{
	32,32,        /* 32*32 pixels */
	4096,         /* 4096 patterns */
	4,	        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		256+0*4, 256+1*4, 256+2*4, 256+3*4, 256+4*4, 256+5*4, 256+6*4, 256+7*4,
		1024+0*4, 1024+1*4, 1024+2*4, 1024+3*4, 1024+4*4, 1024+5*4, 1024+6*4, 1024+7*4,
		1280+0*4, 1280+1*4, 1280+2*4, 1280+3*4, 1280+4*4, 1280+5*4, 1280+6*4, 1280+7*4 },
	{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4,
		512+0*8*4, 512+1*8*4, 512+2*8*4, 512+3*8*4, 512+4*8*4, 512+5*8*4, 512+6*8*4, 512+7*8*4,
		2048+0*8*4, 2048+1*8*4, 2048+2*8*4, 2048+3*8*4, 2048+4*8*4, 2048+5*8*4, 2048+6*8*4, 2048+7*8*4,
		2560+0*8*4, 2560+1*8*4, 2560+2*8*4, 2560+3*8*4, 2560+4*8*4, 2560+5*8*4, 2560+6*8*4, 2560+7*8*4 },
	32*32*4
};

static struct GfxLayout pts_8x8_8bits_layout =
{
	8,8,	        /* 8*8 pixels */
	32768,        /* 32768 patterns */
	8,	        /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8 },
	8*8*8
};

static struct GfxLayout pts_16x16_8bits_layout =
{
	16,16,        /* 16*16 pixels */
	8192,         /* 8192 patterns */
	8,	        /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		512+0*8, 512+1*8, 512+2*8, 512+3*8, 512+4*8, 512+5*8, 512+6*8, 512+7*8 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8,
		1024+0*8*8, 1024+1*8*8, 1024+2*8*8, 1024+3*8*8, 1024+4*8*8, 1024+5*8*8, 1024+6*8*8, 1024+7*8*8 },
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000000, &pts_8x8_4bits_layout,    0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_32x32_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_8x8_8bits_layout,    0, 256 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_8bits_layout,  0, 256 },
	{ -1 }
};

/******************************************
  ND-1 Master clock = 49.152Mhz
  - 680000  = 12288000 (CLK/4)
  - H8/3002 = 16666667 (CLK/3) ??? huh?
  - H8/3002 = 16384000 (CLK/3)
  - The level 1 interrupt to the 68k has been measured at 60Hz.
*******************************************/

static struct MachineDriver machine_driver_namcond1 =
{
	{
		{
			CPU_M68000,
			12288000,
			readmem, writemem, 0, 0,
			namcond1_vb_interrupt, 1,
			ygv608_timed_interrupt, 1000,
			0
		}
	},
	60.0,
	DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100, /* 100 CPU slices per frame */
	namcond1_init_machine,

	/* video hardware */
	512, 512,             // maximum display resolution
	{ 0, 511, 0, 511 },   // default visible area
	gfxdecodeinfo,
	256,256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN,
	0,			                /* Video initialisation    */
	ygv608_vh_start,	            /* Video start             */
	ygv608_vh_stop,	            /* Video stop              */
	ygv608_vh_update,	            /* Video update            */

	/* sound hardware */
	0,0,0,0,
	/* Sound struct here */
	{
		{ 0, 0 }
	},

	namcond1_nvramhandler
};


ROM_START( ncv1 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-eng.14d", 0x00000, 0x80000, 0x4ffc530b )
	ROM_LOAD16_WORD( "nc1-eng.13d", 0x80000, 0x80000, 0x26499a4e )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )
ROM_END

ROM_START( ncv1j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-jpn.14d",  0x00000, 0x80000, 0x48ce0b2b )
	ROM_LOAD16_WORD( "nc1-jpn.13d",  0x80000, 0x80000, 0x49f99235 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )
ROM_END

ROM_START( ncv1j2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc1-jpn2.14d", 0x00000, 0x80000, 0x7207469d )
	ROM_LOAD16_WORD( "nc1-jpn2.13d", 0x80000, 0x80000, 0x52401b17 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )
ROM_END

ROM_START( ncv2j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "nc2-jpn.13d", 0x00000, 0x80000, 0x99991192 )
	ROM_LOAD16_WORD( "nc2-jpn.14d", 0x80000, 0x80000, 0xaf4ba4f6 )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1.1c",          0x00000, 0x80000, 0x48ea0de2 )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "cg0.10c",         0x000000, 0x200000, 0x355e7f29 )
ROM_END



static void init_ncv1( void )
{
    static int patch[] =
    {
      0x15038,                      // check cold boot

      0
    };

    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; patch[i]; i++ )
      // insert a NOP instruction
      WRITE_WORD( &ROM[patch[i]], 0x4e71 );
}

static void init_ncv1j( void )
{
    static int patch[] =
    {
      0x151c0,                      // check cold boot

      0
    };

    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; patch[i]; i++ )
      // insert a NOP instruction
      WRITE_WORD( &ROM[patch[i]], 0x4e71 );
}

static void init_ncv1j2( void )
{
    static int patch[] =
    {
      0x152e4,                      // check cold boot

      0
    };

    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; patch[i]; i++ )
      // insert a NOP instruction
      WRITE_WORD( &ROM[patch[i]], 0x4e71 );
}

static void init_ncv2j( void )
{
    static int patch[] =
    {
      0x17afc,                      // check cold boot

      0
    };

    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; patch[i]; i++ )
      // insert a NOP instruction
      WRITE_WORD( &ROM[patch[i]], 0x4e71 );
}


GAMEX( 1995, ncv1,      0, namcond1, namcond1, ncv1,   ROT90_16BIT, "Namco", "Namco Classics Vol.1", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j,  ncv1, namcond1, namcond1, ncv1j,  ROT90_16BIT, "Namco", "Namco Classics Vol.1 (Japan set 1)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j2, ncv1, namcond1, namcond1, ncv1j2, ROT90_16BIT, "Namco", "Namco Classics Vol.1 (Japan set 2)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv2j,     0, namcond1, namcond1, ncv2j,  ROT90_16BIT, "Namco", "Namco Classics Vol.2 (Japan)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
