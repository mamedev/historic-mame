/***************************************************************************

Gaplus (c) 1984 Namco

MAME driver by:
	Manuel Abadia <manu@teleline.es>
	Ernesto Corvi <someone@secureshell.com>

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *gaplus_snd_sharedram;
extern unsigned char *gaplus_sharedram;
extern unsigned char *gaplus_customio_1, *gaplus_customio_2, *gaplus_customio_3;
extern unsigned char *mappy_soundregs;

/* shared memory functions */
READ_HANDLER( gaplus_sharedram_r );
READ_HANDLER( gaplus_snd_sharedram_r );
WRITE_HANDLER( gaplus_sharedram_w );
WRITE_HANDLER( gaplus_snd_sharedram_w );

/* custom IO chips functions */
WRITE_HANDLER( gaplus_customio_1_w );
WRITE_HANDLER( gaplus_customio_2_w );
WRITE_HANDLER( gaplus_customio_3_w );
READ_HANDLER( gaplus_customio_1_r );
READ_HANDLER( gaplus_customio_2_r );
READ_HANDLER( gaplus_customio_3_r );
READ_HANDLER( gapluso_customio_1_r );
READ_HANDLER( gapluso_customio_2_r );
READ_HANDLER( gapluso_customio_3_r );
READ_HANDLER( gaplusa_customio_1_r );
READ_HANDLER( gaplusa_customio_2_r );
READ_HANDLER( gaplusa_customio_3_r );

extern INTERRUPT_GEN( gaplus_interrupt_1 );
WRITE_HANDLER( gaplus_reset_2_3_w );
extern INTERRUPT_GEN( gaplus_interrupt_2 );
WRITE_HANDLER( gaplus_interrupt_ctrl_2_w );
extern INTERRUPT_GEN( gaplus_interrupt_3 );
WRITE_HANDLER( gaplus_interrupt_ctrl_3a_w );
WRITE_HANDLER( gaplus_interrupt_ctrl_3b_w );

extern VIDEO_START( gaplus );
extern PALETTE_INIT( gaplus );
extern VIDEO_UPDATE( gaplus );
extern MACHINE_INIT( gaplus );
WRITE_HANDLER( gaplus_starfield_control_w );

static MEMORY_READ_START( readmem_cpu1 )
	{ 0x0000, 0x03ff, videoram_r },				/* video RAM */
	{ 0x0400, 0x07ff, colorram_r },				/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_r },		/* shared RAM with CPU #2 & spriteram */
	{ 0x6040, 0x63ff, gaplus_snd_sharedram_r }, /* shared RAM with CPU #3 */
	{ 0x6800, 0x680f, gaplus_customio_1_r },	/* custom I/O chip #1 interface */
	{ 0x6810, 0x681f, gaplus_customio_2_r },	/* custom I/O chip #2 interface */
	{ 0x6820, 0x682f, gaplus_customio_3_r },	/* custom I/O chip #3 interface */
	{ 0x7820, 0x782f, MRA_RAM },				/* ??? */
	{ 0x7c00, 0x7c01, MRA_NOP },				/* ??? */
	{ 0xa000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu1 )
	{ 0x0000, 0x03ff, videoram_w, &videoram, &videoram_size },  /* video RAM */
	{ 0x0400, 0x07ff, colorram_w, &colorram },					/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_w, &gaplus_sharedram },	/* shared RAM with CPU #2 */
	{ 0x6040, 0x63ff, gaplus_snd_sharedram_w, &gaplus_snd_sharedram }, /* shared RAM with CPU #3 */
	{ 0x6800, 0x680f, gaplus_customio_1_w, &gaplus_customio_1 },/* custom I/O chip #1 interface */
	{ 0x6810, 0x681f, gaplus_customio_2_w, &gaplus_customio_2 },/* custom I/O chip #2 interface */
	{ 0x6820, 0x682f, gaplus_customio_3_w, &gaplus_customio_3 },/* custom I/O chip #3 interface */
	{ 0x7820, 0x782f, MWA_RAM },								/* ??? */
