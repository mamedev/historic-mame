/***************************************************************************

							-= Clash Road =-

					driver by	Luca Elia (l.elia@tin.it)

Main  CPU   :	Z80A

Video Chips :	?

Sound CPU   :	Z80A

Sound Chips :	Custom (NAMCO?)

To Do:

- Colors
- Sound

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data8_t *clshroad_sharedram;

/* Variables & functions defined in vidhrdw: */

extern data8_t *clshroad_vram_0, *clshroad_vram_1;
extern data8_t *clshroad_vregs;

WRITE_HANDLER( clshroad_vram_0_w );
WRITE_HANDLER( clshroad_vram_1_w );
WRITE_HANDLER( clshroad_flipscreen_w );

PALETTE_INIT( firebatl );
VIDEO_START( firebatl );
VIDEO_START( clshroad );
VIDEO_UPDATE( clshroad );

extern unsigned char *wiping_soundregs;
int wiping_sh_start(const struct MachineSound *msound);
void wiping_sh_stop(void);
WRITE_HANDLER( wiping_sound_w );



MACHINE_INIT( clshroad )
{
	flip_screen_set(0);
}


/* Shared RAM with the sound CPU */

READ_HANDLER ( clshroad_sharedram_r )	{	return clshroad_sharedram[offset];	}
WRITE_HANDLER( clshroad_sharedram_w )	{	clshroad_sharedram[offset] = data;	}

READ_HANDLER( clshroad_input_r )
{
	return	((~readinputport(0) & (1 << offset)) ? 1 : 0) |
			((~readinputport(1) & (1 << offset)) ? 2 : 0) |
			((~readinputport(2) & (1 << offset)) ? 4 : 0) |
			((~readinputport(3) & (1 << offset)) ? 8 : 0) ;
}


