/****************************************************************************

Traverse USA / Zippy Race  (c) 1983 Irem
Shot Rider                 (c) 1984 Seibu Kaihatsu / Sigma

driver by
Lee Taylor
John Clegg
Tomasz Slanina  dox@space.pl

Notes:
- I haven't understood how char/sprite priority works. This is used for
  tunnels. I hacked it just by making the two needed colors opaque. They
  don't seem to be used anywhere else. Even if it looks like a hack, it might
  really be how the hardware works - see also notes regarding Kung Fu Master
  at the beginning of m62.c.
- shtrider has some weird colors (pink cars, green "Seibu" truck, 'no'
  on hiscore table) but there's no reason to think there's something wrong
  with the driver.

TODO:
- shtrider dip switches

****************************************************************************

Shot Rider
PCB layout:

--------------------CCCCCCCCCCC------------------

  P P              OKI                       2764
 * 2764 2764 2764      AY  AY  x 2764  2764  2764

 E           D  D                            P P

                      2764 2764 2764
--------------------------------------------------
  C  - conector
  P  - proms
  E  - connector for  big black epoxy block
  D  - dips
  *  - empty rom socket
  x  - 40 pin chip, surface scratched (6803)

Epoxy block contains main cpu (Z80)
and 2764 eprom (swapped D3/D4 and D5/D6 data lines)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/irem.h"

extern unsigned char *travrusa_videoram;

PALETTE_INIT( travrusa );
PALETTE_INIT( shtrider );
VIDEO_START( travrusa );
VIDEO_START( shtrider );
WRITE_HANDLER( travrusa_videoram_w );
WRITE_HANDLER( travrusa_scroll_x_low_w );
WRITE_HANDLER( travrusa_scroll_x_high_w );
WRITE_HANDLER( travrusa_flipscreen_w );
VIDEO_UPDATE( travrusa );
VIDEO_UPDATE( shtrider );



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },        /* Video and Color ram */
	{ 0xd000, 0xd000, input_port_0_r },	/* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },	/* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },	/* IN2 */
	{ 0xd003, 0xd003, input_port_3_r },	/* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },	/* DSW2 */
	{ 0xe000, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, travrusa_videoram_w, &travrusa_videoram },
	{ 0x9000, 0x9000, travrusa_scroll_x_low_w },
	{ 0xa000, 0xa000, travrusa_scroll_x_high_w },
	{ 0xc800, 0xc9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd000, irem_sound_cmd_w },
	{ 0xd001, 0xd001, travrusa_flipscreen_w },	/* + coin counters - not written by shtrider */
	{ 0xe000, 0xefff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( travrusa )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN3, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Fuel Reduced on Collision" )
	PORT_DIPSETTING(    0x03, "Low" )
	PORT_DIPSETTING(    0x02, "Med" )
	PORT_DIPSETTING(    0x01, "Hi" )
	PORT_DIPSETTING(    0x00, "Max" )
	PORT_DIPNAME( 0x04, 0x04, "Fuel Consumption" )
	PORT_DIPSETTING(    0x04, "Low" )
	PORT_DIPSETTING(    0x00, "Hi" )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )

	/* PORT_DIPSETTING( 0x10, "INVALID" ) */
	/* PORT_DIPSETTING( 0x00, "INVALID" ) */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Speed Type" )
	PORT_DIPSETTING(    0x08, "M/H" )
	PORT_DIPSETTING(    0x00, "Km/H" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Title" )
	PORT_DIPSETTING(    0x20, "Traverse USA" )
	PORT_DIPSETTING(    0x00, "Zippy Race" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/* same as travrusa, no "Title" switch */
INPUT_PORTS_START( motorace )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN3, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Fuel Reduced on Collision" )
	PORT_DIPSETTING(    0x03, "Low" )
	PORT_DIPSETTING(    0x02, "Med" )
	PORT_DIPSETTING(    0x01, "Hi" )
	PORT_DIPSETTING(    0x00, "Max" )
	PORT_DIPNAME( 0x04, 0x04, "Fuel Consumption" )
	PORT_DIPSETTING(    0x04, "Low" )
	PORT_DIPSETTING(    0x00, "Hi" )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_7C ) )

	/* PORT_DIPSETTING( 0x10, "INVALID" ) */
	/* PORT_DIPSETTING( 0x00, "INVALID" ) */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Speed Type" )
	PORT_DIPSETTING(    0x08, "M/H" )
	PORT_DIPSETTING(    0x00, "Km/H" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( shtrider )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN2, 4 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1, 4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW A */
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

	PORT_START      /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Speed Type" )
	PORT_DIPSETTING(    0x02, "Km/h" )
	PORT_DIPSETTING(    0x00, "Ml/h" )
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
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout sht_spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*8, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo sht_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 16 },
	{ REGION_GFX2, 0, &sht_spritelayout, 16*8, 16 },
	{ -1 }
};



