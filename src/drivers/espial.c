/***************************************************************************

 Espial hardware games

***************************************************************************/

#include "driver.h"
#include "espial.h"
#include "cpu/z80/z80.h"


MACHINE_INIT( espial )
{
	/* we must start with NMI interrupts disabled */
	//interrupt_enable = 0;
	cpu_interrupt_enable(0,0);
}


WRITE_HANDLER( zodiac_master_interrupt_enable_w )
{
	interrupt_enable_w(offset,~data & 1);
}


INTERRUPT_GEN( zodiac_master_interrupt )
{
	if (cpu_getiloops() == 0)
		nmi_line_pulse();
	else
		irq0_line_hold();
}


WRITE_HANDLER( zodiac_master_soundlatch_w )
{
	soundlatch_w(offset, data);
	cpu_set_irq_line(1, 0, HOLD_LINE);
}



static MEMORY_READ_START( espial_readmem )
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x6081, 0x6081, input_port_0_r },
	{ 0x6082, 0x6082, input_port_1_r },
	{ 0x6083, 0x6083, input_port_2_r },
	{ 0x6084, 0x6084, input_port_3_r },
	{ 0x6090, 0x6090, soundlatch_r },	/* the main CPU reads the command back from the slave */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8000, 0x803f, MRA_RAM },
	{ 0x8400, 0x87ff, MRA_RAM },
	{ 0x8c00, 0x903f, MRA_RAM },
	{ 0x9400, 0x97ff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( espial_writemem )
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6090, 0x6090, zodiac_master_soundlatch_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x7100, 0x7100, zodiac_master_interrupt_enable_w },
	{ 0x7200, 0x7200, espial_flipscreen_w },
	{ 0x8000, 0x801f, MWA_RAM, &espial_spriteram_1 },
	{ 0x8400, 0x87ff, espial_videoram_w, &espial_videoram },
	{ 0x8800, 0x880f, MWA_RAM, &espial_spriteram_3 },
	{ 0x8c00, 0x8fff, espial_attributeram_w, &espial_attributeram },
	{ 0x9000, 0x901f, MWA_RAM, &espial_spriteram_2 },
	{ 0x9020, 0x903f, espial_scrollram_w, &espial_scrollram },
	{ 0x9400, 0x97ff, espial_colorram_w, &espial_colorram },
	{ 0xc000, 0xcfff, MWA_ROM },
MEMORY_END


/* there are a lot of unmapped reads from all over memory as the
   code uses POP instructions in a delay loop */
static MEMORY_READ_START( netwars_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x6081, 0x6081, input_port_0_r },
	{ 0x6082, 0x6082, input_port_1_r },
	{ 0x6083, 0x6083, input_port_2_r },
	{ 0x6084, 0x6084, input_port_3_r },
	{ 0x6090, 0x6090, soundlatch_r },	/* the main CPU reads the command back from the slave */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8000, 0x97ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( netwars_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6090, 0x6090, zodiac_master_soundlatch_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x7100, 0x7100, zodiac_master_interrupt_enable_w },
	{ 0x7200, 0x7200, espial_flipscreen_w },
	{ 0x8000, 0x801f, MWA_RAM, &espial_spriteram_1 },
	{ 0x8000, 0x87ff, espial_videoram_w, &espial_videoram },
	{ 0x8800, 0x880f, MWA_RAM, &espial_spriteram_3 },
	{ 0x8800, 0x8fff, espial_attributeram_w, &espial_attributeram },
	{ 0x9000, 0x901f, MWA_RAM, &espial_spriteram_2 },
	{ 0x9020, 0x903f, espial_scrollram_w, &espial_scrollram },
	{ 0x9000, 0x97ff, espial_colorram_w, &espial_colorram },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, interrupt_enable_w },
	{ 0x6000, 0x6000, soundlatch_w },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
PORT_END


INPUT_PORTS_START( espial )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Number of Buttons" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x02, "Enemy Bullets Vulnerable" )	/* you can shoot bullets */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "20k and every 70k" )
	PORT_DIPSETTING(	0x20, "50k and every 100k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode" )	/* ??? */
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x80, "Test" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
INPUT_PORTS_END


