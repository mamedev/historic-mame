/***************************************************************************

	Atari Sprint 2 hardware

	driver by Mike Balfour

	Games supported:
		* Sprint 1
		* Sprint 2

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
	        0000-03FF       WRAM
	        0400-07FF       R: RAM and DISPLAY, W: DISPLAY
	        0800-0BFF       R: SWITCH
	        0C00-0FFF       R: SYNC
	        0C00-0C0F       W: ATTRACT
	        0C10-0C1F       W: SKID1
	        0C20-0C2F       W: SKID2
	        0C30-0C3F       W: LAMP1
	        0C40-0C4F       W: LAMP2
	        0C60-0C6F       W: SPARE
	        0C80-0CFF       W: TIMER RESET (Watchdog)
	        0D00-0D7F       W: COLLISION RESET 1
	        0D80-0DFF       W: COLLISION RESET 2
	        0E00-0E7F       W: STEERING RESET 1
	        0E80-0EFF       W: STEERING RESET 2
	        0F00-0F7F       W: NOISE RESET
	        1000-13FF       R: COLLISION1
	        1400-17FF       R: COLLISION2
	        2000-23FF       PROM1 (Playfield ROM1)
	        2400-27FF       PROM2 (Playfield ROM1)
	        2800-2BFF       PROM3 (Playfield ROM2)
	        2C00-2FFF       PROM4 (Playfield ROM2)
	        3000-33FF       PROM5 (Program ROM1)
	        3400-37FF       PROM6 (Program ROM1)
	        3800-3BFF       PROM7 (Program ROM2)
	        3C00-3FFF       PROM8 (Program ROM2)
	       (FC00-FFFF)      PROM8 (Program ROM2) - only needed for the 6502 vectors

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sprint2.h"



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x01, 0x00, /* Black playfield */
	0x01, 0x03, /* White playfield */
	0x01, 0x03, /* White car */
	0x01, 0x00, /* Black car */
	0x01, 0x02, /* Grey car 1 */
	0x01, 0x02  /* Grey car 2 */
};

