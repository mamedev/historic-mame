/***************************************************************************

Zero Hour / Red Clash

runs on hardware similar to Lady Bug

driver by inkling

Notes:
- In the Tehkan set the ship doesn't move during attract mode. Earlier version?
  Gameplay is different too.

TODO:
- Missing Galaxian-like starfield (speed is controlled by three output ports)

- Colors might be right, need screen shots to verify

- Some graphical problems in both games, but without screenshots its hard to
  know what we're aiming for

- Sound (analog, schematics available for Zero Hour)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern data8_t *redclash_textram;

PALETTE_INIT( redclash );
VIDEO_UPDATE( redclash );

WRITE_HANDLER( redclash_gfxbank_w );
WRITE_HANDLER( redclash_flipscreen_w );

WRITE_HANDLER( redclash_star0_w );
WRITE_HANDLER( redclash_star1_w );
WRITE_HANDLER( redclash_star2_w );
WRITE_HANDLER( redclash_star_reset_w );


/*
  This game doesn't have VBlank interrupts.
  Interrupts are still used, but they are related to coin
  slots. Left slot generates an IRQ, Right slot a NMI.
*/
INTERRUPT_GEN( redclash_interrupt )
{
	if (readinputport(4) & 1)	/* Left Coin */
		cpu_set_irq_line(0,0,ASSERT_LINE);
	else if (readinputport(4) & 2)	/* Right Coin */
		cpu_set_nmi_line(0,PULSE_LINE);
}

static WRITE_HANDLER( irqack_w )
{
	cpu_set_irq_line(0,0,CLEAR_LINE);
}



static MEMORY_READ_START( zero_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x4800, 0x4800, input_port_0_r }, /* IN0 */
	{ 0x4801, 0x4801, input_port_1_r }, /* IN1 */
	{ 0x4802, 0x4802, input_port_2_r }, /* DSW0 */
	{ 0x4803, 0x4803, input_port_3_r }, /* DSW1 */
	{ 0x4000, 0x43ff, MRA_RAM },  /* video RAM */
MEMORY_END

static MEMORY_WRITE_START( zero_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3bff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x4000, 0x43ff, MWA_RAM, &redclash_textram },
	{ 0x5000, 0x5007, MWA_NOP },	/* to sound board */
	{ 0x5800, 0x5800, redclash_star0_w },
	{ 0x5801, 0x5804, MWA_NOP },	/* to sound board */
	{ 0x5805, 0x5805, redclash_star1_w },
	{ 0x5806, 0x5806, redclash_star2_w },
	{ 0x5807, 0x5807, redclash_flipscreen_w },
	{ 0x7000, 0x7000, redclash_star_reset_w },
	{ 0x7800, 0x7800, irqack_w },
MEMORY_END

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4800, 0x4800, input_port_0_r }, /* IN0 */
	{ 0x4801, 0x4801, input_port_1_r }, /* IN1 */
	{ 0x4802, 0x4802, input_port_2_r }, /* DSW0 */
	{ 0x4803, 0x4803, input_port_3_r }, /* DSW1 */
	{ 0x4000, 0x43ff, MRA_RAM },  /* video RAM */
	{ 0x6000, 0x67ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
//	{ 0x3000, 0x3000, MWA_NOP },
//	{ 0x3800, 0x3800, MWA_NOP },
	{ 0x4000, 0x43ff, MWA_RAM, &redclash_textram },
	{ 0x5000, 0x5007, MWA_NOP },	/* to sound board */
	{ 0x5800, 0x5800, redclash_star0_w },
	{ 0x5801, 0x5801, redclash_gfxbank_w },
	{ 0x5805, 0x5805, redclash_star1_w },
	{ 0x5806, 0x5806, redclash_star2_w },
	{ 0x5807, 0x5807, redclash_flipscreen_w },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x6800, 0x6bff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7000, 0x7000, redclash_star_reset_w },
	{ 0x7800, 0x7800, irqack_w },
MEMORY_END



INPUT_PORTS_START( redclash )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?" )
	PORT_DIPSETTING(    0x03, "Easy?" )
	PORT_DIPSETTING(    0x02, "Medium?" )
	PORT_DIPSETTING(    0x01, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "High Score" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x40, "7" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_9C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_9C ) )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 8*8, 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout spritelayout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2) },
	{ STEP8(7*16,-16) },
	16*8
};

static struct GfxLayout spritelayout16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(24*2,2), STEP8(8*64+24*2,2) },
	{ STEP8(23*64,-64), STEP8(7*64,-64) },
	64*32
};

static struct GfxLayout spritelayout24x24 =
{
	24,24,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2), STEP8(8*2,2), STEP8(16*2,2) },
	{ STEP8(23*64,-64), STEP8(15*64,-64), STEP8(7*64,-64) },
	64*32
};

