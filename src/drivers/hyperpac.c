/* Hyper Pacman
preliminary driver by David Haywood

heavily based on snowbros.c

Stephh's notes :

  - According to the "Language" Dip Switch, this game is a Korean game.
  - There is no "cocktail mode", nor way to flip the screen.

todo:

verify vidhrdw

make the originals work.
they're probably all this hardware or a varation on it, they don't work
(most point the interrupt vectors directly at a small area of ram which I'd
guess is shared with the Philips 87c52 mcu, more more plus doesn't point the
vectors there but does have a jump there in the code). See hyperpac for an
example, the protection data for that game was extracted from the bootleg.

*/

/* readme (original, bootleg didn't contain one )

Hyper Pac-Man (Not really sure of the name) Manufacturer : SEMICOM

GAME NOT WORKING

Board Mfg date : 23/08/95

Soft Mfg date : 05/09/95

CPU : MC68000P10

Xtal : 12.000000 MHz & 16.000000 MHz

Miscelaneaous :

87c52ebpn (Philips)

Z8400B (Goldstar)

Small sound chip : AD-65 (OKI M6295)

Strange : KA51 (jbbe) 28 pin ic, in 24 pin socket (2 legs
outside socket on each side !!!)

Lattice pLSI 1032-60LJ (a429b04)

Dip SW : 1x8 (but the board has empty space for a second one)

These infos are provided to you by Thierry (ShinobiZ) & Gerald (COY)

*/



#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_UPDATE( hyperpac );


static data16_t *hyperpac_ram;

/*

68k interrupts
lev 1 : 0x64 : 0000 0138 - x
lev 2 : 0x68 : 0000 04fa - ok
lev 3 : 0x6c : 0000 04c8 - ok
lev 4 : 0x70 : 0000 0464 - ok
lev 5 : 0x74 : 0000 0168 - x
lev 6 : 0x78 : 0000 0174 - x
lev 7 : 0x7c : 0000 0180 - x

*/


static INTERRUPT_GEN( hyperpac_interrupt )
{
	cpu_set_irq_line(0, cpu_getiloops()+2, HOLD_LINE);	/* IRQs 4, 3, and 2 */
}


static WRITE16_HANDLER( soundcmd_w )
{
	if (ACCESSING_LSB) soundlatch_w(0,data & 0xff);
}


static MEMORY_READ16_START( hyperpac_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },

	{ 0x500000, 0x500001, input_port_0_word_r },
	{ 0x500002, 0x500003, input_port_1_word_r },
	{ 0x500004, 0x500005, input_port_2_word_r },

	{ 0x600000, 0x6001ff, MRA16_RAM },
	{ 0x700000, 0x701fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( hyperpac_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &hyperpac_ram },
	{ 0x300000, 0x300001, soundcmd_w },
//	{ 0x400000, 0x400001,  }, ???
	{ 0x600000, 0x6001ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x700000, 0x701fff, MWA16_RAM, &spriteram16, &spriteram_size },

	{ 0x800000, 0x800001, MWA16_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA16_NOP },	/* IRQ 3 acknowledge? */
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* IRQ 2 acknowledge? */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf008, 0xf008, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
//	{ 0xf006, 0xf006,  }, ???
MEMORY_END



INPUT_PORTS_START( hyperpac )
	PORT_START	/* 500000.w */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Lives ) )	// "Language" in the "test mode"
	PORT_DIPSETTING(      0x0002, "3" )					// "Korean"
	PORT_DIPSETTING(      0x0000, "5" )					// "English"
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0060, "Normal" )
	PORT_DIPSETTING(      0x0040, "Hard" )
	PORT_DIPSETTING(      0x0020, "Hardest" )			// "Very Hard"
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500002.w */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500004.w */
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 8*32+4, 8*32+0, 20,16, 8*32+20, 8*32+16,
	  12, 8, 8*32+12, 8*32+8, 28, 24, 8*32+28, 8*32+24 },
	{ 0*32, 2*32, 1*32, 3*32, 16*32+0*32, 16*32+2*32, 16*32+1*32, 16*32+3*32,
	  4*32, 6*32, 5*32, 7*32, 16*32+4*32, 16*32+6*32, 16*32+5*32, 16*32+7*32 },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,	/* 4 MHz??? */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 7575 },		/* 7575Hz playback? */
	{ REGION_SOUND1 },
	{ 100 }
};



static MACHINE_DRIVER_START( hyperpac )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 12 or 16 MHz ????? */
	MDRV_CPU_MEMORY(hyperpac_readmem,hyperpac_writemem)
	MDRV_CPU_VBLANK_INT(hyperpac_interrupt,3)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz??? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
//	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(hyperpac)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


ROM_START( hyperpac )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hyperpac.h12", 0x00001, 0x20000, 0x2cf0531a )
	ROM_LOAD16_BYTE( "hyperpac.i12", 0x00000, 0x20000, 0x9c7d85b8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , 0x03faf88e )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, 0xfb9f468d )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, 0xbd8673da )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, 0x5d90cd82 )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, 0x61d86e63 )
ROM_END