static PALETTE_INIT( sprint2 )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK - oil slicks, text, black car */
	palette_set_color(1,0x55,0x55,0x55); /* DK GREY - default background */
	palette_set_color(2,0x80,0x80,0x80); /* LT GREY - grey cars */
	palette_set_color(3,0xff,0xff,0xff); /* WHITE - track, text, white car */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM }, /* WRAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x0800, 0x083f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0840, 0x087f, sprint2_coins_r },
	{ 0x0880, 0x08bf, sprint2_steering1_r },
	{ 0x08c0, 0x08ff, sprint2_steering2_r },
	{ 0x0900, 0x093f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0940, 0x097f, sprint2_coins_r },
	{ 0x0980, 0x09bf, sprint2_steering1_r },
	{ 0x09c0, 0x09ff, sprint2_steering2_r },
	{ 0x0a00, 0x0a3f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0a40, 0x0a7f, sprint2_coins_r },
	{ 0x0a80, 0x0abf, sprint2_steering1_r },
	{ 0x0ac0, 0x0aff, sprint2_steering2_r },
	{ 0x0b00, 0x0b3f, sprintx_read_ports_r }, /* SWITCH */
	{ 0x0b40, 0x0b7f, sprint2_coins_r },
	{ 0x0b80, 0x0bbf, sprint2_steering1_r },
	{ 0x0bc0, 0x0bff, sprint2_steering2_r },
	{ 0x0c00, 0x0fff, sprint2_read_sync_r }, /* SYNC */
	{ 0x1000, 0x13ff, sprint2_collision1_r }, /* COLLISION 1 */
	{ 0x1400, 0x17ff, sprint2_collision2_r }, /* COLLISION 2 */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROM1-PROM8 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM }, /* WRAM */
	{ 0x0010, 0x0013, MWA_RAM, &sprint2_horiz_ram }, /* WRAM */
	{ 0x0018, 0x001f, MWA_RAM, &sprint2_vert_car_ram }, /* WRAM */
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x0c00, 0x0c0f, MWA_RAM }, /* ATTRACT */
	{ 0x0c10, 0x0c1f, MWA_RAM }, /* SKID1 */
	{ 0x0c20, 0x0c2f, MWA_RAM }, /* SKID2 */
	{ 0x0c30, 0x0c3f, sprint2_lamp1_w }, /* LAMP1 */
	{ 0x0c40, 0x0c4f, sprint2_lamp2_w }, /* LAMP2 */
	{ 0x0c60, 0x0c6f, MWA_RAM }, /* SPARE */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* TIMER RESET (watchdog) */
	{ 0x0d00, 0x0d7f, sprint2_collision_reset1_w }, /* COLLISION RESET 1 */
	{ 0x0d80, 0x0dff, sprint2_collision_reset2_w }, /* COLLISION RESET 2 */
	{ 0x0e00, 0x0e7f, sprint2_steering_reset1_w }, /* STEERING RESET 1 */
	{ 0x0e80, 0x0eff, sprint2_steering_reset2_w }, /* STEERING RESET 2 */
	{ 0x0f00, 0x0f7f, MWA_RAM }, /* NOISE RESET */
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( sprint2 )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x00, "Tracks on Demo" )
	PORT_DIPSETTING(    0x00, "Easy Track Only" )
	PORT_DIPSETTING(    0x01, "Cycle 12 Tracks" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage )  )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0x00, "Real Game Length" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "P1 1st gear", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "P1 2nd gear", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4, "P1 3rd gear", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5, "P1 4th gear", KEYCODE_V, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "P2 1st gear", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "P2 2nd gear", KEYCODE_W, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "P2 3rd gear", KEYCODE_E, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "P2 4th gear", KEYCODE_R, IP_JOY_DEFAULT )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "P1 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON6, "Track Select", KEYCODE_T, IP_JOY_DEFAULT )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )

	PORT_START      /* IN5 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( sprint1 )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x00, "Change Track" )
	PORT_DIPSETTING(    0x01, "Every Lap" )
	PORT_DIPSETTING(    0x00, "Every 2 Laps" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage )  )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Allow Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0x00, "Real Game Length" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "1st gear", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "2nd gear", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4, "3rd gear", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5, "4th gear", KEYCODE_V, IP_JOY_DEFAULT )
	PORT_BIT (0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )

	PORT_START      /* IN5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout carlayout =
{
	16,8,
	32,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8  },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 2 },
	{ REGION_GFX2, 0, &carlayout,  4, 4 },
	{ -1 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_INIT( sprint1 )
{
	sprintx_is_sprint2 = 0;
}

static MACHINE_INIT( sprint2 )
{
	sprintx_is_sprint2 = 1;
}


static MACHINE_DRIVER_START( sprint2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 333333)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(sprint2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))

	MDRV_PALETTE_INIT(sprint2)
	MDRV_VIDEO_START(sprint2)
	MDRV_VIDEO_UPDATE(sprint2)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sprint1 )

	MDRV_IMPORT_FROM(sprint2)

	/* basic machine hardware */
	MDRV_MACHINE_INIT(sprint1)

	/* video hardware */
	MDRV_VIDEO_UPDATE(sprint1)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( sprint1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, 0x41fc985e )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, 0x07f7a920 )
	ROM_LOAD( "6442-01.d1",   0x3000, 0x0800, 0xe9ff0124 )
	ROM_RELOAD(               0xf000, 0x0800 )
	ROM_LOAD( "6443-01.e1",   0x3800, 0x0800, 0xd6bb00d0 )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6396-01.p4",   0x0000, 0x0200, 0x801b42dd )
	ROM_LOAD_NIB_LOW ( "6397-01.r4",   0x0000, 0x0200, 0x135ba1aa )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6399-01.j6",   0x0000, 0x0200, 0x63d685b2 )
	ROM_LOAD_NIB_LOW ( "6398-01.k6",   0x0000, 0x0200, 0xc9e1017e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6401-01.e2",   0x0000, 0x0020, 0x857df8db )	/* unknown */
ROM_END

ROM_START( sprint2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, 0x41fc985e )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, 0x07f7a920 )
	ROM_LOAD( "6404sp2.d1",   0x3000, 0x0800, 0xd2878ff6 )
	ROM_RELOAD(               0xf000, 0x0800 )
	ROM_LOAD( "6405sp2.e1",   0x3800, 0x0800, 0x6c991c80 )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6396-01.p4",   0x0000, 0x0200, 0x801b42dd )
	ROM_LOAD_NIB_LOW ( "6397-01.r4",   0x0000, 0x0200, 0x135ba1aa )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "6399-01.j6",   0x0000, 0x0200, 0x63d685b2 )
	ROM_LOAD_NIB_LOW ( "6398-01.k6",   0x0000, 0x0200, 0xc9e1017e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6401-01.e2",   0x0000, 0x0020, 0x857df8db )	/* unknown */
ROM_END



GAMEX( 1978, sprint1, 0,       sprint1, sprint1, 0, ROT0, "Atari", "Sprint 1", GAME_NO_SOUND )
GAMEX( 1976, sprint2, sprint1, sprint2, sprint2, 0, ROT0, "Atari", "Sprint 2", GAME_NO_SOUND )
