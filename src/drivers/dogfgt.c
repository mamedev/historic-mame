/***************************************************************************

Dog-Fight / Batten O'hara no Sucha-Raka Kuuchuu Sen
(c) 1984 Technos Japan

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "dogfgt.h"


static data8_t *sharedram;

static READ_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE_HANDLER( sharedram_w )
{
	sharedram[offset] = data;
}


static WRITE_HANDLER( subirqtrigger_w )
{
	/* bit 0 used but unknown */

	if (data & 0x04)
		cpu_set_irq_line(1,0,ASSERT_LINE);
}

static WRITE_HANDLER( sub_irqack_w )
{
	cpu_set_irq_line(1,0,CLEAR_LINE);
}


static int soundlatch;

static WRITE_HANDLER( dogfgt_soundlatch_w )
{
	soundlatch = data;
}

static WRITE_HANDLER( dogfgt_soundcontrol_w )
{
	static int last;


	/* bit 5 goes to 8910 #0 BDIR pin  */
	if ((last & 0x20) == 0x20 && (data & 0x20) == 0x00)
	{
		/* bit 4 goes to the 8910 #0 BC1 pin */
		if (last & 0x10)
			AY8910_control_port_0_w(0,soundlatch);
		else
			AY8910_write_port_0_w(0,soundlatch);
	}
	/* bit 7 goes to 8910 #1 BDIR pin  */
	if ((last & 0x80) == 0x80 && (data & 0x80) == 0x00)
	{
		/* bit 6 goes to the 8910 #1 BC1 pin */
		if (last & 0x40)
			AY8910_control_port_1_w(0,soundlatch);
		else
			AY8910_write_port_1_w(0,soundlatch);
	}

	last = data;
}



static MEMORY_READ_START( main_readmem )
	{ 0x0000, 0x07ff, sharedram_r },
	{ 0x1800, 0x1800, input_port_0_r },
	{ 0x1810, 0x1810, input_port_1_r },
	{ 0x1820, 0x1820, input_port_2_r },
	{ 0x1830, 0x1830, input_port_3_r },
	{ 0x2000, 0x3fff, dogfgt_bitmapram_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( main_writemem )
	{ 0x0000, 0x07ff, sharedram_w, &sharedram },
	{ 0x0f80, 0x0fdf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1000, 0x17ff, dogfgt_bgvideoram_w, &dogfgt_bgvideoram },
	{ 0x1800, 0x1800, dogfgt_1800_w },	/* text color, flip screen & coin counters */
	{ 0x1810, 0x1810, subirqtrigger_w },
	{ 0x1820, 0x1823, dogfgt_scroll_w },
	{ 0x1824, 0x1824, dogfgt_plane_select_w },
	{ 0x1830, 0x1830, dogfgt_soundlatch_w },
	{ 0x1840, 0x1840, dogfgt_soundcontrol_w },
	{ 0x1870, 0x187f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0x2000, 0x3fff, dogfgt_bitmapram_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x27ff, sharedram_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x27ff, sharedram_w },
	{ 0x4000, 0x4000, sub_irqack_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( dogfgt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN2, 1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x80, "Upright 2 Players" )
//	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )		// "Cocktail 1 Player" - IMPOSSIBLE !
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )		// "Cocktail 2 Players"

	PORT_START	/* DSW2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   16, 4 },
	{ REGION_GFX2, 0, &spritelayout,  0, 2 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 30, 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( dogfgt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1500000)	/* 1.5 MHz ???? */
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,16)	/* ? controls music tempo */

	MDRV_CPU_ADD(M6502, 1500000)	/* 1.5 MHz ???? */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16+64)

	MDRV_PALETTE_INIT(dogfgt)
	MDRV_VIDEO_START(dogfgt)
	MDRV_VIDEO_UPDATE(dogfgt)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



ROM_START( dogfgt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bx00.52",     0x8000, 0x2000, 0xe602a21c )
	ROM_LOAD( "bx01.37",     0xa000, 0x2000, 0x4921c4fb )
	ROM_LOAD( "bx02.36",     0xc000, 0x2000, 0x91f1b9b3 )
	ROM_LOAD( "bx03.22",     0xe000, 0x2000, 0x959ebf93 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for audio code */
	ROM_LOAD( "bx04.117",    0x8000, 0x2000, 0xf8945f9d )
	ROM_LOAD( "bx05.118",    0xa000, 0x2000, 0x3ade57ad )
	ROM_LOAD( "bx06.119",    0xc000, 0x2000, 0x4a3b34cf )
	ROM_LOAD( "bx07.120",    0xe000, 0x2000, 0xae21f907 )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bx17.56",     0x0000, 0x2000, 0xfd3245d7 )
	ROM_LOAD( "bx18.57",     0x2000, 0x2000, 0x03a5ef06 )
	ROM_LOAD( "bx19.58",     0x4000, 0x2000, 0xf62a16f4 )

	ROM_REGION( 0x12000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bx08.128",    0x00000, 0x2000, 0x8bf41b27 )
	ROM_LOAD( "bx09.127",    0x02000, 0x2000, 0xc3ea6509 )
	ROM_LOAD( "bx10.126",    0x04000, 0x2000, 0x474a1c64 )
	ROM_LOAD( "bx11.125",    0x06000, 0x2000, 0xba67e382 )
	ROM_LOAD( "bx12.124",    0x08000, 0x2000, 0x102c0e1c )
	ROM_LOAD( "bx13.123",    0x0a000, 0x2000, 0xca47de34 )
	ROM_LOAD( "bx14.122",    0x0c000, 0x2000, 0x51b95bb4 )
	ROM_LOAD( "bx15.121",    0x0e000, 0x2000, 0xcf45d025 )
	ROM_LOAD( "bx16.120",    0x10000, 0x2000, 0xd1933837 )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "bx20.52",     0x0000, 0x0020, 0x4e475f05 )
	ROM_LOAD( "bx21.64",     0x0020, 0x0020, 0x5de4319f )
ROM_END



GAME( 1984, dogfgt, 0, dogfgt, dogfgt, 0, ROT0, "Technos", "Dog-Fight" )