static MACHINE_DRIVER_START( travrusa )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(56.75)
	MDRV_VBLANK_DURATION(1790)	/* accurate frequency, measured on a Moon Patrol board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(128+16)
	MDRV_COLORTABLE_LENGTH(16*8+16*8)

	MDRV_PALETTE_INIT(travrusa)
	MDRV_VIDEO_START(travrusa)
	MDRV_VIDEO_UPDATE(travrusa)

	/* sound hardware */
	MDRV_IMPORT_FROM(irem_audio)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( shtrider )

	MDRV_IMPORT_FROM(travrusa)

	/* video hardware */
	MDRV_GFXDECODE(sht_gfxdecodeinfo)

	MDRV_PALETTE_INIT(shtrider)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( travrusa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zippyrac.000", 0x0000, 0x2000, 0xbe066c0a )
	ROM_LOAD( "zippyrac.005", 0x2000, 0x2000, 0x145d6b34 )
	ROM_LOAD( "zippyrac.006", 0x4000, 0x2000, 0xe1b51383 )
	ROM_LOAD( "zippyrac.007", 0x6000, 0x2000, 0x85cd1a51 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, 0xa02ad8a0 )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zippyrac.001", 0x00000, 0x2000, 0xaa8994dd )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, 0x3a046dd1 )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, 0x1cc3d3f4 )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zippyrac.008", 0x00000, 0x2000, 0x3e2c7a6b )
	ROM_LOAD( "zippyrac.009", 0x02000, 0x2000, 0x13be6a14 )
	ROM_LOAD( "zippyrac.010", 0x04000, 0x2000, 0x6fcc9fdb )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, 0xc9724350 ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, 0xa1130007 ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, 0x76062638 ) /* sprite lookup table */
ROM_END

ROM_START( motorace )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mr.cpu",       0x0000, 0x2000, 0x89030b0c )	/* encrypted */
	ROM_LOAD( "mr1.3l",       0x2000, 0x2000, 0x0904ed58 )
	ROM_LOAD( "mr2.3k",       0x4000, 0x2000, 0x8a2374ec )
	ROM_LOAD( "mr3.3j",       0x6000, 0x2000, 0x2f04c341 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, 0xa02ad8a0 )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mr7.3e",       0x00000, 0x2000, 0x492a60be )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, 0x3a046dd1 )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, 0x1cc3d3f4 )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mr4.3n",       0x00000, 0x2000, 0x5cf1a0d6 )
	ROM_LOAD( "mr5.3m",       0x02000, 0x2000, 0xf75f2aad )
	ROM_LOAD( "mr6.3k",       0x04000, 0x2000, 0x518889a0 )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, 0xc9724350 ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, 0xa1130007 ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, 0x76062638 ) /* sprite lookup table */
ROM_END

ROM_START( shtrider )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.bin",   0x0000, 0x2000, 0xeb51315c ) /* was inside epoxy block with cpu, encrypted */
	ROM_LOAD( "2.bin",   0x2000, 0x2000, 0x97675d19 )
	ROM_LOAD( "3.bin",   0x4000, 0x2000, 0x78d051cd )
	ROM_LOAD( "4.bin",   0x6000, 0x2000, 0x02b96eaa )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11.bin",   0xe000, 0x2000, 0xa8396b76 )

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bin",   0x0000, 0x2000, 0x34449f79 )
	ROM_LOAD( "6.bin",   0x2000, 0x2000, 0xde43653d )
	ROM_LOAD( "7.bin",   0x4000, 0x2000, 0x3445b81c )

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8.bin",   0x0000, 0x2000, 0x4072b096 )
	ROM_LOAD( "9.bin",   0x2000, 0x2000, 0xfd4cc7e6 )
	ROM_LOAD( "10.bin",  0x4000, 0x2000, 0x0a117925 )

	ROM_REGION( 0x0420,  REGION_PROMS, 0 )
	ROM_LOAD( "1.bpr",   0x0000, 0x0100, 0xe9e258e5 )
	ROM_LOAD( "2.bpr",   0x0100, 0x0100, 0x6cf4591c )
	ROM_LOAD( "4.bpr",   0x0200, 0x0020, 0xee97c581 )
	ROM_LOAD( "3.bpr",   0x0220, 0x0100, 0x5db47092 )
ROM_END



DRIVER_INIT( motorace )
{
	int A,j;
	unsigned char *rom = memory_region(REGION_CPU1);
	data8_t *buffer = malloc(0x2000);

	if (buffer)
	{
		memcpy(buffer,rom,0x2000);

		/* The first CPU ROM has the address and data lines scrambled */
		for (A = 0;A < 0x2000;A++)
		{
			j = BITSWAP16(A,15,14,13,9,7,5,3,1,12,10,8,6,4,2,0,11);
			rom[j] = BITSWAP8(buffer[A],2,7,4,1,6,3,0,5);
		}

		free(buffer);
	}
}

static DRIVER_INIT( shtrider )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);

	/* D3/D4  and  D5/D6 swapped */
	for (A = 0; A < 0x2000; A++)
		rom[A] = BITSWAP8(rom[A],7,5,6,3,4,2,1,0);
}



GAME( 1983, travrusa, 0,        travrusa, travrusa, 0,        ROT270, "Irem", "Traverse USA / Zippy Race" )
GAME( 1983, motorace, travrusa, travrusa, motorace, motorace, ROT270, "Irem (Williams license)", "MotoRace USA" )
GAMEX(1984, shtrider, 0,        shtrider, shtrider, shtrider, ROT270|ORIENTATION_FLIP_X, "Seibu Kaihatsu (Sigma license)", "Shot Rider", GAME_NO_COCKTAIL )