//	{ 0x7c00, 0x7c00, MWA_NOP },								/* ??? */
//	{ 0x8400, 0x8400, MWA_NOP },								/* ??? */
	{ 0x8c00, 0x8c00, gaplus_reset_2_3_w },	 					/* reset CPU #2 y #3? */
//	{ 0x9400, 0x9400, MWA_NOP },								/* ??? */
//	{ 0x9c00, 0x9c00, MWA_NOP },								/* ??? */
	{ 0xa000, 0xa003, gaplus_starfield_control_w },				/* starfield control */
	{ 0xa000, 0xffff, MWA_ROM },								/* ROM */
MEMORY_END

static MEMORY_READ_START( readmem_cpu2 )
	{ 0x0000, 0x03ff, videoram_r },				/* video RAM */
	{ 0x0400, 0x07ff, colorram_r },				/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_r },		/* shared RAM with CPU #1 & spriteram */
	{ 0xa000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu2 )
	{ 0x0000, 0x03ff, videoram_w },				/* video RAM */
	{ 0x0400, 0x07ff, colorram_w },				/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_w },		/* shared RAM with CPU #1 */
//	{ 0x500f, 0x500f, MWA_NOP },				/* ??? */
//	{ 0x6001, 0x6001, MWA_NOP },				/* ??? */
	{ 0x6080, 0x6081, gaplus_interrupt_ctrl_2_w },/* IRQ 2 enable */
	{ 0xa000, 0xffff, MWA_ROM },				/* ROM */
MEMORY_END

static MEMORY_READ_START( readmem_cpu3 )
	{ 0x0000, 0x003f, MRA_RAM },				/* sound registers? */
	{ 0x0040, 0x03ff, gaplus_snd_sharedram_r }, /* shared RAM with CPU #1 */
