/***************************************************************************

Loco-Motion

driver by Nicola Salmoria

Notes:
- PROM 7a controls the video shape. This is used to hide the rightmost 4 char
  columns in Locomotion and Commando, while showing them in Jungler and
  Tactician.


CPU #1
0000-4fff ROM (empty space at 5000-5fff for diagnostic ROM)
8040-83ff video RAM (top 8 rows of screen)
8440-87ff video RAM
8840-8bff color RAM (top 8 rows of screen)
8c40-8fff color RAM
8000-803f sprite RAM #1
8800-883f sprite RAM #2
9800-9fff RAM

read:
a000      IN0
a080      IN1
a100      IN2
a180      DSW

write:
a080      watchdog reset
a100      command for the audio CPU
a180      interrupt trigger on audio CPU
a181      interrupt enable


CPU #2:
2000-23ff RAM

read:
4000      8910 #0 read
6000      8910 #1 read

write:
5000      8910 #0 control
4000      8910 #0 write
7000      8910 #1 control
6000      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/timeplt.h"


extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarx,*rallyx_radary,*rallyx_radarattr;
extern size_t rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
WRITE_HANDLER( rallyx_videoram2_w );
WRITE_HANDLER( rallyx_colorram2_w );
WRITE_HANDLER( rallyx_flipscreen_w );
PALETTE_INIT( locomotn );
VIDEO_START( rallyx );
VIDEO_UPDATE( locomotn );
VIDEO_UPDATE( jungler );
VIDEO_UPDATE( commsega );


static WRITE_HANDLER( coin_1_w )
{
	coin_counter_w(0,data & 1);
}
static WRITE_HANDLER( coin_2_w )
{
	coin_counter_w(1,data & 1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* IN2 */
	{ 0xa180, 0xa180, input_port_3_r },	/* DSW */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa004, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
//	{ 0xa182, 0xa182, MWA_NOP },	sound mute
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
//	{ 0xa187, 0xa187, MWA_NOP },	stars enable
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END

