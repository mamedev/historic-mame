/***************************************************************************

  Fast Freddie/Jump Coaster hardware
  driver by Zsolt Vasvari

***************************************************************************/

#include "driver.h"
#include "fastfred.h"


// This routine is a big hack, but the only way I can get the game working
// without knowing anything about the way the protection chip works.
// These values were derived based on disassembly of the code. Usually, it
// was pretty obvious what the values should be. Of course, this will have
// to change if a different ROM set ever surfaces.
static READ_HANDLER( fastfred_custom_io_r )
{
    switch (activecpu_get_pc())
    {
    case 0x03c0: return 0x9d;
    case 0x03e6: return 0x9f;
    case 0x0407: return 0x00;
    case 0x0446: return 0x94;
    case 0x049f: return 0x01;
    case 0x04b1: return 0x00;
    case 0x0dd2: return 0x00;
    case 0x0de4: return 0x20;
    case 0x122b: return 0x10;
    case 0x123d: return 0x00;
    case 0x1a83: return 0x10;
    case 0x1a93: return 0x00;
    case 0x1b26: return 0x00;
    case 0x1b37: return 0x80;
    case 0x2491: return 0x10;
    case 0x24a2: return 0x00;
    case 0x46ce: return 0x20;
    case 0x46df: return 0x00;
    case 0x7b18: return 0x01;
    case 0x7b29: return 0x00;
    case 0x7b47: return 0x00;
    case 0x7b58: return 0x20;
    }

    logerror("Uncaught custom I/O read %04X at %04X\n", 0xc800+offset, activecpu_get_pc());
    return 0x00;
}

static READ_HANDLER( jumpcoas_custom_io_r )
{
	if (offset == 0x100)  return 0x63;

	return 0x00;
}


static MEMORY_READ_START( fastfred_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd8ff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( fastfred_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd3ff, fastfred_videoram_w, &fastfred_videoram },
	{ 0xd400, 0xd7ff, fastfred_videoram_w },  // Mirrored for above
	{ 0xd800, 0xd83f, fastfred_attributes_w, &fastfred_attributesram },
	{ 0xd840, 0xd85f, MWA_RAM, &fastfred_spriteram, &fastfred_spriteram_size },
	{ 0xd860, 0xdbff, MWA_RAM }, // Unused, but initialized
	{ 0xe000, 0xe000, fastfred_background_color_w },
	{ 0xf000, 0xf000, MWA_NOP }, // Unused, but initialized
	{ 0xf001, 0xf001, interrupt_enable_w },
	{ 0xf002, 0xf002, fastfred_colorbank1_w },
	{ 0xf003, 0xf003, fastfred_colorbank2_w },
	{ 0xf004, 0xf004, fastfred_charbank1_w },
	{ 0xf005, 0xf005, fastfred_charbank2_w },
	{ 0xf006, 0xf006, fastfred_flip_screen_x_w },
	{ 0xf007, 0xf007, fastfred_flip_screen_y_w },
	{ 0xf116, 0xf116, fastfred_flip_screen_x_w },
	{ 0xf117, 0xf117, fastfred_flip_screen_y_w },
	{ 0xf800, 0xf800, soundlatch_w },
MEMORY_END


static MEMORY_READ_START( jumpcoas_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xdbff, MRA_RAM },
	{ 0xe800, 0xe800, input_port_0_r },
	{ 0xe801, 0xe801, input_port_1_r },
	{ 0xe802, 0xe802, input_port_2_r },
	{ 0xe803, 0xe803, input_port_3_r },
	//{ 0xf800, 0xf800, watchdog_reset_r },  // Why doesn't this work???
	{ 0xf800, 0xf800, MRA_NOP },
MEMORY_END


static MEMORY_WRITE_START( jumpcoas_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd03f, fastfred_attributes_w, &fastfred_attributesram },
	{ 0xd040, 0xd05f, MWA_RAM, &fastfred_spriteram, &fastfred_spriteram_size },
	{ 0xd060, 0xd3ff, MWA_NOP },
	{ 0xd800, 0xdbff, fastfred_videoram_w, &fastfred_videoram },
	{ 0xdc00, 0xdfff, fastfred_videoram_w },	/* mirror address, used in the name entry screen */
	{ 0xe000, 0xe000, fastfred_background_color_w },
	{ 0xf000, 0xf000, MWA_NOP }, // Unused, but initialized
	{ 0xf001, 0xf001, interrupt_enable_w },
	{ 0xf002, 0xf002, fastfred_colorbank1_w },
	{ 0xf003, 0xf003, fastfred_colorbank2_w },
	{ 0xf004, 0xf004, fastfred_charbank1_w },
	{ 0xf005, 0xf005, fastfred_charbank2_w },
	{ 0xf006, 0xf006, fastfred_flip_screen_x_w },
	{ 0xf007, 0xf007, fastfred_flip_screen_y_w },
	{ 0xf116, 0xf116, fastfred_flip_screen_x_w },
	{ 0xf117, 0xf117, fastfred_flip_screen_y_w },
	{ 0xf800, 0xf800, AY8910_control_port_0_w },
	{ 0xf801, 0xf801, AY8910_write_port_0_w },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x3000, 0x3000, interrupt_enable_w },
	{ 0x4000, 0x4000, MWA_RAM },  // Reset PSG's
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x5001, 0x5001, AY8910_write_port_0_w },
	{ 0x6000, 0x6000, AY8910_control_port_1_w },
	{ 0x6001, 0x6001, AY8910_write_port_1_w },