static struct GfxLayout spritelayout16x16bis =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 1, 0 },
	{ STEP8(0,2), STEP8(8*2,2) },
	{ STEP8(15*64,-64), STEP8(7*64,-64) },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,          0,  8 },
	{ REGION_GFX3, 0x0000, &spritelayout8x8,   4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout16x16, 4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout24x24, 4*8, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout16x16bis, 4*8, 16 },
	{ REGION_GFX2, 0x0004, &spritelayout16x16bis, 4*8, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( zerohour )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)  /* 4 MHz */
	MDRV_CPU_MEMORY(zero_readmem,zero_writemem)
	MDRV_CPU_VBLANK_INT(redclash_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(4*24)

	MDRV_PALETTE_INIT(redclash)
	MDRV_VIDEO_UPDATE(redclash)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( redclash )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)  /* 4 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(redclash_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(4*24)

	MDRV_PALETTE_INIT(redclash)
	MDRV_VIDEO_UPDATE(redclash)

	/* sound hardware */
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zerohour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zerohour.1",   0x0000, 0x0800, 0x0dff4b48 )
	ROM_LOAD( "zerohour.2",   0x0800, 0x0800, 0xcf41b6ac )
	ROM_LOAD( "zerohour.3",	  0x1000, 0x0800, 0x5ef48b67 )
	ROM_LOAD( "zerohour.4",	  0x1800, 0x0800, 0x25c5872d )
	ROM_LOAD( "zerohour.5",	  0x2000, 0x0800, 0xd7ce3add )
	ROM_LOAD( "zerohour.6",	  0x2800, 0x0800, 0x8a93ae6e )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zerohour.9",   0x0000, 0x0800, 0x17ae6f13 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zerohour.7",	  0x0000, 0x0800, 0x4c12f59d )
	ROM_LOAD( "zerohour.8",	  0x0800, 0x0800, 0x6b9a6b6e )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "zerohour.ic2", 0x0000, 0x0020, 0xb55aee56 ) /* palette */
	ROM_LOAD( "zerohour.n2",  0x0020, 0x0020, 0x9adabf46 ) /* sprite color lookup table */
	ROM_LOAD( "zerohour.u6",  0x0040, 0x0020, 0x27fa3a50 ) /* ?? */
ROM_END

ROM_START( redclash )
	ROM_REGION(0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "rc1.11c",      0x0000, 0x1000, 0x5b62ff5a )
	ROM_LOAD( "rc3.7c",       0x1000, 0x1000, 0x409c4ee7 )
	ROM_LOAD( "rc2.9c",       0x2000, 0x1000, 0x5f215c9a )

	ROM_REGION(0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc6.12a",      0x0000, 0x0800, 0xda9bbcc2 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rc4.3e",       0x0000, 0x0800, 0x64ca8b63 )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "rc5.3d",       0x0800, 0x0800, 0xfce610a2 )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1.12f",   0x0000, 0x0020, 0x43989681 ) /* palette */
	ROM_LOAD( "6331-2.4a",    0x0020, 0x0020, 0x9adabf46 ) /* sprite color lookup table */
	ROM_LOAD( "6331-3.11e",   0x0040, 0x0020, 0x27fa3a50 ) /* ?? */
ROM_END

ROM_START( redclask )
	ROM_REGION(0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "rc1.8c",       0x0000, 0x0800, 0xfd90622a )
	ROM_LOAD( "rc2.7c",       0x0800, 0x0800, 0xc8f33440 )
	ROM_LOAD( "rc3.6c",       0x1000, 0x0800, 0x2172b1e9 )
	ROM_LOAD( "rc4.5c",       0x1800, 0x0800, 0x55c0d1b5 )
	ROM_LOAD( "rc5.4c",       0x2000, 0x0800, 0x744b5261 )
	ROM_LOAD( "rc6.3c",       0x2800, 0x0800, 0xfa507e17 )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rc6.12a",      0x0000, 0x0800, 0xda9bbcc2 ) /* rc9.7m */

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rc4.4m",       0x0000, 0x0800, 0x483a1293 )
	ROM_CONTINUE(             0x1000, 0x0800 )
	ROM_LOAD( "rc5.5m",       0x0800, 0x0800, 0xc45d9601 )
	ROM_CONTINUE(             0x1800, 0x0800 )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	/* gfx data will be rearranged here for 8x8 sprites */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1.12f",   0x0000, 0x0020, 0x43989681 ) /* 6331.7e */
	ROM_LOAD( "6331-2.4a",    0x0020, 0x0020, 0x9adabf46 ) /* 6331.2r */
	ROM_LOAD( "6331-3.11e",   0x0040, 0x0020, 0x27fa3a50 ) /* 6331.6w */
ROM_END

static DRIVER_INIT( redclash )
{
	int i,j;

	/* rearrange the sprite graphics */
	for (i = 0;i < memory_region_length(REGION_GFX3);i++)
	{
		j = (i & ~0x003e) | ((i & 0x0e) << 2) | ((i & 0x30) >> 3);
		memory_region(REGION_GFX3)[i] = memory_region(REGION_GFX2)[j];
	}
}



GAMEX( 1980?,zerohour, 0,        zerohour, redclash, redclash, ROT270, "Universal", "Zero Hour",          GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1981, redclash, 0,        redclash, redclash, redclash, ROT270, "Tehkan",    "Red Clash",          GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1981, redclask, redclash, redclash, redclash, redclash, ROT270, "Kaneko",    "Red Clash (Kaneko)", GAME_NO_SOUND | GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