ROM_START( hyperpcb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hpacuh12.bin", 0x00001, 0x20000, 0x633ab2c6 )
	ROM_LOAD16_BYTE( "hpacui12.bin", 0x00000, 0x20000, 0x23dc00d1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , 0x03faf88e ) // was missing from this set, using the one from the original

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, 0xfb9f468d )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, 0xbd8673da )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, 0x5d90cd82 )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, 0x61d86e63 )
ROM_END

ROM_START( moremorp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mmp_u52.bin",  0x00001, 0x40000, 0x66baf9b2 )
	ROM_LOAD16_BYTE( "mmp_u74.bin",  0x00000, 0x40000, 0x7c6fede5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "mmp_u35.bin", 0x00000, 0x10000 , 0x4d098cad )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mmp_u14.bin", 0x00000, 0x40000, 0x211a2566 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "mmp_u75.bin", 0x000000, 0x80000, 0xaf9e824e )
	ROM_LOAD( "mmp_u76.bin", 0x080000, 0x80000, 0xc42af064 )
	ROM_LOAD( "mmp_u77.bin", 0x100000, 0x80000, 0x1d7396e1 )
	ROM_LOAD( "mmp_u78.bin", 0x180000, 0x80000, 0x5508d80b )
ROM_END

ROM_START( 3in1semi )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u52",  0x00001, 0x40000, 0xb0e4a0f7 )
	ROM_LOAD16_BYTE( "u74",  0x00000, 0x40000, 0x266862c4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "u35", 0x00000, 0x10000 , 0xe40481da )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u14", 0x00000, 0x40000, 0xc83c11be )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "u75", 0x000000, 0x80000, 0xb66a0db6 )
	ROM_LOAD( "u76", 0x080000, 0x80000, 0x5f4b48ea )
	ROM_LOAD( "u77", 0x100000, 0x80000, 0xd44211e3 )
	ROM_LOAD( "u78", 0x180000, 0x80000, 0xaf596afc )
ROM_END

ROM_START( cookbib2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "cookbib2.01",  0x00000, 0x40000, 0x65aafde2 )
	ROM_LOAD16_BYTE( "cookbib2.02",  0x00001, 0x40000, 0xb2909460 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "cookbib2.07", 0x00000, 0x10000 , 0xf59f1c9a )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "cookbib2.06", 0x00000, 0x20000, 0x5e6f76b8 )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "cookbib2.05", 0x000000, 0x80000, 0x89fb38ce )
	ROM_LOAD( "cookbib2.04", 0x080000, 0x80000, 0xf240111f )
	ROM_LOAD( "cookbib2.03", 0x100000, 0x40000, 0xe1604821 )
ROM_END

/* this one could be more different hw, gfx don't decode the same */
ROM_START( htchctch )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "p03.b16",  0x00001, 0x20000, 0xeff14c40 )
	ROM_LOAD16_BYTE( "p04.b17",  0x00000, 0x20000, 0x6991483a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "p02.b5", 0x00000, 0x10000 , 0xc5a03186 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "p01.c1", 0x00000, 0x20000, 0x18c06829 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "p06srom5.bin", 0x00000, 0x40000, 0x3d2cbb0d )
	ROM_LOAD( "p07srom6.bin", 0x40000, 0x40000, 0x0207949c )
	ROM_LOAD( "p08uor1.bin",  0x80000, 0x20000, 0x6811e7b6 )
	ROM_LOAD( "p09uor2.bin",  0xa0000, 0x20000, 0x1c6549cf )
	ROM_LOAD( "p10uor3.bin",  0xc0000, 0x20000, 0x6462e6e0 )
	ROM_LOAD( "p11uor4.bin",  0xe0000, 0x20000, 0x9c511d98 )
ROM_END

/* cool mini, only 3 roms? */
/* bc story, encrypted?, dif. hardware? */


static DRIVER_INIT( hyperpac )
{
	/* simulate RAM initialization done by the protection MCU */
	/* not verified on real hardware */
	hyperpac_ram[0xe000/2] = 0x4ef9;
	hyperpac_ram[0xe002/2] = 0x0000;
	hyperpac_ram[0xe004/2] = 0x062c;

	hyperpac_ram[0xe080/2] = 0xfedc;
	hyperpac_ram[0xe082/2] = 0xba98;
	hyperpac_ram[0xe084/2] = 0x7654;
	hyperpac_ram[0xe086/2] = 0x3210;
}


GAME( 1995, hyperpac, 0,        hyperpac, hyperpac, hyperpac, ROT0, "SemiCom", "Hyper Pacman" )
GAME( 1995, hyperpcb, hyperpac, hyperpac, hyperpac, 0,        ROT0, "bootleg", "Hyper Pacman (bootleg)" )

/* the following don't work, they either point the interrupts at an area of ram probably shared by
   some kind of mcu which puts 68k code there, or jump to the area in the interrupts */
GAMEX(199?, moremorp, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "More More +", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1997, 3in1semi, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "3-in-1 (SemiCom)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1996, cookbib2, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "Cookie and Bibi 2", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1995, htchctch, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "Hatch Catch", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