INPUT_PORTS_START( netwars )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "20k and every 70k" )
	PORT_DIPSETTING(	0x20, "50k and every 100k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode" )	/* ??? */
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x80, "Test" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(2,2),
	2,
	{ 0, 4 },
	{ STEP4(0,1), STEP4(8*8,1) },
	{ STEP8(0,8) },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(0,1), STEP8(8*8,1) },
	{ STEP8(0,8), STEP8(16*8,8) },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 64 },
	{ REGION_GFX2, 0, &spritelayout,  0, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz?????? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( espial )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3072000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(espial_readmem,espial_writemem)
	MDRV_CPU_VBLANK_INT(zodiac_master_interrupt,2)

	MDRV_CPU_ADD(Z80, 3072000)	/* 2 MHz?????? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(0,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(espial)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(espial)
	MDRV_VIDEO_START(espial)
	MDRV_VIDEO_UPDATE(espial)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( netwars )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(espial)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(netwars_readmem,netwars_writemem)

	/* video hardware */
	MDRV_SCREEN_SIZE(32*8, 64*8)

	MDRV_VIDEO_START(netwars)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( espial )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "espial.3",     0x0000, 0x2000, 0x10f1da30 )
	ROM_LOAD( "espial.4",     0x2000, 0x2000, 0xd2adbe39 )
	ROM_LOAD( "espial.6",     0x4000, 0x1000, 0xbaa60bc1 )
	ROM_LOAD( "espial.5",     0xc000, 0x1000, 0x6d7bbfc1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "espial.1",     0x0000, 0x1000, 0x1e5ec20b )
	ROM_LOAD( "espial.2",     0x1000, 0x1000, 0x3431bb97 )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.8",     0x0000, 0x2000, 0x2f43036f )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, 0xebfef046 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.10",    0x0000, 0x1000, 0xde80fbc1 )
	ROM_LOAD( "espial.9",     0x1000, 0x1000, 0x48c258a0 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, 0xd12de557 ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, 0x4c84fe70 ) /* palette high 4 bits */
ROM_END

ROM_START( espiale )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2764.3",       0x0000, 0x2000, 0x0973c8a4 )
	ROM_LOAD( "2764.4",       0x2000, 0x2000, 0x6034d7e5 )
	ROM_LOAD( "2732.6",       0x4000, 0x1000, 0x357025b4 )
	ROM_LOAD( "2732.5",       0xc000, 0x1000, 0xd03a2fc4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "2732.1",       0x0000, 0x1000, 0xfc7729e9 )
	ROM_LOAD( "2732.2",       0x1000, 0x1000, 0xe4e256da )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.8",     0x0000, 0x2000, 0x2f43036f )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, 0xebfef046 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.10",    0x0000, 0x1000, 0xde80fbc1 )
	ROM_LOAD( "espial.9",     0x1000, 0x1000, 0x48c258a0 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, 0xd12de557 ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, 0x4c84fe70 ) /* palette high 4 bits */
ROM_END

ROM_START( netwars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "netw3.4f",     0x0000, 0x2000, 0x8e782991 )
	ROM_LOAD( "netw4.4h",     0x2000, 0x2000, 0x6e219f61 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "netw1.4n",     0x0000, 0x1000, 0x53939e16 )
	ROM_LOAD( "netw2.4r",     0x1000, 0x1000, 0xc096317a )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "netw8.4b",     0x0000, 0x2000, 0x2320277e )
	ROM_LOAD( "netw7.4a",     0x2000, 0x2000, 0x25cc5b7f )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "netw10.4e",    0x0000, 0x1000, 0x87b65625 )
	ROM_LOAD( "netw9.4d",     0x1000, 0x1000, 0x830d0218 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "netw5.1f",     0x0000, 0x0100, 0xf3ae1fe2 ) /* palette low 4 bits */
	ROM_LOAD( "netw6.1h",     0x0100, 0x0100, 0xc44c3771 ) /* palette high 4 bits */
ROM_END



GAME( 1983, espial,  0,      espial,  espial,  0, ROT0,  "[Orca] Thunderbolt", "Espial (US?)" )
GAME( 1983, espiale, espial, espial,  espial,  0, ROT0,  "[Orca] Thunderbolt", "Espial (Europe)" )
GAME( 1983, netwars, 0,      netwars, netwars, 0, ROT90, "Orca (Esco Trading Co license)", "Net Wars" )
