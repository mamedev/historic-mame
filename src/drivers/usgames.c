/* US Games - Trivia / Quiz / 'Amusement Only' Gambling Games */

/*

there is a 'Security Test' in service mode

'usg82', 'usg83' and 'usg83x' don't seem to be able to record
the changes you make in the "test mode" 8(

*/

/* readme info

US Games
Counter Top Mini Games
1987-1992

In this archive are different versions.
Version 3.2, 8.3, 18.5, and 25.2.

Hardware Specs: MC6809P, MC6845P, MB8146A x 3
Sound: AY-3-8912

*/

#include "driver.h"
#include "vidhrdw/crtc6845.h"

/* vidhrdw */
WRITE_HANDLER( usg_videoram_w );
WRITE_HANDLER( usg_charram_w );
VIDEO_START(usg);
PALETTE_INIT(usg);
VIDEO_UPDATE(usg);
extern struct tilemap *usg_tilemap;


extern data8_t *usg_videoram,*usg_charram;


static WRITE_HANDLER( usg_rombank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

//	logerror ("BANK WRITE? -%02x-\n",data);
//usrintf_showmessage("%02x",data);

	cpu_setbank( 1,&RAM[ 0x10000 + 0x4000 * data] );
}

static WRITE_HANDLER( lamps1_w )
{
	/* button lamps */
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);

	/* bit 5 toggles all the time - extra lamp? */
}

static WRITE_HANDLER( lamps2_w )
{
	/* bit 5 toggles all the time - extra lamp? */
}



static MEMORY_READ_START( usg_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_RAM },

	{ 0x2000, 0x2000, input_port_1_r },
	{ 0x2010, 0x2010, input_port_0_r },
	{ 0x2041, 0x2041, input_port_2_r },
	{ 0x2070, 0x2070, input_port_3_r },

	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START( usg185_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_RAM },

	{ 0x2400, 0x2400, input_port_1_r },
	{ 0x2410, 0x2410, input_port_0_r },
	{ 0x2441, 0x2441, input_port_2_r },
	{ 0x2470, 0x2470, input_port_3_r },

	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( usg_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, MWA_RAM, &generic_nvram, &generic_nvram_size },

	{ 0x2020, 0x2020, lamps1_w },
	{ 0x2030, 0x2030, lamps2_w },

	{ 0x2040, 0x2040, crtc6845_address_w },
	{ 0x2041, 0x2041, crtc6845_register_w },

	{ 0x2400, 0x2400, AY8910_control_port_0_w },
	{ 0x2401, 0x2401, AY8910_write_port_0_w },

	{ 0x2060, 0x2060, usg_rombank_w },

	{ 0x2800, 0x2fff, usg_charram_w, &usg_charram },
	{ 0x3000, 0x3fff, usg_videoram_w, &usg_videoram },
	{ 0x4000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_WRITE_START( usg185_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, MWA_RAM, &generic_nvram, &generic_nvram_size },

	{ 0x2420, 0x2420, lamps1_w },
	{ 0x2430, 0x2430, lamps2_w },

	{ 0x2440, 0x2440, crtc6845_address_w },
	{ 0x2441, 0x2441, crtc6845_register_w },

	{ 0x2000, 0x2000, AY8910_control_port_0_w },
	{ 0x2001, 0x2001, AY8910_write_port_0_w },

	{ 0x2460, 0x2460, usg_rombank_w },

	{ 0x2800, 0x2fff, usg_charram_w, &usg_charram },
	{ 0x3000, 0x3fff, usg_videoram_w, &usg_videoram },
	{ 0x4000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( usg )
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Button 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2, "Button 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3, "Button 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON4, "Button 4", KEYCODE_V, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON5, "Button 5", KEYCODE_B, IP_JOY_DEFAULT )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) // +12 Volts?
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	0x100,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x2800, &charlayout, 0, 256 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,			/* 1 chip */
	2000000,	/* 2 MHz? */
	{ 30 },		/* volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( usg )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, 2000000) /* ?? */
	MDRV_CPU_MEMORY(usg_readmem,usg_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,5) /* ?? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(7*8, 57*8-1, 0*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(2*256)

	MDRV_PALETTE_INIT(usg)
	MDRV_VIDEO_START(usg)
	MDRV_VIDEO_UPDATE(usg)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( usg185 )
	MDRV_IMPORT_FROM(usg)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(usg185_readmem,usg185_writemem)
MACHINE_DRIVER_END



ROM_START( usg32 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "usg32-0.bin", 0x08000, 0x08000, 0xbc313387 )
	/* for the banked region */
	ROM_LOAD( "usg32-1.bin", 0x18000, 0x08000, 0xbaaea800 )
	ROM_LOAD( "usg32-2.bin", 0x28000, 0x08000, 0xd73d7f48 )
	ROM_LOAD( "usg32-3.bin", 0x38000, 0x08000, 0x22747804 )