MEMORY_END


INPUT_PORTS_START( fastfred )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x03, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x05, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x07, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x08, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x09, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0x0a, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0x0e, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0x0b, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0x0c, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0x0d, "A 1/2 B 1/12" )
	PORT_DIPSETTING(    0x0f, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPSETTING(    0x60, "100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( flyboy )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( jumpcoas )
	PORT_START      /* DSW 0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW 1 (maybe) */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( boggy84 )
	PORT_START      /* DSW 0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW 1 (maybe) */
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

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( redrobin )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )	/* most likely "Difficulty" */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )		/* it somehow effects the */
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )		/* monsters */
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
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo fastfred_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo jumpcoas_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 32 },
	{ REGION_GFX1, 0, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};


#define CLOCK 18432000  /* The crystal is 18.432MHz */

static struct AY8910interface fastfred_ay8910_interface =
{
	2,             /* 2 chips */
	CLOCK/12,       /* ? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct AY8910interface jumpcoas_ay8910_interface =
{
	1,             /* 1 chip */
	CLOCK/12,       /* ? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( fastfred )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, CLOCK/6)     /* 3.072 MHz */
	MDRV_CPU_MEMORY(fastfred_readmem,fastfred_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD_TAG("audio", Z80, CLOCK/12)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)    /* 1.536 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)//CLOCK/16/60,

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(fastfred_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(fastfred)
	MDRV_VIDEO_START(fastfred)
	MDRV_VIDEO_UPDATE(fastfred)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("ay8910", AY8910, fastfred_ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jumpcoas )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(fastfred)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(jumpcoas_readmem,jumpcoas_writemem)

	MDRV_CPU_REMOVE("audio")

	/* video hardware */
	MDRV_GFXDECODE(jumpcoas_gfxdecodeinfo)