static MEMORY_WRITE_START( jungler_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa034, 0xa03f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
//	{ 0xa182, 0xa182, MWA_NOP },	sound mute
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
//	{ 0xa187, 0xa187, MWA_NOP },	stars enable
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END

static MEMORY_WRITE_START( commsega_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, timeplt_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
//	{ 0xa182, 0xa182, MWA_NOP },	sound mute
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa184, coin_1_w },
	{ 0xa186, 0xa186, coin_2_w },
//	{ 0xa187, 0xa187, MWA_NOP },	stars enable
	{ 0x8000, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8800, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8020, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8820, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END



INPUT_PORTS_START( jungler )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Debug Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )				// Gives 255 lives
INPUT_PORTS_END


INPUT_PORTS_START( locomotn )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Intermissions" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX(0,  0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )
INPUT_PORTS_END


INPUT_PORTS_START( tactcian )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10k, 80k then every 100k" )
	PORT_DIPSETTING(    0x01, "20k, 80k then every 100k" )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )			// Mode 1
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	/*
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )			// Mode 2
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, "A 2C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, "A 1C/1C  B 1C/6C" )
	*/
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX(0,  0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x01, "Mode 2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


/* Due to an ingame bug at 0x1259, bit 3 of DSW1 ALSO affects the "Bonus Life" value :
     - when bit 3 is OFF, you get an extra life at 30000 points
     - when bit 3 is ON , you get an extra life at 50000 points
   In the IRQ handler (0x0038) of CPU0, there is code to give infinite lives for player 1
   when bit 3 of DSW0 is ON. I can't tell however when it is supposed to be called.
*/
INPUT_PORTS_START( commsega )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )			// "Infinite Lives" - See notes
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )			// Bonus Life : 50000 points
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )			// Bonus Life : 30000 points
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )		// Bonus Life : 50000 points
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )		// Check code at 0x1fc5
	PORT_DIPSETTING(    0x40, "Easy" )					// 16 flying enemies to kill
	PORT_DIPSETTING(    0x00, "Hard" )					// 24 flying enemies to kill
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters (256 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8+0,8*8+1,8*8+2,8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites (64 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout dotlayout =
{
	4,4,	/* 4*4 characters */
	8,	/* 8 characters */
	2,	/* 2 bits per pixel */
	{ 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8 },
	{ 0*32, 1*32, 2*32, 3*32 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 64 },
	{ REGION_GFX1, 0, &spritelayout,    0, 64 },
	{ REGION_GFX2, 0, &dotlayout,    64*4,  1 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( tactcian )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 14318180/8)	/* 1.789772727 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(timeplt_sound_readmem,timeplt_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+4)

	MDRV_PALETTE_INIT(locomotn)
	MDRV_VIDEO_START(rallyx)
	MDRV_VIDEO_UPDATE(locomotn)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, timeplt_ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( jungler )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,jungler_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(jungler)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( locomotn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( commsega )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tactcian)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,commsega_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_VIDEO_UPDATE(commsega)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jungler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jungr1",       0x0000, 0x1000, 0x5bd6ad15 )
	ROM_LOAD( "jungr2",       0x1000, 0x1000, 0xdc99f1e3 )
	ROM_LOAD( "jungr3",       0x2000, 0x1000, 0x3dcc03da )
	ROM_LOAD( "jungr4",       0x3000, 0x1000, 0xf92e9940 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, 0xc59c51b7 ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( junglers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "5c",           0x0000, 0x1000, 0xedd71b28 )
	ROM_LOAD( "5a",           0x1000, 0x1000, 0x61ea4d46 )
	ROM_LOAD( "4d",           0x2000, 0x1000, 0x557c7925 )
	ROM_LOAD( "4c",           0x3000, 0x1000, 0x51aac9a5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s129.10g",   0x0000, 0x0100, 0xc59c51b7 ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* video layout (not used) */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( tactcian )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tacticia.001", 0x0000, 0x1000, 0x99163e39 )
	ROM_LOAD( "tacticia.002", 0x1000, 0x1000, 0x6d3e8a69 )
	ROM_LOAD( "tacticia.003", 0x2000, 0x1000, 0x0f71d0fa )
	ROM_LOAD( "tacticia.004", 0x3000, 0x1000, 0x5e15f3b3 )
	ROM_LOAD( "tacticia.005", 0x4000, 0x1000, 0x76456106 )
	ROM_LOAD( "tacticia.006", 0x5000, 0x1000, 0xb33ca9ea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, 0x97d145a7 )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, 0x067f781b )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tacticia.c1",  0x0000, 0x1000, 0x5d3ee965 )
	ROM_LOAD( "tacticia.c2",  0x1000, 0x1000, 0xe8c59c4f )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, 0x88b0b511 ) /* dots */	// tac.a7

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, 0xb7ef83b7 ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, 0xa92796f2 ) /* loookup table */	// tac.b4
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, 0x8f574815 ) /* video layout (not used) */
//	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( tactcan2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tan1",         0x0000, 0x1000, 0xddf38b75 )
	ROM_LOAD( "tan2",         0x1000, 0x1000, 0xf065ee2e )
	ROM_LOAD( "tan3",         0x2000, 0x1000, 0x2dba64fe )
	ROM_LOAD( "tan4",         0x3000, 0x1000, 0x2ba07847 )
	ROM_LOAD( "tan5",         0x4000, 0x1000, 0x1dae4c61 )
	ROM_LOAD( "tan6",         0x5000, 0x1000, 0x2b36a18d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	/* sound ROMs were missing - using the ones from the other set */
	ROM_LOAD( "tacticia.s2",  0x0000, 0x1000, 0x97d145a7 )
	ROM_LOAD( "tacticia.s1",  0x1000, 0x1000, 0x067f781b )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c1",           0x0000, 0x1000, 0x5399471f )
	ROM_LOAD( "c2",           0x1000, 0x1000, 0x8e8861e8 )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tact6301.004", 0x0000, 0x0100, 0x88b0b511 ) /* dots */	// tac.a7

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "tact6331.002", 0x0000, 0x0020, 0xb7ef83b7 ) /* palette */
	ROM_LOAD( "tact6301.003", 0x0020, 0x0100, 0xa92796f2 ) /* loookup table */	// tac.b4
	ROM_LOAD( "tact6331.001", 0x0120, 0x0020, 0x8f574815 ) /* video layout (not used) */