ROM_END


/* You can't change the status of "Sexy Triv I" and "Sexy Triv II" */
ROM_START( usg83 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "grom08-3.rom", 0x08000, 0x08000, 0xaae84186 )
	/* for the banked region */
	ROM_LOAD( "usg83-1.bin", 0x18000, 0x08000, 0x7b520b6f )
	ROM_LOAD( "usg83-2.bin", 0x28000, 0x08000, 0x29fbb23b )
	ROM_LOAD( "grom3.rom",   0x38000, 0x10000, 0x4e110844 )
	ROM_LOAD( "usg83-4.bin", 0x48000, 0x08000, 0x437697c4 )
ROM_END

/* Similar to 'usg83', but you can change the status of "Sexy Triv I" */
ROM_START( usg83x )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "usg83-0.bin", 0x08000, 0x08000, 0x4ad9b6e0 )
	/* for the banked region */
	ROM_LOAD( "usg83-1.bin", 0x18000, 0x08000, 0x7b520b6f )
	ROM_LOAD( "usg83-2.bin", 0x28000, 0x08000, 0x29fbb23b )
	ROM_LOAD( "usg83-3.bin", 0x38000, 0x08000, 0x41c475ac )
	ROM_LOAD( "usg83-4.bin", 0x48000, 0x08000, 0x437697c4 )
ROM_END

/* Similar to 'usg83', but "Sport Triv" and "Rush Hour" aren't available by default */
ROM_START( usg82 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "rom0.rom",   0x08000, 0x08000, 0x09c20b78 )
	/* for the banked region */
	ROM_LOAD( "grom1.rom",   0x18000, 0x08000, 0x915a9ff4 )
	ROM_LOAD( "usg83-2.bin", 0x28000, 0x08000, 0x29fbb23b )
	ROM_LOAD( "grom3.rom",   0x38000, 0x10000, 0x4e110844 )
	ROM_LOAD( "usg83-4.bin", 0x48000, 0x08000, 0x437697c4 )
ROM_END


ROM_START( usg185 ) // an upgraded 182?
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "usg182.u12",   0x08000, 0x08000, 0x2f4ed125 )
	/* for the banked region */
	ROM_LOAD( "usg185.u36",   0x10000, 0x10000, 0xb104744d ) // ROM 4
	ROM_LOAD( "usg185.u35",   0x20000, 0x10000, 0x795e71c8 ) // ROM 3
	ROM_LOAD( "usg185.u28",   0x30000, 0x10000, 0xc6ba8a81 ) // ROM 2
	ROM_LOAD( "usg185.u18",   0x40000, 0x10000, 0x1cfd934d ) // ROM 1
ROM_END


ROM_START( usg252 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "usg252.u12",   0x08000, 0x08000, 0x766a855a )
	/* for the banked region */
	ROM_LOAD( "usg252.u28",   0x1c000, 0x04000, 0xd44d2ffa )	// ROM 2
	ROM_CONTINUE(             0x18000, 0x04000 )
	ROM_CONTINUE(             0x14000, 0x04000 )
	ROM_CONTINUE(             0x10000, 0x04000 )
	ROM_LOAD( "usg252.u18",   0x2c000, 0x04000, 0x2fff1da2 )	// ROM 1
	ROM_CONTINUE(             0x28000, 0x04000 )
	ROM_CONTINUE(             0x24000, 0x04000 )
	ROM_CONTINUE(             0x20000, 0x04000 )
	ROM_LOAD( "usg252.u36",   0x3c000, 0x04000, 0xb6d007be )	// ROM 4
	ROM_CONTINUE(             0x38000, 0x04000 )
	ROM_CONTINUE(             0x34000, 0x04000 )
	ROM_CONTINUE(             0x30000, 0x04000 )
	ROM_LOAD( "usg252.u35",   0x4c000, 0x04000, 0x9542295b )	// ROM 3
	ROM_CONTINUE(             0x48000, 0x04000 )
	ROM_CONTINUE(             0x44000, 0x04000 )
	ROM_CONTINUE(             0x40000, 0x04000 )
ROM_END



GAME( 1987, usg32,  0,     usg,    usg, 0, ROT0, "U.S. Games", "Super Duper Casino (California V3.2)" )
GAME( 1988, usg83,  0,     usg,    usg, 0, ROT0, "U.S. Games", "Super Ten V8.3" )
GAME( 1988, usg83x, usg83, usg,    usg, 0, ROT0, "U.S. Games", "Super Ten V8.3X" )
GAME( 1988, usg82,  usg83, usg,    usg, 0, ROT0, "U.S. Games", "Super Ten V8.2" )	// "Feb.08,1988"
GAME( 1991, usg185, 0,     usg185, usg, 0, ROT0, "U.S. Games", "Games V18.7C" )
GAME( 1992, usg252, 0,     usg185, usg, 0, ROT0, "U.S. Games", "Games V25.4X" )