	/* sound hardware */
	MDRV_SOUND_REPLACE("ay8910", AY8910, jumpcoas_ay8910_interface)
MACHINE_DRIVER_END

#undef CLOCK

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( fastfred )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "ffr.01",       0x0000, 0x1000, 0x15032c13 )
	ROM_LOAD( "ffr.02",       0x1000, 0x1000, 0xf9642744 )
	ROM_LOAD( "ffr.03",       0x2000, 0x1000, 0xf0919727 )
	ROM_LOAD( "ffr.04",       0x3000, 0x1000, 0xc778751e )
	ROM_LOAD( "ffr.05",       0x4000, 0x1000, 0xcd6e160a )
	ROM_LOAD( "ffr.06",       0x5000, 0x1000, 0x67f7f9b3 )
	ROM_LOAD( "ffr.07",       0x6000, 0x1000, 0x2935c76a )
	ROM_LOAD( "ffr.08",       0x7000, 0x1000, 0x0fb79e7b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for audio CPU */
	ROM_LOAD( "ffr.09",       0x0000, 0x1000, 0xa1ec8d7e )
	ROM_LOAD( "ffr.10",       0x1000, 0x1000, 0x460ca837 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ffr.14",       0x0000, 0x1000, 0xe8a00e81 )
	ROM_LOAD( "ffr.17",       0x1000, 0x1000, 0x701e0f01 )
	ROM_LOAD( "ffr.15",       0x2000, 0x1000, 0xb49b053f )
	ROM_LOAD( "ffr.18",       0x3000, 0x1000, 0x4b208c8b )
	ROM_LOAD( "ffr.16",       0x4000, 0x1000, 0x8c686bc2 )
	ROM_LOAD( "ffr.19",       0x5000, 0x1000, 0x75b613f6 )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ffr.11",       0x0000, 0x1000, 0x0e1316d4 )
	ROM_LOAD( "ffr.12",       0x1000, 0x1000, 0x94c06686 )
	ROM_LOAD( "ffr.13",       0x2000, 0x1000, 0x3fcfaa8e )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "red.9h",       0x0000, 0x0100, 0xb801e294 )
	ROM_LOAD( "green.8h",     0x0100, 0x0100, 0x7da063d0 )
	ROM_LOAD( "blue.7h",      0x0200, 0x0100, 0x85c05c18 )
ROM_END

ROM_START( flyboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "flyboy01.cpu", 0x0000, 0x1000, 0xb05aa900 )
	ROM_LOAD( "flyboy02.cpu", 0x1000, 0x1000, 0x474867f5 )
	ROM_LOAD( "rom3.cpu",     0x2000, 0x1000, 0xd2f8f085 )
	ROM_LOAD( "rom4.cpu",     0x3000, 0x1000, 0x19e5e15c )
	ROM_LOAD( "flyboy05.cpu", 0x4000, 0x1000, 0x207551f7 )
	ROM_LOAD( "rom6.cpu",     0x5000, 0x1000, 0xf5464c72 )
	ROM_LOAD( "rom7.cpu",     0x6000, 0x1000, 0x50a1baff )
	ROM_LOAD( "rom8.cpu",     0x7000, 0x1000, 0xfe2ae95d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for audio CPU */
	ROM_LOAD( "rom9.cpu",     0x0000, 0x1000, 0x5d05d1a0 )
	ROM_LOAD( "rom10.cpu",    0x1000, 0x1000, 0x7a28005b )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom14.rom",    0x0000, 0x1000, 0xaeb07260 )
	ROM_LOAD( "rom17.rom",    0x1000, 0x1000, 0xa834325b )
	ROM_LOAD( "rom15.rom",    0x2000, 0x1000, 0xc10c7ce2 )
	ROM_LOAD( "rom18.rom",    0x3000, 0x1000, 0x2f196c80 )
	ROM_LOAD( "rom16.rom",    0x4000, 0x1000, 0x719246b1 )
	ROM_LOAD( "rom19.rom",    0x5000, 0x1000, 0x00c1c5d2 )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom11.rom",    0x0000, 0x1000, 0xee7ec342 )
	ROM_LOAD( "rom12.rom",    0x1000, 0x1000, 0x84d03124 )
	ROM_LOAD( "rom13.rom",    0x2000, 0x1000, 0xfcb33ff4 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "red.9h",       0x0000, 0x0100, 0xb801e294 )
	ROM_LOAD( "green.8h",     0x0100, 0x0100, 0x7da063d0 )
	ROM_LOAD( "blue.7h",      0x0200, 0x0100, 0x85c05c18 )
ROM_END

ROM_START( flyboyb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "rom1.cpu",     0x0000, 0x1000, 0xe9e1f527 )
	ROM_LOAD( "rom2.cpu",     0x1000, 0x1000, 0x07fbe78c )
	ROM_LOAD( "rom3.cpu",     0x2000, 0x1000, 0xd2f8f085 )
	ROM_LOAD( "rom4.cpu",     0x3000, 0x1000, 0x19e5e15c )
	ROM_LOAD( "rom5.cpu",     0x4000, 0x1000, 0xd56872ea )
	ROM_LOAD( "rom6.cpu",     0x5000, 0x1000, 0xf5464c72 )
	ROM_LOAD( "rom7.cpu",     0x6000, 0x1000, 0x50a1baff )
	ROM_LOAD( "rom8.cpu",     0x7000, 0x1000, 0xfe2ae95d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for audio CPU */
	ROM_LOAD( "rom9.cpu",     0x0000, 0x1000, 0x5d05d1a0 )
	ROM_LOAD( "rom10.cpu",    0x1000, 0x1000, 0x7a28005b )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom14.rom",    0x0000, 0x1000, 0xaeb07260 )
	ROM_LOAD( "rom17.rom",    0x1000, 0x1000, 0xa834325b )
	ROM_LOAD( "rom15.rom",    0x2000, 0x1000, 0xc10c7ce2 )
	ROM_LOAD( "rom18.rom",    0x3000, 0x1000, 0x2f196c80 )
	ROM_LOAD( "rom16.rom",    0x4000, 0x1000, 0x719246b1 )
	ROM_LOAD( "rom19.rom",    0x5000, 0x1000, 0x00c1c5d2 )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom11.rom",    0x0000, 0x1000, 0xee7ec342 )
	ROM_LOAD( "rom12.rom",    0x1000, 0x1000, 0x84d03124 )
	ROM_LOAD( "rom13.rom",    0x2000, 0x1000, 0xfcb33ff4 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "red.9h",       0x0000, 0x0100, 0xb801e294 )
	ROM_LOAD( "green.8h",     0x0100, 0x0100, 0x7da063d0 )
	ROM_LOAD( "blue.7h",      0x0200, 0x0100, 0x85c05c18 )
ROM_END

ROM_START( jumpcoas )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "jumpcoas.001", 0x0000, 0x2000, 0x0778c953 )
	ROM_LOAD( "jumpcoas.002", 0x2000, 0x2000, 0x57f59ce1 )
	ROM_LOAD( "jumpcoas.003", 0x4000, 0x2000, 0xd9fc93be )
	ROM_LOAD( "jumpcoas.004", 0x6000, 0x2000, 0xdc108fc1 )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jumpcoas.005", 0x0000, 0x1000, 0x2dce6b07 )
	ROM_LOAD( "jumpcoas.006", 0x1000, 0x1000, 0x0d24aa1b )
	ROM_LOAD( "jumpcoas.007", 0x2000, 0x1000, 0x14c21e67 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jumpcoas.red", 0x0000, 0x0100, 0x13714880 )
	ROM_LOAD( "jumpcoas.gre", 0x0100, 0x0100, 0x05354848 )
	ROM_LOAD( "jumpcoas.blu", 0x0200, 0x0100, 0xf4662db7 )
ROM_END

ROM_START( boggy84 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cpurom1.bin", 0x0000, 0x2000, 0x665266c0 )
	ROM_LOAD( "cpurom2.bin", 0x2000, 0x2000, 0x6c096798 )
	ROM_LOAD( "cpurom3.bin", 0x4000, 0x2000, 0x9da59104 )
	ROM_LOAD( "cpurom4.bin", 0x6000, 0x2000, 0x73ef6807 )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfx1.bin", 0x0000, 0x1000, 0xf4238c68 )
	ROM_LOAD( "gfx2.bin", 0x1000, 0x1000, 0xce285bd2 )
	ROM_LOAD( "gfx3.bin", 0x2000, 0x1000, 0x02f5f4fa )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "r12e", 0x0000, 0x0100, 0xf3862912 )
	ROM_LOAD( "g12e", 0x0100, 0x0100, 0x80b87220 )
	ROM_LOAD( "b12e", 0x0200, 0x0100, 0x52b7f445 )
ROM_END

ROM_START( redrobin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "redro01f.16d", 0x0000, 0x1000, 0x0788ce10 )
	ROM_LOAD( "redrob02.17d", 0x1000, 0x1000, 0xbf9b95b4 )
	ROM_LOAD( "redrob03.14b", 0x2000, 0x1000, 0x9386e40b )
	ROM_LOAD( "redrob04.16b", 0x3000, 0x1000, 0x5cafffc4 )
	ROM_LOAD( "redrob05.17b", 0x4000, 0x1000, 0xa224d41e )
	ROM_LOAD( "redrob06.14a", 0x5000, 0x1000, 0x822e0bd7 )
	ROM_LOAD( "redrob07.15a", 0x6000, 0x1000, 0x0deacf17 )
	ROM_LOAD( "redrob08.17a", 0x7000, 0x1000, 0x095cf908 )
	ROM_LOAD( "redrob20.15e", 0x8000, 0x4000, 0x5cce22b7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for audio CPU */
	ROM_LOAD( "redrob09.1f",  0x0000, 0x1000, 0x21af2d03 )
	ROM_LOAD( "redro10f.1e",  0x1000, 0x1000, 0xbf0e772f )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "redrob14.17l", 0x0000, 0x1000, 0xf6c571e0 )
	ROM_LOAD( "redrob17.17j", 0x1000, 0x1000, 0x86dcdf21 )
	ROM_LOAD( "redrob15.15k", 0x2000, 0x1000, 0x05f7df48 )
	ROM_LOAD( "redrob18.16j", 0x3000, 0x1000, 0x7aeb2bb9 )
	ROM_LOAD( "redrob16.14l", 0x4000, 0x1000, 0x21349d09 )
	ROM_LOAD( "redrob19.14j", 0x5000, 0x1000, 0x7184d999 )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "redrob11.17m", 0x0000, 0x1000, 0x559f7894 )
	ROM_LOAD( "redrob12.15m", 0x1000, 0x1000, 0xa763b11d )
	ROM_LOAD( "redrob13.14m", 0x2000, 0x1000, 0xd667f45b )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "red.9h",       0x0000, 0x0100, 0xb801e294 )
	ROM_LOAD( "green.8h",     0x0100, 0x0100, 0x7da063d0 )
	ROM_LOAD( "blue.7h",      0x0200, 0x0100, 0x85c05c18 )
ROM_END


extern int fastfred_hardware_type;

static DRIVER_INIT( flyboy )
{
	fastfred_hardware_type = 1;
}

static DRIVER_INIT( fastfred )
{
	install_mem_write_handler(0, 0xc800, 0xcfff, MWA_NOP );
	install_mem_read_handler( 0, 0xc800, 0xcfff, fastfred_custom_io_r);
	fastfred_hardware_type = 1;
}

static DRIVER_INIT( jumpcoas )
{
	install_mem_write_handler(0, 0xc800, 0xcfff, MWA_NOP );
	install_mem_read_handler(0,  0xc800, 0xcfff, jumpcoas_custom_io_r);
	fastfred_hardware_type = 0;
}

static DRIVER_INIT( boggy84 )
{
	install_mem_write_handler(0, 0xc800, 0xcfff, MWA_NOP );
	install_mem_read_handler(0,  0xc800, 0xcfff, jumpcoas_custom_io_r);
	fastfred_hardware_type = 2;
}


GAMEX(1982, flyboy,   0,      fastfred, flyboy,   flyboy,   ROT90, "Kaneko", "Fly-Boy", GAME_NOT_WORKING )	/* protection */
GAME( 1982, flyboyb,  flyboy, fastfred, flyboy,   flyboy,   ROT90, "Kaneko", "Fly-Boy (bootleg)" )
GAME( 1982, fastfred, flyboy, fastfred, fastfred, fastfred, ROT90, "Atari", "Fast Freddie" )
GAME( 1983, jumpcoas, 0,      jumpcoas, jumpcoas, jumpcoas, ROT90, "Kaneko", "Jump Coaster" )
GAME( 1983, boggy84,  0,      jumpcoas, boggy84,  boggy84,  ROT90, "bootleg", "Boggy '84" )
GAME( 1986, redrobin, 0,      fastfred, redrobin, flyboy,   ROT90, "Elettronolo", "Red Robin" )
