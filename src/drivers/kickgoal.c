/* Driver Info


Kick Goal (c)1995 TCH
Hollywood Action (c)1995 TCH

 prelim driver by David Haywood


todo:

Sound - Not possible without PIC dump?
  the PIC is protected, sound will have to be simulated
  the kickgoal sound rom is also bad.

Should the screen size really be doubled in kickgoal or should the fg tiles be 8bpp instead
because otherwise these don't seem much like the same hardware..

you get 64 credits in hwaction (eeprom bug?)
service mode doesn't work in hwaction (reads from unmapped memory, probably a mirror of something)


*/

/* Notes

68k interrupts
lev 1 : 0x64 : 0000 0000 - x
lev 2 : 0x68 : 0000 0000 - x
lev 3 : 0x6c : 0000 0000 - x
lev 4 : 0x70 : 0000 0000 - x
lev 5 : 0x74 : 0000 0000 - x
lev 6 : 0x78 : 0000 0510 - vblank?
lev 7 : 0x7c : 0000 0000 - x

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "sound/okim6295.h"


data16_t *kickgoal_fgram, *kickgoal_bgram, *kickgoal_bg2ram, *kickgoal_scrram;

WRITE16_HANDLER( kickgoal_fgram_w  );
WRITE16_HANDLER( kickgoal_bgram_w  );
WRITE16_HANDLER( kickgoal_bg2ram_w );

VIDEO_START( kickgoal );
VIDEO_UPDATE( kickgoal );

VIDEO_START( hwaction );
VIDEO_UPDATE( hwaction );



static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	0,				/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx"	/* unlock command */
};

static NVRAM_HANDLER( kickgoal )
{
	if (read_or_write) EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file) EEPROM_load(file);
	}
}


static READ16_HANDLER( kickgoal_eeprom_r )
{
	return EEPROM_read_bit();
}


static WRITE16_HANDLER( kickgoal_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		switch (offset)
		{
			case 0:
				EEPROM_set_cs_line((data & 0x0001) ? CLEAR_LINE : ASSERT_LINE);
				break;
			case 1:
				EEPROM_set_clock_line((data & 0x0001) ? ASSERT_LINE : CLEAR_LINE);
				break;
			case 2:
				EEPROM_write_bit(data & 0x0001);
				break;
		}
	}
}


/* Memory Maps ****************************************************************

it doesn't seem able to read from fg/bg/spr/pal ram

*/

static ADDRESS_MAP_START( kickgoal_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x800002, 0x800003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x900006, 0x900007) AM_READ(kickgoal_eeprom_r)
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xc00000, 0xc007ff) AM_READ(MRA16_RAM) // hwaction needs this
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kickgoal_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x800004, 0x800005) AM_WRITE(soundlatch_word_w)
	AM_RANGE(0x900000, 0x900005) AM_WRITE(kickgoal_eeprom_w)
	AM_RANGE(0xa00000, 0xa03fff) AM_WRITE(kickgoal_fgram_w) AM_BASE(&kickgoal_fgram) /* FG Layer */
	AM_RANGE(0xa04000, 0xa07fff) AM_WRITE(kickgoal_bgram_w) AM_BASE(&kickgoal_bgram) /* Higher BG Layer */
	AM_RANGE(0xa08000, 0xa0bfff) AM_WRITE(kickgoal_bg2ram_w) AM_BASE(&kickgoal_bg2ram) /* Lower BG Layer */
	AM_RANGE(0xa0c000, 0xa0ffff) AM_WRITE(MWA16_RAM) // more tilemap?
	AM_RANGE(0xa10000, 0xa1000f) AM_WRITE(MWA16_RAM) AM_BASE(&kickgoal_scrram) /* Scroll Registers */
	AM_RANGE(0xb00000, 0xb007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size) /* Sprites */
	AM_RANGE(0xc00000, 0xc007ff) AM_WRITE(paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16) /* Palette */
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

/* INPUT ports ***************************************************************/

INPUT_PORTS_START( kickgoal )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x0800, IP_ACTIVE_LOW)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* GFX Decodes ***************************************************************/

static struct GfxLayout fg816_charlayout =
{
	8,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static struct GfxLayout bg1632_charlayout =
{
	16,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 0*16, 1*16, 1*16, 2*16,  2*16,  3*16,  3*16,  4*16,  4*16,  5*16,  5*16,  6*16,  6*16,  7*16, 7*16,
	  8*16, 8*16, 9*16, 9*16, 10*16, 10*16, 11*16, 11*16, 12*16, 12*16, 13*16, 13*16, 14*16, 14*16, 15*16, 15*16 },
	16*16
};

static struct GfxLayout bg3264_charlayout =
{
	32,64,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	{ 0*32,  0*32,  1*32,  1*32,  2*32,  2*32,  3*32,  3*32,  4*32,  4*32,  5*32,  5*32,  6*32,  6*32,  7*32,  7*32,
	  8*32,  8*32,  9*32,  9*32,  10*32, 10*32, 11*32, 11*32, 12*32, 12*32, 13*32, 13*32, 14*32, 14*32, 15*32, 15*32,
	  16*32, 16*32, 17*32, 17*32, 18*32, 18*32, 19*32, 19*32, 20*32, 20*32, 21*32, 21*32, 22*32, 22*32, 23*32, 23*32,
	  24*32, 24*32, 25*32, 25*32, 26*32, 26*32, 27*32, 27*32, 28*32, 28*32, 29*32, 29*32, 30*32, 30*32, 31*32, 31*32 },
	32*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fg816_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &bg1632_charlayout,  0x000, 0x40 },
	{ REGION_GFX1, 0, &bg3264_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static struct GfxLayout hwaction_fg88_alt_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8,  2*8,  3*8,  4*8, 5*8, 6*8,  7*8 },
	8*8
};