//	{ 0x3000, 0x3001, MRA_NOP },				/* ???*/
	{ 0xe000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

	/* CPU 3 (SOUND CPU) write addresses */
static MEMORY_WRITE_START( writemem_cpu3 )
	{ 0x0000, 0x003f, mappy_sound_w, &mappy_soundregs },/* sound registers */
	{ 0x0040, 0x03ff, gaplus_snd_sharedram_w },			/* shared RAM with the main CPU */
//	{ 0x2007, 0x2007, MWA_NOP },	/* ??? */
	{ 0x3000, 0x3000, watchdog_reset_w },				/* watchdog */
	{ 0x4000, 0x4000, gaplus_interrupt_ctrl_3a_w },		/* interrupt enable */
	{ 0x6000, 0x6000, gaplus_interrupt_ctrl_3b_w },		/* interrupt disable */
	{ 0xe000, 0xffff, MWA_ROM },						/* ROM */
MEMORY_END

static MEMORY_READ_START( gapluso_readmem_cpu1 )
	{ 0x0000, 0x03ff, videoram_r },				/* video RAM */
	{ 0x0400, 0x07ff, colorram_r },				/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_r },		/* shared RAM with CPU #2 & spriteram */
	{ 0x6040, 0x63ff, gaplus_snd_sharedram_r }, /* shared RAM with CPU #3 */
	{ 0x6800, 0x680f, gapluso_customio_1_r },	/* custom I/O chip #1 interface */
	{ 0x6810, 0x681f, gapluso_customio_2_r },	/* custom I/O chip #2 interface */
	{ 0x6820, 0x682f, gapluso_customio_3_r },	/* custom I/O chip #3 interface */
	{ 0x7820, 0x782f, MRA_RAM },				/* ??? */
	{ 0x7c00, 0x7c01, MRA_NOP },				/* ??? */
	{ 0xa000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_READ_START( gaplusa_readmem_cpu1 )
	{ 0x0000, 0x03ff, videoram_r },				/* video RAM */
	{ 0x0400, 0x07ff, colorram_r },				/* color RAM */
	{ 0x0800, 0x1fff, gaplus_sharedram_r },		/* shared RAM with CPU #2 & spriteram */
	{ 0x6040, 0x63ff, gaplus_snd_sharedram_r }, /* shared RAM with CPU #3 */
	{ 0x6800, 0x680f, gaplusa_customio_1_r },	/* custom I/O chip #1 interface */
	{ 0x6810, 0x681f, gaplusa_customio_2_r },	/* custom I/O chip #2 interface */
	{ 0x6820, 0x682f, gaplusa_customio_3_r },	/* custom I/O chip #3 interface */
	{ 0x7820, 0x782f, MRA_RAM },				/* ??? */
	{ 0x7c00, 0x7c01, MRA_NOP },				/* ??? */
	{ 0xa000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END


/* The dipswitches and player inputs are not memory mapped, they are handled by an I/O chip. */

INPUT_PORTS_START( gaplus )
	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BITX( 0x10,    0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xe0, "30k 70k and every 70k" )
	PORT_DIPSETTING(    0xc0, "30k 100k and every 100k" )
	PORT_DIPSETTING(    0xa0, "30k 100k and every 200k" )
	PORT_DIPSETTING(    0x80, "50k 100k and every 100k" )
	PORT_DIPSETTING(    0x60, "50k 100k and every 200k" )
	PORT_DIPSETTING(    0x00, "50k 150k and every 150k" )
	PORT_DIPSETTING(    0x40, "50k 150k and every 300k" )
	PORT_DIPSETTING(    0x20, "50k 150k" )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
		/* 0x40 service switch (not implemented yet) */

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( gapluso )
	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xe0, "30k 70k and every 70k" )
	PORT_DIPSETTING(    0xc0, "30k 100k and every 100k" )
	PORT_DIPSETTING(    0xa0, "30k 100k and every 200k" )
	PORT_DIPSETTING(    0x80, "50k 100k and every 100k" )
	PORT_DIPSETTING(    0x60, "50k 100k and every 200k" )
	PORT_DIPSETTING(    0x00, "50k 150k and every 150k" )
	PORT_DIPSETTING(    0x40, "50k 150k and every 300k" )
	PORT_DIPSETTING(    0x20, "50k 150k" )

	PORT_START  /* IN0 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_START1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_START2, 1 )
	/* 0x08 service switch (not implemented yet) */
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN2, 1 )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START  /* IN2 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, 1 )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, 1 )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, 0, IP_KEY_PREVIOUS, IP_JOY_PREVIOUS )
INPUT_PORTS_END

INPUT_PORTS_START( galaga3a )
	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BITX( 0x10,    0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xa0, "30k 80k and every 100k" )
	PORT_DIPSETTING(    0x80, "30k 100k and every 100k" )
	PORT_DIPSETTING(    0x60, "30k 100k and every 150k" )
	PORT_DIPSETTING(    0x00, "30k 100k and every 200k" )
	PORT_DIPSETTING(    0x40, "30k 100k and every 300k" )
	PORT_DIPSETTING(    0xe0, "50k 150k and every 150k" )
	PORT_DIPSETTING(    0xc0, "50k 150k and every 200k" )
	PORT_DIPSETTING(    0x20, "30k 150k" )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
		/* 0x40 service switch (not implemented yet) */

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( galaga3m )
	PORT_START  /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BITX( 0x10,    0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xe0, "30k 150k and every 600k" )
	PORT_DIPSETTING(    0xc0, "50k 150k and every 300k" )
	PORT_DIPSETTING(    0x80, "50k 200k and every 300k" )
	PORT_DIPSETTING(    0xa0, "50k 150k and every 600k" )
	PORT_DIPSETTING(    0x60, "100k 300k and every 300k" )
	PORT_DIPSETTING(    0x00, "100k 300k and every 600k" )
	PORT_DIPSETTING(    0x40, "150k 400k and every 900k" )
	PORT_DIPSETTING(    0x20, "150k 400k" )

	PORT_START  /* IN0 */
	PORT_BIT(  0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT(  0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT(  0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT(  0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
		/* 0x40 service switch (not implemented yet) */

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END


static struct GfxLayout charlayout1 =
{
	8,8,											/* 8*8 characters */
	256,											/* 256 characters */
	2,											  	/* 2 bits per pixel */
	{ 4, 6 },				 						/* the 2 bitplanes are packed into one nibble */
	{ 16*8, 16*8+1, 24*8, 24*8+1, 0, 1, 8*8, 8*8+1 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout charlayout2 =
{
	8,8,											/* 8*8 characters */
	256,											/* 256 characters */
	2,												/* 2 bits per pixel */
	{ 0, 2 },										/* the 2 bitplanes are packed into one nibble */
	{ 16*8, 16*8+1, 24*8, 24*8+1, 0, 1, 8*8, 8*8+1 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout spritelayout1 =
{
	16,16,			/* 16*16 sprites */
	128,			/* 128 sprites */
	3,				/* 3 bits per pixel */
	{ 0, 8192*8+0, 8192*8+4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8		   /* every sprite takes 64 bytes */
};


static struct GfxLayout spritelayout2 =
{
	16,16,			/* 16*16 sprites */
	128,			/* 128 sprites */
	3,				/* 3 bits per pixel */
	{ 4, 8192*8*2+0, 8192*8*2+4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8		   /* every sprite takes 64 bytes */
};

static struct GfxLayout spritelayout3 = {
	16,16,										   /* 16*16 sprites */
	128,										   /* 128 sprites */
	3,											   /* 3 bits per pixel (one is always 0) */
	{ 8192*8+0, 0, 4 },							   /* the two bitplanes are packed into one byte */
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8											/* every sprite takes 64 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout1,      0, 64 },
	{ REGION_GFX1, 0x0000, &charlayout2,      0, 64 },
	{ REGION_GFX2, 0x0000, &spritelayout1, 64*4, 64 },
	{ REGION_GFX2, 0x0000, &spritelayout2, 64*4, 64 },
	{ REGION_GFX2, 0x6000, &spritelayout3, 64*4, 64 },
	{ -1 } /* end of table */
};

static struct namco_interface namco_interface =
{
	24000,	/* sample rate */
	8,	  /* number of voices */
	100,	/* playback volume */
	REGION_SOUND1	/* memory region */
};

static const char *gaplus_sample_names[] =
{
	"*galaga",
	"bang.wav",
	0       /* end of array */
};

static struct Samplesinterface samples_interface =
{
	1,	/* one channel */
	80,	/* volume */
	gaplus_sample_names
};



static MACHINE_DRIVER_START( gaplus )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu1,writemem_cpu1)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_1,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_2,1)

	MDRV_CPU_ADD(M6809, 1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_3,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* a high value to ensure proper synchronization of the CPUs */
	MDRV_MACHINE_INIT(gaplus)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*8)

	MDRV_PALETTE_INIT(gaplus)
	MDRV_VIDEO_START(gaplus)
	MDRV_VIDEO_UPDATE(gaplus)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO, namco_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gapluso )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(gapluso_readmem_cpu1,writemem_cpu1)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_1,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_2,1)

	MDRV_CPU_ADD(M6809, 1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_3,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* a high value to ensure proper synchronization of the CPUs */
	MDRV_MACHINE_INIT(gaplus)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*8)

	MDRV_PALETTE_INIT(gaplus)
	MDRV_VIDEO_START(gaplus)
	MDRV_VIDEO_UPDATE(gaplus)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO, namco_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gaplusa )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(gaplusa_readmem_cpu1,writemem_cpu1)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_1,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_2,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 24.576 MHz / 16 = 1.536 MHz */
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_VBLANK_INT(gaplus_interrupt_3,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* a high value to ensure proper synchronization of the CPUs */

	MDRV_MACHINE_INIT(gaplus)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*8)

	MDRV_PALETTE_INIT(gaplus)
	MDRV_VIDEO_START(gaplus)
	MDRV_VIDEO_UPDATE(gaplus)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO, namco_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END



ROM_START( gaplus )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "gp3-4c.8d",    0xa000, 0x2000, 0x10d7f64c )
	ROM_LOAD( "gp3-3c.8c",    0xc000, 0x2000, 0x962411e8 )
	ROM_LOAD( "gp3-2d.8b",    0xe000, 0x2000, 0xecc01bdb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gp3-8b.11d",   0xa000, 0x2000, 0xf5e056d1 )
	ROM_LOAD( "gp2-7.11c",    0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gp3-6b.11b",   0xe000, 0x2000, 0x026491b6 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-5.8s",     0x0000, 0x2000, 0xf3d19987 )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "gp2-6p.bin",   0x0400, 0x0200, 0x6f99c2da )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "gp2-6n.bin",   0x0600, 0x0200, 0xc7d31657 )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END

ROM_START( gapluso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "gp2-4.8d",     0xa000, 0x2000, 0xe525d75d )
	ROM_LOAD( "gp2-3b.8c",    0xc000, 0x2000, 0xd77840a4 )
	ROM_LOAD( "gp2-2b.8b",    0xe000, 0x2000, 0xb3cb90db )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gp2-8.11d",    0xa000, 0x2000, 0x42b9fd7c )
	ROM_LOAD( "gp2-7.11c",    0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gp2-6.11b",    0xe000, 0x2000, 0x75b18652 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-5.8s",     0x0000, 0x2000, 0xf3d19987 )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "gp2-6p.bin",   0x0400, 0x0200, 0x6f99c2da )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "gp2-6n.bin",   0x0600, 0x0200, 0xc7d31657 )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END

ROM_START( gaplusa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "gp2-4.64",     0xa000, 0x2000, 0x484f11e0 )
	ROM_LOAD( "gp2-3.64",     0xc000, 0x2000, 0xa74b0266 )
	ROM_LOAD( "gp2-2.64",     0xe000, 0x2000, 0x69fdfdb7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gp2-8.64",     0xa000, 0x2000, 0xbff601a6 )
	ROM_LOAD( "gp2-7.64",     0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gp2-6.64",     0xe000, 0x2000, 0x14cd61ea )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-5.8s",     0x0000, 0x2000, 0xf3d19987 )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "gp2-6p.bin",   0x0400, 0x0200, 0x6f99c2da )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "gp2-6n.bin",   0x0600, 0x0200, 0xc7d31657 )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END

