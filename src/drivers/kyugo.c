/***************************************************************************

	Kyugo hardware games

	driver by:
	Ernesto Corvi
	someone@secureshell.com

	Games supported:
		* Gyrodine - (c) 1984 Taito Corporation.
		* Son of Phoenix - (c) 1985 Associated Overseas MFR, Inc.
		* Repulse - (c) 1985 Sega
		* '99 The last war - (c) 1985 Proma
		* Flash Gal - (c) 1985 Sega
		* SRD Mission - (c) 1986 Taito Corporation.
		* Legend - no copyright, but readme says: (c) 1986 SEGA/Coreland
		* Air Wolf - (c) 1987 Kyugo

	Known issues:
		* attract mode in Son of Phoenix doesn't work

***************************************************************************/

#include "driver.h"
#include "kyugo.h"


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( main_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },
	{ 0x9800, 0x9fff, kyugo_spriteram_2_r },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xf000, 0xf7ff, kyugo_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( main_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, kyugo_bgvideoram_w, &kyugo_bgvideoram },
	{ 0x8800, 0x8fff, kyugo_bgattribram_w, &kyugo_bgattribram },
	{ 0x9000, 0x97ff, kyugo_fgvideoram_w, &kyugo_fgvideoram },
	{ 0x9800, 0x9fff, MWA_RAM, &kyugo_spriteram_2 },
	{ 0xa000, 0xa7ff, MWA_RAM, &kyugo_spriteram_1 },
	{ 0xa800, 0xa800, kyugo_scroll_x_lo_w },
	{ 0xb000, 0xb000, kyugo_gfxctrl_w },
	{ 0xb800, 0xb800, kyugo_scroll_y_w },
	{ 0xf000, 0xf7ff, kyugo_sharedram_w, &kyugo_sharedram },
MEMORY_END


/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

#define Main_PortMap( name, base )							\
static PORT_WRITE_START( name##_writeport )					\
	{ base+0, base+0, interrupt_enable_w },					\
	{ base+1, base+1, kyugo_flipscreen_w },					\
	{ base+2, base+2, kyugo_sub_cpu_control_w },			\
PORT_END

Main_PortMap( gyrodine, 0x00 )
Main_PortMap( flashgal, 0x40 )
Main_PortMap( srdmissn, 0x08 )


/*************************************
 *
 *	Sub CPU memory handlers
 *
 *************************************/

#define Sub_MemMap( name, rom_end, shared, in0, in1, in2 )	\
static MEMORY_READ_START( name##_sub_readmem )				\
	{ 0x0000, rom_end, MRA_ROM },							\
	{ shared, shared+0x7ff, kyugo_sharedram_r },			\
	{ in0, in0, input_port_2_r },							\
	{ in1, in1, input_port_3_r },							\
	{ in2, in2, input_port_4_r },							\
MEMORY_END													\
															\
static MEMORY_WRITE_START( name##_sub_writemem )			\
	{ 0x0000, rom_end, MWA_ROM },							\
	{ shared, shared+0x7ff, kyugo_sharedram_w },			\
MEMORY_END

Sub_MemMap( gyrodine, 0x1fff, 0x4000, 0x8080, 0x8040, 0x8000 )
Sub_MemMap( sonofphx, 0x7fff, 0xa000, 0xc080, 0xc040, 0xc000 )
Sub_MemMap( srdmissn, 0x7fff, 0x8000, 0xf400, 0xf401, 0xf402 )
Sub_MemMap( legend,   0x7fff, 0xc000, 0xf800, 0xf801, 0xf802 )


/*************************************
 *
 *	Sub CPU port handlers
 *
 *************************************/

#define Sub_PortMap( name, ay0_base, ay1_base )				\
static PORT_READ_START( name##_sub_readport )				\
	{ ay0_base+2, ay0_base+2, AY8910_read_port_0_r },		\
PORT_END													\
															\
static PORT_WRITE_START( name##_sub_writeport )				\
	{ ay0_base+0, ay0_base+0, AY8910_control_port_0_w },	\
	{ ay0_base+1, ay0_base+1, AY8910_write_port_0_w },		\
	{ ay1_base+0, ay1_base+0, AY8910_control_port_1_w },	\
	{ ay1_base+1, ay1_base+1, AY8910_write_port_1_w },		\
PORT_END

Sub_PortMap( gyrodine, 0x00, 0xc0 )
Sub_PortMap( sonofphx, 0x00, 0x40 )
Sub_PortMap( srdmissn, 0x80, 0x84 )


/*************************************
 *
 *	Port definitions
 *
 *************************************/

#define START_COINS \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) 	\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) 	\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )	\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )	\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )

#define JOYSTICK_1 \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )					\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )

#define JOYSTICK_2 \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )	\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )	\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )	\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )	\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL ) 					\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )

#define COIN_A_B \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )		\
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )		\
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )		\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )		\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )		\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )		\
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )		\
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )		\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )	\
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )		\
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )		\
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )		\
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )		\
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )		\
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )		\
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_4C ) )		\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )		\
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )

INPUT_PORTS_START( gyrodine )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "20000 50000" )
	PORT_DIPSETTING(    0x00, "40000 70000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sonofphx )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "Every 50000" )
	PORT_DIPSETTING(    0x00, "Every 70000" )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x10,     0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( airwolf )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/* Same as 'airwolf', but different "Lives" Dip Switch */
INPUT_PORTS_START( skywolf )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( flashgal )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "Every 50000" )
	PORT_DIPSETTING(    0x00, "Every 70000" )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( srdmissn )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Life/Continue" )
	PORT_DIPSETTING(    0x04, "Every 50000/No" )
	PORT_DIPSETTING(    0x00, "Every 70000/Yes" )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x10,     0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( legend )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Life/Continue" )
	PORT_DIPSETTING(    0x04, "Every 50000/No" )
	PORT_DIPSETTING(    0x00, "Every 70000/Yes" )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	COIN_A_B
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	START_COINS
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	JOYSTICK_1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	JOYSTICK_2
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout fg_tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8*2
};

static struct GfxLayout bg_tilelayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{     0,     1,     2,     3,     4,     5,     6,     7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fg_tilelayout, 0, 64 },
	{ REGION_GFX2, 0, &bg_tilelayout, 0, 32 },
	{ REGION_GFX3, 0, &spritelayout,  0, 32 },
	{ -1 }
};


/*************************************
 *
 *	Sound definition
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? */
	{ 30, 30 },
	{ input_port_0_r, 0 },
	{ input_port_1_r, 0 },
	{ 0, 0 },
	{ 0, 0 }
};