static MEMORY_READ_START( clshroad_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	// ROM
	{ 0x8000, 0x95ff, MRA_RAM				},	// Work   RAM
	{ 0x9600, 0x97ff, clshroad_sharedram_r	},	// Shared RAM
	{ 0x9800, 0x9dff, MRA_RAM				},	// Work   RAM
	{ 0x9e00, 0x9fff, MRA_RAM				},	// Sprite RAM
	{ 0xa100, 0xa107, clshroad_input_r		},	// Inputs
	{ 0xa800, 0xafff, MRA_RAM				},	// Layer  1
	{ 0xc000, 0xc7ff, MRA_RAM				},	// Layers 0
MEMORY_END

static MEMORY_WRITE_START( clshroad_writemem )
	{ 0x0000, 0x7fff, MWA_ROM									},	// ROM
	{ 0x8000, 0x95ff, MWA_RAM									},	// Work   RAM
	{ 0x9600, 0x97ff, clshroad_sharedram_w, &clshroad_sharedram	},	// Shared RAM
	{ 0x9800, 0x9dff, MWA_RAM									},	// Work   RAM
	{ 0x9e00, 0x9fff, MWA_RAM, &spriteram, &spriteram_size		},	// Sprite RAM
	{ 0xa001, 0xa001, MWA_NOP									},	// ? Interrupt related
	{ 0xa004, 0xa004, clshroad_flipscreen_w						},	// Flip Screen
	{ 0xa800, 0xafff, clshroad_vram_1_w, &clshroad_vram_1		},	// Layer 1
	{ 0xb000, 0xb003, MWA_RAM, &clshroad_vregs					},	// Scroll
	{ 0xc000, 0xc7ff, clshroad_vram_0_w, &clshroad_vram_0		},	// Layers 0
MEMORY_END

static MEMORY_READ_START( clshroad_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM				},	// ROM
	{ 0x9600, 0x97ff, clshroad_sharedram_r	},	// Shared RAM
MEMORY_END

static MEMORY_WRITE_START( clshroad_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM				},	// ROM
	{ 0x4000, 0x7fff, wiping_sound_w, &wiping_soundregs },
	{ 0x9600, 0x97ff, clshroad_sharedram_w	},	// Shared RAM
	{ 0xa003, 0xa003, MWA_NOP				},	// ? Interrupt related
MEMORY_END



INPUT_PORTS_START( clshroad )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - DSW 1
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )	// Damage when falling
	PORT_DIPSETTING(    0x18, "Normal"  )	// 8
	PORT_DIPSETTING(    0x10, "Hard"    )	// A
	PORT_DIPSETTING(    0x08, "Harder"  )	// C
	PORT_DIPSETTING(    0x00, "Hardest" )	// E
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN3 - DSW 2
/*
first bit OFF is:	0 			0	<- value
					1			1
					2			2
					3			3
					4			4
					5			5
					6			6
					else		FF

But the values seems unused then.
*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )	//?
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout layout_8x8x2 =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ STEP4(0,1), STEP4(8,1) },
	{ STEP8(0,8*2) },
	8*8*2
};

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2) + 0, RGN_FRAC(1,2) + 4, 0, 4 },
	{ STEP4(0,1), STEP4(8,1) },
	{ STEP8(0,8*2) },
	8*8*2
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2) + 0, RGN_FRAC(1,2) + 4, 0, 4 },
	{ STEP4(0,1), STEP4(8,1), STEP4(8*8*2+0,1), STEP4(8*8*2+8,1) },
	{ STEP8(0,8*2), STEP8(8*8*2*2,8*2) },
	16*16*2
};

static struct GfxDecodeInfo firebatl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,   0, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 16, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x2,   512, 64 }, // [2] Layer 1
	{ -1 }
};

static struct GfxDecodeInfo clshroad_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4, 0, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,   0, 16 }, // [2] Layer 1
	{ -1 }
};



static struct CustomSound_interface custom_interface =
{
	wiping_sh_start,
	wiping_sh_stop,
	0
};



static MACHINE_DRIVER_START( firebatl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000)	/* ? */
	MDRV_CPU_MEMORY(clshroad_readmem,clshroad_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* IRQ, no NMI */

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(clshroad_sound_readmem,clshroad_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* IRQ, no NMI */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(clshroad)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x120, 0x100)
	MDRV_VISIBLE_AREA(0, 0x120-1, 0x0+16, 0x100-16-1)
	MDRV_GFXDECODE(firebatl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512+64*4)

	MDRV_PALETTE_INIT(firebatl)
	MDRV_VIDEO_START(firebatl)
	MDRV_VIDEO_UPDATE(clshroad)

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( clshroad )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000)	/* ? */
	MDRV_CPU_MEMORY(clshroad_readmem,clshroad_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* IRQ, no NMI */

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(clshroad_sound_readmem,clshroad_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* IRQ, no NMI */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(clshroad)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x120, 0x100)
	MDRV_VISIBLE_AREA(0, 0x120-1, 0x0+16, 0x100-16-1)
	MDRV_GFXDECODE(clshroad_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(clshroad)
	MDRV_VIDEO_UPDATE(clshroad)

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END



ROM_START( firebatl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "rom01",       0x00000, 0x2000, 0x10e24ef6 )
	ROM_LOAD( "rom02",       0x02000, 0x2000, 0x47f79bee )
	ROM_LOAD( "rom03",       0x04000, 0x2000, 0x693459b9 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "rom04",       0x0000, 0x2000, 0x5f232d9a )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "rom14",       0x0000, 0x2000, 0x36a508a7 )
	ROM_LOAD( "rom13",       0x2000, 0x2000, 0xa2ec508e )
	ROM_LOAD( "rom12",       0x4000, 0x2000, 0xf80ece92 )
	ROM_LOAD( "rom11",       0x6000, 0x2000, 0xb293e701 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "rom09",       0x0000, 0x2000, 0x77ea3e39 )
	ROM_LOAD( "rom08",       0x2000, 0x2000, 0x1b7585dd )
	ROM_LOAD( "rom07",       0x4000, 0x2000, 0xe3ec9825 )
	ROM_LOAD( "rom06",       0x6000, 0x2000, 0xd29fab5f )

	ROM_REGION( 0x01000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "rom15",       0x0000, 0x1000, 0x8b5464d6 )

	ROM_REGION( 0x0a20, REGION_PROMS, 0 )
	ROM_LOAD( "prom6.bpr",   0x0000, 0x0100, 0xb117d22c )	/* palette red? */
	ROM_LOAD( "prom7.bpr",   0x0100, 0x0100, 0x9b6b4f56 )	/* palette green? */
	ROM_LOAD( "prom8.bpr",   0x0200, 0x0100, 0x67cb68ae )	/* palette blue? */
	ROM_LOAD( "prom9.bpr",   0x0300, 0x0100, 0xdd015b80 )	/* char lookup table msb? */
	ROM_LOAD( "prom10.bpr",  0x0400, 0x0100, 0x71b768c7 )	/* char lookup table lsb? */
	ROM_LOAD( "prom4.bpr",   0x0500, 0x0100, 0x06523b81 )	/* unknown */
	ROM_LOAD( "prom5.bpr",   0x0600, 0x0100, 0x75ea8f70 )	/* unknown */
	ROM_LOAD( "prom11.bpr",  0x0700, 0x0100, 0xba42a582 )	/* unknown */
	ROM_LOAD( "prom12.bpr",  0x0800, 0x0100, 0xf2540c51 )	/* unknown */
	ROM_LOAD( "prom13.bpr",  0x0900, 0x0100, 0x4e2a2781 )	/* unknown */
	ROM_LOAD( "prom1.bpr",   0x0a00, 0x0020, 0x1afc04f0 )	/* timing? (on the cpu board) */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "rom05",       0x0000, 0x2000, 0x21544cd6 )

	ROM_REGION( 0x0200, REGION_SOUND2, 0 )	/* 4bit->8bit sample expansion PROMs */
	ROM_LOAD( "prom3.bpr",   0x0000, 0x0100, 0xbd2c080b )	/* low 4 bits */
	ROM_LOAD( "prom2.bpr",   0x0100, 0x0100, 0x4017a2a6 )	/* high 4 bits */