ROM_START( galaga3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "gp3-4c.8d",    0xa000, 0x2000, 0x10d7f64c )
	ROM_LOAD( "gp3-3c.8c",    0xc000, 0x2000, 0x962411e8 )
	ROM_LOAD( "gp3-2c.8b",    0xe000, 0x2000, 0xf72d6fc5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gp3-8b.11d",   0xa000, 0x2000, 0xf5e056d1 )
	ROM_LOAD( "gp2-7.11c",    0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gp3-6b.11b",   0xe000, 0x2000, 0x026491b6 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gal3_9l.bin",  0x0000, 0x2000, 0x8d4dcebf )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "g3_3f.bin",    0x0400, 0x0200, 0xd48c0eef )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "g3_3e.bin",    0x0600, 0x0200, 0x417ba0dc )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END

ROM_START( galaga3a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "gal3_9e.bin",  0xa000, 0x2000, 0xf4845e7f )
	ROM_LOAD( "gal3_9d.bin",  0xc000, 0x2000, 0x86fac687 )
	ROM_LOAD( "gal3_9c.bin",  0xe000, 0x2000, 0xf1b00073 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gal3_6l.bin",  0xa000, 0x2000, 0x9ec3dce5 )
	ROM_LOAD( "gp2-7.11c",    0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gal3_6n.bin",  0xe000, 0x2000, 0x6a2942c5 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gal3_9l.bin",  0x0000, 0x2000, 0x8d4dcebf )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "g3_3f.bin",    0x0400, 0x0200, 0xd48c0eef )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "g3_3e.bin",    0x0600, 0x0200, 0x417ba0dc )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END

