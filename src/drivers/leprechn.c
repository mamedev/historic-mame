/***************************************************************************

 Leprechaun/Pot of Gold

 driver by Zsolt Vasvari

 Hold down F2 while pressing F3 to enter test mode. Hit Advance (F1) to
 cycle through test and hit F2 to execute.


 TODO:
 -----

 - Is the 0800-081e range on the sound board mapped to a VIA?
   I don't think so, but needs to be checked.

 - The following VIA lines appear to be used but aren't mapped:

   VIA #0 CA2
   VIA #0 IRQ
   VIA #2 CB2 - This is probably a sound CPU halt or reset.  See potogold $8a5a

 ***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "vidhrdw/generic.h"
#include "leprechn.h"



static MEMORY_READ_START( readmem )
    { 0x0000, 0x03ff, MRA_RAM },
	{ 0x2000, 0x200f, via_0_r },
	{ 0x2800, 0x280f, via_1_r },
	{ 0x3000, 0x300f, via_2_r },
    { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
    { 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x200f, via_0_w },
	{ 0x2800, 0x280f, via_1_w },
	{ 0x3000, 0x300f, via_2_w },
    { 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x01ff, MRA_RAM },
    { 0x0800, 0x0800, soundlatch_r },
    { 0x0804, 0x0804, MRA_RAM },   // ???
    { 0x0805, 0x0805, leprechn_sh_0805_r },   // ???
    { 0x080c, 0x080c, MRA_RAM },   // ???
    { 0xa001, 0xa001, AY8910_read_port_0_r }, // ???
    { 0xf000, 0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x01ff, MWA_RAM },
    { 0x0801, 0x0803, MWA_RAM },   // ???
    { 0x0806, 0x0806, MWA_RAM },   // ???
    { 0x081e, 0x081e, MWA_RAM },   // ???
    { 0xa000, 0xa000, AY8910_control_port_0_w },
    { 0xa002, 0xa002, AY8910_write_port_0_w },
    { 0xf000, 0xffff, MWA_ROM},
MEMORY_END


INPUT_PORTS_START( leprechn )
    // All of these ports are read indirectly through a VIA mapped at 0x2800
    PORT_START      /* Input Port 0 */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) // This is called "Slam" in the game
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Advance", KEYCODE_F1, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x23, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START      /* Input Port 1 */
    PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )

    PORT_START      /* Input Port 2 */
    PORT_BIT( 0x5f, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

    PORT_START      /* Input Port 3 */
    PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

    PORT_START      /* DSW #1 */
    PORT_DIPNAME( 0x09, 0x09, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x09, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
    PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_7C ) )
    PORT_DIPNAME( 0x22, 0x22, "Max Credits" )
    PORT_DIPSETTING(    0x22, "10" )
    PORT_DIPSETTING(    0x20, "20" )
    PORT_DIPSETTING(    0x02, "30" )
    PORT_DIPSETTING(    0x00, "40" )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )

    PORT_START      /* DSW #2 */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x08, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x40, "30000" )
    PORT_DIPSETTING(    0x80, "60000" )
    PORT_DIPSETTING(    0x00, "90000" )
    PORT_DIPSETTING(    0xc0, "None" )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
    1,      /* 1 chip */
    14318000/8,     /* ? */
    { 50 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};


static MACHINE_DRIVER_START( leprechn )

	/* basic machine hardware */
	// A good test to verify that the relative CPU speeds of the main
	// and sound are correct, is when you finish a level, the sound
	// should stop before the display switches to the name of the
	// next level
	MDRV_CPU_ADD(M6502, 1250000)    /* 1.25 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)    /* 1.5 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(leprechn)
	MDRV_VIDEO_START(leprechn)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( leprechn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the main CPU */
	ROM_LOAD( "lep1",         0x8000, 0x1000, 0x2c4a46ca )
	ROM_LOAD( "lep2",         0x9000, 0x1000, 0x6ed26b3e )
	ROM_LOAD( "lep3",         0xa000, 0x1000, 0xa2eaa016 )
	ROM_LOAD( "lep4",         0xb000, 0x1000, 0x6c12a065 )
	ROM_LOAD( "lep5",         0xc000, 0x1000, 0x21ddb539 )
	ROM_LOAD( "lep6",         0xd000, 0x1000, 0x03c34dce )
	ROM_LOAD( "lep7",         0xe000, 0x1000, 0x7e06d56d )
	ROM_LOAD( "lep8",         0xf000, 0x1000, 0x097ede60 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the audio CPU */
	ROM_LOAD( "lepsound",     0xf000, 0x1000, 0x6651e294 )
ROM_END

ROM_START( potogold )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the main CPU */
	ROM_LOAD( "pog.pg1",      0x8000, 0x1000, 0x9f1dbda6 )
	ROM_LOAD( "pog.pg2",      0x9000, 0x1000, 0xa70e3811 )
	ROM_LOAD( "pog.pg3",      0xa000, 0x1000, 0x81cfb516 )
	ROM_LOAD( "pog.pg4",      0xb000, 0x1000, 0xd61b1f33 )
	ROM_LOAD( "pog.pg5",      0xc000, 0x1000, 0xeee7597e )
	ROM_LOAD( "pog.pg6",      0xd000, 0x1000, 0x25e682bc )
	ROM_LOAD( "pog.pg7",      0xe000, 0x1000, 0x84399f54 )
	ROM_LOAD( "pog.pg8",      0xf000, 0x1000, 0x9e995a1a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the audio CPU */
	ROM_LOAD( "pog.snd",      0xf000, 0x1000, 0xec61f0a4 )
ROM_END



GAME( 1982, leprechn, 0,        leprechn, leprechn, leprechn, ROT0, "Tong Electronic", "Leprechaun" )
GAME( 1982, potogold, leprechn, leprechn, leprechn, leprechn, ROT0, "GamePlan", "Pot of Gold" )