//	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( locomotn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1a.cpu",       0x0000, 0x1000, 0xb43e689a )
	ROM_LOAD( "2a.cpu",       0x1000, 0x1000, 0x529c823d )
	ROM_LOAD( "3.cpu",        0x2000, 0x1000, 0xc9dbfbd1 )
	ROM_LOAD( "4.cpu",        0x3000, 0x1000, 0xcaf6431c )
	ROM_LOAD( "5.cpu",        0x4000, 0x1000, 0x64cf8dd6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, 0xa1105714 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, 0x5732eda9 )
	ROM_LOAD( "c2.cpu",       0x1000, 0x1000, 0xc3035300 )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, 0x2ef89356 ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, 0x75b05da0 ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, 0xaa6cf063 ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, 0x48c8f094 ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( gutangtn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "3d_1.bin",     0x0000, 0x1000, 0xe9757395 )
	ROM_LOAD( "3e_2.bin",     0x1000, 0x1000, 0x11d21d2e )
	ROM_LOAD( "3f_3.bin",     0x2000, 0x1000, 0x4d80f895 )
	ROM_LOAD( "3h_4.bin",     0x3000, 0x1000, 0xaa258ddf )
	ROM_LOAD( "3j_5.bin",     0x4000, 0x1000, 0x52aec87e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1b_s1.bin",    0x0000, 0x1000, 0xa1105714 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5l_c1.bin",    0x0000, 0x1000, 0x5732eda9 )
	ROM_LOAD( "5m_c2.bin",    0x1000, 0x1000, 0x51c542fd )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10g.bpr",      0x0000, 0x0100, 0x2ef89356 ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "8b.bpr",       0x0000, 0x0020, 0x75b05da0 ) /* palette */
	ROM_LOAD( "9d.bpr",       0x0020, 0x0100, 0xaa6cf063 ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, 0x48c8f094 ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( cottong )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "c1",           0x0000, 0x1000, 0x2c256fe6 )
	ROM_LOAD( "c2",           0x1000, 0x1000, 0x1de5e6a0 )
	ROM_LOAD( "c3",           0x2000, 0x1000, 0x01f909fe )
	ROM_LOAD( "c4",           0x3000, 0x1000, 0xa89eb3e3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "c7",           0x0000, 0x1000, 0x3d83f6d3 )
	ROM_LOAD( "c8",           0x1000, 0x1000, 0x323e1937 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c5",           0x0000, 0x1000, 0x992d079c )
	ROM_LOAD( "c6",           0x1000, 0x1000, 0x0149ef46 )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.bpr",        0x0000, 0x0100, 0x21fb583f ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "2.bpr",        0x0000, 0x0020, 0x26f42e6f ) /* palette */
	ROM_LOAD( "3.bpr",        0x0020, 0x0100, 0x4aecc0c8 ) /* loookup table */
	ROM_LOAD( "7a.bpr",       0x0120, 0x0020, 0x48c8f094 ) /* video layout (not used) */
	ROM_LOAD( "10a.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END

ROM_START( commsega )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "csega1",       0x0000, 0x1000, 0x92de3405 )
	ROM_LOAD( "csega2",       0x1000, 0x1000, 0xf14e2f9a )
	ROM_LOAD( "csega3",       0x2000, 0x1000, 0x941dbf48 )
	ROM_LOAD( "csega4",       0x3000, 0x1000, 0xe0ac69b4 )
	ROM_LOAD( "csega5",       0x4000, 0x1000, 0xbc56ebd0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "csega8",       0x0000, 0x1000, 0x588b4210 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "csega7",       0x0000, 0x1000, 0xe8e374f9 )
	ROM_LOAD( "csega6",       0x1000, 0x1000, 0xcf07fd5e )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gg3.bpr",      0x0000, 0x0100, 0xae7fd962 ) /* dots */

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "gg1.bpr",      0x0000, 0x0020, 0xf69e585a ) /* palette */
	ROM_LOAD( "gg2.bpr",      0x0020, 0x0100, 0x0b756e30 ) /* loookup table */
	ROM_LOAD( "gg0.bpr",      0x0120, 0x0020, 0x48c8f094 ) /* video layout (not used) */
	ROM_LOAD( "tt3.bpr",      0x0140, 0x0020, 0xb8861096 ) /* video timing (not used) */
ROM_END



GAME( 1981, jungler,  0,        jungler,  jungler,  0, ROT90, "Konami", "Jungler" )
GAME( 1981, junglers, jungler,  jungler,  jungler,  0, ROT90, "[Konami] (Stern license)", "Jungler (Stern)" )
GAME( 1982, tactcian, 0,        tactcian, tactcian, 0, ROT90, "[Konami] (Sega license)", "Tactician (set 1)" )
GAME( 1981, tactcan2, tactcian, tactcian, tactcian, 0, ROT90, "[Konami] (Sega license)", "Tactician (set 2)" )
GAME( 1982, locomotn, 0,        locomotn, locomotn, 0, ROT90, "Konami (Centuri license)", "Loco-Motion" )
GAME( 1982, gutangtn, locomotn, locomotn, locomotn, 0, ROT90, "Konami (Sega license)", "Guttang Gottong" )
GAME( 1982, cottong,  locomotn, locomotn, locomotn, 0, ROT90, "bootleg", "Cotocoto Cottong" )
GAME( 1983, commsega, 0,        commsega, commsega, 0, ROT90, "Sega", "Commando (Sega)" )