ROM_START( galaga3m )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for the MAIN CPU */
	ROM_LOAD( "mi.9e",        0xa000, 0x2000, 0xe392704e )
	ROM_LOAD( "gal3_9d.bin",  0xc000, 0x2000, 0x86fac687 )
	ROM_LOAD( "gal3_9c.bin",  0xe000, 0x2000, 0xf1b00073 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the SUB CPU */
	ROM_LOAD( "gal3_6l.bin",  0xa000, 0x2000, 0x9ec3dce5 )
	ROM_LOAD( "gp2-7.11c",    0xc000, 0x2000, 0x0621f7df )
	ROM_LOAD( "gal3_6n.bin",  0xe000, 0x2000, 0x6a2942c5 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the SOUND CPU */
	ROM_LOAD( "gp2-1.4b",     0xe000, 0x2000, 0xed8aa206 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gal3_9l.bin",  0x0000, 0x2000, 0x8d4dcebf )	/* characters */

	ROM_REGION( 0xa000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gp2-9.11m",    0x0000, 0x2000, 0xe6a9ae67 )	/* objects */
	ROM_LOAD( "gp2-11.11p",   0x2000, 0x2000, 0x57740ff9 )	/* objects */
	ROM_LOAD( "gp2-10.11n",   0x4000, 0x2000, 0x6cd8ce11 )	/* objects */
	ROM_LOAD( "gp2-12.11r",   0x6000, 0x2000, 0x7316a1f1 )	/* objects */
	/* 0xa000-0xbfff empty space to decode sprite set #3 as 3 bits per pixel */

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "gp2-1.1p",     0x0000, 0x0100, 0xa5091352 )	/* red palette ROM (4 bits) */
	ROM_LOAD( "gp2-1.1n",     0x0100, 0x0100, 0x8bc8022a )	/* green palette ROM (4 bits) */
	ROM_LOAD( "gp2-2.2n",     0x0200, 0x0100, 0x8dabc20b )	/* blue palette ROM (4 bits) */
	ROM_LOAD( "gp2-6s.bin",   0x0300, 0x0100, 0x2faa3e09 )	/* char color ROM */
	ROM_LOAD( "g3_3f.bin",    0x0400, 0x0200, 0xd48c0eef )	/* sprite color ROM (lower 4 bits) */
	ROM_LOAD( "g3_3e.bin",    0x0600, 0x0200, 0x417ba0dc )	/* sprite color ROM (upper 4 bits) */

	ROM_REGION( 0x0100, REGION_SOUND1, 0 ) /* sound prom */
	ROM_LOAD( "gp2-4.3f",     0x0000, 0x0100, 0x2d9fbdd8 )
ROM_END


/*          rom       parent    machine   inp      init                                    */
GAME( 1984, gaplus,   0,        gaplus,   gaplus,   0, ROT90, "Namco", "Gaplus (rev. D)" )
GAME( 1984, gapluso,  gaplus,   gapluso,  gapluso,  0, ROT90, "Namco", "Gaplus (rev. B)" )
GAME( 1984, gaplusa,  gaplus,   gaplusa,  gapluso,  0, ROT90, "Namco", "Gaplus (alternate hardware)" )
GAME( 1984, galaga3,  gaplus,   gaplus,   gaplus,   0, ROT90, "Namco", "Galaga 3 (rev. C)" )
GAME( 1984, galaga3a, gaplus,   gaplus,   galaga3a, 0, ROT90, "Namco", "Galaga 3 (alternate set)" )
GAME( 1984, galaga3m, gaplus,   gaplus,   galaga3m, 0, ROT90, "[Namco] (Midway license)", "Galaga 3 (Midway)" )