ROM_END

ROM_START( clshroad )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "clashr3.bin", 0x0000, 0x8000, 0x865c32ae )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "clashr2.bin", 0x0000, 0x2000, 0xe6389ec1 )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "clashr6.bin", 0x0000, 0x4000, 0xdaa1daf3 )
	ROM_LOAD( "clashr5.bin", 0x4000, 0x4000, 0x094858b8 )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "clashr9.bin", 0x0000, 0x4000, 0xc15e8eed )
	ROM_LOAD( "clashr8.bin", 0x4000, 0x4000, 0xcbb66719 )

	ROM_REGION( 0x04000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "clashr7.bin", 0x0000, 0x2000, 0x97973030 )
	ROM_LOAD( "clashr4.bin", 0x2000, 0x2000, 0x664201d9 )

	ROM_REGION( 0x0b40, REGION_PROMS, 0 )
	/* all other proms that firebatl has are missing */
	ROM_LOAD( "clashrd.a2",  0x0900, 0x0100, 0x4e2a2781 )	/* unknown */
	ROM_LOAD( "clashrd.g4",  0x0a00, 0x0020, 0x1afc04f0 )	/* timing? */
	ROM_LOAD( "clashrd.b11", 0x0a20, 0x0020, 0xd453f2c5 )	/* unknown (possibly bad dump) */
	ROM_LOAD( "clashrd.g10", 0x0a40, 0x0100, 0x73afefd0 )	/* unknown (possibly bad dump) */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "clashr1.bin", 0x0000, 0x2000, 0x0d0a8068 )

	ROM_REGION( 0x0200, REGION_SOUND2, 0 )	/* 4bit->8bit sample expansion PROMs */
	ROM_LOAD( "clashrd.g8",  0x0000, 0x0100, 0xbd2c080b )	/* low 4 bits */
	ROM_LOAD( "clashrd.g7",  0x0100, 0x0100, 0x4017a2a6 )	/* high 4 bits */
ROM_END



GAMEX( 1984, firebatl, 0, firebatl, clshroad, 0, ROT90, "Taito", "Fire Battle", GAME_NOT_WORKING | GAME_WRONG_COLORS )
GAMEX( 1986, clshroad, 0, clshroad, clshroad, 0, ROT0,  "Woodplace Inc.", "Clash Road", GAME_WRONG_COLORS )