/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( gyrodine )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000 / 4)	/* 18.432 MHz crystal */
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_PORTS(0,gyrodine_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD_TAG("sub", Z80, 18432000 / 4)	/* 18.432 MHz crystal */
	MDRV_CPU_MEMORY(gyrodine_sub_readmem,gyrodine_sub_writemem)
	MDRV_CPU_PORTS(gyrodine_sub_readport,gyrodine_sub_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(kyugo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(kyugo)
	MDRV_VIDEO_UPDATE(kyugo)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sonofphx )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gyrodine)
	MDRV_CPU_MODIFY("sub")
	MDRV_CPU_MEMORY(sonofphx_sub_readmem,sonofphx_sub_writemem)
	MDRV_CPU_PORTS(sonofphx_sub_readport,sonofphx_sub_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( srdmissn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gyrodine)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(0,srdmissn_writeport)

	MDRV_CPU_MODIFY("sub")
	MDRV_CPU_MEMORY(srdmissn_sub_readmem,srdmissn_sub_writemem)
	MDRV_CPU_PORTS(srdmissn_sub_readport,srdmissn_sub_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( flashgal )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(sonofphx)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(0,flashgal_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( legend )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gyrodine)
	MDRV_CPU_MODIFY("sub")
	MDRV_CPU_MEMORY(legend_sub_readmem,legend_sub_writemem)
	MDRV_CPU_PORTS(srdmissn_sub_readport,srdmissn_sub_writeport)
MACHINE_DRIVER_END


/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( gyrodine )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "a21.02", 0x0000, 0x2000, 0xc5ec4a50 )
	ROM_LOAD( "a21.03", 0x2000, 0x2000, 0x4e9323bd )
	ROM_LOAD( "a21.04", 0x4000, 0x2000, 0x57e659d4 )
	ROM_LOAD( "a21.05", 0x6000, 0x2000, 0x1e7293f3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "a21.01", 0x0000, 0x2000, 0xb2ce0aa2 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a21.15", 0x00000, 0x1000, 0xadba18d0 ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a21.08", 0x00000, 0x2000, 0xa57df1c9 ) /* tiles - plane 0 */
	ROM_LOAD( "a21.07", 0x02000, 0x2000, 0x63623ba3 ) /* tiles - plane 1 */
	ROM_LOAD( "a21.06", 0x04000, 0x2000, 0x4cc969a9 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a21.14", 0x00000, 0x2000, 0x9c5c4d5b ) /* sprites - plane 0 */
	/* 0x03000-0x04fff empty */
	ROM_LOAD( "a21.13", 0x04000, 0x2000, 0xd36b5aad ) /* sprites - plane 0 */
	/* 0x07000-0x08fff empty */
	ROM_LOAD( "a21.12", 0x08000, 0x2000, 0xf387aea2 ) /* sprites - plane 1 */
	/* 0x0b000-0x0cfff empty */
	ROM_LOAD( "a21.11", 0x0c000, 0x2000, 0x87967d7d ) /* sprites - plane 1 */
	/* 0x0f000-0x10fff empty */
	ROM_LOAD( "a21.10", 0x10000, 0x2000, 0x59640ab4 ) /* sprites - plane 2 */
	/* 0x13000-0x14fff empty */
	ROM_LOAD( "a21.09", 0x14000, 0x2000, 0x22ad88d8 ) /* sprites - plane 2 */
	/* 0x17000-0x18fff empty */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "a21.16", 0x0000, 0x0100, 0xcc25fb56 ) /* red */
	ROM_LOAD( "a21.17", 0x0100, 0x0100, 0xca054448 ) /* green */
	ROM_LOAD( "a21.18", 0x0200, 0x0100, 0x23c0c449 ) /* blue */
	ROM_LOAD( "a21.20", 0x0300, 0x0020, 0xefc4985e ) /* char lookup table */
	ROM_LOAD( "m1.2c",  0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( sonofphx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "5.f4",   0x0000, 0x2000, 0xe0d2c6cf )
	ROM_LOAD( "6.h4",   0x2000, 0x2000, 0x3a0d0336 )
	ROM_LOAD( "7.j4",   0x4000, 0x2000, 0x57a8e900 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "1.f2",   0x0000, 0x2000, 0xc485c621 )
	ROM_LOAD( "2.h2",   0x2000, 0x2000, 0xb3c6a886 )
	ROM_LOAD( "3.j2",   0x4000, 0x2000, 0x197e314c )
	ROM_LOAD( "4.k2",   0x6000, 0x2000, 0x4f3695a1 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "14.4a",  0x00000, 0x1000, 0xb3859b8b ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.9h",  0x00000, 0x2000, 0xc9213469 ) /* tiles - plane 0 */
	ROM_LOAD( "16.10h", 0x02000, 0x2000, 0x7de5d39e ) /* tiles - plane 1 */
	ROM_LOAD( "17.11h", 0x04000, 0x2000, 0x0ba5f72c ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "8.6a",   0x00000, 0x4000, 0x0e9f757e ) /* sprites - plane 0 */
	ROM_LOAD( "9.7a",   0x04000, 0x4000, 0xf7d2e650 ) /* sprites - plane 0 */
	ROM_LOAD( "10.8a",  0x08000, 0x4000, 0xe717baf4 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",  0x0c000, 0x4000, 0x04b2250b ) /* sprites - plane 1 */
	ROM_LOAD( "12.10a", 0x10000, 0x4000, 0xd110e140 ) /* sprites - plane 2 */
	ROM_LOAD( "13.11a", 0x14000, 0x4000, 0x8fdc713c ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "b.1j",   0x0000, 0x0100, 0x3ea35431 ) /* blue */
	ROM_LOAD( "g.1h",   0x0100, 0x0100, 0xacd7a69e ) /* green */
	ROM_LOAD( "r.1f",   0x0200, 0x0100, 0xb7f48b41 ) /* red */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",  0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( repulse )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "repulse.b5",   0x0000, 0x2000, 0xfb2b7c9d )
	ROM_LOAD( "repulse.b6",   0x2000, 0x2000, 0x99129918 )
	ROM_LOAD( "7.j4",         0x4000, 0x2000, 0x57a8e900 )

	ROM_REGION( 0x10000, REGION_CPU2 , 0 ) /* 64k for code */
	ROM_LOAD( "1.f2",         0x0000, 0x2000, 0xc485c621 )
	ROM_LOAD( "2.h2",         0x2000, 0x2000, 0xb3c6a886 )
	ROM_LOAD( "3.j2",         0x4000, 0x2000, 0x197e314c )
	ROM_LOAD( "repulse.b4",   0x6000, 0x2000, 0x86b267f3 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "repulse.a11",  0x00000, 0x1000, 0x8e1de90a ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.9h",        0x00000, 0x2000, 0xc9213469 ) /* tiles - plane 0 */
	ROM_LOAD( "16.10h",       0x02000, 0x2000, 0x7de5d39e ) /* tiles - plane 1 */
	ROM_LOAD( "17.11h",       0x04000, 0x2000, 0x0ba5f72c ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "8.6a",         0x00000, 0x4000, 0x0e9f757e ) /* sprites - plane 0 */
	ROM_LOAD( "9.7a",         0x04000, 0x4000, 0xf7d2e650 ) /* sprites - plane 0 */
	ROM_LOAD( "10.8a",        0x08000, 0x4000, 0xe717baf4 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",        0x0c000, 0x4000, 0x04b2250b ) /* sprites - plane 1 */
	ROM_LOAD( "12.10a",       0x10000, 0x4000, 0xd110e140 ) /* sprites - plane 2 */
	ROM_LOAD( "13.11a",       0x14000, 0x4000, 0x8fdc713c ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "b.1j",         0x0000, 0x0100, 0x3ea35431 ) /* blue */
	ROM_LOAD( "g.1h",         0x0100, 0x0100, 0xacd7a69e ) /* green */
	ROM_LOAD( "r.1f",         0x0200, 0x0100, 0xb7f48b41 ) /* red */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",        0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( 99lstwar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "1999.4f",      0x0000, 0x2000, 0xe3cfc09f )
	ROM_LOAD( "1999.4h",      0x2000, 0x2000, 0xfd58c6e1 )
	ROM_LOAD( "7.j4",         0x4000, 0x2000, 0x57a8e900 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "1.f2",         0x0000, 0x2000, 0xc485c621 )
	ROM_LOAD( "2.h2",         0x2000, 0x2000, 0xb3c6a886 )
	ROM_LOAD( "3.j2",         0x4000, 0x2000, 0x197e314c )
	ROM_LOAD( "repulse.b4",   0x6000, 0x2000, 0x86b267f3 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1999.4a",      0x00000, 0x1000, 0x49a2383e ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.9h",        0x00000, 0x2000, 0xc9213469 ) /* tiles - plane 0 */
	ROM_LOAD( "16.10h",       0x02000, 0x2000, 0x7de5d39e ) /* tiles - plane 1 */
	ROM_LOAD( "17.11h",       0x04000, 0x2000, 0x0ba5f72c ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "8.6a",         0x00000, 0x4000, 0x0e9f757e ) /* sprites - plane 0 */
	ROM_LOAD( "9.7a",         0x04000, 0x4000, 0xf7d2e650 ) /* sprites - plane 0 */
	ROM_LOAD( "10.8a",        0x08000, 0x4000, 0xe717baf4 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",        0x0c000, 0x4000, 0x04b2250b ) /* sprites - plane 1 */
	ROM_LOAD( "12.10a",       0x10000, 0x4000, 0xd110e140 ) /* sprites - plane 2 */
	ROM_LOAD( "13.11a",       0x14000, 0x4000, 0x8fdc713c ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "b.1j",         0x0000, 0x0100, 0x3ea35431 ) /* blue */
	ROM_LOAD( "g.1h",         0x0100, 0x0100, 0xacd7a69e ) /* green */
	ROM_LOAD( "r.1f",         0x0200, 0x0100, 0xb7f48b41 ) /* red */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",        0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( 99lstwra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "4f.bin",       0x0000, 0x2000, 0xefe2908d )
	ROM_LOAD( "4h.bin",       0x2000, 0x2000, 0x5b79c342 )
	ROM_LOAD( "4j.bin",       0x4000, 0x2000, 0xd2a62c1b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "2f.bin",       0x0000, 0x2000, 0xcb9d8291 )
	ROM_LOAD( "2h.bin",       0x2000, 0x2000, 0x24dbddc3 )
	ROM_LOAD( "2j.bin",       0x4000, 0x2000, 0x16879c4c )
	ROM_LOAD( "repulse.b4",   0x6000, 0x2000, 0x86b267f3 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1999.4a",      0x00000, 0x1000, 0x49a2383e ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "9h.bin",       0x00000, 0x2000, 0x59993c27 ) /* tiles - plane 0 */
	ROM_LOAD( "10h.bin",      0x02000, 0x2000, 0xdfbf0280 ) /* tiles - plane 1 */
	ROM_LOAD( "11h.bin",      0x04000, 0x2000, 0xe4f29fc0 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "6a.bin",       0x00000, 0x4000, 0x98d44410 ) /* sprites - plane 0 */
	ROM_LOAD( "7a.bin",       0x04000, 0x4000, 0x4c54d281 ) /* sprites - plane 0 */
	ROM_LOAD( "8a.bin",       0x08000, 0x4000, 0x81018101 ) /* sprites - plane 1 */
	ROM_LOAD( "9a.bin",       0x0c000, 0x4000, 0x347b91fd ) /* sprites - plane 1 */
	ROM_LOAD( "10a.bin",      0x10000, 0x4000, 0xf07de4fa ) /* sprites - plane 2 */
	ROM_LOAD( "11a.bin",      0x14000, 0x4000, 0x34a04f48 ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "b.1j",         0x0000, 0x0100, 0x3ea35431 ) /* blue */
	ROM_LOAD( "g.1h",         0x0100, 0x0100, 0xacd7a69e ) /* green */
	ROM_LOAD( "r.1f",         0x0200, 0x0100, 0xb7f48b41 ) /* red */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",        0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( flashgal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "15.4f",        0x0000, 0x2000, 0xcf5ad733 )
	ROM_LOAD( "16.4h",        0x2000, 0x2000, 0x00c4851f )
	ROM_LOAD( "17.4j",        0x4000, 0x2000, 0x1ef0b8f7 )
	ROM_LOAD( "18.4k",        0x6000, 0x2000, 0x885d53de )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "11.2f",        0x0000, 0x2000, 0xeee2134d )
	ROM_LOAD( "12.2h",        0x2000, 0x2000, 0xe5e0cd22 )
	ROM_LOAD( "13.2j",        0x4000, 0x2000, 0x4cd3fe5e )
	ROM_LOAD( "14.2k",        0x6000, 0x2000, 0x552ca339 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "19.4b",        0x00000, 0x1000, 0xdca9052f ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "26.9h",        0x00000, 0x2000, 0x2f5b62c0 ) /* tiles - plane 0 */
	ROM_LOAD( "27.10h",       0x02000, 0x2000, 0x8fbb49b5 ) /* tiles - plane 1 */
	ROM_LOAD( "28.11h",       0x04000, 0x2000, 0x26a8e5c3 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "20.6b",        0x00000, 0x4000, 0x62caf2a1 ) /* sprites - plane 0 */
	ROM_LOAD( "21.7b",        0x04000, 0x4000, 0x10f78a10 ) /* sprites - plane 0 */
	ROM_LOAD( "22.8b",        0x08000, 0x4000, 0x36ea1d59 ) /* sprites - plane 1 */
	ROM_LOAD( "23.9b",        0x0c000, 0x4000, 0xf527d837 ) /* sprites - plane 1 */
	ROM_LOAD( "24.10b",       0x10000, 0x4000, 0xba76e4c1 ) /* sprites - plane 2 */
	ROM_LOAD( "25.11b",       0x14000, 0x4000, 0xf095d619 ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "flashgal.prr", 0x0000, 0x0100, 0x02c4043f ) /* red */
	ROM_LOAD( "flashgal.prg", 0x0100, 0x0100, 0x225938d1 ) /* green */
	ROM_LOAD( "flashgal.prb", 0x0200, 0x0100, 0x1e0a1cd3 ) /* blue */
	ROM_LOAD( "flashgal.pr2", 0x0300, 0x0020, 0xcce2e29f ) /* char lookup table */
	ROM_LOAD( "m1.2c",        0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */
ROM_END

ROM_START( srdmissn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "5.t2",   0x0000, 0x4000, 0xa682b48c )
	ROM_LOAD( "7.t3",   0x4000, 0x4000, 0x1719c58c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "1.t7",   0x0000, 0x4000, 0xdc48595e )
	ROM_LOAD( "3.t8",   0x4000, 0x4000, 0x216be1e8 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.4a",  0x00000, 0x1000, 0x4961f7fd ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "17.9h",  0x00000, 0x2000, 0x41211458 ) /* tiles - plane 1 */
	ROM_LOAD( "18.10h", 0x02000, 0x2000, 0x740eccd4 ) /* tiles - plane 0 */
	ROM_LOAD( "16.11h", 0x04000, 0x2000, 0xc1f4a5db ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "14.6a",  0x00000, 0x4000, 0x3d4c0447 ) /* sprites - plane 0 */
	ROM_LOAD( "13.7a",  0x04000, 0x4000, 0x22414a67 ) /* sprites - plane 0 */
	ROM_LOAD( "12.8a",  0x08000, 0x4000, 0x61e34283 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",  0x0c000, 0x4000, 0xbbbaffef ) /* sprites - plane 1 */
	ROM_LOAD( "10.10a", 0x10000, 0x4000, 0xde564f97 ) /* sprites - plane 2 */
	ROM_LOAD( "9.11a",  0x14000, 0x4000, 0x890dc815 ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "mr.1j",  0x0000, 0x0100, 0x110a436e ) /* red */
	ROM_LOAD( "mg.1h",  0x0100, 0x0100, 0x0fbfd9f0 ) /* green */
	ROM_LOAD( "mb.1f",  0x0200, 0x0100, 0xa342890c ) /* blue */
	ROM_LOAD( "m2.5j",  0x0300, 0x0020, 0x190a55ad ) /* char lookup table */
	ROM_LOAD( "m1.2c",  0x0320, 0x0020, 0x83a39201 ) /* timing? not used */
ROM_END

ROM_START( airwolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "b.2s",        0x0000, 0x8000, 0x8c993cce )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "a.7s",        0x0000, 0x8000, 0xa3c7af5c )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "f.4a",        0x00000, 0x1000, 0x4df44ce9 ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09h_14.bin",  0x00000, 0x2000, 0x25e57e1f ) /* tiles - plane 1 */
	ROM_LOAD( "10h_13.bin",  0x02000, 0x2000, 0xcf0de5e9 ) /* tiles - plane 0 */
	ROM_LOAD( "11h_12.bin",  0x04000, 0x2000, 0x4050c048 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "e.6a",        0x00000, 0x2000, 0xe8fbc7d2 ) /* sprites - plane 0 */
	ROM_CONTINUE(            0x04000, 0x2000 )
	ROM_CONTINUE(            0x02000, 0x2000 )
	ROM_CONTINUE(            0x06000, 0x2000 )
	ROM_LOAD( "d.8a",        0x08000, 0x2000, 0xc5d4156b ) /* sprites - plane 1 */
	ROM_CONTINUE(            0x0c000, 0x2000 )
	ROM_CONTINUE(            0x0a000, 0x2000 )
	ROM_CONTINUE(            0x0e000, 0x2000 )
	ROM_LOAD( "c.10a",       0x10000, 0x2000, 0xde91dfb1 ) /* sprites - plane 2 */
	ROM_CONTINUE(            0x14000, 0x2000 )
	ROM_CONTINUE(            0x12000, 0x2000 )
	ROM_CONTINUE(            0x16000, 0x2000 )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "01j.bin",     0x0000, 0x0100, 0x6a94b2a3 ) /* red */
	ROM_LOAD( "01h.bin",     0x0100, 0x0100, 0xec0923d3 ) /* green */
	ROM_LOAD( "01f.bin",     0x0200, 0x0100, 0xade97052 ) /* blue */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",       0x0320, 0x0020, 0x83a39201 ) /* timing? not used */
ROM_END

ROM_START( skywolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "02s_03.bin",  0x0000, 0x4000, 0xa0891798 )
	ROM_LOAD( "03s_04.bin",  0x4000, 0x4000, 0x5f515d46 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "07s_01.bin",  0x0000, 0x4000, 0xc680a905 )
	ROM_LOAD( "08s_02.bin",  0x4000, 0x4000, 0x3d66bf26 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "04a_11.bin",  0x00000, 0x1000, 0x219de9aa ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09h_14.bin",  0x00000, 0x2000, 0x25e57e1f ) /* tiles - plane 1 */
	ROM_LOAD( "10h_13.bin",  0x02000, 0x2000, 0xcf0de5e9 ) /* tiles - plane 0 */
	ROM_LOAD( "11h_12.bin",  0x04000, 0x2000, 0x4050c048 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "06a_10.bin",  0x00000, 0x4000, 0x1c809383 ) /* sprites - plane 0 */
	ROM_LOAD( "07a_09.bin",  0x04000, 0x4000, 0x5665d774 ) /* sprites - plane 0 */
	ROM_LOAD( "08a_08.bin",  0x08000, 0x4000, 0x6dda8f2a ) /* sprites - plane 1 */
	ROM_LOAD( "09a_07.bin",  0x0c000, 0x4000, 0x6a21ddb8 ) /* sprites - plane 1 */
	ROM_LOAD( "10a_06.bin",  0x10000, 0x4000, 0xf2e548e0 ) /* sprites - plane 2 */
	ROM_LOAD( "11a_05.bin",  0x14000, 0x4000, 0x8681b112 ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "01j.bin",     0x0000, 0x0100, 0x6a94b2a3 ) /* red */
	ROM_LOAD( "01h.bin",     0x0100, 0x0100, 0xec0923d3 ) /* green */
	ROM_LOAD( "01f.bin",     0x0200, 0x0100, 0xade97052 ) /* blue */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",       0x0320, 0x0020, 0x83a39201 ) /* timing? not used */
ROM_END

ROM_START( skywolf2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "z80_2.bin",   0x0000, 0x8000, 0x34db7bda )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	ROM_LOAD( "07s_01.bin",  0x0000, 0x4000, 0xc680a905 )
	ROM_LOAD( "08s_02.bin",  0x4000, 0x4000, 0x3d66bf26 )

	ROM_REGION( 0x01000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "04a_11.bin",  0x00000, 0x1000, 0x219de9aa ) /* chars */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09h_14.bin",  0x00000, 0x2000, 0x25e57e1f ) /* tiles - plane 1 */
	ROM_LOAD( "10h_13.bin",  0x02000, 0x2000, 0xcf0de5e9 ) /* tiles - plane 0 */
	ROM_LOAD( "11h_12.bin",  0x04000, 0x2000, 0x4050c048 ) /* tiles - plane 2 */

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "06a_10.bin",  0x00000, 0x4000, 0x1c809383 ) /* sprites - plane 0 */
	ROM_LOAD( "07a_09.bin",  0x04000, 0x4000, 0x5665d774 ) /* sprites - plane 0 */
	ROM_LOAD( "08a_08.bin",  0x08000, 0x4000, 0x6dda8f2a ) /* sprites - plane 1 */
	ROM_LOAD( "09a_07.bin",  0x0c000, 0x4000, 0x6a21ddb8 ) /* sprites - plane 1 */
	ROM_LOAD( "10a_06.bin",  0x10000, 0x4000, 0xf2e548e0 ) /* sprites - plane 2 */
	ROM_LOAD( "11a_05.bin",  0x14000, 0x4000, 0x8681b112 ) /* sprites - plane 2 */

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "01j.bin",     0x0000, 0x0100, 0x6a94b2a3 ) /* red */
	ROM_LOAD( "01h.bin",     0x0100, 0x0100, 0xec0923d3 ) /* green */
	ROM_LOAD( "01f.bin",     0x0200, 0x0100, 0xade97052 ) /* blue */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",       0x0320, 0x0020, 0x83a39201 ) /* timing? not used */
ROM_END

ROM_START( legend )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a_r2.rom",    0x0000, 0x4000, 0x0cc1c4f4 )
	ROM_LOAD( "a_r3.rom",    0x4000, 0x4000, 0x4b270c6b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "a_r7.rom",    0x0000, 0x2000, 0xabfe5eb4 )
	ROM_LOAD( "a_r8.rom",    0x2000, 0x2000, 0x7e7b9ba9 )
	ROM_LOAD( "a_r9.rom",    0x4000, 0x2000, 0x66737f1e )
	ROM_LOAD( "a_n7.rom",    0x6000, 0x2000, 0x13915a53 )

	ROM_REGION(  0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* fg tiles */
	ROM_LOAD( "b_a4.rom",    0x0000, 0x1000, 0xc7dd3cf7 )

	ROM_REGION(  0x6000, REGION_GFX2, ROMREGION_DISPOSE )	/* bg tiles */
	ROM_LOAD( "b_h9.rom",    0x0000, 0x2000, 0x1fe8644a )
	ROM_LOAD( "b_h10.rom",   0x2000, 0x2000, 0x5f7dc82e )
	ROM_LOAD( "b_h11.rom",   0x4000, 0x2000, 0x46741643 )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "b_a6.rom",   0x00000, 0x4000, 0x1689f21c )
	ROM_LOAD( "b_a7.rom",   0x04000, 0x4000, 0xf527c909 )
	ROM_LOAD( "b_a8.rom",   0x08000, 0x4000, 0x8d618629 )
	ROM_LOAD( "b_a9.rom",   0x0c000, 0x4000, 0x7d7e2d55 )
	ROM_LOAD( "b_a10.rom",  0x10000, 0x4000, 0xf12232fe )
	ROM_LOAD( "b_a11.rom",  0x14000, 0x4000, 0x8c09243d )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "legend.red",  0x0000, 0x0100, 0x00000000 ) /* red */
	ROM_LOAD( "legend.grn",  0x0100, 0x0100, 0x00000000 ) /* green */
	ROM_LOAD( "legend.blu",  0x0200, 0x0100, 0x00000000 ) /* blue */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "m1.2c",       0x0320, 0x0020, 0x83a39201 ) /* timing? not used */
ROM_END


/*************************************
 *
 *	Game specific initialization
 *
 *************************************/

static DRIVER_INIT( gyrodine )
{
	/* add watchdog */
	install_mem_write_handler(0, 0xe000, 0xe000, watchdog_reset_w);
}

static DRIVER_INIT( srdmissn )
{
	/* shared RAM is mapped at 0xe000 as well  */
	install_mem_read_handler (0, 0xe000, 0xe7ff, kyugo_sharedram_r);
	install_mem_write_handler(0, 0xe000, 0xe7ff, kyugo_sharedram_w);

	/* extra RAM on sub CPU  */
	install_mem_read_handler (1, 0x8800, 0x8fff, MRA_RAM);
	install_mem_write_handler(1, 0x8800, 0x8fff, MWA_RAM);
}


/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1984, gyrodine, 0,        gyrodine, gyrodine, gyrodine, ROT90, "Taito Corporation", "Gyrodine" )
GAME( 1985, sonofphx, 0,        sonofphx, sonofphx, 0,        ROT90, "Associated Overseas MFR, Inc", "Son of Phoenix" )
GAME( 1985, repulse,  sonofphx, sonofphx, sonofphx, 0,        ROT90, "Sega", "Repulse" )
GAME( 1985, 99lstwar, sonofphx, sonofphx, sonofphx, 0,        ROT90, "Proma", "'99 The Last War" )
GAME( 1985, 99lstwra, sonofphx, sonofphx, sonofphx, 0,        ROT90, "Proma", "'99 The Last War (alternate)" )
GAME( 1985, flashgal, 0,        flashgal, flashgal, 0,        ROT0,  "Sega", "Flash Gal" )
GAME( 1986, srdmissn, 0,        srdmissn, srdmissn, srdmissn, ROT90, "Taito Corporation", "S.R.D. Mission" )
GAMEX(1986?,legend,   0,        legend,   legend,   srdmissn, ROT0,  "Sega/Coreland (?)", "Legend", GAME_WRONG_COLORS )
GAME( 1987, airwolf,  0,        srdmissn, airwolf,  srdmissn, ROT0,  "Kyugo", "Air Wolf" )
GAME( 1987, skywolf,  airwolf,  srdmissn, skywolf,  srdmissn, ROT0,  "bootleg", "Sky Wolf (set 1)" )
GAME( 1987, skywolf2, airwolf,  srdmissn, airwolf,  srdmissn, ROT0,  "bootleg", "Sky Wolf (set 2)" )