static struct GfxLayout hwaction_bg1616_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16,  1*16,  2*16,  3*16,   4*16,   5*16,   6*16,   7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};



static struct GfxDecodeInfo hwaction_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &hwaction_fg88_alt_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &hwaction_bg1616_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

/* MACHINE drivers ***********************************************************/



static MACHINE_DRIVER_START( kickgoal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(kickgoal_readmem,kickgoal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* pic16c57? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(9*8, 55*8-1, 4*8, 60*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(kickgoal)
	MDRV_VIDEO_UPDATE(kickgoal)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
//  MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hwaction )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(kickgoal_readmem,kickgoal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* pic16c57? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal) // 93C46 really

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(10*8+2, 54*8-1+2, 2*8, 30*8-1)
	MDRV_GFXDECODE(hwaction_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(hwaction)
	MDRV_VIDEO_UPDATE(hwaction)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
//  MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



/* Rom Loading ***************************************************************/

ROM_START( kickgoal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ic6",   0x000000, 0x40000, CRC(498ca792) SHA1(c638c3a1755870010c5961b58bcb02458ff4e238) )
	ROM_LOAD16_BYTE( "ic5",   0x000001, 0x40000, CRC(d528740a) SHA1(d56a71004aabc839b0833a6bf383e5ef9d4948fa) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound? (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic33",   0x000000, 0x80000, CRC(5038f52a) SHA1(22ed0e2c8a99056e73cff912731626689996a276) )
	ROM_LOAD( "ic34",   0x080000, 0x80000, CRC(06e7094f) SHA1(e41b893ef91d541d2623d76ce6c69ecf4218c16d) )
	ROM_LOAD( "ic35",   0x100000, 0x80000, CRC(ea010563) SHA1(5e474db372550e9d33f933ab00881a9b29a712d1) )
	ROM_LOAD( "ic36",   0x180000, 0x80000, CRC(b6a86860) SHA1(73ab43830d5e62154bc8953615cdb397c7a742aa) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic13",   0x00000, 0x080000, BAD_DUMP CRC(c6cb56e9) SHA1(835773b3f0647d3c553180bcf10e57ad44d68353) ) // BAD ADDRESS LINES (mask=010000)
ROM_END

ROM_START( holywact )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2.ic6",  0x000000, 0x80000, CRC(2b71d58c) SHA1(3e58531fa56d41a3c7944e3beab4850907564a89) )
	ROM_LOAD16_BYTE( "1.ic5",  0x000001, 0x80000, CRC(136b9711) SHA1(553f9fdd99bb9ce2e1492d0755633075e59ba587) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound? (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.ic29",  0x000000, 0x80000, CRC(df076744) SHA1(4b2c8e21a201e1491e4ba3cda8d71b51e0943431) )
	ROM_LOAD( "5.ic33",  0x080000, 0x80000, CRC(8551fdd4) SHA1(f29bdfb75af7607534de171d7b3927419c00377c) )
	ROM_LOAD( "6.ic30",  0x100000, 0x80000, CRC(5cb005a5) SHA1(d3a5ab8f9a520bfaa53fdf6145142ccba416fbb8) )
	ROM_LOAD( "7.ic34",  0x180000, 0x80000, CRC(c2f7d284) SHA1(b3c3d6aa932c813affd667344ea5ddefa55f219b) )
	ROM_LOAD( "8.ic31",  0x200000, 0x80000, CRC(50dffa47) SHA1(33da3b2cabb7b0e480158d343e876563bd0f0930) )
	ROM_LOAD( "9.ic35",  0x280000, 0x80000, CRC(c1ea0370) SHA1(c836611e478d2bf9ae2a5d7e7665982c2b731189) )
	ROM_LOAD( "10.ic32", 0x300000, 0x80000, CRC(5ee5db3e) SHA1(c79f84548ce5311acac478c5180330bf56485863) )
	ROM_LOAD( "11.ic36", 0x380000, 0x80000, CRC(8d376b1e) SHA1(37f16b3237d9813a8d153ab5640252e7643f3b99) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "3.ic13",   0x00000, 0x080000, CRC(b8f6705d) SHA1(55116e14aba6dac7334e26f704b3e6b0b9f856c2) )
ROM_END

/* GAME drivers **************************************************************/

DRIVER_INIT( kickgoal )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);

	/* fix "bug" that prevents game from writing to EEPROM */
	rom[0x12b0/2] = 0x0001;
}


GAMEX( 1995, kickgoal,0, kickgoal, kickgoal, kickgoal, ROT0, "TCH", "Kick Goal", GAME_NO_SOUND )
GAMEX( 1995, holywact,0, hwaction, kickgoal, 0, ROT0, "TCH", "Hollywood Action", GAME_NO_SOUND )
